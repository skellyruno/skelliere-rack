#pragma once
#include "Look.h"
namespace skl::ui {
struct PanelSpec { Colour metal,accent,cap; bool light; const char* series; };
inline PanelSpec originalSpec(int type,int finish) {
    static constexpr unsigned metals[]={0x25292c,0x9eaaa6,0xc9b398,0x75463f,0x5d4132,0xc4c0ad,0x384859,0x414b35,0x45433e,0x323235,0x6b716e,0x242d34,0x97876e,0x425c61,0x62403b,0x344c59,0xc1aa83,0x454f3e,0x71614d,0x344b61,0x5e6354,0x784d3e,0x3c594d,0x4e4059,0x663d38,0x706449,0x496464,0x45484a};
    static constexpr unsigned accents[]={0x9a9a9a,0x39534b,0xaed5bf,0xe0b88e,0xe9be7b,0x365b55,0xa4bedc,0xe7c68c,0xaed0aa,0xe2ad9f,0xb5d3c4,0x9bddb7,0x3c514b,0xa4d4df,0xe5d9bf,0xb7d8cf,0x26392f,0xc0d58e,0xf0cc9b,0x405e50,0xe1d7ab,0xf3b397,0xc1d4ab,0xd5b7de,0xa7d5e5,0xf0cd98,0xc6e8da,0xc6c7b4};
    type=std::clamp(type,0,static_cast<int>(kNumRackTypes)-1);
    auto metal=Colour(0xff000000u|metals[type]);const auto accent=Colour(0xff000000u|accents[type]);
    if(finish==0)metal=Colour(0xff303437);
    if(finish==2)metal=Colour(0xffb1b6b1);
    if(finish==3)metal=Colour(0xff222829);
    const bool light=metal.getPerceivedBrightness()>.45f;
    const auto cap=type%4==0?Colour(0xffc6b69e):Colour(0xffd9ccba);
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
    // Decorative tape-inspired mechanical illustration. It is not a signal meter and does not
    // show tape physics; the real Time / Feedback / Tone / Mix controls are the knobs below.
    g.setColour(juce::Colours::black.withAlpha(.45f));g.fillRoundedRectangle(r.translated(0,2),7);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff3b362d),r.getY(),Colour(0xff0f0e0c),r.getBottom()));
    g.fillRoundedRectangle(r,6);                                                 // machined bezel
    g.setColour(Colour(0xff000000).withAlpha(.6f));g.drawRoundedRectangle(r.reduced(2.5f),5,1);
    for(int i=0;i<4;++i)paintScrew(g,i%2?r.getRight()-5.f:r.getX()+5.f,i<2?r.getY()+5.f:r.getBottom()-5.f,2.8f,.9f*i);
    const auto win=r.reduced(7.f,6.f);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff3a2c1b),win.getY(),Colour(0xff15120e),win.getBottom()));
    g.fillRoundedRectangle(win,4);                                                // dark cavity
    g.setColour(juce::Colours::black.withAlpha(.55f));g.drawRoundedRectangle(win,4,1.2f);

    const float radius=std::min(win.getWidth()*.235f,win.getHeight()*.38f);
    const float cy=win.getCentreY()-4.f;
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
        const float fr=radius*.80f;
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(machinedCap(Colour(0xffb6a58b)),juce::Rectangle<float>(cx-fr,cy-fr,2*fr,2*fr));
        const float turn=k==0?.30f:.62f;                                                // reels deliberately out of phase
        for(int spoke=0;spoke<3;++spoke){
            const float from=turn+spoke*juce::MathConstants<float>::twoPi/3+.20f;
            const float to=turn+(spoke+1)*juce::MathConstants<float>::twoPi/3-.20f;
            juce::Path opening;
            opening.addPieSegment(cx-fr*.82f,cy-fr*.82f,fr*1.64f,fr*1.64f,from,to,.36f);
            g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff1a140d),cy-fr*.8f,Colour(0xff0b0907),cy+fr*.8f));
            g.fillPath(opening);
            g.setColour(Colour(0xffe9dcc0).withAlpha(.30f));                          // lit rim of the opening
            g.strokePath(opening,juce::PathStrokeType(.9f),juce::AffineTransform::translation(.5f,.7f));
            g.setColour(Colour(0xff000000).withAlpha(.7f));g.strokePath(opening,juce::PathStrokeType(.8f));
        }
        g.setColour(juce::Colours::white.withAlpha(.22f));g.drawEllipse(cx-fr*.97f,cy-fr*.97f,fr*1.94f,fr*1.94f,.9f);   // rim highlight
        g.setColour(Colour(0xff17120d));g.fillEllipse(cx-fr*.36f,cy-fr*.36f,fr*.72f,fr*.72f);
        g.setGradientFill(juce::ColourGradient(Colour(0xfff1e2bf),cx-fr*.16f,cy-fr*.2f,Colour(0xff7c6a48),cx+fr*.2f,cy+fr*.24f,false));
        g.fillEllipse(cx-fr*.26f,cy-fr*.26f,fr*.52f,fr*.52f);                           // brass hub
        g.setColour(Colour(0xff0b0a08));g.fillEllipse(cx-fr*.07f,cy-fr*.07f,fr*.14f,fr*.14f);
    }
    // Curved glass over the window.
    {   juce::Graphics::ScopedSaveState keep(g);
        juce::Path clipWin;clipWin.addRoundedRectangle(win.getX(),win.getY(),win.getWidth(),win.getHeight(),4.f);
        g.reduceClipRegion(clipWin);
        juce::Path glass;glass.startNewSubPath(win.getX(),win.getY());glass.lineTo(win.getRight(),win.getY());
        glass.lineTo(win.getRight(),win.getY()+win.getHeight()*.20f);
        glass.quadraticTo(win.getCentreX(),win.getY()+win.getHeight()*.40f,win.getX(),win.getY()+win.getHeight()*.20f);
        glass.closeSubPath();
        g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::white.withAlpha(.16f),win.getY(),juce::Colours::white.withAlpha(.02f),win.getY()+win.getHeight()*.38f));
        g.fillPath(glass);
    }
    g.setColour(Colour(0xffd0b68e));g.setFont(font(9,true));
    g.drawText("ECHO",r.removeFromBottom(19),juce::Justification::centred,false);
}
inline void paintOriginalSurface(Graphics& g,juce::Rectangle<float> r,int type,int finish) {
    const auto p=originalSpec(type,finish);
    g.setColour(Colour(0xff08090a));g.fillRoundedRectangle(r,3);               // recessed frame shadow
    const auto plate=r.reduced(2.5f);
    // Measured against the approved reference, dark finishes sat 20-50 levels too bright. Light
    // finishes already matched (within ~8/255) and are left alone.
    const Colour base=p.light?p.metal:p.metal.withMultipliedBrightness(.76f);
    // Nearly flat body colour; the old strong top-to-bottom ramp produced visible horizontal bands.
    g.setGradientFill(juce::ColourGradient::vertical(base.brighter(.04f),plate.getY(),base.darker(.05f),plate.getBottom()));
    g.fillRoundedRectangle(plate,5);
    {
        juce::Graphics::ScopedSaveState clip(g);
        juce::Path outline;outline.addRoundedRectangle(plate.getX(),plate.getY(),plate.getWidth(),plate.getHeight(),5.f);
        g.reduceClipRegion(outline);
        // Soft specular pool from the upper left.
        const float lx=plate.getX()+plate.getWidth()*.30f,ly=plate.getY()+plate.getHeight()*.20f;
        g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(p.light?.15f:.10f),lx,ly,
            juce::Colours::transparentWhite,lx+plate.getWidth()*.95f,ly,true));
        g.fillRect(plate);
        // Darkening toward the edges, clear through the middle.
        const float mx=plate.getCentreX(),my=plate.getCentreY();
        juce::ColourGradient vignette(juce::Colours::transparentBlack,mx,my,
            juce::Colours::black.withAlpha(p.light?.20f:.30f),mx+plate.getWidth()*.62f,my+plate.getHeight()*.62f,true);
        vignette.addColour(.55,juce::Colours::transparentBlack);
        g.setGradientFill(vignette);g.fillRect(plate);
        paintBrushed(g,plate,p.light?.54f:.33f);                                   // coherent fibre grain
        // Bevel: lit top and left edges, shaded bottom and right edges.
        g.setColour(juce::Colours::white.withAlpha(.39f));
        g.drawHorizontalLine(int(plate.getY())+1,plate.getX()+4,plate.getRight()-4);
        g.setColour(juce::Colours::white.withAlpha(.12f));
        g.drawVerticalLine(int(plate.getX())+1,plate.getY()+4,plate.getBottom()-4);
        g.setColour(juce::Colours::black.withAlpha(.50f));
        g.drawHorizontalLine(int(plate.getBottom())-2,plate.getX()+4,plate.getRight()-4);
        g.drawVerticalLine(int(plate.getRight())-2,plate.getY()+4,plate.getBottom()-4);
    }
    g.setColour(juce::Colours::white.withAlpha(.31f));g.drawRoundedRectangle(plate.reduced(.5f),5,1.2f);
    g.setColour(juce::Colours::black.withAlpha(.40f));g.drawRoundedRectangle(plate.reduced(3.2f),4,1.1f);   // engraved panel line
    g.setColour(juce::Colours::white.withAlpha(.07f));g.drawRoundedRectangle(plate.reduced(3.5f),4,.8f);
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
