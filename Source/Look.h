#pragma once
// Skelliere look: blacked-out analog gear. One chassis, graphite brushed faceplates, knurled knobs,
// printed legends and cream needle meters. All colours live here so the whole plugin can be re-skinned in one place.
#include <JuceHeader.h>
#include "Modules.h"
#include "SkelliereKnobData.h"
#include "AnalogHardwareMath.h"
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

// Seamless, anisotropic microstructure without the previous repeating lines.
inline const juce::Image& brushedImage() {
    static const juce::Image img=[]{
        constexpr int n=512;juce::Image im(juce::Image::ARGB,n,n,true);
        juce::Image::BitmapData pixels(im,juce::Image::BitmapData::writeOnly);
        auto noise=[](unsigned x,unsigned y){
            unsigned h=x*0x9e3779b9u+y*0x85ebca6bu+0x12345u;
            h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;h^=h>>16;
            return float(h&65535u)/65535.f-.5f;
        };
        auto stripe=[&](int x,int y,int span){
            const int count=n/span,k=x/span;float t=float(x%span)/float(span);t=t*t*(3.f-2.f*t);
            return noise(unsigned(k),unsigned(y))*(1.f-t)+noise(unsigned((k+1)%count),unsigned(y))*t;
        };
        for(int y=0;y<n;++y)for(int x=0;x<n;++x){
            const float v=.095f*stripe(x,y,64)+.045f*stripe(x,y,8)+.037f*noise(unsigned(x),unsigned(y));
            pixels.setPixelColour(x,y,(v>0?juce::Colours::white:juce::Colours::black).withAlpha(std::abs(v)));
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
            data.setPixelColour(x,y,(v>=0?juce::Colours::white:juce::Colours::black).withAlpha(std::abs(v)*.10f));
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

// Common physical enclosure: broad convex edge, directional reflected light,
// recessed contact lip. Rendered into the callers' static image caches.
inline void paintHardwarePlate(Graphics& g,juce::Rectangle<float> r,Colour metal,bool brushed,float radius=9.f,float bevel=4.f){
    if(r.getWidth()<=bevel*2||r.getHeight()<=bevel*2)return;
    g.setColour(Colour(0xff080706));g.fillRoundedRectangle(r,radius);
    auto rim=r.reduced(.75f);
    juce::ColourGradient edge(metal.interpolatedWith(Colour(0xffe6d7bc),.40f),rim.getX(),rim.getY(),
        metal.interpolatedWith(Colour(0xff080706),.80f),rim.getRight(),rim.getBottom(),false);
    edge.addColour(.22,metal.interpolatedWith(Colour(0xffc7bbab),.15f));
    edge.addColour(.58,metal.interpolatedWith(Colour(0xff100e0c),.46f));
    g.setGradientFill(edge);g.fillRoundedRectangle(rim,radius-.5f);
    g.setColour(Colour(0xff0b0907).withAlpha(.22f));g.drawRoundedRectangle(rim.reduced(1.9f),radius-1.5f,.65f);
    const auto face=r.reduced(bevel);
    const float rr=juce::jmax(2.f,radius-bevel*.70f);
    g.setColour(juce::Colours::black.withAlpha(.62f));g.fillRoundedRectangle(face.expanded(.6f),rr+.5f);
    {
        juce::Graphics::ScopedSaveState save(g);
        juce::Path clip;clip.addRoundedRectangle(face.getX(),face.getY(),face.getWidth(),face.getHeight(),rr);g.reduceClipRegion(clip);
        const bool light=metal.getPerceivedBrightness()>.46f;
        juce::ColourGradient body(metal.interpolatedWith(Colour(0xfff8e9d6),light?.13f:.065f),face.getX(),face.getY(),
            metal.interpolatedWith(Colour(0xff0c0a08),light?.14f:.28f),face.getRight(),face.getBottom(),false);
        body.addColour(.44,metal);g.setGradientFill(body);g.fillRect(face);
        g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(light?.15f:.075f),
            face.getX()+face.getWidth()*.22f,face.getY()+face.getHeight()*.14f,juce::Colours::transparentWhite,
            face.getRight(),face.getCentreY(),true));g.fillRect(face);
        if(brushed)paintBrushed(g,face,light?.85f:.64f);else paintPeened(g,face,.67f);
        // Large reflected softbox and darker return, clipped to the metal.
        g.setGradientFill(juce::ColourGradient(Colour(0xffffe9c7).withAlpha(light?.055f:.018f),face.getX()+face.getWidth()*.18f,face.getY()+face.getHeight()*.18f,juce::Colours::transparentWhite,face.getRight(),face.getBottom(),true));g.fillRect(face);
        g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(light?.06f:.18f),face.getRight(),face.getBottom(),juce::Colours::transparentBlack,face.getX(),face.getCentreY(),true));g.fillRect(face);
        g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(.18f),face.getRight(),face.getCentreY(),juce::Colours::transparentBlack,face.getRight()-7.f,face.getCentreY(),false));g.fillRect(face);
        g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::transparentBlack,face.getBottom()-8.f,
            juce::Colours::black.withAlpha(.28f),face.getBottom()));g.fillRect(face);
    }
    // Directional reflection and occlusion: no uniform bright hairline.
    juce::Path upper;upper.startNewSubPath(rim.getX()+.9f,rim.getBottom()-radius);
    upper.lineTo(rim.getX()+.9f,rim.getY()+radius);
    upper.quadraticTo(rim.getX()+.9f,rim.getY()+.9f,rim.getX()+radius,rim.getY()+.9f);
    upper.lineTo(rim.getRight()-radius,rim.getY()+.9f);
    g.setColour(Colour(0xffffe6c0).withAlpha(.45f));g.strokePath(upper,juce::PathStrokeType(.85f,juce::PathStrokeType::curved));
    juce::Path lower;lower.startNewSubPath(rim.getRight()-1.f,rim.getY()+radius);
    lower.lineTo(rim.getRight()-1.f,rim.getBottom()-radius);
    lower.quadraticTo(rim.getRight()-1.f,rim.getBottom()-1.f,rim.getRight()-radius,rim.getBottom()-1.f);
    lower.lineTo(rim.getX()+radius,rim.getBottom()-1.f);
    g.setColour(juce::Colours::black.withAlpha(.72f));g.strokePath(lower,juce::PathStrokeType(1.6f,juce::PathStrokeType::curved));
    // A narrow contact seam adds depth without moving any control or slot.
    g.setColour(Colour(0xff070605).withAlpha(.42f));g.drawRoundedRectangle(face.expanded(.9f),rr+.7f,1.25f);
    g.setColour(Colour(0xffffefda).withAlpha(.18f));g.drawRoundedRectangle(face,rr,.7f);
}
inline void paintRecessedWindow(Graphics& g,juce::Rectangle<float> r,float radius=7.f){
    g.setColour(juce::Colours::black.withAlpha(.65f));g.fillRoundedRectangle(r.translated(0,1.8f),radius+1);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff080706),r.getY(),Colour(0xff25211c),r.getBottom()));
    g.fillRoundedRectangle(r,radius);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff171310),r.getY(),Colour(0xff8b7960),r.getBottom()));
    g.drawRoundedRectangle(r.reduced(.6f),radius,.9f);
    g.setColour(juce::Colours::black.withAlpha(.72f));g.drawRoundedRectangle(r.reduced(2.f),radius-1.f,.75f);
}

