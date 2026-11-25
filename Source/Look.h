#pragma once
// Skelliere look: blacked-out analog gear. One chassis, graphite brushed faceplates, knurled knobs,
// printed legends and cream needle meters. All colours live here so the whole plugin can be re-skinned in one place.
#include <JuceHeader.h>
#include "Modules.h"
#include <map>


namespace skl::ui {

using juce::Colour;
using juce::Graphics;

namespace pal {
inline const Colour chassis   {0xff100f0e};
inline const Colour chassis2  {0xff201e1b};
inline const Colour barTop    {0xff393631};
inline const Colour barBot    {0xff201e1b};
inline const Colour plateTop  {0xff2a2c30};
inline const Colour plateBot  {0xff201e1b};
inline const Colour ink       {0xffeee3cc};
inline const Colour inkDim    {0xffb2aa99};
inline const Colour line      {0xff3a3d42};
inline const Colour amber     {0xffe4c69b};
inline const Colour lcdBg     {0xff060a07};
inline const Colour lcdInk    {0xffb9d7a7};
inline const Colour warn      {0xffef6a5b};
inline const Colour vuFaceTop {0xffffe5b4};
inline const Colour vuFaceBot {0xffc69859};
inline const Colour vuInk     {0xff2b2418};
inline const Colour vuRed     {0xffa02a1d};
}

inline juce::Font font(float h, bool bold = false) {
    return juce::Font(juce::FontOptions(h).withStyle(bold ? "Bold" : "Regular"));
}

inline Colour categoryColour(int cat) {
    switch (cat) {
        case CatDynamics: return Colour(0xff3fb0a6);
        case CatTone:     return Colour(0xffe0a634);
        case CatSpace:    return Colour(0xff8f84dc);
        case CatUtility:  return Colour(0xff9aa0a8);
        default:          return Colour(0xff55585d);
    }
}
// Knob cap colour per family, so a glance down the rack tells you what is what.
inline Colour capColour(int cat) {
    switch (cat) {
        case CatDynamics: return Colour(0xff25272b);   // black caps
        case CatSpace:    return Colour(0xffe6dec6);   // cream caps
        default:          return Colour(0xffc9cbce);   // aluminium caps
    }
}
inline Colour pointerColour(int cat) { return cat == CatDynamics ? Colour(0xfff1eee2) : Colour(0xff201e1b); }

inline juce::String knobText(const Knob& k, float norm) {
    const float v = toReal(k, norm);
    switch (k.unit) {
        case Unit::Db:    return juce::String(v > 0.05f ? "+" : "") + juce::String(v, 1) + " dB";
        case Unit::Hz:
            if (juce::String(k.label) == "HPF" && v <= 22.0f) return "Off";
            return v >= 1000.0f ? juce::String(v / 1000.0f, 1) + " kHz" : juce::String((int) std::lround(v)) + " Hz";
        case Unit::Ms:    return v < 10.0f ? juce::String(v, 1) + " ms" : juce::String((int) std::lround(v)) + " ms";
        case Unit::Pct:   return juce::String((int) std::lround(v * 100.0f)) + "%";
        case Unit::Ratio: return juce::String(v, 1) + ":1";
        case Unit::Scalar:return juce::String(v,2);
        case Unit::Sec:   return juce::String(v, 1) + " s";
        case Unit::LfoHz: return juce::String(v, 2) + " Hz";
        case Unit::Pan:   return std::fabs(v) < 0.02f ? juce::String("Center") : juce::String(v < 0 ? "L " : "R ") + juce::String((int) std::lround(std::fabs(v) * 100.0f));
        case Unit::Steps: { auto names = juce::StringArray::fromTokens(k.names ? k.names : "", ",", ""); return names[(int) v]; }
    }
    return {};
}

inline const juce::Image& brushedImage() {
    static const juce::Image img = [] {
        juce::Image im(juce::Image::ARGB, 256, 256, true);
        Graphics g(im);
        juce::Random rng(1234);
        for (int y = 0; y < 256; ++y) {
            g.setColour((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha(rng.nextFloat() * 0.025f));
            g.drawHorizontalLine(y, 0.0f, 256.0f);
        }
        return im;
    }();
    return img;
}

inline void paintBrushed(Graphics& g, juce::Rectangle<float> r, float opacity = 1.0f) {
    g.setTiledImageFill(brushedImage(), 0, 0, opacity);
    g.fillRect(r);
}

// Original machined cap, drawn once per metal finish at high resolution.
inline const juce::Image& machinedCap(Colour colour) {
    static std::map<juce::uint32,juce::Image> cache;
    const auto key=colour.getARGB();
    if(auto it=cache.find(key);it!=cache.end())return it->second;
    constexpr int n=192;
    juce::Image image(juce::Image::ARGB,n,n,true);
    juce::Image::BitmapData pixels(image,juce::Image::BitmapData::writeOnly);
    for(int y=0;y<n;++y)for(int x=0;x<n;++x){
        const float dx=(x+.5f-n*.5f)/(n*.5f),dy=(y+.5f-n*.5f)/(n*.5f);
        const float r=std::sqrt(dx*dx+dy*dy);
        if(r>=1.f)continue;
        const float a=std::atan2(dy,dx);
        const float grain=std::sin(r*425.f+.35f*std::sin(a*19.f))*.022f
                         +std::sin(r*143.f)*.018f;
        const float reflection=.62f+.29f*std::cos(a+.85f)+.12f*std::cos(2*a-1.2f);
        float shine=reflection+grain;
        if(r>.92f)shine=.65f+.25f*std::cos(a+.9f);
        if(r>.975f)shine=.22f+.38f*juce::jmax(0.f,std::cos(a+1.4f));
        const float alpha=juce::jlimit(0.f,1.f,(1-r)*n*.5f);
        const auto tint=colour.interpolatedWith(shine>.72f?juce::Colours::white:juce::Colours::black,
            shine>.72f?(shine-.72f)*.55f:juce::jlimit(0.f,.68f,(.72f-shine)*.93f));
        pixels.setPixelColour(x,y,tint.withAlpha(alpha));
    }
    return cache.emplace(key,image).first->second;
}

inline void paintMonogram(Graphics& g,juce::Rectangle<float> r,Colour ink) {
    const float x=r.getX(),y=r.getY(),w=r.getWidth(),h=r.getHeight();
    juce::Path shield;shield.startNewSubPath(x+w*.5f,y);
    shield.lineTo(x+w*.91f,y+h*.16f);shield.lineTo(x+w*.91f,y+h*.82f);
    shield.lineTo(x+w*.5f,y+h);shield.lineTo(x+w*.09f,y+h*.82f);shield.lineTo(x+w*.09f,y+h*.16f);shield.closeSubPath();
    g.setColour(ink);g.strokePath(shield,juce::PathStrokeType(1.1f));
    g.setFont(font(h*.73f,true));g.drawFittedText("S",r.toNearestInt(),juce::Justification::centred,1);
}

inline void paintScrew(Graphics& g, float cx, float cy, float r, float turn) {
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillEllipse(cx - r, cy - r + 1.0f, 2 * r, 2 * r);
    g.setGradientFill(juce::ColourGradient(Colour(0xfff0f0f0), cx - r * 0.4f, cy - r * 0.5f, Colour(0xff6c6c6c), cx + r * 0.7f, cy + r * 0.8f, true));
    g.fillEllipse(cx - r, cy - r, 2 * r, 2 * r);
    g.setColour(Colour(0xff2a2a2a));
    g.drawEllipse(cx - r, cy - r, 2 * r, 2 * r, 0.8f);
    juce::Path cross;
    cross.addRectangle(-r * 0.75f, -0.9f, r * 1.5f, 1.8f);
    cross.addRectangle(-0.9f, -r * 0.75f, 1.8f, r * 1.5f);
    g.fillPath(cross, juce::AffineTransform::rotation(turn).translated(cx, cy));
}

// The needle meter used by the compressor, de-esser and limiter. value is gain reduction in dB (0 to 20).
inline void paintVU(Graphics& g, juce::Rectangle<float> r, float value, bool outputPeak=false) {
    g.setColour(juce::Colours::black.withAlpha(.45f));g.fillRoundedRectangle(r.translated(0,2),8);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff514a3e),r.getY(),Colour(0xff151411),r.getBottom()));
    g.fillRoundedRectangle(r,7);
    g.setColour(Colour(0xff9c8b6b).withAlpha(.70f));g.drawRoundedRectangle(r.reduced(.7f),6,1.1f);
    const auto inner=r.reduced(5);
    g.setColour(Colour(0xff100e0c));g.fillRoundedRectangle(inner,6);
    auto face=inner.reduced(3.0f);
    g.setGradientFill(juce::ColourGradient(pal::vuFaceTop,face.getCentreX(),face.getCentreY(),pal::vuFaceBot,face.getX(),face.getBottom(),true));
    g.fillRoundedRectangle(face,5);
    g.setColour(Colour(0xff8c6234));g.drawRoundedRectangle(face,5,.9f);

    juce::Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(face.toNearestInt());
    // The meter is much wider than it is tall. Fit the arc in both axes,
    // keeping its full scale, labels and hub inside the actual face.
    const float cx = face.getCentreX(), cy = face.getBottom() - 7.0f;
    const float sc = face.getWidth() / 130.0f;
    const float sy = juce::jmax(0.1f, (face.getHeight() - 20.0f) / 57.0f);
    const float labelSize = juce::jlimit(8.0f, 11.0f, 8.0f * sc);
    auto pt = [&](float rad, float v) {
        const float a = juce::degreesToRadians(-50.0f + v * 5.0f);
        return juce::Point<float>(cx + rad * sc * std::sin(a), cy - rad * sy * std::cos(a));
    };
    const float redStart=outputPeak?20.f*20.f/24.f:12.f;
    for (float v = 0.0f; v <= 20.0f; v += 2.0f) {
        const bool major = std::fmod(v, 4.0f) < 0.01f;
        g.setColour(v >= redStart ? pal::vuRed : pal::vuInk);
        g.drawLine(juce::Line<float>(pt(57.0f, v), pt(major ? 49.0f : 53.0f, v)), major ? 1.4f : 0.9f);
        if (major) {
            g.setFont(font(labelSize,true));
            const int label=outputPeak?juce::roundToInt(-20.f+v*1.2f):int(v);
            g.drawText((label>0&&outputPeak?"+":"")+juce::String(label),juce::Rectangle<float>(0,0,24,13).withCentre(pt(39.f,v)),juce::Justification::centred,false);
        }
    }
    juce::Path arc;
    arc.startNewSubPath(pt(57.0f, 0.0f));
    for (float v = 0.0f; v <= 20.0f; v += 1.0f) arc.lineTo(pt(57.0f, v));
    g.setColour(pal::vuInk); g.strokePath(arc, juce::PathStrokeType(1.1f));
    juce::Path red;
    red.startNewSubPath(pt(54.0f,redStart));
    for(float v=redStart;v<20.f;v+=.5f)red.lineTo(pt(54.f,v));
    red.lineTo(pt(54.f,20.f));
    g.setColour(pal::vuRed); g.strokePath(red, juce::PathStrokeType(2.0f));
    g.setColour(pal::vuInk); g.setFont(font(labelSize, true));
    g.drawText(outputPeak?"PEAK dBFS":"GR dB", face.withTrimmedLeft(7.0f).removeFromBottom(14.0f), juce::Justification::centredLeft, false);

    const float v=juce::jlimit(0.f,20.f,outputPeak?(value+20.f)*20.f/24.f:value);
    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.drawLine(juce::Line<float>(juce::Point<float>(cx + 1.5f, cy + 1.5f), pt(52.0f, v) + juce::Point<float>(1.5f, 1.5f)), 1.6f);
    g.setColour(pal::vuRed);
    g.drawLine(juce::Line<float>(juce::Point<float>(cx, cy), pt(52.0f, v)), 1.5f);
    g.setColour(Colour(0xff1a1a1a)); g.fillEllipse(cx - 4.5f, cy - 4.5f, 9.0f, 9.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.28f), face.getX(), face.getY(), juce::Colours::white.withAlpha(0.0f), face.getX(), face.getY() + face.getHeight() * 0.5f, false));
    g.fillRect(face.withHeight(face.getHeight() * 0.45f));
}

