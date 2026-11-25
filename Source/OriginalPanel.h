#pragma once
#include "Look.h"
namespace skl::ui {
struct PanelSpec { Colour metal,accent,cap; bool light; const char* series; };
inline PanelSpec originalSpec(int type,int finish) {
    static constexpr unsigned metals[]={0x25292c,0x9eaaa6,0xc9b398,0x75463f,0x5d4132,0xc4c0ad,0x384859,0x414b35,0x45433e,0x323235,0x6b716e,0x242d34,0x97876e,0x425c61,0x62403b,0x344c59,0xc1aa83,0x454f3e,0x71614d,0x344b61,0x5e6354,0x784d3e,0x3c594d,0x4e4059,0x663d38,0x706449,0x496464};
    static constexpr unsigned accents[]={0x9a9a9a,0x39534b,0xaed5bf,0xe0b88e,0xe9be7b,0x365b55,0xa4bedc,0xe7c68c,0xaed0aa,0xe2ad9f,0xb5d3c4,0x9bddb7,0x3c514b,0xa4d4df,0xe5d9bf,0xb7d8cf,0x26392f,0xc0d58e,0xf0cc9b,0x405e50,0xe1d7ab,0xf3b397,0xc1d4ab,0xd5b7de,0xa7d5e5,0xf0cd98,0xc6e8da};
    type=std::clamp(type,0,static_cast<int>(kNumRackTypes)-1);
    auto metal=Colour(0xff000000u|metals[type]);const auto accent=Colour(0xff000000u|accents[type]);
    if(finish==0)metal=Colour(0xff303437);
    if(finish==2)metal=Colour(0xffb1b6b1);
    if(finish==3)metal=Colour(0xff222829);
    const bool light=metal.getPerceivedBrightness()>.45f;
    const auto cap=type%4==0?Colour(0xffc4b297):Colour(0xffd0c4af);
    const char* series=info(type).cat==CatDynamics?"DYNAMICS":info(type).cat==CatTone?"TONE":info(type).cat==CatSpace?"MOD / SPACE":"UTILITY";
    return {metal,accent,cap,light,series};
}
struct OriginalLayout {
    std::array<juce::Rectangle<int>,6> cells{};
    juce::Rectangle<int> meter, detail;
    bool sideReadouts=false;
};
inline OriginalLayout originalLayout(int type,int width) {
    OriginalLayout l;const auto& mi=info(type);const int n=mi.numKnobs;
    const int half=(width-24)/2,mid=width/2;
    // Keep every APVTS parameter, including controls absent from the concept.
    if(mi.hasVU||type==Flanger)l.meter={18,107,width-36,118};
    if(type==Delay||type==Slap)l.detail={19,107,width-38,118};
    const int top=!l.meter.isEmpty()||!l.detail.isEmpty()?241:112,bottom=589;
    if(type==Bell||type==DualFilter||type==Tilt){
        l.sideReadouts=true;
        const int row=std::min(122,(bottom-top)/std::max(1,n));
        for(int k=0;k<n;++k)l.cells[k]={12,top+k*row,width-24,row-4};
    }else if(n==2){
        l.cells[0]={mid-86,top+18,172,215};
        l.cells[1]={mid-74,top+248,148,198};
    }else if(n==3){
        const int hero=mi.hasVU?136:202;l.cells[0]={mid-82,top,164,hero};
        for(int k=1;k<3;++k)l.cells[k]={12+(k-1)*half,top+hero+12,half,std::min(176,bottom-top-hero-12)};
    }else if(n==5&&!mi.hasVU){
        l.cells[0]={mid-70,top,140,151};
        for(int k=1;k<5;++k)l.cells[k]={12+((k-1)%2)*half,top+157+((k-1)/2)*153,half,145};
    }else if(n==5&&mi.hasVU){
        // Two large rows followed by a smaller central makeup control.
        for(int k=0;k<4;++k)l.cells[k]={12+(k%2)*half,top+(k/2)*127,half,123};
        l.cells[4]={mid-43,top+254,86,bottom-top-254};
    }else{
        const int rows=(n+1)/2,rh=(bottom-top)/std::max(1,rows);
        for(int k=0;k<n;++k)l.cells[k]={n%2==1&&k==n-1?mid-half/2:12+(k%2)*half,top+(k/2)*rh,half,rh-4};
    }
    return l;
}
inline void paintTapeDetail(Graphics& g,juce::Rectangle<float> r) {
    // Decorative tape-inspired mechanical illustration, not a fake signal meter.
    g.setColour(Colour(0xff14110d));g.fillRoundedRectangle(r,6);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff392c1b),r.getY(),Colour(0xff181612),r.getBottom()));
    g.fillRoundedRectangle(r.reduced(5),4);
    const float radius=std::min(r.getWidth()*.20f,r.getHeight()*.34f);
    for(int k=0;k<2;++k){
        const float cx=r.getX()+r.getWidth()*(k==0?.28f:.72f),cy=r.getCentreY()-6;
        g.setColour(Colour(0xff0c0b09));g.fillEllipse(cx-radius-2,cy-radius,2*radius+4,2*radius+4);
        g.drawImage(machinedCap(Colour(0xffb6a58b)),juce::Rectangle<float>(cx-radius,cy-radius,2*radius,2*radius));
        for(int spoke=0;spoke<3;++spoke){
            const float angle=spoke*juce::MathConstants<float>::twoPi/3+.35f;
            juce::Path cut;cut.addRoundedRectangle(-radius*.15f,-radius*.83f,radius*.30f,radius*.42f,2);
            g.setColour(Colour(0xff29241b));g.fillPath(cut,juce::AffineTransform::rotation(angle).translated(cx,cy));
        }
        g.setColour(Colour(0xff30271e));g.fillEllipse(cx-8,cy-8,16,16);
        g.setColour(Colour(0xffd2c0a0));g.fillEllipse(cx-4,cy-4,8,8);
    }
    g.setColour(Colour(0xffb29c77).withAlpha(.55f));g.drawRoundedRectangle(r.reduced(1),6,1);
    g.setColour(Colour(0xffd0b68e));g.setFont(font(9,true));
    g.drawText("ECHO",r.removeFromBottom(19),juce::Justification::centred,false);
}
inline void paintOriginalSurface(Graphics& g,juce::Rectangle<float> r,int type,int finish) {
    const auto p=originalSpec(type,finish);g.setColour(Colour(0xff0d0f10));g.fillRoundedRectangle(r,3);
    const auto plate=r.reduced(2);
    g.setGradientFill(juce::ColourGradient::vertical(p.metal.brighter(.13f),plate.getY(),p.metal.darker(.14f),plate.getBottom()));
    g.fillRoundedRectangle(plate,5);paintBrushed(g,plate,.32f);
    g.setColour(juce::Colours::white.withAlpha(.17f));g.drawRoundedRectangle(plate.reduced(.5f),5,1);
    g.setColour(juce::Colours::black.withAlpha(.55f));g.drawRoundedRectangle(plate.reduced(2),4,.8f);
    for(int i=0;i<4;++i)paintScrew(g,i%2?plate.getRight()-10:plate.getX()+10,i<2?plate.getY()+10:plate.getBottom()-10,5.f,.8f*i);
}
inline void paintOriginalMini(Graphics& g,juce::Rectangle<float> r,int type) {
    const auto p=originalSpec(type,1);paintOriginalSurface(g,r,type,1);
    g.setColour(p.accent);g.fillRect(r.reduced(5).removeFromTop(3));
    auto f=r.reduced(6);
    if(info(type).hasVU||type==Flanger){
        // A legible meter icon, not a reduced full-size scale.
        f=r.reduced(4);
        g.setColour(Colour(0xff18140f));g.fillRoundedRectangle(f,3);
        f=f.reduced(2);g.setGradientFill(juce::ColourGradient::vertical(Colour(0xfff2d7a3),f.getY(),Colour(0xffad8050),f.getBottom()));g.fillRoundedRectangle(f,2);
        const float cx=f.getCentreX(),cy=f.getBottom()-2,rx=f.getWidth()*.47f,ry=f.getHeight()*.76f;
        for(int k=0;k<=6;++k){const float a=juce::degreesToRadians(-55.f+k*110.f/6);
            g.setColour(Colour(0xff413225));g.drawLine(cx+rx*.85f*std::sin(a),cy-ry*.85f*std::cos(a),cx+rx*std::sin(a),cy-ry*std::cos(a),.7f);}
        g.setColour(Colour(0xff9b3224));g.drawLine(cx,cy,cx-rx*.32f,cy-ry*.93f,1);return;
    }
    if(type==Delay||type==Slap){
        f=r.reduced(4);g.setColour(Colour(0xff211b14));g.fillRoundedRectangle(f,3);
        const float radius=std::min(f.getWidth()*.22f,f.getHeight()*.33f);
        for(int k=0;k<2;++k){const float cx=f.getX()+f.getWidth()*(k?.74f:.26f),cy=f.getCentreY();
            g.drawImage(machinedCap(Colour(0xffb6a58b)),juce::Rectangle<float>(cx-radius,cy-radius,2*radius,2*radius));
            g.setColour(Colour(0xff352c21));g.fillEllipse(cx-radius*.32f,cy-radius*.32f,radius*.64f,radius*.64f);}
        return;
    }
    const auto center=f.getCentre();const float d=std::min(f.getWidth(),f.getHeight())*.58f;
    g.setColour(p.cap);g.fillEllipse(center.x-d/2,center.y-d/2,d,d);
    g.setColour(p.light?juce::Colours::white:juce::Colours::black);g.drawLine(center.x,center.y,center.x+d*.17f,center.y-d*.39f,1.2f);
}
}