// Smooth metal pushbutton, separate from textured enclosures.
inline void paintRaisedKey(Graphics& g,juce::Rectangle<float> r,Colour body,bool on,bool pressed){
    const float dy=pressed?.4f:1.8f;
    for(int i=3;i>=0;--i){g.setColour(juce::Colours::black.withAlpha(.16f+.09f*(3-i)));g.fillRoundedRectangle(r.expanded(i*.4f).translated(0,dy+i*.5f),6.f+i*.4f);}
    juce::ColourGradient rim(body.interpolatedWith(Colour(0xffd8c19f),.36f),r.getX(),r.getY(),Colour(0xff0a0907),r.getRight(),r.getBottom(),false);
    g.setGradientFill(rim);g.fillRoundedRectangle(r,5.f);
    const auto face=r.reduced(2.1f);
    g.setColour(juce::Colours::black.withAlpha(.46f));g.drawRoundedRectangle(face.expanded(.65f),4.7f,.75f);
    const auto top=on?body.brighter(.13f):body.brighter(.09f);
    g.setGradientFill(juce::ColourGradient::vertical(pressed?body.darker(.18f):top,face.getY(),pressed?top:body.darker(.28f),face.getBottom()));
    g.fillRoundedRectangle(face,4.f);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xffffead0).withAlpha(on?.10f:.055f),face.getY(),juce::Colours::transparentWhite,face.getCentreY()));g.fillRoundedRectangle(face.withHeight(face.getHeight()*.48f),3.f);
    g.setColour(Colour(0xffffead0).withAlpha(on?.48f:.28f));g.drawHorizontalLine(int(face.getY())+1,face.getX()+4,face.getRight()-4);
    g.setColour(juce::Colours::black.withAlpha(.58f));g.drawHorizontalLine(int(face.getBottom())-1,face.getX()+4,face.getRight()-4);
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
        float shine=metal::cap(r,a,.57f).brightness+grain*.65f;
        shine-=.055f*(1.f-smooth(.0f,.13f,r));                              // shallow centre dish
        // Engraved groove at r~.80 with a lit lower lip, then the chamfer.
        const float groove=smooth(.855f,.872f,r)*(1.f-smooth(.881f,.891f,r));
        shine=shine*(1.f-groove)+.38f*groove;
        const float lip=smooth(.891f,.902f,r)*(1.f-smooth(.911f,.921f,r));
        shine=shine*(1.f-lip)+(.80f-.30f*lit)*lip;
        const float bevel=smooth(.895f,.930f,r)*(1.f-smooth(.968f,.98f,r));
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
// Necromancer ReanimateDial's original cap, directly reused at rack size.
// The original PNG remains unchanged; no substitute skull or well.
inline const juce::Image& necroCap(){
    static const auto image=juce::ImageFileFormat::loadFrom(
        SkelliereKnobData::knob_cap_big_png,SkelliereKnobData::knob_cap_big_pngSize);
    return image;
}
inline bool necromancerArtworkReady(){
    return necroCap().isValid()&&necroCap().getWidth()==256&&necroCap().getHeight()==256;
}
// Signature: stationary machined-metal reflection and rotating original engraving.
inline const juce::Image& litNecroCap(){
    static const auto image=[]{
        const auto& src=necroCap();if(!src.isValid())return juce::Image{};
        juce::Image im(juce::Image::ARGB,src.getWidth(),src.getHeight(),true);
        juce::Image::BitmapData pixels(im,juce::Image::BitmapData::writeOnly);
        const float centre=im.getWidth()*.5f;
        for(int y=0;y<im.getHeight();++y)for(int x=0;x<im.getWidth();++x){
            const float dx=(x+.5f-centre)/centre,dy=(y+.5f-centre)/centre;
            const float rad=std::sqrt(dx*dx+dy*dy),angle=std::atan2(dy,dx);
            const auto outer=src.getPixelAt(x,y);
            if(rad>=1.f)continue;
            const float lum=outer.getFloatRed()*.30f+outer.getFloatGreen()*.59f+outer.getFloatBlue()*.11f;
            // The source perimeter contains partial-alpha ring detail. Render the
            // circular metal rim with continuous coverage to avoid a dotted edge.
            const float source=.57f+(lum-.57f)*metal::smooth(.43f,.59f,rad)*(1.f-metal::smooth(.80f,.93f,rad))*.36f;
            const float v=metal::cap(rad,angle,source).brightness;
            pixels.setPixelColour(x,y,juce::Colour::fromFloatRGBA(metal::sat(v*1.09f),
                metal::sat(v*1.025f),metal::sat(v*.91f),metal::sat((.995f-rad)*centre+.5f)));
        }
        return im;
    }();
    return image;
}
inline const juce::Image& necroEngraving(bool highlight=false){
    static const auto image=[]{
        const auto& src=necroCap();if(!src.isValid())return juce::Image{};
        juce::Image im(juce::Image::ARGB,src.getWidth(),src.getHeight(),true);
        juce::Image::BitmapData pixels(im,juce::Image::BitmapData::writeOnly);
        const float centre=im.getWidth()*.5f;
        for(int y=0;y<im.getHeight();++y)for(int x=0;x<im.getWidth();++x){
            const float dx=(x+.5f-centre)/centre,dy=(y+.5f-centre)/centre;
            const float rad=std::sqrt(dx*dx+dy*dy);
            if(rad>=.46f)continue;
            const float scale=metal::engravingScale(rad);
            const float sx=centre+(x+.5f-centre)*scale-.5f,sy=centre+(y+.5f-centre)*scale-.5f;
            const int ix=juce::jlimit(0,im.getWidth()-2,int(std::floor(sx))),iy=juce::jlimit(0,im.getHeight()-2,int(std::floor(sy)));
            const float fx=sx-ix,fy=sy-iy;
            const float a0=src.getPixelAt(ix,iy).getFloatAlpha()*(1.f-fx)+src.getPixelAt(ix+1,iy).getFloatAlpha()*fx;
            const float a1=src.getPixelAt(ix,iy+1).getFloatAlpha()*(1.f-fx)+src.getPixelAt(ix+1,iy+1).getFloatAlpha()*fx;
            const float mark=metal::engravingWeight(rad,a0*(1.f-fy)+a1*fy);
            pixels.setPixelColour(x,y,Colour(0xff33291f).interpolatedWith(Colour(0xff120f0c),mark).withAlpha(mark*.96f));
        }
        return im;
    }();
    if(highlight){
        static const auto lip=[] {
            auto im=necroEngraving(false).createCopy();
            if(!im.isValid())return im;
            juce::Image::BitmapData pixels(im,juce::Image::BitmapData::readWrite);
            for(int y=0;y<im.getHeight();++y)for(int x=0;x<im.getWidth();++x){
                const float a=pixels.getPixelColour(x,y).getFloatAlpha();
                pixels.setPixelColour(x,y,Colour(0xffffe9c5).withAlpha(a*.70f));
            }
            return im;
        }();
        return lip;
    }
    return image;
}
// Preserve original ReanimateDial rotation and unmodified Silver asset.
inline void paintNecromancerCap(Graphics& g,float cx,float cy,float radius,float p,bool silver){
    juce::Graphics::ScopedSaveState save(g);g.setOpacity(1.f);
    const juce::Rectangle<float> dest(cx-radius,cy-radius,2*radius,2*radius);
    if(!necroCap().isValid()){g.drawImage(machinedCap(Colour(0xffb4aea4)),dest);return;}
    if(!silver)g.drawImage(litNecroCap(),dest);
    const auto rotation=juce::AffineTransform::rotation(p*juce::MathConstants<float>::pi*1.5f-1.18f,cx,cy);
    if(!silver){
        // Fixed light catches the lower lip of the rotating original mark.
        juce::Graphics::ScopedSaveState lip(g);
        g.addTransform(rotation.translated(.38f,.48f));g.drawImage(necroEngraving(true),dest);
    }
    g.addTransform(rotation);g.drawImage(silver?necroCap():necroEngraving(),dest);
}
// Only the physical thickness and contact shadow of the metal knob itself.
// There is no black dial-well image, surrounding ring or rune field.
// Cache the sidewall and engraved cutout's backing on the UI thread.
inline const juce::Image& capMount(){
    static const juce::Image cached=[]{
        constexpr int n=384;constexpr float c=192.f,rad=132.f;
        juce::Image im(juce::Image::ARGB,n,n,true);Graphics g(im);
        for(int i=20;i>=0;--i){
            const float rr=rad*.94f+float(i)*1.85f;
            g.setColour(juce::Colours::black.withAlpha(i>10?.026f:.075f));
            g.fillEllipse(c-rr+18.f,c-rr+30.f,2*rr,2*rr);
        }
        g.setColour(juce::Colours::black.withAlpha(.65f));g.fillEllipse(c-rad*.98f+3.f,c-rad*.98f+10.f,rad*1.96f,rad*1.96f);
        // Sidewall sits underneath the face; its lower edge alone is visible.
        juce::ColourGradient side(Colour(0xffb6a18a),c-rad,c-rad,
            Colour(0xff100e0c),c+rad,c+rad,false);
        side.addColour(.38,Colour(0xff71624f));side.addColour(.74,Colour(0xff251e17));
        g.setGradientFill(side);
        g.fillEllipse(c-rad*.985f,c-rad*.985f+17.f,rad*1.97f,rad*1.97f);
        g.setColour(juce::Colours::black.withAlpha(.46f));g.drawEllipse(c-rad*.982f,c-rad*.982f+7.f,rad*1.964f,rad*1.964f,1.3f);
        // Fluting on the exposed lower sidewall, not an added outer ring.
        for(int i=0;i<46;++i){const float a=.25f+float(i)*2.64f/45.f;
            const float x=c+std::cos(a)*rad*.968f,y=c+std::sin(a)*rad*.968f;
            g.setColour(Colour(0xffc4ac88).withAlpha(.19f));g.drawLine(x,y+4.f,x,y+10.f,.75f);
        }
        // The PNG skull is an alpha cutout. This backing is strictly inside
        // the original metal cap and makes its engraved mark legible.
        g.setColour(Colour(0xff080706));g.fillEllipse(c-rad*.91f,c-rad*.91f,rad*1.82f,rad*1.82f);
        return im;
    }();
    return cached;
}

