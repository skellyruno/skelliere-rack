#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>
#include "Modules.h"
#include "SpriteFrames.h"
#include "RackResource.h"

namespace skl::ui {
enum RackSprite : int {
    NoSprite=0, AirLarge, AirSmall, AirSmallAlt, AirPower,
    ClassicEqA, ClassicEqB, ClassicEqC, ClassicEqD, ClassicEqE, ClassicLiftBig, ClassicLiftSmall, ClassicPower,
    BlackbirdFader,
    CustomOptoLarge, CustomOptoSmall, CustomOptoSwitch, FetClassic, FetModern, FetVintage,
    Optical, OpticalSwitch, BusBlue, BusSwitch, InstrumentFader,
    RackSpriteCount
};
struct PackedSprite {
    juce::Image atlas;
    PackedFrameLayout layout;
    juce::Rectangle<int> crop;
    int columns=1;
    PackedSprite() = default;
    PackedSprite(const juce::Image& raw, int frames, int expectedW, int expectedH) {
        if (!raw.isValid()) return;
        const auto dims = packedFrameLayout(raw.getWidth(), raw.getHeight(), frames);
        if (!dims.valid()) return;
        const bool one = dims.frameWidth == expectedW && dims.frameHeight == expectedH;
        const bool two = dims.frameWidth == 2*expectedW && dims.frameHeight == 2*expectedH;
        if (!one && !two) return;
        layout=dims;
        // Keep even 2x atlases below common GPU texture dimension limits.
        columns=std::min(9,frames);
        const int rows=(frames+columns-1)/columns;
        atlas=juce::Image(juce::Image::ARGB, dims.frameWidth*columns, dims.frameHeight*rows, true);
        if (!atlas.isValid()) { layout={}; return; }
        juce::Image::BitmapData source(raw, juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData dest(atlas, juce::Image::BitmapData::writeOnly);
        int x0=dims.frameWidth,y0=dims.frameHeight,x1=-1,y1=-1;
        for (int frame=0;frame<frames;++frame) {
            const int dx=(frame%columns)*dims.frameWidth, dy=(frame/columns)*dims.frameHeight;
            for (int y=0;y<dims.frameHeight;++y) for (int x=0;x<dims.frameWidth;++x) {
                const int offset=(frame*dims.frameHeight+y)*dims.frameWidth+x;
                const auto px=source.getPixelColour(offset%raw.getWidth(),offset/raw.getWidth());
                dest.setPixelColour(dx+x,dy+y,px);
                if (frames==2 && px.getAlpha()>8) {
                    x0=std::min(x0,x);x1=std::max(x1,x);y0=std::min(y0,y);y1=std::max(y1,y);
                }
            }
        }
        crop={0,0,dims.frameWidth,dims.frameHeight};
        if (frames==2 && x1>=x0 && y1>=y0)
            crop=juce::Rectangle<int>(x0,y0,x1-x0+1,y1-y0+1).expanded(2).getIntersection(crop);
    }
    bool valid() const noexcept { return atlas.isValid() && layout.valid() && !crop.isEmpty(); }
    juce::Rectangle<int> source(double position) const noexcept {
        const int index=spriteFrame(position,layout.count);
        return crop.translated((index%columns)*layout.frameWidth,(index/columns)*layout.frameHeight);
    }
    juce::Rectangle<int> destination(juce::Rectangle<int> bounds) const {
        if(!valid()||bounds.isEmpty()) return {};
        const float scale=std::min(float(bounds.getWidth())/float(crop.getWidth()),
                                   float(bounds.getHeight())/float(crop.getHeight()));
        return juce::Rectangle<int>(std::max(1,juce::roundToInt(crop.getWidth()*scale)),
                                    std::max(1,juce::roundToInt(crop.getHeight()*scale))).withCentre(bounds.getCentre());
    }
    bool endpointsDiffer() const {
        if(!valid()||layout.count<2) return false;
        const auto first=source(0.0),last=source(1.0);
        for(int y=0;y<first.getHeight();++y)for(int x=0;x<first.getWidth();++x)
            if(atlas.getPixelAt(first.getX()+x,first.getY()+y)!=atlas.getPixelAt(last.getX()+x,last.getY()+y)) return true;
        return false;
    }
    bool draw(juce::Graphics& g, juce::Rectangle<int> bounds, double value) const {
        if (!valid() || bounds.isEmpty()) return false;
        const auto src=source(value),dst=destination(bounds);
        g.setOpacity(1.0f);
        g.drawImage(atlas,dst.getX(),dst.getY(),dst.getWidth(),dst.getHeight(),src.getX(),src.getY(),src.getWidth(),src.getHeight());
        return true;
    }
};
inline const PackedSprite& rackSprite(int id) {
    // Decode once per resource on the GUI thread, never in DSP.
#define SKELL_SPRITE(ID,NAME,N,W,H) case ID: { static const PackedSprite s(loadRackResource(#NAME),N,W,H); return s; }
    switch(id) {
        SKELL_SPRITE(AirLarge,air_knob_large,65,111,118)
        SKELL_SPRITE(AirSmall,air_knob_small,65,77,80)
        SKELL_SPRITE(AirSmallAlt,air_knob_small_alt,65,77,80)
        SKELL_SPRITE(AirPower,air_switch,2,77,52)
        SKELL_SPRITE(ClassicEqA,classic_eq_knob_a,65,95,95)
        SKELL_SPRITE(ClassicEqB,classic_eq_knob_b,65,95,95)
        SKELL_SPRITE(ClassicEqC,classic_eq_knob_c,65,95,95)
        SKELL_SPRITE(ClassicEqD,classic_eq_knob_d,65,95,95)
        SKELL_SPRITE(ClassicEqE,classic_eq_knob_e,65,95,95)
        SKELL_SPRITE(ClassicLiftBig,classic_lift_knob_big,65,129,129)
        SKELL_SPRITE(ClassicLiftSmall,classic_lift_knob_small,65,83,83)
        SKELL_SPRITE(ClassicPower,classic_in_switch,2,56,46)
        SKELL_SPRITE(BlackbirdFader,blackbird_fader,65,195,70)
        SKELL_SPRITE(CustomOptoLarge,custom_opto_large,65,75,102)
        SKELL_SPRITE(CustomOptoSmall,custom_opto_small,65,43,63)
        SKELL_SPRITE(CustomOptoSwitch,custom_opto_switch,2,46,81)
        SKELL_SPRITE(FetClassic,fet_classic,65,52,59)
        SKELL_SPRITE(FetModern,fet_modern,65,62,70)
        SKELL_SPRITE(FetVintage,fet_vintage,65,60,76)
        SKELL_SPRITE(Optical,optical_knob,65,63,68)
        SKELL_SPRITE(OpticalSwitch,optical_switch,2,43,77)
        SKELL_SPRITE(BusBlue,bus_knob,65,88,106)
        SKELL_SPRITE(BusSwitch,bus_switch,2,76,44)
        SKELL_SPRITE(InstrumentFader,instrument_fader,65,180,63)
        default: { static const PackedSprite empty; return empty; }
    }
#undef SKELL_SPRITE
}
inline bool isFaderSprite(int id) noexcept {
    return id==BlackbirdFader
        || id==InstrumentFader
        ;
}
inline int knobSpriteFor(int type,int choice,int knob) noexcept {
    if(choice<=0 || choice>3) return NoSprite;
    if(type==skl::Eq) {
        constexpr int eqs[]={ClassicEqA,ClassicEqB,ClassicEqC,ClassicEqD,ClassicEqE};
        if(choice==2) return eqs[std::clamp(knob,0,4)];
        if(choice==3) return ClassicEqA;
        return knob==0?AirSmall:knob%2?AirSmallAlt:AirLarge;
    }
    if(type==skl::Air) return choice==2?(knob==0?ClassicLiftBig:ClassicLiftSmall):
                                  choice==3?ClassicEqA:(knob==0?AirLarge:AirSmall);
    if(type==skl::Trim) {
        if(choice==1) return BlackbirdFader;
        if(choice==2) return InstrumentFader;
        return NoSprite;
    }
    if(type==skl::Comp && choice==1) return knob==0||knob==4?CustomOptoLarge:CustomOptoSmall;
    if(type==skl::ProgramOpto && choice==1) return Optical;
    if(type==skl::FastComp) return choice==1?FetClassic:choice==2?FetModern:FetVintage;
    if(type==skl::BusComp && choice==1) return BusBlue;
    return NoSprite;
}
inline int switchSpriteFor(int type,int choice) noexcept {
    if(knobSpriteFor(type,choice,0)==NoSprite) return NoSprite;
    if(type==skl::Comp) return CustomOptoSwitch;
    // FG-2A source toggle is labelled COMPRESS/LIMIT, not bypass.
    if(type==skl::ProgramOpto || type==skl::FastComp || type==skl::Trim) return NoSprite;
    if(type==skl::BusComp) return BusSwitch;
    return choice==2 || choice==3 ? ClassicPower : AirPower;
}
inline bool hasAnimatedSkin(int type) { return rackSprite(knobSpriteFor(type,1,0)).valid(); }
}
