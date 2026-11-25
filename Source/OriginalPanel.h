#pragma once
#include "Look.h"
namespace skl::ui {
struct PanelSpec { Colour metal,accent,cap; bool light; const char* series; };
inline PanelSpec originalSpec(int type,int finish) {
    static constexpr unsigned metals[]={0x25292c,0xb6b8ae,0x374a48,0x563a30,0x5c4432,0xc6c8c1,0x343f56,0x514638,0x39433b,0x292b31,0x53595a,0x30383e,0x8c8777,0x3e4956,0x3d4144,0x294c50,0x9e8c5b,0x36392e,0x8d6c45,0xaeb5b0,0x51584a,0x663d38,0x3d4b42,0x4a3a51,0x29475b,0x74614a};
    static constexpr unsigned accents[]={0x9a9a9a,0x39534b,0xaed5bf,0xe0b88e,0xe9be7b,0x365b55,0xa4bedc,0xe7c68c,0xaed0aa,0xe2ad9f,0xb5d3c4,0x9bddb7,0x3c514b,0xa4d4df,0xe5d9bf,0xb7d8cf,0x26392f,0xc0d58e,0xf0cc9b,0x405e50,0xe1d7ab,0xf3b397,0xc1d4ab,0xd5b7de,0xa7d5e5,0xf0cd98};
    type=std::clamp(type,0,static_cast<int>(kNumRackTypes)-1);
    auto metal=Colour(0xff000000u|metals[type]);const auto accent=Colour(0xff000000u|accents[type]);
    if(finish==0)metal=Colour(0xff303437);
    if(finish==2)metal=Colour(0xffb1b6b1);
    if(finish==3)metal=Colour(0xff222829);
    const bool light=metal.getPerceivedBrightness()>.45f;
    const auto cap=light?Colour(0xff343b3b):(type%3==0?Colour(0xffd1c5ac):Colour(0xffb7bfc0));
    const char* series=info(type).cat==CatDynamics?"DYNAMICS":info(type).cat==CatTone?"TONE":info(type).cat==CatSpace?"MOD / SPACE":"UTILITY";
    return {metal,accent,cap,light,series};
}
struct OriginalLayout {std::array<juce::Rectangle<int>,6> cells{};juce::Rectangle<int> meter;};
inline OriginalLayout originalLayout(int type,int width) {
    OriginalLayout l;const auto& mi=info(type);const int n=mi.numKnobs;
    const int half=(width-24)/2,mid=width/2;
    if(mi.hasVU)l.meter={20,83,width-40,108};
    const int top=mi.hasVU?204:103,bottom=515;
    if(n==2){
        l.cells[0]={mid-77,top,154,178};l.cells[1]={mid-62,top+197,124,137};
    }else if(n==3){
        const int hero=mi.hasVU?140:184;l.cells[0]={mid-80,top,160,hero};
        for(int i=1;i<3;++i)l.cells[i]={12+(i-1)*half,top+hero+10,half,bottom-top-hero-10};
    }else if(n==5&&!mi.hasVU){
        l.cells[0]={mid-65,top,130,135};
        for(int i=1;i<5;++i)l.cells[i]={12+((i-1)%2)*half,top+143+((i-1)/2)*130,half,124};
    }else{
        const int rows=(n+1)/2,rh=(bottom-top)/std::max(1,rows);
        for(int i=0;i<n;++i)l.cells[i]={n%2==1&&i==n-1?mid-half/2:12+(i%2)*half,top+(i/2)*rh,half,rh-6};
    }
    return l;
}
inline void paintOriginalSurface(Graphics& g,juce::Rectangle<float> r,int type,int finish) {
    const auto p=originalSpec(type,finish);g.setColour(Colour(0xff0d0f10));g.fillRoundedRectangle(r,3);
    const auto plate=r.reduced(3);
    g.setGradientFill(juce::ColourGradient::vertical(p.metal.brighter(.20f),plate.getY(),p.metal.darker(.22f),plate.getBottom()));
    g.fillRoundedRectangle(plate,2);paintBrushed(g,plate,.40f);
    g.setColour(juce::Colours::white.withAlpha(.17f));g.drawRoundedRectangle(plate.reduced(.5f),2,1);
    g.setColour(juce::Colours::black.withAlpha(.55f));g.drawRoundedRectangle(plate.reduced(2),2,.8f);
    for(int i=0;i<4;++i)paintScrew(g,i%2?plate.getRight()-8:plate.getX()+8,i<2?plate.getY()+9:plate.getBottom()-9,3.8f,.8f*i);
}
inline void paintOriginalMini(Graphics& g,juce::Rectangle<float> r,int type) {
    const auto p=originalSpec(type,1);paintOriginalSurface(g,r,type,1);
    g.setColour(p.accent);g.fillRect(r.reduced(5).removeFromTop(3));
    auto f=r.reduced(6);
    if(info(type).hasVU){g.setColour(Colour(0xffdcc9a0));g.fillRoundedRectangle(f.removeFromTop(f.getHeight()*.32f),1);}
    const auto center=f.getCentre();const float d=std::min(f.getWidth(),f.getHeight())*.58f;
    g.setColour(p.cap);g.fillEllipse(center.x-d/2,center.y-d/2,d,d);
    g.setColour(p.light?juce::Colours::white:juce::Colours::black);g.drawLine(center.x,center.y,center.x+d*.17f,center.y-d*.39f,1.2f);
}
}