inline void paintMonogram(Graphics& g,juce::Rectangle<float> r,Colour ink) {
    const float x=r.getX(),y=r.getY(),w=r.getWidth(),h=r.getHeight();
    juce::Path shield;shield.startNewSubPath(x+w*.5f,y);
    shield.lineTo(x+w*.91f,y+h*.16f);shield.lineTo(x+w*.91f,y+h*.82f);
    shield.lineTo(x+w*.5f,y+h);shield.lineTo(x+w*.09f,y+h*.82f);shield.lineTo(x+w*.09f,y+h*.16f);shield.closeSubPath();
    g.setColour(ink.withAlpha(.09f));g.fillPath(shield);
    g.setColour(ink.withAlpha(.35f));g.strokePath(shield,juce::PathStrokeType(2.4f),juce::AffineTransform::translation(.45f,.65f));
    g.setColour(ink);g.strokePath(shield,juce::PathStrokeType(1.25f));
    juce::Path mark;mark.startNewSubPath(x+w*.68f,y+h*.22f);
    mark.lineTo(x+w*.36f,y+h*.16f);mark.lineTo(x+w*.36f,y+h*.45f);
    mark.lineTo(x+w*.64f,y+h*.58f);mark.lineTo(x+w*.64f,y+h*.83f);
    mark.lineTo(x+w*.31f,y+h*.71f);
    g.strokePath(mark,juce::PathStrokeType(std::max(1.5f,w*.13f),juce::PathStrokeType::mitered));
}

