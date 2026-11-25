#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>
#include "Modules.h"
#include "RackSprites.h"
#include "NativeFaceLayout.h"
#include <map>
#include <cmath>

namespace skl::ui {
// Asset-specific row strides, confirmed against the exact embedded PNGs.
// Four metallic faces use 215 pixels. Blackbird uses 216; the landscape
// container truncates its last partial row, so retain only complete rows.
inline juce::Image decodeRackPanel(const void* bytes, int size, int stride = 215) {
    const auto raw = juce::ImageFileFormat::loadFrom(bytes, static_cast<size_t>(size));
    if (!raw.isValid() || (stride != 215 && stride != 216)) return {};
    if (raw.getWidth() == 1140 && raw.getHeight() == 430 && stride == 215) stride=430;
    else if(raw.getWidth()!=570 || raw.getHeight()!=215) return {};
    const int rows = raw.getWidth() * raw.getHeight() / stride;
    juce::Image panel(juce::Image::ARGB, stride, rows, true);
    juce::Image::BitmapData source(raw, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData destination(panel, juce::Image::BitmapData::writeOnly);
    for (int i = 0; i < stride * rows; ++i)
        destination.setPixelColour(i % stride, i / stride, source.getPixelColour(i % raw.getWidth(), i / raw.getWidth()));
    return panel;
}
inline bool hasToneMaterial(int type) { return type == skl::Eq || type == skl::Air; }
inline bool hasRackSkin(int type) { return hasAnimatedSkin(type); }
inline const juce::Image& rackPanel(int type, int choice) {
    static const juce::Image air = decodeRackPanel(BinaryData::air_lift_faceplate_png, BinaryData::air_lift_faceplate_pngSize);
    static const juce::Image earth = decodeRackPanel(BinaryData::earth_faceplate_png, BinaryData::earth_faceplate_pngSize);
    static const juce::Image eq = decodeRackPanel(BinaryData::classic_eq_faceplate_png, BinaryData::classic_eq_faceplate_pngSize);
    static const juce::Image lift = decodeRackPanel(BinaryData::classic_lift_faceplate_png, BinaryData::classic_lift_faceplate_pngSize);
    static const juce::Image bird = decodeRackPanel(BinaryData::blackbird_faceplate_png, BinaryData::blackbird_faceplate_pngSize, 216);
    static const juce::Image empty;
    if (choice < 1 || choice > 3) return empty;
    if (type == skl::Trim && choice == 1) return bird;
#if SKELLIERE_FULL_ARTWORK
#define SKELL_PANEL(NAME,STRIDE) { static const auto panel=decodeRackPanel(BinaryData::NAME##_png,BinaryData::NAME##_pngSize,STRIDE); return panel; }
    if(type==skl::Trim && choice==2) SKELL_PANEL(instrument_faceplate,216)
    if(type==skl::Comp && choice==1) SKELL_PANEL(custom_opto_faceplate,216)
    if(type==skl::ProgramOpto && choice==1) SKELL_PANEL(optical_faceplate,216)
    if(type==skl::BusComp && choice==1) SKELL_PANEL(bus_faceplate,215)
    if(type==skl::FastComp) {
        if(choice==1) SKELL_PANEL(fet_faceplate,215)
        if(choice==2) SKELL_PANEL(fet_modern_faceplate,215)
        if(choice==3) SKELL_PANEL(fet_vintage_faceplate,215)
    }
#undef SKELL_PANEL
#endif
    if(!hasToneMaterial(type)) return empty;
    if (choice == 3) return bird;
    if (type == skl::Eq) return choice == 1 ? earth : eq;
    return choice == 1 ? air : lift;
}
// The supplied artwork's printed controls are not our DSP controls. Sample
// inspected, label-free material regions, not the unrelated printed legends.
// Mirroring the tile prevents a hard seam at the texture boundaries.
inline juce::Image materialTile(const juce::Image& panel, juce::Rectangle<int> sample) {
    if (!panel.isValid() || !panel.getBounds().contains(sample) || sample.isEmpty()) return {};
    const int w = sample.getWidth(), h = sample.getHeight();
    juce::Image result(juce::Image::ARGB, w * 2, h * 2, true);
    juce::Image::BitmapData source(panel, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dest(result, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < h * 2; ++y)
        for (int x = 0; x < w * 2; ++x)
            dest.setPixelColour(x, y, source.getPixelColour(
                sample.getX() + (x < w ? x : 2*w - 1 - x),
                sample.getY() + (y < h ? y : 2*h - 1 - y)));
    return result;
}
inline const juce::Image& rackMaterial(int type, int choice) {
    static const auto earth = materialTile(rackPanel(skl::Eq, 1), {28, 39, 159, 26});
    static const auto air = materialTile(rackPanel(skl::Air, 1), {28, 39, 159, 26});
    static const auto eq = materialTile(rackPanel(skl::Eq, 2), {26, 38, 163, 9});
    static const auto lift = materialTile(rackPanel(skl::Air, 2), {26, 38, 163, 9});
    static const auto bird = materialTile(rackPanel(skl::Eq, 3), {28, 103, 160, 237});
    static const juce::Image empty;
    if (!hasToneMaterial(type) || choice < 1 || choice > 3) return empty;
    if (choice == 3) return bird;
    if (type == skl::Eq) return choice == 1 ? earth : eq;
    return choice == 1 ? air : lift;
}

/*
 * Non-destructive native faceplate restoration.
 *
 * The archived staticcanvas sources have baked text, white knob holes and
 * scales that do not correspond to Skelliere's independently authored DSP.
 * Previously painting rectangular covers on the JUCE editor left visible
 * seams and ghost markings. Reconstruct ONE continuous panel raster from
 * the supplied brushed/grain material, retaining the panel perimeter,
 * mounting hardware and source meter bezel, then draw our live controls.
 *
 * Generated on first use on the UI thread, cached by type/appearance.
 * Neither the original PNG bytes nor the user's archives are modified.
 */
inline juce::Image restoreNativePanel(const juce::Image& original, int type, int choice) {
    if (!original.isValid()) return {};
    const int w=original.getWidth(),h=original.getHeight();
    juce::Rectangle<int> sample;
    if (type==skl::Air || type==skl::Eq) {
        if(choice==1) sample={28,39,159,26};
        else if(choice==2) sample={26,38,163,9};
        else sample={28,103,160,237};
    } else if(type==skl::Trim) sample=choice==1?juce::Rectangle<int>(28,103,160,237)
                                              :juce::Rectangle<int>(36,90,140,210);
    else if(type==skl::Comp) sample={177,175,26,37};
    else if(type==skl::FastComp) sample={180,185,23,35};
    else if(type==skl::ProgramOpto) sample={179,185,24,36};
    else if(type==skl::BusComp) sample={178,186,24,36};
    else sample={27,40,150,25};
    sample=sample.getIntersection(original.getBounds());
    if(sample.isEmpty()) return original;
    // Mirror across both tile boundaries: no hard seams in the source grain.
    const auto tile=materialTile(original,sample);
    if(!tile.isValid()) return original;
    const auto face=nativeFace(type,choice);
    auto restored=original.createCopy();
    juce::Image::BitmapData raw(original,juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData grain(tile,juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dest(restored,juce::Image::BitmapData::writeOnly);
    const bool oldBlackbird=choice==3 && (type==skl::Air || type==skl::Eq);
    const bool blackbird=oldBlackbird || (type==skl::Trim && choice==1);
    const bool meter=skl::info(type).hasVU && face.meter.w>0;
    for(int y=0;y<h;++y) {
        for(int x=0;x<w;++x) {
            const auto px=raw.getPixelColour(x,y);
            // All labels/knob holes are removed from the same coherent field.
            // Preserve the 5px external relief and source corner hardware.
            float fill=juce::jlimit(0.0f,1.0f,static_cast<float>(juce::jmin(x-3,w-4-x))/5.0f);
            fill*=juce::jlimit(0.0f,1.0f,static_cast<float>(juce::jmin(y-3,h-4-y))/5.0f);
            for(int corner=0;corner<4;++corner) {
                const int cx=corner%2?w-18:18,cy=corner/2?h-18:18;
                const float dx=static_cast<float>(x-cx),dy=static_cast<float>(y-cy);
                const float radius=std::sqrt(dx*dx+dy*dy);
                fill*=juce::jlimit(0.0f,1.0f,(radius-11.0f)/5.0f);
            }
            if(blackbird && y<370) fill=0.0f; // original blank chamber & relief
            if(meter) {
                const auto box=face.meter;
                const int xx=juce::roundToInt(static_cast<float>(x)*215.0f/w);
                const int yy=juce::roundToInt(static_cast<float>(y)*570.0f/h);
                const int dx=juce::jmax(box.x-xx,xx-(box.x+box.w),0);
                const int dy=juce::jmax(box.y-yy,yy-(box.y+box.h),0);
                fill*=juce::jlimit(0.0f,1.0f,static_cast<float>(juce::jmax(dx,dy))/9.0f);
            }
            const int sx=(x+9)%tile.getWidth();
            // Small deterministic grain offset avoids obvious 18px vertical
            // repetition in the thin CS-EQ/Lift clean source strips.
            const int sy=(y+(x/29)*7)%tile.getHeight();
            const auto replacement=grain.getPixelColour(sx,sy);
            dest.setPixelColour(x,y,px.interpolatedWith(replacement,fill));
        }
    }
    return restored;
}
inline const juce::Image& restoredRackPanel(int type,int choice) {
    // UI-only lazy cache. Never called from processBlock.
    static std::map<int,juce::Image> cache;
    static const juce::Image empty;
    if(choice<=0 || !rackPanel(type,choice).isValid()) return empty;
    const int id=type*4+choice;
    if(auto it=cache.find(id);it!=cache.end()) return it->second;
    const auto& raw=rackPanel(type,choice);
    return cache.emplace(id,restoreNativePanel(raw,type,choice)).first->second;
}

inline bool lightRackSkin(int type, int choice) {
    return (choice == 1 || choice == 2) && rackMaterial(type, choice).isValid();
}
inline bool rackSkinAvailable(int type,int choice) {
    if(choice==0) return true;
    if(!hasRackSkin(type) || !rackSprite(switchSpriteFor(type,choice)).valid()) return false;
    for(int i=0;i<info(type).numKnobs;++i)
        if(!rackSprite(knobSpriteFor(type,choice,i)).valid()) return false;
    return rackPanel(type,choice).isValid();
}
inline juce::String rackPanelName(int type, int choice) {
    if (choice == 0) return "Original";
    if(type==skl::Trim) return choice==1?"Blackbird fader":"Classic Instruments fader";
    if(type==skl::Comp) return "Custom optical";
    if(type==skl::ProgramOpto) return "Optical";
    if(type==skl::FastComp) return choice==1?"FET classic":choice==2?"FET modern":"FET vintage";
    if(type==skl::BusComp) return "Blue bus";
    if (choice == 3) return "Blackbird graphite";
    if (type == skl::Eq) return choice == 1 ? "AirEQ Earth" : "CS-EQ";
    return choice == 1 ? "AirEQ Air" : "CS-Lift";
}
class ArtworkButton final : public juce::TextButton {
public:
    ArtworkButton(std::function<int()> getType, std::function<int()> getChoice,
                  std::function<void(int)> apply)
        : juce::TextButton("SKIN"), getType_(std::move(getType)),
          getChoice_(std::move(getChoice)), apply_(std::move(apply)) {
        setTooltip("Select a native faceplate layout; all controls retain their real parameter bindings.");
        onClick = [this] {
            const int type = getType_();
            if (!hasRackSkin(type)) return;
            juce::PopupMenu menu;
            for (int choice = 0; choice < 4; ++choice)
                if (choice == 0 || knobSpriteFor(type,choice,0)!=NoSprite)
                    menu.addItem(choice + 1, rackPanelName(type, choice), rackSkinAvailable(type,choice), getChoice_() == choice);
            juce::Component::SafePointer<ArtworkButton> safeThis(this);
            menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [safeThis, type](int result) {
                if (safeThis == nullptr || safeThis->getType_() != type || result < 1 || result > 4) return;
                if(rackSkinAvailable(type,result-1)) safeThis->apply_(result - 1);
            });
        };
    }
private:
    std::function<int()> getType_, getChoice_;
    std::function<void(int)> apply_;
};
}