class SkellierLook final : public juce::LookAndFeel_V4 {
public:
    SkellierLook() {
        setColour(juce::ResizableWindow::backgroundColourId, pal::chassis);
        setColour(juce::TextButton::textColourOffId, pal::ink);
        setColour(juce::TextButton::textColourOnId, pal::amber);
        setColour(juce::Label::textColourId, pal::ink);
        setColour(juce::ComboBox::backgroundColourId, Colour(0xff0d0e10));
        setColour(juce::ComboBox::outlineColourId, Colour(0xff000000));
        setColour(juce::ComboBox::textColourId, pal::ink);
        setColour(juce::ComboBox::arrowColourId, pal::amber);
        setColour(juce::PopupMenu::backgroundColourId, Colour(0xff201e1b));
        setColour(juce::PopupMenu::textColourId, pal::ink);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, pal::amber);
        setColour(juce::PopupMenu::highlightedTextColourId, Colour(0xff1d1704));
        setColour(juce::TextEditor::backgroundColourId, Colour(0xff0d0e10));
        setColour(juce::TextEditor::textColourId, pal::ink);
        setColour(juce::TextEditor::outlineColourId, Colour(0xff000000));
        setColour(juce::TextEditor::focusedOutlineColourId, pal::amber);
        setColour(juce::CaretComponent::caretColourId, pal::amber);
        setColour(juce::ScrollBar::thumbColourId, Colour(0xff5b5e64));
        setColour(juce::ScrollBar::backgroundColourId, Colour(0xff0c0d0e));
    }

    juce::Font getTextButtonFont(juce::TextButton&, int) override { return font(13.0f, true); }
    juce::Font getComboBoxFont(juce::ComboBox&) override { return font(13.0f); }
    juce::Font getPopupMenuFont() override { return font(13.0f); }

    void drawButtonBackground(Graphics& g, juce::Button& b, const Colour&, bool hover, bool down) override {
        auto r = b.getLocalBounds().toFloat().reduced(0.5f);
        const bool on = b.getToggleState();
        Colour top=on?Colour(0xffc9ab7f):Colour(0xff34312c),bot=on?Colour(0xff8b7153):Colour(0xff201e1b);
        if(b.getProperties().contains("quiet")) {top=on?Colour(0xff494238):Colour(0xff282522);bot=Colour(0xff191714);}
        if (down) std::swap(top, bot);
        if (hover && !down) { top = top.brighter(0.12f); bot = bot.brighter(0.08f); }
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(r.translated(0.0f, 1.2f), 3.0f);
        g.setGradientFill(juce::ColourGradient::vertical(top, r.getY(), bot, r.getBottom()));
        g.fillRoundedRectangle(r, 3.0f);
        g.setColour(Colour(0xff000000)); g.drawRoundedRectangle(r, 3.0f, 1.0f);
        if (!on) { g.setColour(juce::Colours::white.withAlpha(0.16f)); g.drawHorizontalLine((int) r.getY() + 1, r.getX() + 3.0f, r.getRight() - 3.0f); }
        if (on && b.getClickingTogglesState() && !b.getProperties().contains("plain")) {   // lit LED on latching buttons
            const float cx = r.getX() + 9.0f, cy = r.getCentreY();
            g.setColour(pal::amber.withAlpha(0.35f)); g.fillEllipse(cx - 5.0f, cy - 5.0f, 10.0f, 10.0f);
            g.setColour(pal::amber); g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
        }
    }
    void drawButtonText(Graphics& g, juce::TextButton& b, bool, bool) override {
        g.setFont(font(11.5f, true));
        g.setColour(b.getToggleState()&&!b.getProperties().contains("quiet") ? Colour(0xff20180f) : pal::ink);
        auto r = b.getLocalBounds();
        if (b.getToggleState() && b.getClickingTogglesState() && !b.getProperties().contains("plain")) r = r.withTrimmedLeft(12);
        g.drawFittedText(b.getButtonText(), r, juce::Justification::centred, 1);
    }

    void drawComboBox(Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override {
        auto r = juce::Rectangle<float>(0, 0, (float) w, (float) h);
        g.setColour(Colour(0xff0d0e10)); g.fillRoundedRectangle(r, 3.0f);
        g.setColour(Colour(0xff000000)); g.drawRoundedRectangle(r.reduced(0.5f), 3.0f, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.08f)); g.drawHorizontalLine(h - 1, 3.0f, (float) w - 3.0f);
        juce::Path p;
        p.addTriangle((float) w - 18.0f, h * 0.42f, (float) w - 8.0f, h * 0.42f, (float) w - 13.0f, h * 0.62f);
        g.setColour(pal::amber); g.fillPath(p);
    }

    void drawRotarySlider(Graphics& g,int x,int y,int w,int h,float pos,float a0,float a1,juce::Slider& s) override {
        const float size=float(juce::jmin(w,h)),cx=x+w*.5f,cy=y+h*.5f,R=size*.385f;
        const float ang=a0+pos*(a1-a0);
        const Colour cap((juce::uint32)(int)s.getProperties().getWithDefault("cap",(int)0xffc9c0ad));
        const Colour ptr((juce::uint32)(int)s.getProperties().getWithDefault("ptr",(int)0xff1b1815));
        const bool light=bool(s.getProperties().getWithDefault("lightFace",false));
        const auto tick=light?Colour(0xff403a30):Colour(0xffe6ddc6);
        for(int n=0;n<=10;++n){
            const float a=a0+(a1-a0)*n/10.f,rad=R*1.19f;
            const float dot=n%5==0?1.45f:1.f;
            g.setColour(tick.withAlpha(n%5==0?.94f:.65f));
            g.fillEllipse(cx+std::sin(a)*rad-dot,cy-std::cos(a)*rad-dot,2*dot,2*dot);
        }
        g.setColour(juce::Colours::black.withAlpha(.30f));g.fillEllipse(cx-R-1,cy-R+3,2*R+2,2*R+2);
        g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff5b554a),cy-R,Colour(0xff100f0e),cy+R));
        g.fillEllipse(cx-R,cy-R,2*R,2*R);
        g.setColour(Colour(0xff0b0a09));g.drawEllipse(cx-R,cy-R,2*R,2*R,1.f);
        const float r=R*.91f;
        g.drawImage(machinedCap(cap),juce::Rectangle<float>(cx-r,cy-r,2*r,2*r));
        const float sn=std::sin(ang),cs=-std::cos(ang);
        g.setColour(juce::Colours::white.withAlpha(.32f));
        g.drawLine(cx+sn*r*.34f+.7f,cy+cs*r*.34f+.6f,cx+sn*r*.91f+.7f,cy+cs*r*.91f+.6f,2.f);
        g.setColour(ptr);g.drawLine(cx+sn*r*.34f,cy+cs*r*.34f,cx+sn*r*.91f,cy+cs*r*.91f,2.2f);
        if(s.hasKeyboardFocus(false)){g.setColour(pal::amber);g.drawEllipse(cx-R*1.1f,cy-R*1.1f,R*2.2f,R*2.2f,1.f);}
    }

    void drawScrollbar(Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart, int thumbSize, bool over, bool down) override {
        g.setColour(Colour(0xff0c0d0e)); g.fillRect(x, y, w, h);
        auto t = vertical ? juce::Rectangle<int>(x + 2, thumbStart, w - 4, thumbSize) : juce::Rectangle<int>(thumbStart, y + 2, thumbSize, h - 4);
        g.setColour(Colour(down ? 0xff7a7d84 : over ? 0xff6a6d74 : 0xff55585e)); g.fillRoundedRectangle(t.toFloat(), 3.0f);
    }
};

} // namespace skl::ui