inline void paintScrew(Graphics& g, float cx, float cy, float r, float turn) {
    g.setColour(juce::Colours::black.withAlpha(.60f));
    g.fillEllipse(cx-r-1.1f,cy-r+.65f,2*r+2.2f,2*r+2.2f);
    juce::ColourGradient head(Colour(0xffe9d7b7),cx-r*.43f,cy-r*.65f,Colour(0xff29241d),cx+r*.82f,cy+r*.85f,true);
    head.addColour(.24,Colour(0xffded0b6));head.addColour(.52,Colour(0xff9d8c75));head.addColour(.84,Colour(0xff4a4135));
    g.setGradientFill(head);g.fillEllipse(cx-r,cy-r,2*r,2*r);
    g.setColour(Colour(0xff26211a));g.drawEllipse(cx-r,cy-r,2*r,2*r,.6f);
    juce::Path rim;rim.addCentredArc(cx,cy,r-.45f,r-.45f,0.f,-1.45f,.8f,true);
    g.setColour(Colour(0xffffe8c4).withAlpha(.72f));g.strokePath(rim,juce::PathStrokeType(.65f));
    const float arm=r*.58f,half=r*.15f;
    juce::Path cross;cross.startNewSubPath(-half,-arm);cross.lineTo(half,-arm);
    cross.lineTo(half,-half);cross.lineTo(arm,-half);cross.lineTo(arm,half);
    cross.lineTo(half,half);cross.lineTo(half,arm);cross.lineTo(-half,arm);
    cross.lineTo(-half,half);cross.lineTo(-arm,half);cross.lineTo(-arm,-half);
    cross.lineTo(-half,-half);cross.closeSubPath();
    const auto tr=juce::AffineTransform::rotation(turn).translated(cx,cy);
    g.setColour(Colour(0xffffdcb1).withAlpha(.68f));g.fillPath(cross,tr.translated(.35f,.45f));
    g.setColour(Colour(0xff201b15));g.fillPath(cross,tr);
    g.setColour(Colour(0xff080706).withAlpha(.65f));g.strokePath(cross,juce::PathStrokeType(.45f),tr);
    g.setColour(Colour(0xfffff0d8).withAlpha(.31f));g.fillEllipse(cx-r*.52f,cy-r*.60f,r*.44f,r*.25f);
}

