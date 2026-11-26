#pragma once
// Original Skell engravings. Articulated decorative artwork; not DSP displays.
// Included inside skl::ui, after Look.h. All coordinates are local to the artwork.
enum class BonePose { floatUp, stand, mirror, reverse, warm };
inline void paintBoneFigure(Graphics& g,juce::Rectangle<float> r,Colour ink,BonePose pose,float phase=0.f){
    juce::Graphics::ScopedSaveState save(g);
    const float s=std::min(r.getWidth()/44.f,r.getHeight()/76.f);
    if(s<=0.f)return;
    const float sway=std::sin(phase), counter=std::sin(phase*2.f);
    const float bob=pose==BonePose::floatUp?5.2f:pose==BonePose::stand?1.3f:1.9f;
    g.addTransform(juce::AffineTransform::rotation(.085f*sway,22.f,44.f)
        .scaled(s).translated(r.getCentreX()-22.f*s,r.getCentreY()-38.f*s+bob*s*sway));
    auto line=[&](float x,float y,float xx,float yy){g.setColour(ink);g.drawLine(x,y,xx,yy,1.65f);};
    auto joint=[&](float x,float y){g.setColour(ink);g.fillEllipse(x-1.15f,y-1.15f,2.3f,2.3f);};
    // Skull has visible sockets and jaw, rather than a stick-person circle.
    juce::Path skull;skull.startNewSubPath(15,14);skull.cubicTo(10,1,33,1,29,14);skull.lineTo(26,17);skull.lineTo(26,21);skull.lineTo(18,21);skull.lineTo(18,17);skull.closeSubPath();
    g.setColour(ink);g.strokePath(skull,juce::PathStrokeType(1.55f,juce::PathStrokeType::curved));
    g.fillEllipse(16,9,4.4f,4.8f);g.fillEllipse(23,9,4.4f,4.8f);g.fillEllipse(21,14,2.f,2.4f);
    for(int i=0;i<3;++i)line(19+i*3,18,19+i*3,20);
    line(22,22,22,48);line(11,26,33,26);
    for(int i=0;i<4;++i){float y=28+i*4.f;juce::Path rib;rib.startNewSubPath(22,y+2);rib.quadraticTo(7+i*1.8f,y+5,13+i*1.5f,y-1);rib.startNewSubPath(22,y+2);rib.quadraticTo(37-i*1.8f,y+5,31-i*1.5f,y-1);g.strokePath(rib,juce::PathStrokeType(1.15f));}
    juce::Path pelvis;pelvis.startNewSubPath(22,44);pelvis.lineTo(15,44);pelvis.lineTo(17,51);pelvis.lineTo(22,53);pelvis.lineTo(27,51);pelvis.lineTo(29,44);pelvis.closeSubPath();g.strokePath(pelvis,juce::PathStrokeType(1.4f));
    float ex1=6,ey1=37,hx1=2,hy1=47,ex2=38,ey2=37,hx2=42,hy2=47;
    if(pose==BonePose::floatUp){ex1=6;ey1=20;hx1=2;hy1=12;ex2=38;ey2=20;hx2=42;hy2=12;}
    if(pose==BonePose::warm){ex1=7;ey1=38;hx1=18;hy1=35;ex2=37;ey2=38;hx2=26;hy2=35;}
    if(pose==BonePose::mirror){ex1=6;ey1=34;hx1=2;hy1=27;ex2=36;ey2=18;hx2=42;hy2=13;}
    if(pose==BonePose::reverse){ex1=8;ey1=18;hx1=2;hy1=13;ex2=38;ey2=34;hx2=42;hy2=27;}
    // Rotate linked limb segments about the actual joints, preserving bone length.
    auto rotate=[](float& x,float& y,float cx,float cy,float a){const float dx=x-cx,dy=y-cy;x=cx+dx*std::cos(a)-dy*std::sin(a);y=cy+dx*std::sin(a)+dy*std::cos(a);};
    const float arm=(pose==BonePose::warm?.12f:.31f)*sway, wrist=.20f*counter;
    rotate(ex1,ey1,11,26,arm);rotate(hx1,hy1,11,26,arm);rotate(hx1,hy1,ex1,ey1,wrist);
    rotate(ex2,ey2,33,26,-arm);rotate(hx2,hy2,33,26,-arm);rotate(hx2,hy2,ex2,ey2,-wrist);
    line(11,26,ex1,ey1);line(ex1,ey1,hx1,hy1);joint(ex1,ey1);joint(hx1,hy1);
    line(33,26,ex2,ey2);line(ex2,ey2,hx2,hy2);joint(ex2,ey2);joint(hx2,hy2);
    float ly=pose==BonePose::floatUp?61.f:63.f,rx=pose==BonePose::floatUp?31.f:28.f;
    float lx=15.f,ry=ly,lfx=12.f,lfy=72.f,rfx=30.f,rfy=72.f;
    rotate(lx,ly,18,51,.19f*counter);rotate(lfx,lfy,18,51,.19f*counter);
    rotate(rx,ry,26,51,-.19f*counter);rotate(rfx,rfy,26,51,-.19f*counter);
    line(18,51,lx,ly);line(lx,ly,lfx,lfy);line(lfx,lfy,lfx-4,lfy+1);joint(lx,ly);
    line(26,51,rx,ry);line(rx,ry,rfx,rfy);line(rfx,rfy,rfx+5,rfy+1);joint(rx,ry);
}
inline void paintEngravedFigure(Graphics& g,juce::Rectangle<float> r,Colour ink,BonePose pose,float phase=0.f){
    paintBoneFigure(g,r.translated(.55f,.7f),Colour(0xff060607).withAlpha(.70f),pose,phase);
    paintBoneFigure(g,r,ink,pose,phase);
}
inline void paintOpticalFinish(Graphics& g,juce::Rectangle<float> r){
    juce::Graphics::ScopedSaveState save(g);juce::Path clip;clip.addRoundedRectangle(r.getX(),r.getY(),r.getWidth(),r.getHeight(),5.f);g.reduceClipRegion(clip);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff090b0d).withAlpha(.76f),r.getY(),juce::Colours::transparentBlack,r.getY()+14));g.fillRect(r.withHeight(14));
    g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::transparentWhite,r.getBottom()-6,Colour(0xffe1c99d).withAlpha(.25f),r.getBottom()));g.fillRect(r.withY(r.getBottom()-6).withHeight(6));
    juce::Path glint;glint.startNewSubPath(r.getX()+5,r.getY()+5);glint.quadraticTo(r.getCentreX(),r.getY()+1,r.getRight()-5,r.getY()+5);
    g.setColour(Colour(0xfff8e7c8).withAlpha(.24f));g.strokePath(glint,juce::PathStrokeType(.8f));
    g.setGradientFill(juce::ColourGradient(Colour(0xffdce3df).withAlpha(.18f),r.getX(),r.getCentreY(),juce::Colours::transparentWhite,r.getX()+9.f,r.getCentreY(),false));g.fillRect(r.withWidth(9.f));
    g.setGradientFill(juce::ColourGradient(Colour(0xff050608).withAlpha(.55f),r.getRight(),r.getCentreY(),juce::Colours::transparentBlack,r.getRight()-9.f,r.getCentreY(),false));g.fillRect(r.withTrimmedLeft(r.getWidth()-9.f));
    g.setColour(Colour(0xff090b0d).withAlpha(.7f));g.drawRoundedRectangle(r,5.f,1.2f);
}
inline void paintWorkshopDetail(Graphics& g,juce::Rectangle<float> r,int type,float phase=0.f,float level=0.f,bool frame=true){
    const bool heat=type==Sat||type==SoftClip,ember=type==SoftClip;
    if(frame)paintHardwarePlate(g,r,Colour(heat?(ember?0xff63372a:0xff543d2b):0xff303c40),true,8.f,3.2f);
    auto f=r.reduced(6.f);const float x=f.getX(),y=f.getY(),w=f.getWidth(),h=f.getHeight();
    {juce::Graphics::ScopedSaveState save(g);juce::Path clip;clip.addRoundedRectangle(x,y,w,h,5.f);g.reduceClipRegion(clip);
    g.setGradientFill(juce::ColourGradient(Colour(heat?(ember?0xff6b271b:0xff51301b):0xff30454b),x+w*.45f,y+h*.35f,Colour(0xff090b0c),x+w*.91f,y+h*.91f,true));g.fillRect(f);
    if(heat){
        // A lit vacuum-tube chamber with reflected filament and mica supports.
        const float cx=x+w*.48f,cy=y+h*.40f;
        for(int i=9;i>=0;--i){float rr=12.f+i*3.1f;g.setColour(Colour(ember?0xffec6430:0xffe99c43).withAlpha(.012f+(.009f+.009f*level+.002f*std::sin(phase))*(9-i)));g.fillEllipse(cx-rr,cy-rr,rr*2,rr*2);}
        const auto tube=juce::Rectangle<float>(cx-24,y+9,48,h-34);
        // The lit plate and socket sit physically behind the glass envelope.
        auto socket=tube.withY(tube.getBottom()-2.f).withHeight(13.f).expanded(5.f,0.f);
        g.setColour(juce::Colours::black.withAlpha(.70f));g.fillRoundedRectangle(socket.translated(1.f,2.f),4.f);
        g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff9b7751),socket.getY(),Colour(0xff27190f),socket.getBottom()));g.fillRoundedRectangle(socket,3.f);
        g.setGradientFill(juce::ColourGradient(Colour(0xffecd6a6).withAlpha(.40f),tube.getX(),tube.getY(),Colour(0xff231b13).withAlpha(.08f),tube.getRight(),tube.getBottom(),false));g.fillRoundedRectangle(tube,13.f);
        g.setColour(Colour(0xffe3c18b).withAlpha(.67f));g.drawRoundedRectangle(tube,13.f,1.f);
        g.setGradientFill(juce::ColourGradient::vertical(Colour(0xffbfb8a5).withAlpha(.65f),tube.getY(),juce::Colours::transparentBlack,tube.getY()+16.f));g.fillRoundedRectangle(tube.withHeight(16.f),10.f);
        const auto plate=tube.reduced(9.f,13.f);
        g.setGradientFill(juce::ColourGradient(Colour(0xff39261a),plate.getX(),plate.getY(),Colour(0xffb6854a),plate.getRight(),plate.getBottom(),false));g.fillRoundedRectangle(plate,3.f);
        for(int k=0;k<7;++k){const float yy=plate.getY()+4+k*(plate.getHeight()-8)/6.f;g.setColour(Colour(0xffd3a76a).withAlpha(.26f));g.drawLine(plate.getX()+2,yy,plate.getRight()-2,yy,.8f);}
        for(int i=0;i<4;++i){float xx=cx-10+i*6.5f;g.setColour(Colour(0xff37271a));g.drawLine(xx+.65f,tube.getY()+13,xx+.65f,tube.getBottom()-10,1.7f);g.setColour(Colour(0xffe2b475).withAlpha(.8f));g.drawLine(xx,tube.getY()+13,xx,tube.getBottom()-10,.9f);}
        juce::Path filament;filament.startNewSubPath(cx-12,cy+9);for(int k=0;k<9;++k)filament.lineTo(cx-12+k*3,cy+(k%2?1.f:9.f));g.setColour(Colour(0xffffaa43).withAlpha(.19f+.36f*level+.04f*std::sin(phase*2.f)));g.strokePath(filament,juce::PathStrokeType(4.f));g.setColour(Colour(0xffffd59a));g.strokePath(filament,juce::PathStrokeType(1.4f));
        g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff92714c),tube.getBottom()-7,Colour(0xff21180e),tube.getBottom()+5));g.fillRoundedRectangle(tube.withY(tube.getBottom()-7).withHeight(12),2.f);
        juce::Path shine;shine.startNewSubPath(tube.getX()+5,tube.getY()+18);shine.quadraticTo(tube.getX()+2,tube.getCentreY(),tube.getX()+6,tube.getBottom()-8);g.setColour(Colour(0xffffe6b9).withAlpha(.55f));g.strokePath(shine,juce::PathStrokeType(1.7f));
        if(ember){
            // Rising embers behind the glass, distinct from Warm Drive's steady tube.
            for(int i=0;i<7;++i){const float t=.5f+.5f*std::sin(phase+i*1.9f);
                const float xx=x+w*(.13f+.02f*i)+3.f*std::sin(phase+i),yy=y+h*(.65f-.50f*t);
                g.setColour(Colour(0xffef7a36).withAlpha(.12f+.16f*t+.16f*level));g.fillEllipse(xx-2,yy-2,4,4);
                g.setColour(Colour(0xffffdcaa).withAlpha(.32f+.36f*t));g.fillEllipse(xx-.65f,yy-.65f,1.3f,1.3f);}
        }else{
            g.setColour(Colour(0xffedcfa0).withAlpha(.20f));g.drawVerticalLine(int(tube.getRight()-5),tube.getY()+16,tube.getBottom()-12);
        }
        paintEngravedFigure(g,{x+w*.72f,y+13,w*.19f,h*.60f},Colour(0xffe7c28b).withAlpha(.83f),BonePose::warm,phase);
    }else if(type!=Eq){
        // Distinct static workshop illustrations, deliberately not live traces.
        const float cx=x+w*.50f,cy=y+h*.42f;
        if(type==Phaser){for(int i=0;i<5;++i){float rr=7+i*7.f+1.6f*std::sin(phase+i*.5f);g.setColour(Colour(0xffb1cdbf).withAlpha(.67f-i*.08f));g.drawEllipse(cx-rr,cy-rr*.68f,rr*2,rr*1.36f,1.15f);}paintEngravedFigure(g,{cx-10,cy-20,20,40},Colour(0xffe9dbc0),BonePose::stand,phase);}
        else if(type==Stereo){for(int i=0;i<2;++i){float xx=cx+(i?1.f:-1.f)*w*.23f;g.setColour(Colour(0xffd4e2c4).withAlpha(.48f));g.drawEllipse(xx-16,cy-24,32,48,1.1f);paintEngravedFigure(g,{xx-12,cy-21,24,42},Colour(0xffc7dbc9),i?BonePose::reverse:BonePose::mirror,i?-phase:phase);}}
        else {for(int i=0;i<3;++i){juce::Path curve;for(int k=0;k<=100;++k){float t=k/100.f;float amp=type==Transient?std::exp(-t*6.f):.5f+.5f*std::sin(t*6.283185f);float xx=x+12+(w-24)*t,yy=cy+std::sin(t*19.f+i*.72f-phase)*amp*h*.28f;if(k==0)curve.startNewSubPath(xx,yy);else curve.lineTo(xx,yy);}g.setColour(Colour(0xffd8c6a4).withAlpha(.64f+.20f*level-i*.19f));g.strokePath(curve,juce::PathStrokeType(1.2f));}}
    }else{
        // Abstract etched frequency sculpture, intentionally not a live graph.
        for(int i=0;i<5;++i){float yy=y+12+i*(h-36)/4.f;g.setColour(Colour(0xffa7bfc2).withAlpha(.16f));g.drawLine(x+12,yy,x+w-12,yy,.55f);}
        for(int i=0;i<4;++i){juce::Path curve;for(int k=0;k<=90;++k){float t=k/90.f;float yy=y+h*.40f-18.f*std::exp(-std::pow((t-(.18f+i*.20f))/.105f,2.f));float xx=x+9+(w-18)*t;if(k==0)curve.startNewSubPath(xx,yy);else curve.lineTo(xx,yy);}g.setColour(Colour(0xffd2e1cc).withAlpha(.24f+i*.10f+.12f*level+.035f*std::sin(phase+i)));g.strokePath(curve,juce::PathStrokeType(1.f));}
    }
    g.setColour(Colour(0xfff3e7d1));g.setFont(font(9.5f,true).withExtraKerningFactor(.12f));g.drawText(heat?(type==Sat?"HARMONIC WARMTH":"SOFT PEAK SHAPING"):type==Eq?"FOUR BAND TONE":type==Phaser?"PHASE MOTION":type==Stereo?"STEREO FIELD":type==Transient?"TRANSIENT SHAPE":"PULSE MOTION",f.withY(f.getBottom()-17).withHeight(13),juce::Justification::centred,false);
    paintOpticalFinish(g,f);}
}
