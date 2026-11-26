#pragma once
// Skelliere look: blacked-out analog gear. One chassis, graphite brushed faceplates, knurled knobs,
// printed legends and cream needle meters. All colours live here so the whole plugin can be re-skinned in one place.
#include <JuceHeader.h>
#include "Modules.h"
#include "SkelliereKnobData.h"
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
    // 512 px tile of long, irregular horizontal fibres plus a fine isotropic tooth. Fibre
    // length, strength and position are random and wrap at the seam, so there is no row
    // banding and no visible repeat at panel scale.
    static const juce::Image img = [] {
        constexpr int n = 512;
        juce::Image im(juce::Image::ARGB, n, n, true);
        Graphics g(im);
        juce::Random rng(1234);
        for (int i = 0; i < 2600; ++i) {
            const int y = rng.nextInt(n), x = rng.nextInt(n), len = 8 + rng.nextInt(68);
            g.setColour((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha(0.009f + rng.nextFloat() * 0.018f));
            g.fillRect(x, y, len, 1);
            if (x + len > n) g.fillRect(0, y, x + len - n, 1);
        }
        for (int i = 0; i < 21000; ++i) {
            g.setColour((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha(0.018f + rng.nextFloat() * 0.032f));
            g.fillRect(rng.nextInt(n), rng.nextInt(n), 1, 1);
        }
        return im;
    }();
    return img;
}

// Fine peened metal tooth, separate from horizontal brushed fibres. Cached once,
// composed into the static 2x faceplate raster, never on the audio thread.
inline const juce::Image& peenedMetal() {
    static const juce::Image img=[]{
        constexpr int n=256;juce::Image im(juce::Image::ARGB,n,n,true);
        juce::Image::BitmapData data(im,juce::Image::BitmapData::writeOnly);
        juce::Random rng(32671);
        for(int y=0;y<n;++y)for(int x=0;x<n;++x){
            const float v=(rng.nextFloat()+rng.nextFloat()-1.f);
            data.setPixelColour(x,y,(v>=0?juce::Colours::white:juce::Colours::black).withAlpha(std::abs(v)*.19f));
        }
        return im;
    }();
    return img;
}
inline void paintPeened(Graphics& g,juce::Rectangle<float> r,float opacity){
    g.setTiledImageFill(peenedMetal(),0,0,opacity);g.fillRect(r);
}
inline void paintBrushed(Graphics& g, juce::Rectangle<float> r, float opacity = 1.0f) {
    g.setTiledImageFill(brushedImage(), 0, 0, opacity);
    g.fillRect(r);
}

// Original machined cap, drawn once per metal finish at high resolution. Light comes from the upper left.
// Zones from the centre outwards: shallow centre dish, finely brushed concentric face with a soft
// directional gradient and a gentle two-lobe anisotropic sheen, an engraved groove with a lit lower
// lip, a chamfered bevel that carries the strongest directional highlight, and a dark outer edge.
inline const juce::Image& machinedCap(Colour colour) {
    static std::map<juce::uint32,juce::Image> cache;
    const auto key=colour.getARGB();
    if(auto it=cache.find(key);it!=cache.end())return it->second;
    constexpr int n=256;
    juce::Image image(juce::Image::ARGB,n,n,true);
    juce::Image::BitmapData pixels(image,juce::Image::BitmapData::writeOnly);
    const float lightAngle=-1.94f;   // atan2(-1,-1): up and to the left
    const bool darkCap=colour.getPerceivedBrightness()<.25f;
    auto hash=[](int v){unsigned h=(unsigned)v*2654435761u;h^=h>>15;h*=2246822519u;h^=h>>13;return (h&1023u)/1023.f;};
    auto smooth=[](float a,float b,float x){const float t=juce::jlimit(0.f,1.f,(x-a)/(b-a));return t*t*(3-2*t);};
    for(int y=0;y<n;++y)for(int x=0;x<n;++x){
        const float dx=(x+.5f-n*.5f)/(n*.5f),dy=(y+.5f-n*.5f)/(n*.5f);
        const float r=std::sqrt(dx*dx+dy*dy);
        if(r>=1.f)continue;
        const float a=std::atan2(dy,dx),lit=std::cos(a-lightAngle);
        // Face: concentric lathe grain (value noise on radius, strength varies slowly with angle).
        const float ring=hash(int(r*300.f))*.5f+hash(int(r*95.f)+7)*.5f-.5f;
        const float grain=ring*(.085f+.018f*std::cos(2*a-lightAngle*2));
        // Narrow bright anisotropic reflection with a wide dark opposing lobe.
        // Fade the angular response in at the spindle to avoid a hard pinwheel.
        const float directional=std::cos(2*(a-lightAngle));
        const float specular=std::pow(juce::jmax(0.f,directional),3.f);
        float shine=.53f+smooth(.025f,.14f,r)*(.23f*directional+.26f*specular+.16f*r*lit)+grain;
        shine-=.055f*(1.f-smooth(.0f,.13f,r));                              // shallow centre dish
        // Engraved groove at r~.80 with a lit lower lip, then the chamfer.
        const float groove=smooth(.855f,.872f,r)*(1.f-smooth(.881f,.891f,r));
        shine=shine*(1.f-groove)+.38f*groove;
        const float lip=smooth(.891f,.902f,r)*(1.f-smooth(.911f,.921f,r));
        shine=shine*(1.f-lip)+(.80f-.30f*lit)*lip;
        const float bevel=smooth(.921f,.931f,r)*(1.f-smooth(.968f,.98f,r));
        shine=shine*(1.f-bevel)+(.68f+.43f*lit)*bevel;
        const float edge=smooth(.965f,.975f,r);
        shine=shine*(1.f-edge)+(.16f+.10f*juce::jmax(0.f,lit))*edge;
        const float alpha=juce::jlimit(0.f,1.f,(1-r)*n*.5f);
        // Dark caps have no headroom to darken, so they need highlights from a lower threshold to show their form.
        const float pivot=darkCap?.55f:.62f;
        const auto tint=colour.interpolatedWith(shine>pivot?juce::Colours::white:juce::Colours::black,
            shine>pivot?juce::jlimit(0.f,darkCap?.50f:.60f,(shine-pivot)*(darkCap?.85f:1.55f)):juce::jlimit(0.f,darkCap?.40f:.72f,(pivot-shine)*(darkCap?.50f:1.60f)));
        pixels.setPixelColour(x,y,tint.withAlpha(alpha));
    }
    return cache.emplace(key,image).first->second;
}

// Static knob body: soft ground shadow, contact occlusion, dark knurled sidewall with lit flutes,
// rim highlight and the machined cap. Rendered once per cap colour; drawRotarySlider only adds the
// scale marks and the rotating pointer. The image spans +/-1.3 knob radii so the shadow fits.
// Necromancer ReanimateDial's original assets, adapted to rack-sized controls.
// Both images stay unchanged in the binary. The well treatment mirrors the
// source hero renderer; no substitute skull, no painted-over face.
inline const juce::Image& necroCap(){
    static const auto image=juce::ImageFileFormat::loadFrom(
        SkelliereKnobData::knob_cap_big_png,SkelliereKnobData::knob_cap_big_pngSize);
    return image;
}
inline const juce::Image& necroWell(){
    static const auto image=[]{
        auto result=juce::ImageFileFormat::loadFrom(
            SkelliereKnobData::dial_well_png,SkelliereKnobData::dial_well_pngSize).createCopy();
        if(result.isValid()){
            Graphics g(result);
            g.addTransform(juce::AffineTransform::scale(result.getWidth()/440.f,result.getHeight()/440.f));
            juce::Path quiet;quiet.addEllipse(32.f,32.f,376.f,376.f);
            g.setColour(Colour(0xff101418).withAlpha(.94f));
            g.strokePath(quiet,juce::PathStrokeType(38.f));
            g.setColour(Colour(0xff9ba4aa).withAlpha(.28f));
            g.drawEllipse(11.5f,11.5f,417.f,417.f,1.f);
        }
        return result;
    }();
    return image;
}
inline bool necromancerArtworkReady(){
    return necroCap().isValid()&&necroWell().isValid()&&necroCap().getWidth()==256&&necroWell().getWidth()==440;
}
// The recessed collar is raster-cached, including contact occlusion. The live
// renderer rotates the actual hero cap separately, just as ReanimateDial does.
inline const juce::Image& heroKnobWell(){
    static const juce::Image cached=[]{
        constexpr int n=320;const float c=n*.5f,rad=137.f;
        juce::Image im(juce::Image::ARGB,n,n,true);Graphics g(im);
        for(int i=0;i<12;++i){
            const float rr=rad+float(i)*1.55f;
            g.setColour(juce::Colours::black.withAlpha(.055f));
            g.fillEllipse(c-rr,c-rr+6.f,2*rr,2*rr);
        }
        g.setGradientFill(juce::ColourGradient(Colour(0xff625c50),c-rad,c-rad,
            Colour(0xff08090a),c+rad,c+rad,false));
        g.fillEllipse(c-rad,c-rad,2*rad,2*rad);
        const float wr=rad-3.f;
        if(necroWell().isValid()){
            juce::Graphics::ScopedSaveState save(g);
            juce::Path clip;clip.addEllipse(c-wr,c-wr,2*wr,2*wr);
            g.reduceClipRegion(clip);g.setOpacity(1.f);
            g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
            g.drawImage(necroWell(),juce::Rectangle<float>(c-wr,c-wr,2*wr,2*wr));
        }
        g.setColour(Colour(0xff0b0d0e));g.drawEllipse(c-rad,c-rad,2*rad,2*rad,1.8f);
        juce::Path rim;rim.addCentredArc(c,c,rad-1.f,rad-1.f,0.f,-1.5f,.45f,true);
        g.setColour(Colour(0xffebdcc3).withAlpha(.52f));g.strokePath(rim,juce::PathStrokeType(1.6f));
        return im;
    }();
    return cached;
}

inline void paintMonogram(Graphics& g,juce::Rectangle<float> r,Colour ink) {
    const float x=r.getX(),y=r.getY(),w=r.getWidth(),h=r.getHeight();
    juce::Path shield;shield.startNewSubPath(x+w*.5f,y);
    shield.lineTo(x+w*.91f,y+h*.16f);shield.lineTo(x+w*.91f,y+h*.82f);
    shield.lineTo(x+w*.5f,y+h);shield.lineTo(x+w*.09f,y+h*.82f);shield.lineTo(x+w*.09f,y+h*.16f);shield.closeSubPath();
    g.setColour(ink);g.strokePath(shield,juce::PathStrokeType(1.1f));
    juce::Path mark;mark.startNewSubPath(x+w*.68f,y+h*.22f);
    mark.lineTo(x+w*.36f,y+h*.16f);mark.lineTo(x+w*.36f,y+h*.45f);
    mark.lineTo(x+w*.64f,y+h*.58f);mark.lineTo(x+w*.64f,y+h*.83f);
    mark.lineTo(x+w*.31f,y+h*.71f);
    g.strokePath(mark,juce::PathStrokeType(std::max(1.2f,w*.105f),juce::PathStrokeType::mitered));
}

inline void paintScrew(Graphics& g, float cx, float cy, float r, float turn) {
    g.setColour(juce::Colours::black.withAlpha(0.50f));                       // recess
    g.fillEllipse(cx - r - 0.9f, cy - r - 0.3f, 2 * r + 1.8f, 2 * r + 1.8f);
    juce::ColourGradient head(Colour(0xffe9d9bc), cx - r * 0.5f, cy - r * 0.6f, Colour(0xff4a4338), cx + r * 0.8f, cy + r * 0.9f, false);
    head.addColour(0.42, Colour(0xffb9ab94));head.addColour(.73,Colour(0xff777061));
    g.setGradientFill(head);
    g.fillEllipse(cx - r, cy - r, 2 * r, 2 * r);
    g.setColour(Colour(0xff1f1e1c));
    g.drawEllipse(cx - r, cy - r, 2 * r, 2 * r, 0.8f);
    juce::Path cross;
    cross.addRectangle(-r * 0.70f, -0.8f, r * 1.4f, 1.6f);
    cross.addRectangle(-0.8f, -r * 0.70f, 1.6f, r * 1.4f);
    g.setColour(Colour(0xff2a2927));
    g.fillPath(cross, juce::AffineTransform::rotation(turn).translated(cx, cy));
    g.setColour(juce::Colours::white.withAlpha(0.35f));                       // pin-point specular
    g.fillEllipse(cx - r * 0.55f, cy - r * 0.62f, r * 0.45f, r * 0.35f);
}

// The needle meter used by the compressor, de-esser and limiter. value is gain reduction in dB (0 to 20).
inline void paintVU(Graphics& g, juce::Rectangle<float> r, float value, bool outputPeak=false) {
    g.setColour(juce::Colours::black.withAlpha(.62f));g.fillRoundedRectangle(r.translated(0,3),10);
    juce::ColourGradient bezel(Colour(0xff8b7960),r.getX(),r.getY(),Colour(0xff151310),r.getRight(),r.getBottom(),false);
    bezel.addColour(.24,Colour(0xff39342b));bezel.addColour(.78,Colour(0xff090807));g.setGradientFill(bezel);
    g.fillRoundedRectangle(r,9);                                              // dark machined housing
    g.setColour(Colour(0xff9c8b6b).withAlpha(.55f));g.drawRoundedRectangle(r.reduced(.7f),8,1.2f);
    g.setColour(Colour(0xff000000).withAlpha(.55f));g.drawRoundedRectangle(r.reduced(2.2f),5,1.0f);
    const auto inner=r.reduced(6);
    g.setColour(Colour(0xff0b0a09));g.fillRoundedRectangle(inner,11);          // recess
    g.setColour(juce::Colours::white.withAlpha(.10f));                       // lit lower lip of the recess
    g.drawHorizontalLine(int(inner.getBottom())-1,inner.getX()+6,inner.getRight()-6);
    for(int i=0;i<4;++i)                                                      // bezel fasteners
        paintScrew(g,i%2?r.getRight()-6.7f:r.getX()+6.7f,i<2?r.getY()+6.7f:r.getBottom()-6.7f,3.7f,.7f*i);
    auto face=inner.reduced(3.0f);
    g.setGradientFill(juce::ColourGradient(Colour(0xfffff0cd),face.getCentreX(),face.getCentreY()-face.getHeight()*.06f,Colour(0xffbe8341),face.getX(),face.getBottom(),true));
    g.fillRoundedRectangle(face,10);                                           // warm lamp-lit paper
    {   // soft shadow cast by the bezel along the top and sides of the face
        juce::Graphics::ScopedSaveState shade(g);
        juce::Path facePath;facePath.addRoundedRectangle(face.getX(),face.getY(),face.getWidth(),face.getHeight(),10.f);
        g.reduceClipRegion(facePath);
        g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::black.withAlpha(.34f),face.getY(),juce::Colours::transparentBlack,face.getY()+11.f));
        g.fillRect(face);
        g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(.16f),face.getX(),face.getCentreY(),juce::Colours::transparentBlack,face.getX()+9.f,face.getCentreY(),false));
        g.fillRect(face);
        g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(.16f),face.getRight(),face.getCentreY(),juce::Colours::transparentBlack,face.getRight()-9.f,face.getCentreY(),false));
        g.fillRect(face);
    }
    g.setColour(Colour(0xff6f4b27));g.drawRoundedRectangle(face,10,1.2f);

    juce::Graphics::ScopedSaveState save(g);
    {   juce::Path clipFace;clipFace.addRoundedRectangle(face.getX(),face.getY(),face.getWidth(),face.getHeight(),10.f);
        g.reduceClipRegion(clipFace);   }
    // The meter is much wider than it is tall. Fit the arc in both axes,
    // keeping its full scale, labels and hub inside the actual face.
    const float cx=face.getCentreX(),cy=face.getBottom()-18.f;
    const float sc = face.getWidth() / 103.0f;
    const float sy = juce::jmax(0.1f, (face.getHeight() - 30.f) / 57.f);
    const float labelSize=juce::jlimit(9.f,11.5f,8.f*sc);
    auto pt = [&](float rad, float v) {
        const float a = juce::degreesToRadians(-50.0f + v * 5.0f);
        return juce::Point<float>(cx + rad * sc * std::sin(a), cy - rad * sy * std::cos(a));
    };
    const float redStart=outputPeak?20.f*20.f/24.f:12.f;
    auto tick=[&](float v,int label,bool major){
        g.setColour(v>=redStart?pal::vuRed:pal::vuInk);
        g.drawLine(juce::Line<float>(pt(57.f,v),pt(major?49.f:53.f,v)),major?1.4f:.9f);
        if(major){
            g.setFont(font(labelSize,true));
            g.drawText((label>0&&outputPeak?"+":"")+juce::String(label),
                juce::Rectangle<float>(0,0,24,13).withCentre(pt(41.f,v)),juce::Justification::centred,false);
        }
    };
    if(outputPeak)for(int db=-20;db<=4;db+=2)tick((db+20.f)*20.f/24.f,db,db%4==0);
    else for(int db=0;db<=20;db+=2)tick(float(db),db,db%4==0);
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
    auto legend=face.reduced(9.f,0.f).removeFromBottom(15.f);
    g.drawText(outputPeak?"PEAK dBFS":"GR dB",legend,juce::Justification::centredLeft,false);
    g.drawText(outputPeak?(value<=-59.f?juce::String("-inf"):juce::String(value,1)+" dBFS"):
        juce::String(value<.05f?0.f:-value,1)+" dB",legend,juce::Justification::centredRight,false);

    const float v=juce::jlimit(0.f,20.f,outputPeak?(value+20.f)*20.f/24.f:value);
    const auto tip=pt(52.0f, v);
    g.setColour(juce::Colours::black.withAlpha(0.22f));                       // needle shadow on the paper
    g.drawLine(juce::Line<float>(juce::Point<float>(cx + 1.6f, cy + 1.8f), tip + juce::Point<float>(1.6f, 1.8f)), 1.3f);
    g.setColour(pal::vuRed);
    g.drawLine(juce::Line<float>(juce::Point<float>(cx, cy), tip), 1.5f);
    {   const float dx=tip.x-cx,dy=tip.y-cy,len=std::sqrt(dx*dx+dy*dy)+1e-4f;   // short counterweight tail
        g.drawLine(juce::Line<float>(juce::Point<float>(cx,cy),juce::Point<float>(cx-dx/len*6.f,cy-dy/len*6.f)),2.4f);   }
    g.setColour(Colour(0xff0e0e0d)); g.fillEllipse(cx - 3.5f, cy - 3.5f, 7.0f, 7.0f);   // pivot
    g.setGradientFill(juce::ColourGradient(Colour(0xff8a867d),cx-2.2f,cy-2.6f,Colour(0xff1c1b19),cx+2.6f,cy+3.0f,false));
    g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
    g.setColour(juce::Colours::white.withAlpha(.45f)); g.fillEllipse(cx - 1.9f, cy - 2.4f, 1.8f, 1.2f);
    // Curved lens reflection: a soft band whose lower edge bows downward, rather than a flat rectangle.
    juce::Path glass;
    glass.startNewSubPath(face.getX(), face.getY());
    glass.lineTo(face.getRight(), face.getY());
    glass.lineTo(face.getRight(), face.getY() + face.getHeight() * .22f);
    glass.quadraticTo(face.getCentreX(), face.getY() + face.getHeight() * .50f, face.getX(), face.getY() + face.getHeight() * .22f);
    glass.closeSubPath();
    g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::white.withAlpha(0.17f), face.getY(), juce::Colours::white.withAlpha(0.015f), face.getY() + face.getHeight() * .46f));
    g.fillPath(glass);
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
        setColour(juce::TextEditor::backgroundColourId, Colour(0xff171512));
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

    void fillTextEditorBackground(Graphics& g,int w,int h,juce::TextEditor&) override {
        auto r=juce::Rectangle<float>(0,0,float(w),float(h)).reduced(.5f);
        g.setColour(Colour(0xff171512));g.fillRoundedRectangle(r,5.f);
    }
    void drawTextEditorOutline(Graphics& g,int w,int h,juce::TextEditor& editor) override {
        auto r=juce::Rectangle<float>(0,0,float(w),float(h)).reduced(.6f);
        g.setColour(editor.hasKeyboardFocus(true)?pal::amber:Colour(0xff827664).withAlpha(.65f));
        g.drawRoundedRectangle(r,5.f,1.f);
        if(editor.getProperties().contains("skellSearch")){
            const float cy=h*.5f;g.setColour(pal::inkDim);
            g.drawEllipse(10.f,cy-6.f,10.f,10.f,1.2f);
            g.drawLine(18.f,cy+2.f,23.f,cy+7.f,1.2f);
        }
    }
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
        g.setColour(on?Colour(0xffe9cba0):Colour(0xff93816a).withAlpha(.75f));g.drawRoundedRectangle(r,4.0f,.9f);
        g.setColour(juce::Colours::black.withAlpha(.48f));g.drawRoundedRectangle(r.reduced(2.f),3.f,.65f);
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
        g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff080706),r.getY(),Colour(0xff28221a),r.getBottom()));g.fillRoundedRectangle(r,5.f);
        g.setColour(juce::Colours::white.withAlpha(.045f));g.fillRoundedRectangle(r.reduced(3.f).withHeight(r.getHeight()*.35f),3.f);
        g.setColour(Colour(0xff847766).withAlpha(.75f));g.drawRoundedRectangle(r.reduced(.6f),5.f,1.f);
        g.setColour(juce::Colours::black.withAlpha(.65f));g.drawRoundedRectangle(r.reduced(2.f),4.f,.8f);
        g.setColour(juce::Colours::white.withAlpha(0.13f)); g.drawHorizontalLine(h - 2, 4.0f, (float) w - 4.0f);
        juce::Path p;
        p.addTriangle((float) w - 18.0f, h * 0.42f, (float) w - 8.0f, h * 0.42f, (float) w - 13.0f, h * 0.62f);
        g.setColour(pal::amber); g.fillPath(p);
    }

    void drawRotarySlider(Graphics& g,int x,int y,int w,int h,float pos,float a0,float a1,juce::Slider& s) override {
        const float size=float(juce::jmin(w,h)),cx=x+w*.5f,cy=y+h*.5f;
        if(size<2.f)return;
        const bool light=bool(s.getProperties().getWithDefault("lightFace",false));
        const bool utility=size<61.f;
        const bool hero=size>=115.f;
        const float p=juce::jlimit(0.f,1.f,pos);
        const float angle=a0+p*(a1-a0);
        const float outer=size*.425f;
        const float capR=outer*(utility?.81f:hero?.72f:.76f);
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.setOpacity(1.f);
        const float imageR=outer*320.f/274.f;
        g.drawImage(heroKnobWell(),juce::Rectangle<float>(cx-imageR,cy-imageR,2*imageR,2*imageR));
        // Dark backing preserves the original skull cutout rather than allowing
        // a bright generic cap to shine through the engraving.
        g.setColour(Colour(0xff07090a));
        g.fillEllipse(cx-capR,cy-capR+outer*.035f,capR*2,capR*2);
        {
            juce::Graphics::ScopedSaveState save(g);
            // Exact source ReanimateDial rotation, adapted only to rack size.
            g.addTransform(juce::AffineTransform::rotation(p*juce::MathConstants<float>::pi*1.5f-1.18f,cx,cy));
            juce::Path clip;clip.addEllipse(cx-capR,cy-capR,capR*2,capR*2);
            g.reduceClipRegion(clip);g.setOpacity(1.f);
            const juce::Rectangle<float> rect(cx-capR,cy-capR,capR*2,capR*2);
            if(necroCap().isValid())g.drawImage(necroCap(),rect);
            else g.drawImage(machinedCap(Colour(0xffaaa7a1)),rect);
        }
        // Source-style radial pointer outside the rotating cap, so it never
        // covers the skull. Champagne instead of the hero plugin's neon arc.
        const auto mark=Colour(0xfff2dcb4);
        const float sn=std::sin(angle),cs=-std::cos(angle);
        const float start=capR+outer*.04f,end=outer*.91f;
        g.setColour(juce::Colours::black.withAlpha(.9f));
        g.drawLine(cx+sn*start,cy+cs*start,cx+sn*end,cy+cs*end,juce::jmax(2.f,outer*.075f));
        g.setColour(mark);
        g.drawLine(cx+sn*start,cy+cs*start,cx+sn*end,cy+cs*end,juce::jmax(1.2f,outer*.043f));
        // Physical dial scale, sparse at utility sizes and finely engraved
        // around medium/hero controls. The active position is the actual value.
        const auto tick=light?Colour(0xff3a3024):Colour(0xffe4d6bc);
        const int marks=utility?10:hero?30:20;
        for(int k=0;k<=marks;++k){
            const float a=a0+(a1-a0)*k/marks,sine=std::sin(a),cosine=-std::cos(a);
            const bool major=k%(marks/5)==0;
            const float inner=outer*(major?1.035f:1.075f);
            const float far=outer*1.13f;
            g.setColour(tick.withAlpha(major?.93f:.48f));
            g.drawLine(cx+sine*inner,cy+cosine*inner,cx+sine*far,cy+cosine*far,major?1.25f:.65f);
        }
        if(s.isMouseOverOrDragging()||s.hasKeyboardFocus(false)){
            g.setColour(pal::amber.withAlpha(s.hasKeyboardFocus(false)?.9f:.4f));
            g.drawEllipse(cx-outer,cy-outer,2*outer,2*outer,1.f);
        }
    }

    void drawScrollbar(Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart, int thumbSize, bool over, bool down) override {
        g.setColour(Colour(0xff151310)); g.fillRect(x, y, w, h);
        auto t = vertical ? juce::Rectangle<int>(x + 2, thumbStart, w - 4, thumbSize) : juce::Rectangle<int>(thumbStart, y + 2, thumbSize, h - 4);
        g.setColour(Colour(down ? 0xffaa9c82 : over ? 0xff8f806b : 0xff5f584b)); g.fillRoundedRectangle(t.toFloat(), 3.0f);
    }
};

} // namespace skl::ui