// The needle meter used by the compressor, de-esser and limiter. value is gain reduction in dB (0 to 20).
enum class MeterLayer { all, plate, needle };
inline void paintLampGlass(Graphics& g,juce::Rectangle<float> face){
    // Static faceplate only: a continuous rounded-distance field models the lens shadow.
    const int w=juce::jmax(2,juce::roundToInt(face.getWidth()*2.f)),h=juce::jmax(2,juce::roundToInt(face.getHeight()*2.f));
    juce::Image lens(juce::Image::ARGB,w,h,true,juce::SoftwareImageType());
    { juce::Image::BitmapData pixels(lens,juce::Image::BitmapData::writeOnly);
    const float fw=w*.5f,fh=h*.5f,r=juce::jmin(30.f,fw,fh);
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){
        const float px=x+.5f-fw,py=y+.5f-fh,qx=std::abs(px)-(fw-r),qy=std::abs(py)-(fh-r);
        const float d=r-std::sqrt(std::max(0.f,qx)*std::max(0.f,qx)+std::max(0.f,qy)*std::max(0.f,qy))-std::min(0.f,std::max(qx,qy));
        if(d<-.5f)continue;
        const float u=px/fw,v=py/fh;
        const float edge=std::exp(-std::max(0.f,d)/(5.f+4.f*(1.f-v)));
        const float pool=std::exp(-1.4f*u*u-1.7f*(v-.24f)*(v-.24f));
        const float top=std::exp(-std::max(0.f,d)*.12f)*std::max(0.f,-v);
        const float shade=metal::sat(.50f+.48f*pool-.70f*edge-.50f*top);
        // Curved lower return catches the warm lamp above the dark gasket.
        const float returnLight=std::exp(-std::pow((d-5.f)/3.2f,2.f))*metal::smooth(.05f,.85f,v)*.42f;
        const auto paper=Colour(0xff78431b).interpolatedWith(Colour(0xffffe5b5),shade);
        pixels.setPixelColour(x,y,paper.interpolatedWith(Colour(0xffffdb98),returnLight).withAlpha(metal::sat(d+.5f)));
    }
    } // commit writable bitmap before using it as a source image
    juce::Graphics::ScopedSaveState save(g);
    g.setOpacity(1.f); // image blits must not inherit a preceding shadow colour
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);g.drawImage(lens,face);
}
inline void paintVU(Graphics& g, juce::Rectangle<float> r, float value, bool outputPeak=false, MeterLayer layer=MeterLayer::all) {
    const auto inner=r.reduced(7);
    const auto face=inner.reduced(2.4f);
    if(layer!=MeterLayer::needle){
    g.setColour(juce::Colours::black.withAlpha(.78f));g.fillRoundedRectangle(r.expanded(.7f).translated(1,3.4f),10);
    juce::ColourGradient bezel(Colour(0xff8b7960),r.getX(),r.getY(),Colour(0xff151310),r.getRight(),r.getBottom(),false);
    bezel.addColour(.16,Colour(0xff544b3e));bezel.addColour(.40,Colour(0xff211e1a));bezel.addColour(.76,Colour(0xff0b0a09));g.setGradientFill(bezel);
    g.fillRoundedRectangle(r,9);                                              // dark machined housing
    // Wide bevel with a bright lower return, like a cast meter housing.
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff130f0b),r.getY(),Colour(0xff827360),r.getBottom()));g.drawRoundedRectangle(r.reduced(3.1f),7.f,2.4f);
    // A single physical lens surround rather than stacked outlines.
    g.setGradientFill(juce::ColourGradient(Colour(0xffb4a18a),r.getX(),r.getY(),Colour(0xff41372b),r.getRight(),r.getBottom(),false));g.drawRoundedRectangle(r.reduced(.9f),8,1.5f);
    
    g.setColour(Colour(0xff0b0a09));g.fillRoundedRectangle(inner,11);          // recess
    g.setColour(juce::Colours::white.withAlpha(.10f));                       // lit lower lip of the recess
    g.drawHorizontalLine(int(inner.getBottom())-1,inner.getX()+6,inner.getRight()-6);
    for(int i=0;i<4;++i)                                                      // bezel fasteners
        paintScrew(g,i%2?r.getRight()-6.7f:r.getX()+6.7f,i<2?r.getY()+6.7f:r.getBottom()-6.7f,3.7f,.7f*i);
    paintLampGlass(g,face);
    g.setColour(Colour(0xff38200f));g.drawRoundedRectangle(face,15,1.4f);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff51331c),face.getY(),Colour(0xffffe7b3),face.getBottom()));g.drawRoundedRectangle(face.reduced(1.4f),14.f,1.05f);
    }


    juce::Graphics::ScopedSaveState save(g);
    {   juce::Path clipFace;clipFace.addRoundedRectangle(face.getX(),face.getY(),face.getWidth(),face.getHeight(),15.f);
        g.reduceClipRegion(clipFace);   }
    // The meter is much wider than it is tall. Fit the arc in both axes,
    // keeping its full scale, labels and hub inside the actual face.
    const float cx=face.getCentreX(),cy=face.getBottom()+1.f;
    const float sc = face.getWidth() / 103.0f;
    const float sy = juce::jmax(0.1f, (face.getHeight() - 19.f) / 57.f);
    const float labelSize=juce::jlimit(10.f,11.8f,8.f*sc);
    auto pt = [&](float rad, float v) {
        const float a = juce::degreesToRadians(-53.0f + v * 5.3f);
        return juce::Point<float>(cx + rad * sc * std::sin(a), cy - rad * sy * std::cos(a));
    };
    const float redStart=outputPeak?20.f*20.f/24.f:12.f;
    if(layer!=MeterLayer::needle){
    auto tick=[&](float v,int label,bool major){
        g.setColour(v>=redStart?pal::vuRed:pal::vuInk);
        g.drawLine(juce::Line<float>(pt(57.f,v),pt(major?49.f:53.f,v)),major?1.4f:.9f);
        if(major){
            g.setFont(font(labelSize+1.f,false));
            g.drawText((label>0&&outputPeak?"+":"")+juce::String(label),
                juce::Rectangle<float>(0,0,24,13).withCentre(pt(41.f,v)),juce::Justification::centred,false);
        }
    };
    if(outputPeak)for(int db=-20;db<=4;++db)tick((db+20.f)*20.f/24.f,db,db%4==0);
    else for(int db=0;db<=20;++db)tick(float(db),db,db%4==0);
    juce::Path arc;
    arc.startNewSubPath(pt(57.f,0.f));
    for(float v=.5f;v<redStart;v+=.5f)arc.lineTo(pt(57.f,v));
    arc.lineTo(pt(57.f,redStart));
    g.setColour(pal::vuInk);g.strokePath(arc,juce::PathStrokeType(1.1f));
    juce::Path red;red.startNewSubPath(pt(57.f,redStart));
    for(float v=redStart+.5f;v<20.f;v+=.5f)red.lineTo(pt(57.f,v));
    red.lineTo(pt(57.f,20.f));
    g.setColour(pal::vuRed); g.strokePath(red, juce::PathStrokeType(1.25f));
    g.setColour(pal::vuInk); g.setFont(font(labelSize, true));
    auto legend=face.reduced(9.f,0.f).removeFromBottom(15.f);
    g.drawText(outputPeak?"PEAK dBFS":"GR dB",legend.withTrimmedRight(44),juce::Justification::centred,false);
    }
    if(layer==MeterLayer::plate)return;
    g.setColour(pal::vuInk);g.setFont(font(10.f,false));
    auto legend=face.reduced(9.f,0.f).removeFromBottom(15.f);
    g.drawText(outputPeak?(value<=-59.f?juce::String("-inf"):juce::String(value,1)+" dBFS"):
        juce::String(value<.05f?0.f:-value,1)+" dB",legend,juce::Justification::centredRight,false);

    const float v=juce::jlimit(0.f,20.f,outputPeak?(value+20.f)*20.f/24.f:value);
    const auto tip=pt(55.0f, v);
    // The spindle is behind the lower meter trim. Keep the printed legend
    // clear by clipping the visible needle at the mechanical aperture.
    const auto pivot=juce::Point<float>(cx,cy);
    const float cut=juce::jlimit(0.f,1.f,(cy-(legend.getY()-2.f))/juce::jmax(.01f,cy-tip.y));
    const auto root=pivot+(tip-pivot)*cut;
    g.setColour(juce::Colours::black.withAlpha(.22f));
    g.drawLine(juce::Line<float>(root+juce::Point<float>(1.2f,1.2f),tip+juce::Point<float>(1.2f,1.2f)),1.3f);
    g.setColour(pal::vuRed);g.drawLine(juce::Line<float>(root,tip),1.65f);
    g.setColour(Colour(0xffe4a071).withAlpha(.65f));
    g.drawLine(juce::Line<float>(root+juce::Point<float>(.35f,0),tip+juce::Point<float>(.35f,0)),.45f);
    // Curved lens reflection: a soft band whose lower edge bows downward, rather than a flat rectangle.
    juce::Path glass;
    glass.startNewSubPath(face.getX(), face.getY());
    glass.lineTo(face.getRight(), face.getY());
    glass.lineTo(face.getRight(),face.getY()+face.getHeight()*.15f);
    glass.quadraticTo(face.getCentreX(),face.getY()+face.getHeight()*.25f,face.getX(),face.getY()+face.getHeight()*.15f);
    glass.closeSubPath();
    g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::white.withAlpha(.16f),face.getY(),juce::Colours::transparentWhite,face.getY()+face.getHeight()*.24f));
    g.fillPath(glass);
    g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::transparentWhite,face.getBottom()-9.f,Colour(0xffffe3ad).withAlpha(.46f),face.getBottom()));g.fillRoundedRectangle(face.withY(face.getBottom()-9.f).withHeight(9.f),3.f);
    g.setColour(Colour(0xffffeac1).withAlpha(.25f));g.drawRoundedRectangle(face.reduced(1.f),14.f,.65f);
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
    juce::Font getComboBoxFont(juce::ComboBox&) override { return font(17.0f); }
    void positionComboBoxText(juce::ComboBox& box,juce::Label& label) override {
        label.setBounds(6,1,juce::jmax(1,box.getWidth()-28),juce::jmax(1,box.getHeight()-2));
        label.setFont(getComboBoxFont(box));label.setJustificationType(juce::Justification::centred);
    }
    juce::Font getPopupMenuFont() override { return font(15.0f); }

    void fillTextEditorBackground(Graphics& g,int w,int h,juce::TextEditor&) override {
        auto r=juce::Rectangle<float>(0,0,float(w),float(h)).reduced(.5f);
        paintRecessedWindow(g,r,6.f);
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
        auto r = b.getLocalBounds().toFloat().reduced(1.f);
        const bool on = b.getToggleState();
        Colour top=on?Colour(0xffe0c39a):Colour(0xff35312c),bot=on?Colour(0xffb08f65):Colour(0xff1c1916);
        if(b.getProperties().contains("quiet")) {top=on?Colour(0xff494238):Colour(0xff282522);bot=Colour(0xff191714);}
        if (down) std::swap(top, bot);
        if (hover && !down) { top = top.brighter(0.12f); bot = bot.brighter(0.08f); }
        paintRaisedKey(g,r,top.interpolatedWith(bot,.35f),on,down);
        g.setColour(on?Colour(0xffe9cba0):Colour(0xff928573).withAlpha(.55f));g.drawRoundedRectangle(r,5.f,.7f);
        if(b.hasKeyboardFocus(false)){g.setColour(pal::amber);g.drawRoundedRectangle(r.reduced(.5f),4.5f,1.f);}
        if (on && b.getClickingTogglesState() && !b.getProperties().contains("plain")) {   // lit LED on latching buttons
            const float cx = r.getX() + 9.0f, cy = r.getCentreY();
            g.setColour(pal::amber.withAlpha(0.35f)); g.fillEllipse(cx - 5.0f, cy - 5.0f, 10.0f, 10.0f);
            g.setColour(pal::amber); g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
        }
    }
    void drawButtonText(Graphics& g, juce::TextButton& b, bool, bool) override {
        g.setFont(font(b.getHeight()>=32?15.f:12.f,false).withExtraKerningFactor(.035f));
        g.setColour((b.getToggleState()&&!b.getProperties().contains("quiet") ? Colour(0xff20180f) : pal::ink).withMultipliedAlpha(b.isEnabled()?1.f:.36f));
        auto r = b.getLocalBounds();
        if (b.getToggleState() && b.getClickingTogglesState() && !b.getProperties().contains("plain")) r = r.withTrimmedLeft(12);
        g.drawFittedText(b.getButtonText(), r, juce::Justification::centred, 1);
    }

    void drawComboBox(Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override {
        auto r = juce::Rectangle<float>(0, 0, (float) w, (float) h);
        g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff080706),r.getY(),Colour(0xff28221a),r.getBottom()));g.fillRoundedRectangle(r,5.f);
        g.setColour(juce::Colours::white.withAlpha(.045f));g.fillRoundedRectangle(r.reduced(3.f).withHeight(r.getHeight()*.35f),3.f);
        g.setColour(box.hasKeyboardFocus(true)?pal::amber:Colour(0xff847766).withAlpha(.75f));g.drawRoundedRectangle(r.reduced(.6f),5.f,1.f);
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
        const float p=juce::jlimit(0.f,1.f,pos),angle=a0+p*(a1-a0);
        // Nearly all of the original rack knob area is now the actual metal
        // cap rather than a small cap surrounded by Necromancer's black well.
        const float capR=size*(utility?.397f:.405f);
        const float shadowR=capR*192.f/132.f;
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.setOpacity(1.f);
        g.drawImage(capMount(),juce::Rectangle<float>(cx-shadowR,cy-shadowR,2*shadowR,2*shadowR));
        paintNecromancerCap(g,cx,cy,capR,p,bool(s.getProperties().getWithDefault("silverCap",false)));
        // Fixed reflected studio light; the original cap itself still rotates.
        {juce::Graphics::ScopedSaveState save(g);
            juce::Path clip;clip.addEllipse(cx-capR*.94f,cy-capR*.94f,capR*1.88f,capR*1.88f);g.reduceClipRegion(clip);
            juce::ColourGradient sheen(Colour(0xffffe8c5).withAlpha(.12f),cx-capR*.65f,cy-capR*.7f,
                juce::Colours::transparentWhite,cx+capR*.35f,cy+capR*.25f,true);
            sheen.addColour(.5,juce::Colours::transparentWhite);g.setGradientFill(sheen);
            g.fillEllipse(cx-capR*.94f,cy-capR*.94f,capR*1.88f,capR*1.88f);
        }
        juce::Path edge;edge.addCentredArc(cx,cy,capR*.955f,capR*.955f,0.f,-1.6f,.65f,true);
        g.setColour(Colour(0xffffebcb).withAlpha(.76f));g.strokePath(edge,juce::PathStrokeType(juce::jmax(.55f,capR*.022f)));
        // Original rack-style scale engraved on the FACEPLATE, not on a ring.
        const auto tick=light?Colour(0xff392f24):Colour(0xffe9ddc6);
        const int marks=utility?10:12;
        for(int k=0;k<=marks;++k){
            const float a=a0+(a1-a0)*k/marks,sine=std::sin(a),cosine=-std::cos(a);
            const bool major=k==0||k==marks/2||k==marks;
            if(major){
                g.setColour(tick.withAlpha(.94f));
                g.drawLine(cx+sine*capR*1.105f,cy+cosine*capR*1.105f,
                           cx+sine*capR*1.205f,cy+cosine*capR*1.205f,juce::jmax(.8f,capR*.033f));
            }else{
                const float rr=capR*1.16f,dot=juce::jmax(.95f,capR*.043f);
                g.setColour(tick.withAlpha(.94f));
                g.fillEllipse(cx+sine*rr-dot,cy+cosine*rr-dot,2*dot,2*dot);
            }
        }
        // A narrow pointer at the metal rim: never crosses the skull, and
        // remains visible without an extra black circle or luminous arc.
        const float sn=std::sin(angle),cs=-std::cos(angle);
        const float start=capR*.47f,end=capR*.94f;
        g.setColour(Colour(0xff10100f));
        g.drawLine(cx+sn*start,cy+cs*start,cx+sn*end,cy+cs*end,juce::jmax(2.f,capR*.082f));
        g.setColour(Colour(0xfff8e8c9).withAlpha(.46f));
        g.drawLine(cx+sn*start+.65f,cy+cs*start+.65f,cx+sn*end+.65f,cy+cs*end+.65f,juce::jmax(.55f,capR*.023f));
        if(s.isMouseOverOrDragging()||s.hasKeyboardFocus(false)){
            g.setColour(pal::amber.withAlpha(s.hasKeyboardFocus(false)?.94f:.55f));
            g.fillEllipse(cx+sn*capR*1.16f-1.7f,cy+cs*capR*1.16f-1.7f,3.4f,3.4f);
        }
    }

    void drawScrollbar(Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart, int thumbSize, bool over, bool down) override {
        g.setColour(Colour(0xff0d0c0b));g.fillRect(x,y,w,h);
        auto t = vertical ? juce::Rectangle<int>(x + 2, thumbStart, w - 4, thumbSize) : juce::Rectangle<int>(thumbStart, y + 2, thumbSize, h - 4);
        g.setColour(Colour(down?0xffaa9c82:over?0xff8f806b:0xff2a2722)); g.fillRoundedRectangle(t.toFloat(), 3.0f);
    }
};

} // namespace skl::ui
