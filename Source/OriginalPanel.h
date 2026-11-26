#pragma once
#include "Look.h"
namespace skl::ui {
struct PanelSpec { Colour metal,accent,cap; bool light; const char* series; };
inline PanelSpec originalSpec(int type,int finish) {
    static constexpr unsigned metals[]={0x25292c,0x9eaaa6,0xdfc6a7,0x62352e,0x5d4132,0xc4c0ad,0x293541,0x303423,0x45433e,0x323235,0x6b716e,0x242d34,0x97876e,0x425c61,0x62403b,0x344c59,0xc1aa83,0x454f3e,0x71614d,0x303e4b,0x5e6354,0x784d3e,0x3c594d,0x4e4059,0x542d2b,0x706449,0x496464,0x45484a};
    static constexpr unsigned accents[]={0x9a9a9a,0x39534b,0xaed5bf,0xe0b88e,0xe9be7b,0x365b55,0xa4bedc,0xe7c68c,0xaed0aa,0xe2ad9f,0xb5d3c4,0x9bddb7,0x3c514b,0xa4d4df,0xe5d9bf,0xb7d8cf,0x26392f,0xc0d58e,0xf0cc9b,0x405e50,0xe1d7ab,0xf3b397,0xc1d4ab,0xd5b7de,0xa7d5e5,0xf0cd98,0xc6e8da,0xc6c7b4};
    type=std::clamp(type,0,static_cast<int>(kNumRackTypes)-1);
    auto metal=Colour(0xff000000u|metals[type]);const auto accent=Colour(0xff000000u|accents[type]);
    if(finish==0)metal=Colour(0xff303437);
    if(finish==2)metal=Colour(0xffb1b6b1);
    if(finish==3)metal=Colour(0xff222829);
    const bool light=metal.getPerceivedBrightness()>.45f;
    const auto cap=type%4==0?Colour(0xffbca487):Colour(0xffccbaa1);
    const char* series=info(type).cat==CatDynamics?"DYNAMICS":info(type).cat==CatTone?"TONE":info(type).cat==CatSpace?"MOD / SPACE":"UTILITY";
    return {metal,accent,cap,light,series};
}
struct OriginalLayout {
    std::array<juce::Rectangle<int>,6> cells{};
    juce::Rectangle<int> meter, detail;
    bool sideReadouts=false;
};
// Reference proportions: the tall 650px panels are replaced by a compact
// hardware panel of 574px. Preserve every knob, including Bell's four real controls.
inline OriginalLayout originalLayout(int type,int width,bool editing=false) {
    OriginalLayout l;const auto& mi=info(type);const int n=mi.numKnobs;
    const int half=(width-24)/2,mid=width/2;
    if(mi.hasVU||type==Flanger)l.meter={15,106,width-30,120};
    if(type==Delay||type==Slap)l.detail={16,106,width-32,120};
    const bool window=!l.meter.isEmpty()||!l.detail.isEmpty();
    // Four-window controls leave a lower breathing area like real rack gear.
    // Five-control compressors need the bottom row for makeup gain.
    const int top=window?(n==5?(editing?248:253):242):108;
    const int bottom=editing?520:(window&&n==4?515:window&&n==5?565:548),available=bottom-top;
    if(type==Bell||type==DualFilter||type==Tilt){
        l.sideReadouts=true;const int row=available/std::max(1,n);
        for(int k=0;k<n;++k)l.cells[k]={12,top+k*row,width-24,row-4};
    }else if(n==2){
        const int row=available/2;
        for(int k=0;k<2;++k)l.cells[k]={mid-83,top+k*row,166,row-4};
    }else if(n==3&&!window){
        const int hero=available/2;l.cells[0]={mid-82,top,164,hero-4};
        for(int k=1;k<3;++k)l.cells[k]={12+(k-1)*half,top+hero,half,available-hero-4};
    }else if(n==5&&window){
        const int row=(available-(editing?76:88))/2;
        for(int k=0;k<4;++k)l.cells[k]={12+(k%2)*half,top+(k/2)*row,half,row-4};
        l.cells[4]={mid-40,top+2*row,80,available-2*row-3};
    }else if(n==5){
        const int row=available/3;l.cells[0]={mid-70,top,140,row-4};
        for(int k=1;k<5;++k)l.cells[k]={12+((k-1)%2)*half,top+row+((k-1)/2)*row,half,row-4};
    }else{
        const int rows=std::max(1,(n+1)/2),rh=available/rows;
        for(int k=0;k<n;++k)l.cells[k]={n%2==1&&k==n-1?mid-half/2:12+(k%2)*half,top+(k/2)*rh,half,rh-4};
    }
    return l;
}
inline void paintTapeDetail(Graphics& g,juce::Rectangle<float> r) {
    // Decorative tape-inspired mechanical illustration. It is not a signal meter and does not
    // show tape physics; the real Time / Feedback / Tone / Mix controls are the knobs below.
    g.setColour(juce::Colours::black.withAlpha(.45f));g.fillRoundedRectangle(r.translated(0,2),7);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff3b362d),r.getY(),Colour(0xff0f0e0c),r.getBottom()));
    g.fillRoundedRectangle(r,7);                                                 // machined bezel
    g.setGradientFill(juce::ColourGradient(Colour(0xffd8bb8a).withAlpha(.75f),r.getX(),r.getY(),Colour(0xff3c291a),r.getRight(),r.getBottom(),false));g.drawRoundedRectangle(r.reduced(.9f),6.f,1.3f);
    g.setColour(Colour(0xff000000).withAlpha(.6f));g.drawRoundedRectangle(r.reduced(2.5f),5,1);
    for(int i=0;i<4;++i)paintScrew(g,i%2?r.getRight()-5.f:r.getX()+5.f,i<2?r.getY()+5.f:r.getBottom()-5.f,2.8f,.9f*i);
    const auto win=r.reduced(6.f,5.f);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff57432b),win.getY(),Colour(0xff15120e),win.getBottom()));
    g.fillRoundedRectangle(win,4);                                                // dark cavity
    // Actual enclosure lip casts a narrow shadow into the tape window.
    g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::black.withAlpha(.65f),win.getY(),juce::Colours::transparentBlack,win.getY()+10.f));g.fillRect(win.withHeight(10.f));
    g.setColour(juce::Colours::black.withAlpha(.55f));g.drawRoundedRectangle(win,4,1.2f);

    // A warm cavity lamp and metal transport bed beneath the reels.
    g.setGradientFill(juce::ColourGradient(Colour(0xffd5a15e).withAlpha(.34f),win.getCentreX(),win.getY()+8.f,juce::Colours::transparentBlack,win.getCentreX(),win.getBottom(),true));g.fillRoundedRectangle(win.reduced(1.f),3.f);
    const auto bed=win.reduced(3.f).removeFromBottom(18.f);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff8a6337),bed.getY(),Colour(0xff2e2113),bed.getBottom()));g.fillRoundedRectangle(bed,2.f);
    g.setColour(Colour(0xffd8b17d).withAlpha(.55f));g.drawHorizontalLine(int(bed.getY()),bed.getX()+2,bed.getRight()-2);
    const float radius=std::min(win.getWidth()*.239f,win.getHeight()*.435f);
    const float cy=win.getCentreY()-5.f;
    const float cxs[2]={win.getX()+win.getWidth()*.27f,win.getX()+win.getWidth()*.73f};
    const float floorY=std::min(cy+radius*.98f,win.getBottom()-22.f);

    // Tape path: off each reel pack, round a guide post, across the head block.
    {   const float headW=win.getWidth()*.13f,hx=win.getCentreX(),hy=floorY;
        juce::Path tape;
        tape.startNewSubPath(cxs[0]+radius*.62f,cy+radius*.74f);
        tape.lineTo(cxs[0]+radius*1.18f,floorY);
        tape.lineTo(hx-headW*.5f,hy);tape.lineTo(hx+headW*.5f,hy);
        tape.lineTo(cxs[1]-radius*1.18f,floorY);
        tape.lineTo(cxs[1]-radius*.62f,cy+radius*.74f);
        g.setColour(juce::Colours::black.withAlpha(.45f));
        g.strokePath(tape,juce::PathStrokeType(2.6f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded),juce::AffineTransform::translation(0,1));
        g.setColour(Colour(0xff6a4a2a));
        g.strokePath(tape,juce::PathStrokeType(1.7f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        g.setColour(Colour(0xffa57a48).withAlpha(.55f));
        g.strokePath(tape,juce::PathStrokeType(.7f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded),juce::AffineTransform::translation(0,-.6f));
        juce::Rectangle<float> head(hx-headW*.5f,hy+1.5f,headW,6.f);                // head block
        g.setGradientFill(juce::ColourGradient::vertical(Colour(0xffc9b48d),head.getY(),Colour(0xff6c5a3d),head.getBottom()));
        g.fillRoundedRectangle(head,1.5f);
        g.setColour(Colour(0xff000000).withAlpha(.6f));g.drawRoundedRectangle(head,1.5f,.8f);
        for(int k=0;k<2;++k){                                                        // guide posts
            const float gx=k==0?cxs[0]+radius*1.18f:cxs[1]-radius*1.18f;
            g.setColour(juce::Colours::black.withAlpha(.6f));g.fillEllipse(gx-3.f,floorY-2.f,6.f,6.f);
            g.setGradientFill(juce::ColourGradient(Colour(0xffe6dac0),gx-1.2f,floorY-1.5f,Colour(0xff6b6150),gx+1.5f,floorY+2.f,false));
            g.fillEllipse(gx-2.2f,floorY-2.8f,4.4f,4.4f);
        }
    }
    // Reels: tape pack, machined flange with three curved openings, hub and spindle.
    for(int k=0;k<2;++k){
        const float cx=cxs[k];
        g.setColour(juce::Colours::black.withAlpha(.55f));g.fillEllipse(cx-radius-2,cy-radius+1.5f,2*radius+4,2*radius+4);
        g.setGradientFill(juce::ColourGradient(Colour(0xff4a3524),cx-radius*.5f,cy-radius*.6f,Colour(0xff150f0a),cx+radius*.8f,cy+radius*.9f,false));
        g.fillEllipse(cx-radius,cy-radius,2*radius,2*radius);                         // wound tape
        g.setColour(Colour(0xff000000).withAlpha(.5f));g.drawEllipse(cx-radius,cy-radius,2*radius,2*radius,1.2f);
        const float fr=radius*.98f;
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.setOpacity(1.f);
        g.drawImage(machinedCap(Colour(0xffd8c6a8)),juce::Rectangle<float>(cx-fr,cy-fr,2*fr,2*fr));
        {juce::Graphics::ScopedSaveState save(g);
         juce::Path disc;disc.addEllipse(cx-fr*.94f,cy-fr*.94f,fr*1.88f,fr*1.88f);g.reduceClipRegion(disc);
         juce::ColourGradient reflect(Colour(0xfff6dcb4).withAlpha(.38f),cx-fr*.62f,cy-fr*.62f,Colour(0xff100d09).withAlpha(.19f),cx+fr*.82f,cy+fr*.83f,false);
         reflect.addColour(.58,juce::Colours::transparentBlack);g.setGradientFill(reflect);g.fillPath(disc);}
        // Fine machining rings on the reel flange, behind the cutouts.
        for(int ring=0;ring<5;++ring){const float rr=fr*(.88f+ring*.018f);g.setColour(Colour(0xfff4dfb9).withAlpha(.16f));g.drawEllipse(cx-rr,cy-rr,2*rr,2*rr,.5f);}
        const float turn=k==0?.17f:.69f;                                                // reels deliberately out of phase
        for(int spoke=0;spoke<3;++spoke){
            const float from=turn+spoke*juce::MathConstants<float>::twoPi/3+.49f;
            const float to=turn+(spoke+1)*juce::MathConstants<float>::twoPi/3-.49f;
            // Rounded tapered triangular ports rather than broad pie slices.
            auto point=[&](float a,float rr){return juce::Point<float>(cx+std::sin(a)*rr,cy-std::cos(a)*rr);};
            const float mid=(from+to)*.5f;
            juce::Path opening;opening.startNewSubPath(point(from,fr*.81f));
            opening.quadraticTo(point(mid,fr*.91f),point(to,fr*.81f));
            opening.lineTo(point(mid+.26f,fr*.39f));opening.quadraticTo(point(mid,fr*.35f),point(mid-.26f,fr*.39f));opening.closeSubPath();
            opening=opening.createPathWithRoundedCorners(1.6f);
            g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff392e20),cy-fr*.8f,Colour(0xff110d09),cy+fr*.8f));
            g.fillPath(opening);
            {juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(opening);
             // Windings visible through each recessed port.
             for(int band=0;band<8;++band){const float rr=fr*(.40f+band*.053f);g.setColour(Colour(0xffc4a77b).withAlpha(.10f));g.drawEllipse(cx-rr,cy-rr,2*rr,2*rr,.55f);}}
            g.setColour(Colour(0xffbe9663).withAlpha(.11f));g.strokePath(opening,juce::PathStrokeType(2.1f),juce::AffineTransform::translation(.2f,1.f));
            g.setColour(Colour(0xffe9dcc0).withAlpha(.36f));                          // lit rim of the opening
            g.strokePath(opening,juce::PathStrokeType(.9f),juce::AffineTransform::translation(.5f,.7f));
            g.setColour(Colour(0xff000000).withAlpha(.84f));g.strokePath(opening,juce::PathStrokeType(.85f));
            g.setColour(Colour(0xffcdb48b).withAlpha(.22f));g.strokePath(opening,juce::PathStrokeType(.55f),juce::AffineTransform::translation(-.45f,-.35f));
        }
        juce::Path flange;flange.addCentredArc(cx,cy,fr*.96f,fr*.96f,0.f,-1.7f,.65f,true);g.setColour(Colour(0xfff9e6c6).withAlpha(.65f));g.strokePath(flange,juce::PathStrokeType(.85f));
        g.setColour(juce::Colours::black.withAlpha(.65f));g.drawEllipse(cx-fr*.99f,cy-fr*.99f,fr*1.98f,fr*1.98f,.8f);   // rim highlight
        g.setColour(juce::Colours::black.withAlpha(.65f));g.fillEllipse(cx-fr*.37f+1.f,cy-fr*.37f+1.5f,fr*.74f,fr*.74f);
        g.setGradientFill(juce::ColourGradient(Colour(0xffd9c4a1),cx-fr*.28f,cy-fr*.28f,Colour(0xff241b11),cx+fr*.32f,cy+fr*.32f,false));g.fillEllipse(cx-fr*.36f,cy-fr*.36f,fr*.72f,fr*.72f);
        g.setGradientFill(juce::ColourGradient(Colour(0xfff1e2bf),cx-fr*.16f,cy-fr*.2f,Colour(0xff7c6a48),cx+fr*.2f,cy+fr*.24f,false));
        g.fillEllipse(cx-fr*.26f,cy-fr*.26f,fr*.52f,fr*.52f);                           // brass hub
        g.setColour(Colour(0xff211b14));g.fillEllipse(cx-fr*.17f,cy-fr*.17f,fr*.34f,fr*.34f);
        g.setColour(Colour(0xfff4e3c6).withAlpha(.55f));g.drawEllipse(cx-fr*.254f,cy-fr*.254f,fr*.508f,fr*.508f,.7f);
        g.setGradientFill(juce::ColourGradient(Colour(0xffb6a78e),cx-fr*.10f,cy-fr*.10f,Colour(0xff19140e),cx+fr*.12f,cy+fr*.13f,false));g.fillEllipse(cx-fr*.125f,cy-fr*.125f,fr*.25f,fr*.25f);
        g.setColour(Colour(0xff0b0a08));g.fillEllipse(cx-fr*.045f,cy-fr*.045f,fr*.09f,fr*.09f);
        g.setColour(Colour(0xffffe4bc).withAlpha(.65f));g.fillEllipse(cx-fr*.095f,cy-fr*.10f,fr*.07f,fr*.05f);
    }
    // Curved glass over the window.
    {   juce::Graphics::ScopedSaveState keep(g);
        juce::Path clipWin;clipWin.addRoundedRectangle(win.getX(),win.getY(),win.getWidth(),win.getHeight(),4.f);
        g.reduceClipRegion(clipWin);
        juce::Path glass;glass.startNewSubPath(win.getX(),win.getY());glass.lineTo(win.getRight(),win.getY());
        glass.lineTo(win.getRight(),win.getY()+win.getHeight()*.20f);
        glass.quadraticTo(win.getCentreX(),win.getY()+win.getHeight()*.40f,win.getX(),win.getY()+win.getHeight()*.20f);
        glass.closeSubPath();
        g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::white.withAlpha(.11f),win.getY(),juce::Colours::white.withAlpha(.012f),win.getY()+win.getHeight()*.38f));
        g.fillPath(glass);
        g.setColour(Colour(0xfff5dbad).withAlpha(.30f));g.drawRoundedRectangle(win.reduced(.9f),3.f,.75f);
        g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::transparentWhite,win.getBottom()-7.f,Colour(0xffe5ba76).withAlpha(.26f),win.getBottom()));g.fillRoundedRectangle(win.withY(win.getBottom()-7.f).withHeight(7.f),2.f);
    }
    // The tape mechanism is decorative; its live controls are below the window.
}
inline void paintOriginalSurface(Graphics& g,juce::Rectangle<float> r,int type,int finish) {
    const auto p=originalSpec(type,finish);
    // The reference's dark anodised panels are substantially darker and more
    // saturated than the previous washed-out capture. Avoid additive white
    // gradients that lift the entire blue/olive/burgundy material.
    const Colour base=p.light?p.metal.interpolatedWith(Colour(0xffead6b8),.08f)
        :p.metal.interpolatedWith(Colour(0xff0e0d0b),.25f);
    paintHardwarePlate(g,r.reduced(.3f),base,p.light,9.f,3.8f);
    for(int i=0;i<4;++i)paintScrew(g,i%2?r.getRight()-12:r.getX()+12,i<2?r.getY()+12:r.getBottom()-12,6.2f,.8f*i);
}
inline void paintOriginalMini(Graphics& g,juce::Rectangle<float> r,int type) {
    const auto p=originalSpec(type,1);
    // Small library symbols use a single hardware badge, not tiny versions
    // of the entire faceplate (which produced four distracting screw dots).
    g.setGradientFill(juce::ColourGradient(p.metal.brighter(.22f),r.getX(),r.getY(),p.metal.darker(.42f),r.getRight(),r.getBottom(),false));
    g.fillRoundedRectangle(r.reduced(1.f),4.f);
    g.setColour(Colour(0xffd0bda1).withAlpha(.56f));g.drawRoundedRectangle(r.reduced(1.4f),4.f,.75f);
    g.setColour(juce::Colours::black.withAlpha(.36f));g.drawRoundedRectangle(r.reduced(2.8f),3.f,.7f);
    auto f=r.reduced(6);
    if(type==Bell){
        // Physical equalizer control rather than a generic curve: like the
        // reference rack library, a tiny metal knob reads at a glance.
        const float cx=f.getCentreX(),cy=f.getCentreY(),rad=juce::jmin(f.getWidth(),f.getHeight())*.40f;
        g.setColour(juce::Colours::black.withAlpha(.65f));g.fillEllipse(cx-rad+1,cy-rad+2,2*rad,2*rad);
        g.setOpacity(1.f);
        g.drawImage(machinedCap(Colour(0xffb6a58b)),juce::Rectangle<float>(cx-rad,cy-rad,2*rad,2*rad));
        for(int i=0;i<9;++i){const float a=juce::degreesToRadians(225.f+i*270.f/8.f);g.setColour(Colour(0xffe9d9bb));g.fillEllipse(cx+std::sin(a)*rad*1.26f-.55f,cy-std::cos(a)*rad*1.26f-.55f,1.1f,1.1f);}
        g.setColour(Colour(0xff201914));g.drawLine(cx,cy-rad*.32f,cx+rad*.21f,cy-rad*.9f,1.2f);return;
    }
    if(type==DeEss||type==SkellDeEsserFX){
        // A fine grille of attenuation dots is easier to identify than yet another meter.
        const auto field=r.reduced(5.f);
        for(int row=0;row<5;++row)for(int col=0;col<7;++col){
            const float x=field.getX()+(col+.5f)*field.getWidth()/7.f;
            const float y=field.getY()+(row+.5f)*field.getHeight()/5.f;
            const float a=.42f+.48f*(1.f-std::abs(row-2)/2.f)*(1.f-std::abs(col-3)/3.f);
            g.setColour(Colour(0xffead7b4).withAlpha(a));g.fillEllipse(x-.62f,y-.62f,1.24f,1.24f);
        }return;
    }
    if(type==Sat){
        // A vacuum-tube-inspired badge, hand drawn and not a borrowed image.
        const auto tube=r.withSizeKeepingCentre(r.getWidth()*.29f,r.getHeight()*.74f);
        g.setGradientFill(juce::ColourGradient::vertical(Colour(0xfff3d4a0),tube.getY(),Colour(0xff68401d),tube.getBottom()));
        g.drawRoundedRectangle(tube,5.f,1.2f);
        auto inner=tube.reduced(2.6f,4.f);g.setColour(Colour(0xffcf9552));
        for(int k=0;k<3;++k)g.drawVerticalLine(int(inner.getX()+k*inner.getWidth()/2.f),inner.getY(),inner.getBottom());
        g.setColour(Colour(0xfff0c789));g.drawHorizontalLine(int(tube.getBottom()-5),tube.getX()+2,tube.getRight()-2);
        g.fillRoundedRectangle(tube.withY(tube.getBottom()-2.f).withHeight(3.f),1.f);return;
    }
    if(info(type).hasVU||type==Flanger){
        // A legible meter icon, not a reduced full-size scale.
        f=r.reduced(3.5f);
        g.setColour(Colour(0xff18140f));g.fillRoundedRectangle(f,3);
        f=f.reduced(2);g.setGradientFill(juce::ColourGradient::vertical(Colour(0xfff2d7a3),f.getY(),Colour(0xffad8050),f.getBottom()));g.fillRoundedRectangle(f,2);
        const float cx=f.getCentreX(),cy=f.getBottom()-2,rx=f.getWidth()*.47f,ry=f.getHeight()*.76f;
        juce::Path arc;
        for(int k=0;k<=20;++k){const float a=juce::degreesToRadians(-55.f+k*110.f/20.f);const float x=cx+rx*std::sin(a),y=cy-ry*std::cos(a);if(k==0)arc.startNewSubPath(x,y);else arc.lineTo(x,y);}
        g.setColour(Colour(0xff57402b));g.strokePath(arc,juce::PathStrokeType(.65f));
        for(int k=0;k<=6;++k){const float a=juce::degreesToRadians(-55.f+k*110.f/6);
            g.setColour(Colour(0xff413225));g.drawLine(cx+rx*.85f*std::sin(a),cy-ry*.85f*std::cos(a),cx+rx*std::sin(a),cy-ry*std::cos(a),.7f);}
        g.setColour(Colour(0xff9b3224));g.drawLine(cx,cy,cx-rx*.32f,cy-ry*.93f,1);return;
    }
    if(type==Delay||type==Slap){
        f=r.reduced(4);g.setColour(Colour(0xff211b14));g.fillRoundedRectangle(f,3);
        const float radius=std::min(f.getWidth()*.22f,f.getHeight()*.33f);
        for(int k=0;k<2;++k){const float cx=f.getX()+f.getWidth()*(k?.74f:.26f),cy=f.getCentreY();
            g.setOpacity(1.f);
            g.drawImage(machinedCap(Colour(0xffb6a58b)),juce::Rectangle<float>(cx-radius,cy-radius,2*radius,2*radius));
            for(int spoke=0;spoke<3;++spoke){juce::Path cut;const float a=k*.55f+spoke*juce::MathConstants<float>::twoPi/3.f;cut.addPieSegment(cx-radius*.8f,cy-radius*.8f,radius*1.6f,radius*1.6f,a+.5f,a+1.59f,.38f);g.setColour(Colour(0xff211b13));g.fillPath(cut);}
            g.setColour(Colour(0xff352c21));g.fillEllipse(cx-radius*.32f,cy-radius*.32f,radius*.64f,radius*.64f);
            g.setColour(Colour(0xffd2bd99));g.fillEllipse(cx-radius*.12f,cy-radius*.12f,radius*.24f,radius*.24f);}
        return;
    }
    // Family glyphs so the library never shows the same icon for different effects.
    {
        const auto savedFrame=f;f=r.reduced(5);
        const bool dark=!p.light;
        const auto ink=dark?Colour(0xffeee3cc):Colour(0xff2b2418);
        const float x0=f.getX()+2,x1=f.getRight()-2,w=x1-x0,my=f.getCentreY(),amp=f.getHeight()*.30f;
        auto strokePathInk=[&](const juce::Path& path,float width,float alpha=1.f){
            g.setColour(ink.withAlpha(alpha));
            g.strokePath(path,juce::PathStrokeType(width,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));};
        auto wave=[&](float phase,float scale,float cycles){
            juce::Path path;
            for(int i=0;i<=40;++i){const float t=i/40.f,x=x0+w*t,y=my-amp*scale*std::sin(juce::MathConstants<float>::twoPi*cycles*t+phase);
                if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}
            return path;};
        auto guideLines=[&]{g.setColour(ink.withAlpha(.14f));
            for(int k=-1;k<=1;++k)g.drawHorizontalLine(int(my+k*amp),x0,x1);};
        bool drawn=true;
        switch(type){
        case Eq:case Bell:case Tilt:case DualFilter:{
            guideLines();juce::Path curve;
            for(int i=0;i<=40;++i){const float t=i/40.f,x=x0+w*t;float y=my;
                if(type==Eq){y=my-amp*(.75f*std::exp(-std::pow((t-.28f)/.12f,2.f))-.55f*std::exp(-std::pow((t-.72f)/.13f,2.f)));}
                else if(type==Bell){y=my-amp*1.05f*std::exp(-std::pow((t-.5f)/.16f,2.f));}
                else if(type==Tilt){y=my+amp*(t-.5f)*-1.6f;}
                else{y=my-amp*(1.f/(1.f+std::exp(-(t-.12f)*28.f)))*(1.f/(1.f+std::exp((t-.88f)*28.f)))*1.1f+amp*.55f;}
                if(i==0)curve.startNewSubPath(x,y);else curve.lineTo(x,y);}
            strokePathInk(curve,1.8f);break;}
        case Sat:{   // soft saturation transfer curve
            guideLines();juce::Path curve;
            for(int i=0;i<=40;++i){const float t=i/40.f*2.f-1.f,x=x0+w*(t*.5f+.5f),y=my-amp*1.15f*std::tanh(2.4f*t);
                if(i==0)curve.startNewSubPath(x,y);else curve.lineTo(x,y);}
            strokePathInk(curve,1.8f);break;}
        case SoftClip:{ // sine flattened at the ceiling
            guideLines();juce::Path clipped;
            for(int i=0;i<=40;++i){const float t=i/40.f,x=x0+w*t,s=std::sin(juce::MathConstants<float>::twoPi*1.5f*t)*1.6f,y=my-amp*std::clamp(s,-.8f,.8f);
                if(i==0)clipped.startNewSubPath(x,y);else clipped.lineTo(x,y);}
            strokePathInk(clipped,1.8f);break;}
        case Chorus:case Phaser:strokePathInk(wave(0.f,1.f,1.5f),1.6f);strokePathInk(wave(type==Chorus?.9f:1.7f,1.f,1.5f),1.6f,.55f);break;
        case Tremolo:{
            juce::Path top,bottom;
            for(int i=0;i<=40;++i){const float t=i/40.f,x=x0+w*t,e=.35f+.65f*std::abs(std::sin(juce::MathConstants<float>::pi*2.f*t));
                if(i==0){top.startNewSubPath(x,my-amp*e);bottom.startNewSubPath(x,my+amp*e);}else{top.lineTo(x,my-amp*e);bottom.lineTo(x,my+amp*e);}}
            strokePathInk(top,1.5f);strokePathInk(bottom,1.5f);strokePathInk(wave(0.f,.45f,6.f),1.1f,.5f);break;}
        case Reverb:{   // impulse followed by decaying reflections
            for(int k=0;k<9;++k){const float x=x0+w*(.06f+k*.105f),hgt=amp*1.5f*std::pow(.72f,float(k));
                g.setColour(ink.withAlpha(.95f-.07f*k));g.drawLine(x,my+amp*.9f,x,my+amp*.9f-hgt*(k==0?1.f:1.25f),1.8f);}
            break;}
        case Air:case SkellAirFX:{
            for(int k=0;k<2;++k){juce::Path chev;const float cy=my+amp*(.55f-k*.9f),cx=f.getCentreX(),sp=w*.28f;
                chev.startNewSubPath(cx-sp,cy+amp*.35f);chev.lineTo(cx,cy-amp*.35f);chev.lineTo(cx+sp,cy+amp*.35f);strokePathInk(chev,2.f,k?1.f:.55f);}
            break;}
        case Tune:{
            for(int k=0;k<5;++k)g.setColour(ink.withAlpha(.22f)),g.drawHorizontalLine(int(f.getY()+3+k*(f.getHeight()-6)/4),x0,x1);
            juce::Path steps;const float ys[4]={.78f,.78f,.45f,.18f};
            for(int k=0;k<4;++k){const float xa=x0+w*k/4.f,xb=x0+w*(k+1)/4.f,y=f.getY()+3+ys[k]*(f.getHeight()-6);
                if(k==0)steps.startNewSubPath(xa,y);else steps.lineTo(xa,y);steps.lineTo(xb,y);}
            strokePathInk(steps,1.9f);
            g.setColour(p.accent);g.fillEllipse(x1-5.f,f.getY()+3+.18f*(f.getHeight()-6)-2.5f,5.f,5.f);break;}
        case Stereo:{
            const float rad=f.getHeight()*.33f;
            g.setColour(ink.withAlpha(.9f));g.drawEllipse(f.getCentreX()-rad*1.15f,my-rad,2*rad,2*rad,1.6f);
            g.setColour(ink.withAlpha(.55f));g.drawEllipse(f.getCentreX()-rad*.85f,my-rad,2*rad,2*rad,1.6f);break;}
        case Trim:{
            g.setColour(ink.withAlpha(.35f));g.fillRoundedRectangle(x0,my-1.5f,w,3.f,1.5f);
            g.setColour(ink.withAlpha(.9f));g.fillRoundedRectangle(x0,my-1.5f,w*.62f,3.f,1.5f);
            g.setGradientFill(juce::ColourGradient(juce::Colours::white,x0+w*.62f-2.f,my-3.f,p.cap.darker(.5f),x0+w*.62f+4.f,my+4.f,false));
            g.fillEllipse(x0+w*.62f-4.5f,my-4.5f,9.f,9.f);break;}
        case Transient:{   // attack spike with a decaying sustain tail
            juce::Path env;env.startNewSubPath(x0,my+amp);env.lineTo(x0+w*.18f,my+amp);env.lineTo(x0+w*.26f,my-amp*1.15f);
            env.quadraticTo(x0+w*.46f,my-amp*.1f,x1,my+amp*.7f);strokePathInk(env,1.9f);break;}
        default:drawn=false;break;
        }
        if(drawn)return;
        f=savedFrame;
    }
    const auto center=f.getCentre();const float d=std::min(f.getWidth(),f.getHeight())*.58f;
    g.setColour(p.cap);g.fillEllipse(center.x-d/2,center.y-d/2,d,d);
    g.setColour(p.light?juce::Colours::white:juce::Colours::black);g.drawLine(center.x,center.y,center.x+d*.17f,center.y-d*.39f,1.2f);
}
}
