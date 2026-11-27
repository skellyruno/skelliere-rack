#pragma once
#include "Look.h"
namespace skl::ui {
// Distinct instrument windows; only the GR/readout values are measured. All
// filter/transfer linework is labeled as a schematic, not a spectrum analyzer.
inline bool identityMeter(int t) noexcept {
    return t==DynamicEQ||t==Multiband||t==Resonance||t==NoiseGateUnified||t==SmartGate||t==Gate||t==DeEsserUnified||t==FastComp||t==BusComp||t==Limiter;
}
inline bool identityArt(int t) noexcept { return t==Exciter||t==Doubler||t==ClipUnified||t==Trim||t==Tune||t==DualFilter; }
inline Colour identityInk(int t) noexcept {
    switch(t){case DynamicEQ:return Colour(0xff83c6d5);case Multiband:return Colour(0xffdcbd79);case Resonance:return Colour(0xffd3a4df);case NoiseGateUnified:case Gate:case SmartGate:return Colour(0xffa6db9c);case DeEsserUnified:return Colour(0xffe5b276);case FastComp:return Colour(0xfff3aa74);case BusComp:return Colour(0xff91c6d6);case Limiter:return Colour(0xffe99d81);case Exciter:return Colour(0xffe8cb80);case Doubler:return Colour(0xff8ec9df);case Trim:return Colour(0xffc6d2c6);case Tune:return Colour(0xff9bddb7);case DualFilter:return Colour(0xffd3d79a);case VoxRider:return Colour(0xffa7d4a1);case StereoUnified:return Colour(0xff91c6d6);default:return Colour(0xffff9271);}
}
inline void identityLabel(Graphics& g,juce::Rectangle<float> r,const juce::String& text,Colour c,float size=10.f,bool bold=true){g.setColour(c);g.setFont(font(size,bold).withExtraKerningFactor(.04f));g.drawFittedText(text,r.toNearestInt(),juce::Justification::centred,1);}
inline void identityFrame(Graphics& g,juce::Rectangle<float> r,Colour ink,int type=Empty){
    const auto bezel=type==FastComp||type==Exciter||type==Multiband?Colour(0xff655038):type==BusComp||type==DynamicEQ||type==Trim?Colour(0xff626b68):type==Resonance?Colour(0xff4c4059):Colour(0xff393732);
    paintHardwarePlate(g,r,bezel,false,5.f,2.f);paintRecessedWindow(g,r.reduced(4),3.f);
    auto glass=r.reduced(7);const bool paper=type==BusComp||type==Trim;
    g.setGradientFill(juce::ColourGradient::vertical(paper?Colour(0xffe5d9ba):ink.darker(5.f),glass.getY(),paper?Colour(0xffad9b79):Colour(0xff0b1112),glass.getBottom()));g.fillRoundedRectangle(glass,2.f);
    g.setColour((paper?Colour(0xff332d22):ink).withAlpha(.30f));g.drawRoundedRectangle(glass,2.f,.7f);
    if(type!=Empty){g.setColour(Colour(0xffd9c6a1).withAlpha(.40f));g.drawHorizontalLine(int(r.getY()+2),r.getX()+8,r.getRight()-8);}

}
inline void identityLEDs(Graphics& g,juce::Rectangle<float> r,Colour c,float val,int count=20){
    const float step=r.getWidth()/count;
    for(int i=0;i<count;++i){auto b=juce::Rectangle<float>(r.getX()+i*step,r.getY(),std::max(1.f,step-1.7f),r.getHeight());auto lit=val*count>i;auto col=lit?c:Colour(0xff263034);g.setGradientFill(juce::ColourGradient::vertical(col.brighter(.15f),b.getY(),col.darker(.45f),b.getBottom()));g.fillRoundedRectangle(b,1.f);}
}
inline void paintIdentityMeter(Graphics& g,juce::Rectangle<float> r,int type,float db,float freq,float q,float range,bool advanced,const std::array<float,8>& bands,bool frame){
    const auto ink=identityInk(type);if(frame){identityFrame(g,r,ink,type);return;}
    auto a=r.reduced(10.f);juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(a.toNearestInt());
    const auto faint=ink.withAlpha(.35f),dim=ink.withAlpha(.17f);const float gr=std::clamp(std::isfinite(db)?db:0.f,0.f,20.f),norm=gr/20.f;
    const auto footer=a.withY(a.getBottom()-17.f).withHeight(16.f);auto chart=a.withTrimmedTop(19.f).withTrimmedBottom(24.f);
    auto text=[&](juce::Rectangle<float> b,const juce::String& s,float sz=10.f){identityLabel(g,b,s,ink,sz);};
    auto scale=[&](const juce::Rectangle<float>& b){g.setColour(dim);for(int i=0;i<=4;++i){const float y=b.getY()+b.getHeight()*i/4.f;g.drawHorizontalLine(int(y),b.getX(),b.getRight());}};
    const juce::String value=(gr>=19.99f?juce::String("20+"):juce::String(gr,1))+" dB";
    if(type==Multiband){
        text(a.withHeight(17),advanced?"BAND REDUCTION":"SIX-BAND NETWORK");scale(chart);
        const float step=chart.getWidth()/6.f;
        const char* zone[]={"SUB","LOW","LM","MID","HM","AIR"};const unsigned palette[]={0xffbaa476,0xffc3ad7a,0xffcfb975,0xffd9c37e,0xffb5c093,0xffa6c9b4};
        for(int j=0;j<6;++j){auto cell=juce::Rectangle<float>(chart.getX()+step*j+2,chart.getY()+5,step-4,chart.getHeight()-19);const auto col=Colour(palette[j]);g.setColour(col.withAlpha(.10f));g.fillRoundedRectangle(cell,2);g.setColour(col.withAlpha(.55f));g.drawRoundedRectangle(cell,2,.7f);if(advanced){const float v=std::clamp(bands[j]/24.f,0.f,1.f);auto bar=cell.reduced(2.f).withHeight(cell.reduced(2.f).getHeight()*v);if(v>0){g.setGradientFill(juce::ColourGradient::vertical(col.brighter(.24f),bar.getY(),col.darker(.30f),bar.getBottom()));g.fillRoundedRectangle(bar,1);}}else{for(int k=0;k<3;++k){g.setColour(col.withAlpha(.35f));g.drawHorizontalLine(int(cell.getY()+5+k*(cell.getHeight()-10)/2),cell.getX()+4,cell.getRight()-4);}}
            identityLabel(g,cell.withY(cell.getBottom()+2).withHeight(12),advanced?juce::String(j+1):juce::String(zone[j]),col,8.5f);}

        if(!advanced){identityLEDs(g,footer.withWidth(footer.getWidth()*.40f).reduced(0,4),ink,norm,12);text(footer.withTrimmedLeft(footer.getWidth()*.41f),"LINKED GR "+value,10.f);}else text(footer,"MAX GR  "+value,11.f);
    } else if(type==DynamicEQ||type==Resonance||type==DeEsserUnified){
        const bool res=type==Resonance,deess=type==DeEsserUnified;
        text(a.withHeight(17),res?"PEAK FOCUS":deess?"SIBILANCE BAND":"DYNAMIC BELL");scale(chart);
        const float cx=chart.getX()+chart.getWidth()*std::clamp(std::log(std::max(40.f,freq)/40.f)/std::log(400.f),0.f,1.f);
        const float width=chart.getWidth()*(deess?.17f:std::clamp(.23f/std::sqrt(std::max(.3f,q)),.035f,.27f));
        const float axis=chart.getY()+chart.getHeight()*(range>0&&type==DynamicEQ?.57f:.26f);g.setColour(faint);g.drawHorizontalLine(int(axis),chart.getX(),chart.getRight());g.drawVerticalLine(int(cx),chart.getY(),chart.getBottom()-12);
        if(res){ // Optical focus reticle: only the footer reports measured GR.
            auto scope=chart.withTrimmedBottom(12);const float rcx=scope.getCentreX(),rcy=scope.getCentreY(),rr=scope.getHeight()*.43f;
            g.setColour(Colour(0xff15101f));g.fillRect(scope);for(int k=1;k<=3;++k){const float rad=rr*k/3.f;g.setColour(ink.withAlpha(k==3?.60f:.25f));g.drawEllipse(rcx-rad,rcy-rad,2*rad,2*rad,.8f);}
            for(int k=0;k<12;++k){float ang=k*juce::MathConstants<float>::twoPi/12;g.setColour(ink.withAlpha(k%3==0?.62f:.22f));g.drawLine(rcx+std::cos(ang)*(rr+2),rcy+std::sin(ang)*(rr+2),rcx+std::cos(ang)*(rr+6),rcy+std::sin(ang)*(rr+6),.7f);}
            const float ang=(std::clamp(std::log(std::max(40.f,freq)/40.f)/std::log(400.f),0.f,1.f)-.5f)*4.6f;const float px=rcx+std::sin(ang)*rr*.69f,py=rcy-std::cos(ang)*rr*.69f;
            g.setColour(ink.withAlpha(.55f));g.drawLine(rcx,rcy,px,py,1.1f);g.setColour(ink);g.drawEllipse(px-4,py-4,8,8,1.2f);g.fillEllipse(rcx-2,rcy-2,4,4);
            identityLabel(g,scope.withWidth((scope.getWidth()-2*rr)*.5f-4),"FOCUS",ink,8.5f);identityLabel(g,scope.withTrimmedLeft((scope.getWidth()+2*rr)*.5f+4),"Q "+juce::String(q,1),ink,9.5f);
        }else if(deess){ // Sibilance detector coverage, not per-bin audio levels.
            for(int j=0;j<17;++j){const float xx=chart.getX()+(j+.5f)*chart.getWidth()/17;auto b=juce::Rectangle<float>(xx-2,axis,4.f,chart.getHeight()*.39f);g.setColour(ink.withAlpha(.12f+.78f*std::exp(-.5f*std::pow((xx-cx)/std::max(2.f,width),2.f))));g.fillRoundedRectangle(b,1.f);}
        }else {juce::Path curve;for(int i=0;i<=100;++i){const float x=chart.getX()+chart.getWidth()*i/100.f;const float z=(x-cx)/std::max(2.f,width);const float d=(range>0&&type==DynamicEQ?-1.f:1.f)*chart.getHeight()*.48f*std::exp(-.5f*z*z);const float y=axis+d;if(!i)curve.startNewSubPath(x,y);else curve.lineTo(x,y);}
        g.setColour(ink.withAlpha(.18f));g.strokePath(curve,juce::PathStrokeType(5.f));g.setColour(ink);g.strokePath(curve,juce::PathStrokeType(1.4f));}
        const auto f=freq>=1000?juce::String(freq/1000,1)+"k":juce::String(int(freq));text(chart.withY(chart.getBottom()-13).withHeight(12),"FOCUS "+f+" Hz / SHAPE",8.5f);
        identityLEDs(g,footer.withWidth(footer.getWidth()*.47f).reduced(0,4),ink,norm,12);text(footer.withTrimmedLeft(footer.getWidth()*.49f),(type==DynamicEQ?juce::String("GAIN ")+(gr<.05f?"":range<0?"-":"+"):juce::String("GR "))+value,10.f);
    }else if(type==NoiseGateUnified||type==SmartGate||type==Gate){
        text(a.withHeight(17),"ATTENUATION / GATE");
        const auto iris=chart.withWidth(chart.getHeight()).withCentre({chart.getX()+chart.getHeight()*.5f,chart.getCentreY()});const float rad=iris.getWidth()*.43f,cx=iris.getCentreX(),cy=iris.getCentreY();
        g.setColour(Colour(0xff080b09));g.fillEllipse(iris.reduced(1));g.setColour(faint);g.drawEllipse(iris.reduced(1),1.4f);g.setColour(dim);g.drawEllipse(iris.reduced(5),.8f);
        for(int k=0;k<6;++k){const float angle=k*juce::MathConstants<float>::twoPi/6.f;juce::Path blade;auto pt=[&](float radius,float t){return juce::Point<float>(cx+radius*std::cos(t),cy+radius*std::sin(t));};blade.startNewSubPath(pt(rad,angle));blade.lineTo(pt(rad,angle+1.05f));blade.lineTo(pt(rad*(.22f+.55f*(1-norm)),angle+.6f));blade.closeSubPath();g.setColour(ink.withAlpha(.28f+.4f*norm));g.fillPath(blade);g.setColour(ink.withAlpha(.36f));g.strokePath(blade,juce::PathStrokeType(.65f));}
        auto right=chart.withTrimmedLeft(iris.getWidth()+4);text(right.withHeight(24),value,16.f);text(right.withY(right.getCentreY()+1).withHeight(20),gr<.1f?"OPEN":"REDUCING",10.f);identityLEDs(g,footer.reduced(1,5),ink,norm,22);
    }else{
        const bool fet=type==FastComp,bus=type==BusComp; text(a.withHeight(17),fet?"PEAK / FAST":bus?"BUS / LINKED":"PEAK LIMITER");
        if(fet){
            auto b=chart.reduced(4,5);g.setColour(Colour(0xff070908));g.fillRoundedRectangle(b,3);g.setColour(Colour(0xff70573b));g.drawRoundedRectangle(b,3,.7f);
            const int count=20;const float step=b.getWidth()/count;for(int j=0;j<count;++j){float xx=b.getX()+step*j+1;auto lamp=juce::Rectangle<float>(xx,b.getCentreY()-8,step-2,16);const auto c=norm*count>j?(j>15?Colour(0xffe86b45):Colour(0xffefba70)):Colour(0xff30251c);g.setGradientFill(juce::ColourGradient::vertical(c.brighter(.35f),lamp.getY(),c.darker(.48f),lamp.getBottom()));g.fillRoundedRectangle(lamp,1);}
            for(int j=0;j<=4;++j){auto tx=b.getX()+b.getWidth()*j/4;identityLabel(g,{std::clamp(tx-12,b.getX(),b.getRight()-24),b.getY()+1,24,11},juce::String(j*5),ink,8.f);}
            identityLabel(g,footer,"GAIN REDUCTION  "+value,ink,11.f);
        }else if(bus){
            const auto black=Colour(0xff382d24);g.setColour(black);g.fillRect(a);g.setGradientFill(juce::ColourGradient::vertical(Colour(0xffeadfc2),a.getY(),Colour(0xffcbb58e),a.getBottom()));g.fillRoundedRectangle(a.reduced(1),2.f);
            identityLabel(g,a.withHeight(17),"BUS / GAIN REDUCTION",black,10.f);
            const float cx=chart.getCentreX(),cy=chart.getBottom()+5,rad=std::min(chart.getWidth()*.47f,chart.getHeight()*.94f);const float low=-1.12f,high=1.12f;
            juce::Path arc;arc.addCentredArc(cx,cy,rad,rad,0,low,high,true);g.setColour(black);g.strokePath(arc,juce::PathStrokeType(.8f));
            for(int j=0;j<=20;++j){const float ang=low+(high-low)*j/20;const auto pos=[&](float rr){return juce::Point<float>(cx+std::sin(ang)*rr,cy-std::cos(ang)*rr);};g.setColour(j>=15?Colour(0xff983b27):black);g.drawLine({pos(rad),pos(rad-(j%5==0?8:4))},j%5==0?1.f:.65f);if(j%5==0){auto p=pos(rad-17);identityLabel(g,{p.x-9,p.y-5,18,10},juce::String(j),black,8,false);}}
            const float ang=low+(high-low)*norm;g.setColour(Colour(0xff712d25));g.drawLine(cx,cy,cx+std::sin(ang)*(rad-3),cy-std::cos(ang)*(rad-3),1.5f);g.setColour(black);g.fillEllipse(cx-4,cy-4,8,8);
            identityLabel(g,footer,"GR  "+value,black,11.f);
        }else{
            const auto amber=Colour(0xfff2b16f),red=Colour(0xffeb6a52);auto b=chart.reduced(6,4);const float step=b.getHeight()/16;auto track=b.withWidth(b.getWidth()*.25f).withX(b.getX()+3);
            for(int j=0;j<16;++j){auto cell=juce::Rectangle<float>(track.getX(),track.getY()+j*step,track.getWidth(),std::max(1.f,step-1.2f));g.setColour(norm*16>j?(j<10?amber:red):Colour(0xff302723));g.fillRoundedRectangle(cell,1);}
            for(int j=0;j<=4;++j){const float yy=b.getY()+b.getHeight()*j/4;g.setColour(ink.withAlpha(.60f));g.drawHorizontalLine(int(yy),track.getRight()+3,track.getRight()+8);identityLabel(g,{track.getRight()+10,yy-6,24,12},juce::String(j*5),ink,8);}
            auto read=b.withTrimmedLeft(track.getWidth()+34);identityLabel(g,read.withHeight(read.getHeight()*.65f),value,ink,21);identityLabel(g,read.withY(read.getCentreY()+6).withHeight(15),gr>.1f?"LIMITING":"READY",ink,9);
            identityLabel(g,footer,"REDUCTION dB",ink,9.f);
        }
    }
}
inline void paintIdentityMini(Graphics& g,juce::Rectangle<float> r,int t){
    auto c=identityInk(t);g.setColour(Colour(0xff393732));g.fillRoundedRectangle(r.reduced(1),4);paintRecessedWindow(g,r.reduced(3),3);auto a=r.reduced(7);g.setColour(c);const float x=a.getX(),y=a.getY(),w=a.getWidth(),h=a.getHeight();
    if(t==Multiband||t==FastComp||t==BusComp||t==Limiter||t==DeEsserUnified||t==Resonance){int n=t==Multiband?6:t==Resonance?9:t==DeEsserUnified?7:5;for(int i=0;i<n;++i){float height=h*(t==Resonance||t==DeEsserUnified?.24f+.68f*std::exp(-.5f*std::pow((i-(n-1)*.5f)/1.6f,2.f)):.75f);g.setColour(c.withAlpha(t==Resonance||t==DeEsserUnified?.75f:.8f));g.fillRoundedRectangle(x+i*w/n+1,y+(h-height)*.5f,w/n-2,height,1);}}
    else if(t==NoiseGateUnified||t==Gate||t==SmartGate){g.drawEllipse(a.withSizeKeepingCentre(h,h),1.2f);juce::Path p;for(int j=0;j<6;++j){float ang=j*juce::MathConstants<float>::twoPi/6;float xx=a.getCentreX()+std::cos(ang)*h*.36f,yy=a.getCentreY()+std::sin(ang)*h*.36f;if(!j)p.startNewSubPath(xx,yy);else p.lineTo(xx,yy);}p.closeSubPath();g.strokePath(p,juce::PathStrokeType(1));}
    else if(t==Doubler){g.drawLine(x+3,y+h*.5f,x+w-3,y+h*.5f,1.1f);for(int i=0;i<2;++i)g.drawEllipse(x+i*(w-10),y+h*.5f-5,10,10,1.5f);}
    else {juce::Path p;for(int i=0;i<=32;++i){float q=i/32.f;float v=t==DynamicEQ?-.7f*std::exp(-std::pow((q-.6f)*5,2.f)):t==ClipUnified?std::tanh((q-.5f)*5):std::sin(q*juce::MathConstants<float>::twoPi*3)*.7f;float yy=y+h*(.5f-v*.45f);if(!i)p.startNewSubPath(x+w*q,yy);else p.lineTo(x+w*q,yy);}g.strokePath(p,juce::PathStrokeType(1.5f));}
}
inline void paintSignedInstrument(Graphics& g,juce::Rectangle<float> r,int t,float value){
    const auto ink=identityInk(t);auto a=r.reduced(10);juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(a.toNearestInt());const bool rider=t==VoxRider;identityLabel(g,a.withHeight(17),rider?"AUTOMATIC GAIN":"STEREO CORRELATION",ink,11);
    auto b=a.withTrimmedTop(21).withTrimmedBottom(20);const float v=std::clamp(value/(rider?18.f:1.f),-1.f,1.f);
    if(rider){const float cx=b.getCentreX(),cy=b.getCentreY(),h=b.getHeight()*.83f;auto track=juce::Rectangle<float>(cx-8,cy-h*.5f,16,h);g.setColour(Colour(0xff0b0f0e));g.fillRoundedRectangle(track,3);g.setColour(ink.withAlpha(.4f));g.drawRoundedRectangle(track,3,.8f);for(int i=0;i<=6;++i){float y=track.getY()+track.getHeight()*i/6;g.drawHorizontalLine(int(y),cx-23,cx-12);g.drawHorizontalLine(int(y),cx+12,cx+23);}const float y=cy-v*h*.5f;g.setGradientFill(juce::ColourGradient::vertical(Colour(0xffe1d7b8),y-5,Colour(0xff786e56),y+5));g.fillRoundedRectangle(cx-15,y-5,30,10,2);g.setColour(Colour(0xff322c20));g.drawHorizontalLine(int(y),cx-12,cx+12);identityLabel(g,b.withWidth(40),"+ / -",ink,9);}
    else{const float cx=b.getCentreX(),cy=b.getBottom()-7,rad=std::min(b.getWidth()*.43f,b.getHeight()*.83f);juce::Path arc;arc.addCentredArc(cx,cy,rad,rad,0,-juce::MathConstants<float>::halfPi,juce::MathConstants<float>::halfPi,true);g.setColour(ink.withAlpha(.25f));g.strokePath(arc,juce::PathStrokeType(4));for(int j=0;j<=8;++j){const float angle=-juce::MathConstants<float>::halfPi+j*juce::MathConstants<float>::pi/8;g.setColour(ink.withAlpha(.6f));g.drawLine(cx+std::sin(angle)*(rad-7),cy-std::cos(angle)*(rad-7),cx+std::sin(angle)*(rad+2),cy-std::cos(angle)*(rad+2),.8f);}const float angle=v*juce::MathConstants<float>::halfPi;g.setColour(v<0?Colour(0xffdfa776):ink);g.drawLine(cx,cy,cx+std::sin(angle)*(rad-4),cy-std::cos(angle)*(rad-4),1.6f);g.fillEllipse(cx-3,cy-3,6,6);identityLabel(g,b.withWidth(20).withY(b.getBottom()-16).withHeight(15),"-1",ink,8);identityLabel(g,b.withTrimmedLeft(b.getWidth()-20).withY(b.getBottom()-16).withHeight(15),"+1",ink,8);}
    identityLabel(g,a.withY(a.getBottom()-17).withHeight(16),(value>0.05f&&rider?juce::String("+"):juce::String())+juce::String(value,rider?1:2)+(rider?" dB":""),ink,12);
}
// A cached spherical dielectric material, original procedural art. The glow
// responds to aggregate audio level only, never represents a detected note.
inline const juce::Image& tuneOrbMaterial(){static const auto im=[](){constexpr int n=256;juce::Image out(juce::Image::ARGB,n,n,true);juce::Image::BitmapData px(out,juce::Image::BitmapData::writeOnly);for(int y=0;y<n;++y)for(int x=0;x<n;++x){const float u=(x-n*.5f)/(n*.49f),v=(y-n*.5f)/(n*.49f),d=u*u+v*v;if(d>=1)continue;const float z=std::sqrt(1-d);const float diffuse=std::max(0.f,-u*.45f-v*.65f+z*.48f),fresnel=std::pow(1-z,2.8f);const float veins=std::pow(.5f+.5f*std::sin(26*u+8*std::sin(7*v+3*z)+5*std::sin(12*z)),8.f)*(.28f+.72f*z);const float nebula=.5f+.5f*std::sin(9*u+3*v+2*std::sin(8*v+z));auto c=Colour(0xff020906).interpolatedWith(Colour(0xff244d31),std::clamp(diffuse*.70f+nebula*.12f,0.f,1.f));c=c.interpolatedWith(Colour(0xff80cda0),std::clamp(veins*.25f+fresnel*.40f,0.f,1.f));const float spec=std::exp(-((u+.33f)*(u+.33f)+(v+.49f)*(v+.49f))*22.f);c=c.interpolatedWith(Colour(0xffd1e9c8),spec*.84f);px.setPixelColour(x,y,c.withAlpha(std::clamp((1-d)*35.f,0.f,1.f)));}return out;}();return im;}
inline juce::Rectangle<float> tuneOrbBounds(juce::Rectangle<float> detail){return detail.withHeight(216.f);}
inline juce::Rectangle<float> tuneRootBounds(juce::Rectangle<float> detail){return detail.reduced(6,0).withY(detail.getY()+229.f).withHeight(38.f);}
inline juce::Rectangle<float> tuneScaleBounds(juce::Rectangle<float> detail){return detail.reduced(6,0).withY(detail.getY()+281.f).withHeight(22.f);}
inline juce::Rectangle<float> tunePianoBounds(juce::Rectangle<float> detail){return detail.reduced(6,0).withY(detail.getBottom()-57.f).withHeight(52.f);}
inline int tuneRootAt(juce::Rectangle<float> detail,juce::Point<float> p){const auto r=tuneRootBounds(detail);if(!r.contains(p))return -1;return juce::jlimit(0,5,int((p.x-r.getX())*6/r.getWidth()))+6*juce::jlimit(0,1,int((p.y-r.getY())*2/r.getHeight()));}
inline int tuneKeyAt(juce::Rectangle<float> detail,juce::Point<float> p){
    const int root=tuneRootAt(detail,p);if(root>=0)return root;
    const auto keys=tunePianoBounds(detail);if(!keys.contains(p))return -1;
    const float kw=keys.getWidth()/7.f;const int black[]={1,3,6,8,10},after[]={0,1,3,4,5},white[]={0,2,4,5,7,9,11};
    for(int j=0;j<5;++j)if(juce::Rectangle<float>(keys.getX()+(after[j]+1)*kw-kw*.30f,keys.getY(),kw*.59f,keys.getHeight()*.58f).contains(p))return black[j];
    return white[juce::jlimit(0,6,int((p.x-keys.getX())/kw))];
}
// Reuse the original sculpted Demon relief geometry, not a flat icon. The
// neutral bone variant removes the outer horns and foreground hands and is
// tinted once per decoded frame; the orb glass supplies the green lighting.
inline const std::array<juce::Image,9>& tuneSkeletonFrames(){
    static const auto frames=[](){std::array<juce::Image,9> result{};if(!demonReliefIsValid())return result;
        const auto& atlas=demonReliefAtlas();
        for(int i=0;i<9;++i){auto& im=result[i];im=juce::Image(juce::Image::ARGB,352,360,true);juce::Image::BitmapData dst(im,juce::Image::BitmapData::writeOnly);
            for(int y=0;y<360;++y)for(int x=0;x<352;++x){const int sx=x+40,sy=y+62;const auto c=atlas.getPixelAt((i%3)*432+sx,(i/3)*496+sy);if(c.getAlpha()==0)continue;
                const float d=std::abs(float(sx)-216.f);const float edge=sy<150?82.f+std::clamp((sy-90.f)/60.f,0.f,1.f)*29.f:190.f;
                const float mask=std::clamp((edge-d)/9.f,0.f,1.f);const float tone=std::clamp((c.getFloatRed()*.78f+c.getFloatGreen()*.22f)*1.38f,0.f,1.f);
                const auto bone=Colour(0xff102018).interpolatedWith(Colour(0xffe1dfc5),std::pow(tone,.90f));dst.setPixelColour(x,y,bone.withAlpha(c.getFloatAlpha()*mask));}
        }return result;}();return frames;
}
inline void paintOrbSkeleton(Graphics& g,juce::Rectangle<float> r,float phase,float humanize){
    const auto& frames=tuneSkeletonFrames();const int index=juce::jlimit(0,8,int(std::round(2.f+1.4f*std::sin(phase*1.7f))));const auto& frame=frames[index];
    if(!frame.isValid())return;const float w=std::min(r.getWidth(),r.getHeight()*352.f/360.f),h=w*360.f/352.f;
    const auto dst=juce::Rectangle<float>(r.getCentreX()-w*.5f,r.getCentreY()-h*.5f,w,h);
    juce::Graphics::ScopedSaveState save(g);g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);g.setOpacity(1.f);g.drawImage(frame,dst);
    if(humanize>0){g.setGradientFill(juce::ColourGradient(Colour(0xffd9e0ae).withAlpha(humanize*.07f),dst.getCentreX(),dst.getY(),juce::Colours::transparentWhite,dst.getCentreX(),dst.getBottom(),false));g.fillEllipse(dst.reduced(w*.12f,h*.08f));}
}
inline void paintTuneOrb(Graphics& g,juce::Rectangle<float> r,float phase,float level,float humanize=0.f){
    const float rad=std::min(r.getHeight()*.45f,r.getWidth()*.455f),cx=r.getCentreX(),cy=r.getCentreY();const auto sphere=juce::Rectangle<float>(cx-rad,cy-rad,rad*2,rad*2);
    g.setGradientFill(juce::ColourGradient(Colour(0xff448c50).withAlpha(.20f+level*.17f),cx,cy,juce::Colours::transparentBlack,cx+rad*1.45f,cy,true));g.fillEllipse(sphere.expanded(rad*.45f));
    g.setColour(Colour(0xff171d17));g.fillEllipse(sphere.expanded(5.f));g.setGradientFill(juce::ColourGradient::vertical(Colour(0xffb4ae83),cy-rad-5,Colour(0xff36372b),cy+rad+5));g.drawEllipse(sphere.expanded(3.5f),2.f);
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);g.drawImage(tuneOrbMaterial(),sphere);g.setColour(Colour(0xff9bd6a1).withAlpha(.38f));g.drawEllipse(sphere,.7f);
    {juce::Graphics::ScopedSaveState clip(g);juce::Path mask;mask.addEllipse(sphere.reduced(2));g.reduceClipRegion(mask);for(int b=0;b<3;++b){juce::Path bolt;const float t=phase*.6f+b*2.094f;const float sx=cx+std::cos(t)*rad*.88f,sy=cy+std::sin(t)*rad*.88f;bolt.startNewSubPath(sx,sy);for(int i=1;i<=9;++i){float a=float(i)/9;float dx=(1-a)*(sx-cx),dy=(1-a)*(sy-cy);float jitter=std::sin(i*7.31f+b*11.3f+phase*.5f)*rad*.105f*std::sin(a*3.14159f);bolt.lineTo(cx+dx+jitter,cy+dy-jitter*.4f);}g.setColour(Colour(0xff76d28a).withAlpha(.13f+level*.12f));g.strokePath(bolt,juce::PathStrokeType(3.2f));g.setColour(Colour(0xffc3efc3).withAlpha(.62f+level*.22f));g.strokePath(bolt,juce::PathStrokeType(.75f));}}
    {juce::Graphics::ScopedSaveState clip(g);juce::Path mask;mask.addEllipse(sphere.reduced(3));g.reduceClipRegion(mask);
        paintOrbSkeleton(g,sphere.reduced(rad*.07f,rad*.08f),phase,humanize);
        juce::Path glass;glass.addEllipse(cx-rad*.73f,cy-rad*.81f,rad*.96f,rad*.35f);g.setGradientFill(juce::ColourGradient::vertical(Colour(0xffe1ffdc).withAlpha(.21f),cy-rad,juce::Colours::transparentWhite,cy-rad*.35f));g.fillPath(glass);
        juce::Path rim;rim.addCentredArc(cx,cy,rad*.92f,rad*.92f,0,-1.02f,.32f,true);g.setColour(Colour(0xffd8f3d9).withAlpha(.60f));g.strokePath(rim,juce::PathStrokeType(1.5f));
    }
    for(int i=0;i<12;++i){const float a=i*juce::MathConstants<float>::twoPi/12;const float rr=rad+8;g.setColour(Colour(0xffb9ad80).withAlpha(i%3==0?.68f:.28f));g.drawLine(cx+std::sin(a)*rr,cy+std::cos(a)*rr,cx+std::sin(a)*(rr+3),cy+std::cos(a)*(rr+3),.75f);}
}
inline void paintIdentityArt(Graphics& g,juce::Rectangle<float> r,int type,float phase,float level,int mode,bool frame,const std::array<float,6>& values={},int hoverNote=-1){
    auto ink=identityInk(type);if(type==Trim)ink=Colour(0xff3d4a42);if(frame){if(type!=Tune)identityFrame(g,r,ink,type);return;}auto a=r.reduced(10);juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion((type==Tune?r:a).toNearestInt());
    auto chart=a.withTrimmedTop(18).withTrimmedBottom(18);const float x=chart.getX(),y=chart.getY(),w=chart.getWidth(),h=chart.getHeight();
    identityLabel(g,a.withHeight(16),type==Exciter?"HARMONIC CELL":type==Doubler?"DUAL VOICE":type==Trim?"GAIN / PAN":type==Tune?"":type==DualFilter?"FILTER WINDOW":"PEAK SHAPER",ink,11);
    if(type==Trim){const float v=std::clamp(values[1],-1.f,1.f);auto line=chart.reduced(8,chart.getHeight()*.33f);g.setColour(Colour(0xff8f8066));g.drawRoundedRectangle(line.expanded(1),3,1);g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff635b49),line.getY(),Colour(0xffbbae90),line.getBottom()));g.fillRoundedRectangle(line,2);g.setColour(Colour(0xff302c24));g.drawHorizontalLine(int(line.getCentreY()),line.getX()+4,line.getRight()-4);
        for(int j=0;j<=10;++j){const float xx=line.getX()+line.getWidth()*j/10;g.setColour(ink.withAlpha(j%5==0?.85f:.52f));g.drawLine(xx,line.getBottom()+3,xx,line.getBottom()+(j%5==0?10:6),.8f);}
        const float cx=line.getCentreX()+v*line.getWidth()*.46f;auto thumb=juce::Rectangle<float>(cx-6,line.getY()-4,12,line.getHeight()+8);g.setColour(Colour(0xff372f22).withAlpha(.52f));g.fillRoundedRectangle(thumb.translated(0,1.5f),2);g.setGradientFill(juce::ColourGradient(Colour(0xffe6d4a5),thumb.getX(),thumb.getY(),Colour(0xff827456),thumb.getRight(),thumb.getBottom(),false));g.fillRoundedRectangle(thumb,2);g.setColour(ink);g.drawVerticalLine(int(cx),thumb.getY()+3,thumb.getBottom()-3);
        identityLabel(g,chart.withHeight(17),juce::String(values[0]>0?"+":"")+juce::String(values[0],1)+" dB",ink,15);identityLabel(g,a.withY(a.getBottom()-15).withHeight(14),"L      "+(std::abs(v)<.02f?juce::String("CENTER"):juce::String(v<0?"L ":"R ")+juce::String(int(std::abs(v)*100)))+"      R",ink,9);
    }else if(type==Tune){
        const int key=std::clamp(int(std::lround(values[3])),0,11),scale=std::clamp(int(std::lround(values[4])),0,2);const int white[]={0,2,4,5,7,9,11};const int black[]={1,3,6,8,10};const int after[]={0,1,3,4,5};auto enabled=[&](int n){int i=(n-key+12)%12;return scale==0||(scale==1?(i==0||i==2||i==4||i==5||i==7||i==9||i==11):(i==0||i==2||i==3||i==5||i==7||i==8||i==10));};
        paintTuneOrb(g,tuneOrbBounds(r),phase,level,values[5]);const auto keys=tunePianoBounds(r);const float kw=keys.getWidth()/7.f;
        for(int j=0;j<7;++j){auto b=juce::Rectangle<float>(keys.getX()+j*kw,keys.getY(),kw-1,keys.getHeight());const auto c=white[j]==hoverNote?Colour(0xffc7f57e):white[j]==key?ink:enabled(white[j])?Colour(0xffded9c7):Colour(0xff5b605c);g.setGradientFill(juce::ColourGradient::vertical(c.brighter(.15f),b.getY(),c.darker(.18f),b.getBottom()));g.fillRoundedRectangle(b,1.5f);}
        for(int j=0;j<5;++j){auto b=juce::Rectangle<float>(keys.getX()+(after[j]+1)*kw-kw*.30f,keys.getY(),kw*.59f,keys.getHeight()*.58f);g.setColour(black[j]==hoverNote?Colour(0xffc7f57e):black[j]==key?ink:enabled(black[j])?Colour(0xff304c40):Colour(0xff111513));g.fillRoundedRectangle(b,1.5f);}
        const char* notes[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};const auto root=tuneRootBounds(r);const float rw=root.getWidth()/6.f,rh=root.getHeight()/2.f;
        identityLabel(g,root.withY(root.getY()-17).withHeight(15),juce::String("KEY  ")+notes[hoverNote>=0?hoverNote:key],ink,11);
        for(int j=0;j<12;++j){auto b=juce::Rectangle<float>(root.getX()+(j%6)*rw,root.getY()+(j/6)*rh,rw-2,rh-2);const bool selected=j==key,hover=j==hoverNote;g.setColour(selected?Colour(0xff3e6c4d):Colour(0xff111914).withAlpha(.65f));g.fillRoundedRectangle(b,2);if(hover){g.setColour(ink.withAlpha(.65f));g.drawRoundedRectangle(b,2,1);}identityLabel(g,b,notes[j],selected||hover?Colour(0xffe0f4d0):ink,10);}
        const auto scales=tuneScaleBounds(r);const char* modes[]={"CHROM","MAJOR","MINOR"};identityLabel(g,scales.withY(scales.getY()-14).withHeight(13),"SCALE",ink,9);
        for(int j=0;j<3;++j){auto b=scales.withX(scales.getX()+j*scales.getWidth()/3).withWidth(scales.getWidth()/3-2);g.setColour(j==scale?Colour(0xff43664b):Colour(0xff111b15));g.fillRoundedRectangle(b,2);identityLabel(g,b,modes[j],j==scale?Colour(0xffd5f3b5):ink,9);}

    }else if(type==DualFilter){
        auto fx=[&](float f){return x+w*std::clamp(std::log(std::max(20.f,f)/20.f)/std::log(1000.f),0.f,1.f);};const float hp=fx(values[0]),lp=fx(values[1]);g.setColour(ink.withAlpha(.19f));for(int i=0;i<5;++i)g.drawVerticalLine(int(x+i*w*.25f),y,y+h);juce::Path p;for(int i=0;i<=80;++i){const float px=x+i*w/80;const float f=std::pow(1000.f,i/80.f)*20;const float hpf=values[0]<=22?1.f:1/std::sqrt(1+std::pow(values[0]/f,4.f));const float lpf=values[1]>=19999?1.f:1/std::sqrt(1+std::pow(f/std::max(200.f,values[1]),4.f));float py=y+h*(.88f-.72f*hpf*lpf);if(!i)p.startNewSubPath(px,py);else p.lineTo(px,py);}g.setColour(ink);g.strokePath(p,juce::PathStrokeType(1.4f));g.setColour(ink.withAlpha(.6f));g.drawVerticalLine(int(hp),y,y+h);g.drawVerticalLine(int(lp),y,y+h);identityLabel(g,a.withY(a.getBottom()-15).withHeight(14),"HP / LP  |  SHAPE",ink,9);
    }else if(type==Exciter){
        // Copper excitation coils and ceramic terminal block. Animated light
        // represents aggregate level, not spectral measurements or circuitry.
        const auto brass=Colour(0xffe7be79);const float cx=x+w*.5f,cy=y+h*.49f;
        auto base=chart.reduced(4,6);g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff2b2720),base.getY(),Colour(0xff171813),base.getBottom()));g.fillRoundedRectangle(base,4);g.setColour(Colour(0xff896d45).withAlpha(.55f));g.drawRoundedRectangle(base,4,.9f);
        const float coilW=std::min(43.f,w*.23f),coilH=h*.63f;
        for(int side:{-1,1}){const float xx=cx+side*w*.27f;auto coil=juce::Rectangle<float>(xx-coilW*.5f,cy-coilH*.5f,coilW,coilH);
            g.setColour(Colour(0xff090909));g.fillRoundedRectangle(coil.expanded(3),5);g.setGradientFill(juce::ColourGradient(Colour(0xff8d6543),coil.getX(),cy,Colour(0xff39251b),coil.getRight(),cy,false));g.fillRoundedRectangle(coil,4);
            for(int j=0;j<10;++j){auto turn=juce::Rectangle<float>(coil.getX()-1,coil.getY()+coilH*j/10,coilW+2,coilH/10-1);juce::ColourGradient wire(Colour(0xff573626),turn.getX(),turn.getY(),Colour(0xff6c3e28),turn.getRight(),turn.getY(),false);wire.addColour(.36,Colour(0xffe5a665));wire.addColour(.51,Colour(0xffb67a4a));g.setGradientFill(wire);g.fillRoundedRectangle(turn,2);g.setColour(Colour(0xfff3cd8d).withAlpha(.32f));g.drawHorizontalLine(int(turn.getY()+1),turn.getX()+3,turn.getRight()-3);}
            for(int j:{-1,1}){auto terminal=juce::Rectangle<float>(xx-9,cy+j*(coilH*.5f+4)-3,18,6);g.setGradientFill(juce::ColourGradient::vertical(Colour(0xffe3d0a0),terminal.getY(),Colour(0xff6d553a),terminal.getBottom()));g.fillRoundedRectangle(terminal,1.5f);}
            g.setColour(brass.withAlpha(.58f));g.drawLine(xx-side*coilW*.5f,cy,cx+side*15,cy,1.4f);
        }
        auto cell=juce::Rectangle<float>(cx-16,cy-19,32,38);g.setGradientFill(juce::ColourGradient::vertical(Colour(0xffcbbd9c),cell.getY(),Colour(0xff6d644f),cell.getBottom()));g.fillRoundedRectangle(cell,3);g.setColour(Colour(0xff28261f));g.fillRoundedRectangle(cell.reduced(4),2);
        g.setGradientFill(juce::ColourGradient(Colour(0xffffbb65).withAlpha(.20f+level*.28f),cx,cy,juce::Colours::transparentBlack,cx+24,cy,true));g.fillEllipse(cx-24,cy-24,48,48);
        juce::Path spark;spark.startNewSubPath(cx-9,cy);for(int j=1;j<=8;++j)spark.lineTo(cx-9+j*2.25f,cy+std::sin(j*4.53f+phase*2)*5.f*std::sin(j*3.14159f/8));g.setColour(Colour(0xffe9a954).withAlpha(.22f+level*.30f));g.strokePath(spark,juce::PathStrokeType(3.2f));g.setColour(Colour(0xffffe1aa).withAlpha(.65f+level*.30f));g.strokePath(spark,juce::PathStrokeType(.85f));
        identityLabel(g,a.withY(a.getBottom()-15).withHeight(14),"EXCITATION CELL / ART",ink,9);
    }else if(type==Doubler){
        const float cx=x+w*.5f,cy=y+h*.48f;g.setColour(ink.withAlpha(.17f));g.drawLine(cx,y,cx,y+h,1);for(int i=0;i<3;++i){float rr=8+i*7;g.drawEllipse(cx-rr,cy-rr,rr*2,rr*2,.7f);}
        for(int side:{-1,1}){const float vx=cx+side*w*.28f,vy=cy+std::sin(phase+side)*3;const auto col=side<0?Colour(0xffa8d4c5):Colour(0xffc9c1a7);g.setGradientFill(juce::ColourGradient(col.withAlpha(.17f+level*.15f),vx,vy,juce::Colours::transparentBlack,vx+25,vy,true));g.fillEllipse(vx-25,vy-30,50,60);g.setColour(col.withAlpha(.46f));g.drawEllipse(vx-23,vy-29,46,58,.8f);paintEngravedFigure(g,{vx-13,vy-25,26,49},col.withAlpha(.90f),side<0?BonePose::mirror:BonePose::reverse,side*phase);g.setColour(col.withAlpha(.40f));g.drawLine(cx+side*6,cy,vx-side*23,vy,.9f);identityLabel(g,{vx-12,chart.getBottom()-9,24,12},side<0?"I":"II",col,9);}
        identityLabel(g,a.withY(a.getBottom()-15).withHeight(14),"TWO WET VOICES / ART",ink,9);
    }else{
        g.setColour(ink.withAlpha(.19f));for(int i=0;i<=4;++i){g.drawHorizontalLine(int(y+h*i/4),x,x+w);g.drawVerticalLine(int(x+w*i/4),y,y+h);}g.setColour(ink.withAlpha(.35f));g.drawLine(x,y+h,x+w,y,.8f);
        juce::Path curve;for(int i=0;i<=120;++i){const float t=(i/120.f)*4-2;const float v=(mode&1)?std::clamp(t,-1.f,1.f):std::tanh(t);float px=x+w*i/120.f,py=y+h*(.5f-.42f*v);if(!i)curve.startNewSubPath(px,py);else curve.lineTo(px,py);}g.setColour(ink.withAlpha(.20f));g.strokePath(curve,juce::PathStrokeType(5));g.setColour(ink);g.strokePath(curve,juce::PathStrokeType(1.7f));identityLabel(g,a.withY(a.getBottom()-15).withHeight(14),juce::String((mode&1)?"HARD":"SOFT")+" / "+((mode>=2)?"4X":"1X")+" / SHAPE",ink,9);
    }
}
}
