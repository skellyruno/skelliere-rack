#pragma once
#include "Look.h"
namespace skl::ui {
#include "AtelierArt.h"
#include "ReferenceTapeArtwork.h"
struct PanelSpec { Colour metal,accent,cap; bool light; const char* series; };
inline PanelSpec originalSpec(int type,int finish) {
    static constexpr unsigned metals[]={0x25292c,0x9eaaa6,0xe1c4a6,0x62352e,0x5d4132,0xc4c0ad,0x293640,0x34382a,0x45433e,0x323235,0x6b716e,0x242d34,0x97876e,0x425c61,0x62403b,0x344c59,0xc1aa83,0x454f3e,0x71614d,0x303e4b,0x5e6354,0x784d3e,0x3c594d,0x4e4059,0x59322e,0x706449,0x496464,0x45484a,0x3c4144,0x992c20,0x8a232a,0x461719,0x3d4549,0x292e34,0x35433d,0x354c43,0x303f52,0x3f4b57,0x533d4b,0x344950,0x6a4736,0x284251,0x45484a,0x35433d,0x34382a,0x45433e,0x5d4132,0x784d3e,0x425c61};
    static constexpr unsigned accents[]={0x9a9a9a,0x39534b,0xaed5bf,0xe0b88e,0xe9be7b,0x365b55,0xa4bedc,0xe7c68c,0xaed0aa,0xe2ad9f,0xb5d3c4,0x9bddb7,0x3c514b,0xa4d4df,0xe5d9bf,0xb7d8cf,0x26392f,0xc0d58e,0xf0cc9b,0x405e50,0xe1d7ab,0xf3b397,0xc1d4ab,0xd5b7de,0xa7d5e5,0xf0cd98,0xc6e8da,0xc6c7b4,0xf2b367,0xf6c06a,0xe4c27e,0xf2a470,0xd6d9d4,0x99ded6,0xb4d4ad,0xcde2b7,0xaed8ea,0xd4c2a5,0xe7b7c9,0xb6d8da,0xf4d0a0,0xc6e8da,0xc6c7b4,0xb4d4ad,0xe7c68c,0xaed0aa,0xe9be7b,0xf3b397,0xa4d4df};
    static_assert(std::size(metals)==kNumRackTypes && std::size(accents)==kNumRackTypes,"Panel palette must cover catalog");
    type=std::clamp(type,0,static_cast<int>(kNumRackTypes)-1);
    auto metal=Colour(0xff000000u|metals[type]);const auto accent=Colour(0xff000000u|accents[type]);
    if(finish==1&&(type==Air||type==SkellAirFX))metal=Colour(type==Air?0xff344e60:0xff284251);
    if(finish==1&&type==Sat)metal=Colour(0xff594033);
    if(finish==1&&type==Reverb)metal=Colour(0xff423b4a);
    if(finish==1&&type==Chorus)metal=Colour(0xff29454a);
    if(finish==1&&type==SkellDope)metal=Colour(0xffd2d3cc);
    if(finish==1&&type==SkellSSL)metal=Colour(0xff252827);
    if(finish==1&&type==SkellDemon)metal=Colour(0xff64120e);
    if(finish==1&&type==SkellMixHead)metal=Colour(0xffa52b19);
    if(finish==1){switch(type){
        case FastComp:metal=Colour(0xff46372c);break;
        case BusComp:metal=Colour(0xff223d47);break;
        case Limiter:metal=Colour(0xff292c30);break;
        case Trim:metal=Colour(0xff666960);break;
        case Tune:metal=Colour(0xff151f1b);break;
        case DynamicEQ:metal=Colour(0xff293e44);break;
        case Multiband:metal=Colour(0xff54462c);break;
        case Resonance:metal=Colour(0xff352b43);break;
        case Exciter:metal=Colour(0xff4b3525);break;
        case Doubler:metal=Colour(0xff243e40);break;
        case DualFilter:metal=Colour(0xff414a37);break;
        default:break;
    }}
    if(finish==0)metal=Colour(0xff303437);
    if(finish==2)metal=Colour(0xffb1b6b1);
    if(finish==3)metal=Colour(0xff222829);
    if(type==FastComp)metal=Colour(finish==3?0xff202324:0xff899ba4);
    const bool light=metal.getPerceivedBrightness()>.45f;
    const auto cap=type%4==0?Colour(0xffbca487):Colour(0xffccbaa1);
    const char* series=info(type).cat==CatDynamics?"DYNAMICS":info(type).cat==CatTone?"TONE":info(type).cat==CatSpace?"MOD / SPACE":"UTILITY";
    return {metal,accent,cap,light,series};
}
struct OriginalLayout {
    std::array<juce::Rectangle<int>,kMaxKnobs+kSslExtraCount> cells{};
    juce::Rectangle<int> meter, detail;
    bool sideReadouts=false;
};
// Reference proportions: the tall 650px panels are replaced by a compact
// hardware panel of 574px. Preserve every knob, including Bell's four real controls.
inline OriginalLayout originalLayoutBase(int type,int width,bool editing=false) {
    OriginalLayout l;const auto& mi=info(type);const int n=mi.numKnobs;
    const int half=(width-24)/2,mid=width/2;
    if(mi.hasVU||hasOutputPeak(type))l.meter={15,106,width-30,120};
    if(type==EchoUnified||type==RoomUnified||type==DriveUnified||type==ClipUnified||type==AirUnified||type==Doubler||type==Exciter||type==Delay||type==Slap||type==Air||type==SkellAirFX||type==Reverb||type==Chorus||type==Sat||type==SoftClip||type==Eq||type==Tremolo||type==Phaser||type==Stereo||type==Transient)l.detail={16,106,width-32,120};
    if(type==Eq)l.detail.setHeight(82);
    if(type==Trim||type==Tune||type==DualFilter)l.detail={16,106,width-32,type==Trim?120:90};
    if(type==Tune){const int bottom=editing?520:548;l.detail={12,100,width-24,bottom-100};const int top=editing?365:374,knobBottom=editing?465:480;l.cells[0]={12,top,half,knobBottom-top};l.cells[5]={12+half,top,half,knobBottom-top};return l;}
    if(type==Trim){l.detail.setHeight(117);const int bottom=editing?520:548;l.cells[0]={mid-86,240,172,156};l.cells[1]={mid-65,402,130,bottom-406};return l;}
    if(type==FastComp){l.meter={14,106,width-28,140};const int top=258,bottom=editing?520:548;const int row=(bottom-top)/3;
        l.cells[0]={12,top,half,row-3};l.cells[4]={12+half,top,half,row-3};
        l.cells[2]={12,top+row,half,row-3};l.cells[3]={12+half,top+row,half,row-3};
        l.cells[1]={12,top+2*row,half,row-3};l.cells[5]={12+half,top+2*row,half,row-3};return l;}
    if(type==Limiter){const int top=239,bottom=editing?520:548,hero=161;l.cells[1]={mid-73,top,146,hero};l.cells[0]={12,top+hero+4,half,bottom-top-hero-8};l.cells[2]={12+half,top+hero+4,half,bottom-top-hero-8};return l;}
    if(type==StereoUnified){const int top=242,bottom=editing?520:548,lane=width*60/100,row=(bottom-top)/2;l.cells[0]={12,top,lane-12,row-4};l.cells[3]={lane,top,width-lane-12,row-4};l.cells[1]={12,top+row,half,bottom-top-row-4};l.cells[2]={12+half,top+row,half,bottom-top-row-4};return l;}
    if(type==VoxRider){const int top=243,bottom=editing?520:548,remaining=bottom-top;const int row=remaining/3;l.cells[0]={mid-76,top,152,row-4};for(int j=1;j<5;++j)l.cells[j]={12+((j-1)%2)*half,top+row+((j-1)/2)*row,half,row-4};return l;}
    if(type==BusComp||type==DynamicEQ){
        l.sideReadouts=true;const int top=240,bottom=editing?520:548,row=(bottom-top)/n;
        for(int k=0;k<n;++k)l.cells[k]={13,top+k*row,width-26,row-3};return l;
    }
    if(type==Resonance){
        const int top=240,bottom=editing?520:548,row=(bottom-top)/3;
        const int wide=width*58/100;
        l.cells[0]={12,top,wide-12,row-4};l.cells[1]={wide,top,width-wide-12,row-4};
        for(int k=2;k<6;++k)l.cells[k]={12+(k%2)*half,top+(k/2)*row,half,row-4};return l;
    }
    if(type==SkellDemon){
        l.detail={7,90,width-14,281};
        const int cw=(width-24)/3,rh=(editing?519:547)-376;
        for(int k=0;k<6;++k)l.cells[k]={12+(k%3)*cw,376+(k/3)*(rh/2),cw,rh/2-3};
        return l;
    }
    if(type==SkellSSL){
        // Reference console proportions: a compact left dynamics lane and a
        // slightly wider four-band EQ lane. Preserve all 27 host attachments.
        const int split=width*46/100,left=10,right=split+6;
        const int lw=split-15,rw=width-right-10;
        auto row=[&](int x,int w,int y,int h,std::initializer_list<int> ids){const int n=int(ids.size());int i=0;for(int j:ids){const int a=i*w/n,b=(i+1)*w/n;l.cells[j]={x+a,y,b-a,h};++i;}};
        auto e=[](int i){return kMaxKnobs+i;};
        row(left,lw,100,56,{e(SslInput),e(SslPhase)});
        row(left,lw,173,58,{e(SslHP),e(SslLP)});
        row(left,lw,247,56,{e(SslRatio),e(SslThreshold)});
        row(left,lw,305,46,{e(SslRelease),5,e(SslFast)});
        row(left,lw,370,54,{e(SslExpThreshold),e(SslExpRange)});
        row(left,lw,426,46,{e(SslExpRelease),e(SslGate)});
        l.cells[e(SslOutput)]={left,491,lw,editing?38:45};
        auto band=[&](int y,int gain,int freq,int shape){
            const int g=rw*52/100;
            l.cells[gain]={right,y+6,g,64};
            l.cells[freq]={right+g,y,rw-g,42};
            l.cells[shape]={right+g,y+42,rw-g,28};
        };
        band(100,4,e(SslHFFreq),e(SslHFBell));
        band(184,3,e(SslHMFreq),e(SslHMQ));
        band(270,2,e(SslLMFreq),e(SslLMQ));
        band(356,1,e(SslLFFreq),e(SslLFBell));
        l.cells[0]={right+rw-45,439,45,29};
        l.meter={right,480,rw,55};return l;
    }
    if(type==SkellDope){
        // Vertical rack adaptation of the two-mode hardware. All four
        // existing APVTS attachments remain in their original order.
        l.cells[0]={mid-32,100,64,71};
        for(int k=1;k<=4;++k)l.cells[k]={mid-41,174+(k-1)*86,82,84};
        l.meter={width-69,174,57,340};return l;
    }
    if(type==SkellMixHead){
        // Vertical adaptation of the reference transport hardware.
        const int lane=width-80,cw=lane/2;
        for(int k=0;k<4;++k)l.cells[k]={10+(k%2)*cw,176+(k/2)*105,cw,100};
        l.cells[4]={10,394,cw,105};l.cells[5]={10+cw,394,lane-cw,105};
        l.meter={width-62,176,50,323};return l;
    }
    if(type==SkellNoise){l.meter={18,108,68,260};l.cells[0]={89,108,width-105,260};l.cells[1]={12,385,half,editing?126:158};l.cells[2]={12+half,385,half,editing?126:158};return l;}
    const bool window=!l.meter.isEmpty()||!l.detail.isEmpty();
    // Four-window controls leave a lower breathing area like real rack gear.
    // Five-control compressors need the bottom row for makeup gain.
    const int top=type==SkellDemon?250:type==Eq?199:type==DualFilter?212:window?(n==5?(editing?248:253):242):108;
    const int bottom=editing?520:(window&&n==4?515:window&&n==5&&!l.meter.isEmpty()?565:548),available=bottom-top;
    if(type==Bell||type==DualFilter||type==Tilt){
        l.sideReadouts=true;const int row=available/std::max(1,n);
        for(int k=0;k<n;++k)l.cells[k]={12,top+k*row,width-24,row-4};
    }else if(n==2){
        const int row=available/2;
        for(int k=0;k<2;++k)l.cells[k]={mid-83,top+k*row,166,row-4};
    }else if(n==3&&!window){
        const int hero=available/2;l.cells[0]={mid-82,top,164,hero-4};
        for(int k=1;k<3;++k)l.cells[k]={12+(k-1)*half,top+hero,half,available-hero-4};
    }else if(type==Eq){
        const int row=available/3;
        for(int k=0;k<5;++k)l.cells[k]={k==4?mid-half/2:12+(k%2)*half,top+(k/2)*row,half,row-4};
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
// Enlarge the usable panel area while preserving fixed top/bottom hardware.
// Use one coordinate map for layout, engraved labels and meter windows.
inline int tallPanelY(int y) { return 100 + juce::roundToInt(float(y-100)*1.17f); }
inline OriginalLayout originalLayout(int type,int width,bool editing=false){
    auto l=originalLayoutBase(type,width,editing);
    auto stretch=[](juce::Rectangle<int>& r){if(!r.isEmpty()){const int top=tallPanelY(r.getY()),bottom=tallPanelY(r.getBottom());r.setY(top);r.setHeight(bottom-top);}};
    for(auto& c:l.cells)stretch(c);stretch(l.meter);stretch(l.detail);
    if(type==Tune)l.cells[6]={18,378,width-36,26};
    if(type==SkellDope){
        // Do not round individual rows: all four controls use an exact pitch.
        const int top=tallPanelY(174),pitch=101,height=98;
        for(int k=1;k<=4;++k)l.cells[k].setY(top+(k-1)*pitch),l.cells[k].setHeight(height);
    }
    return l;
}
inline int legacyArtworkType(int type) noexcept {
    switch(type){case AirUnified:return SkellAirFX;case EchoUnified:return Delay;case RoomUnified:return Reverb;case Doubler:return Chorus;case DriveUnified:case Exciter:return Sat;case ClipUnified:return SoftClip;default:return type;}
}
inline bool isAirPanel(int type) { return type==Air||type==SkellAirFX||type==AirUnified; }
// Original deterministic cloud field, cached once at twice logical resolution.
// A decorative scene, not a measurement or an indication of DSP activity.
inline const juce::Image& airAtmosphere(){
    static const auto image=[] {
        constexpr int w=432,h=252;
        juce::Image im(juce::Image::ARGB,w,h,true,juce::SoftwareImageType());
        auto noise=[](int x,int y){unsigned v=unsigned(x)*0x9e3779b9u+unsigned(y)*0x85ebca6bu+0xA17F83u;v^=v>>16;v*=0x7feb352du;v^=v>>15;return float(v&65535u)/65535.f;};
        auto field=[&](float x,float y){int ix=int(std::floor(x)),iy=int(std::floor(y));float a=x-ix,b=y-iy;a=a*a*(3.f-2.f*a);b=b*b*(3.f-2.f*b);return (noise(ix,iy)*(1-a)+noise(ix+1,iy)*a)*(1-b)+(noise(ix,iy+1)*(1-a)+noise(ix+1,iy+1)*a)*b;};
        {juce::Image::BitmapData pixels(im,juce::Image::BitmapData::writeOnly);
        for(int y=0;y<h;++y)for(int x=0;x<w;++x){
            const float u=float(x)/w,v=float(y)/h;
            auto fbm=[&](float xx,float yy){float n=0.f,a=.54f,f=3.f;for(int k=0;k<7;++k){n+=a*field(xx*f+13.f,yy*f*.86f+7.f);f*=2.03f;a*=.49f;}return n;};
            const float warp=fbm(u+2.3f,v+1.8f)*.18f;
            const float n=fbm(u+warp,v*.95f+warp);
            const float light=metal::sat(.55f+(n-fbm(u+warp-.024f,v*.95f+warp-.039f))*5.f);
            const float cloud=metal::smooth(.37f,.68f,n+.09f*std::sin(v*5.f));
            const float glow=std::exp(-7.f*(u-.31f)*(u-.31f)-5.f*(v-.18f)*(v-.18f));
            const float beam=std::pow(std::max(0.f,std::cos(19.f*(u-.31f)-7.f*v)),16.f)*glow*.065f;
            auto sky=Colour(0xff142c43).interpolatedWith(Colour(0xff6595ad),metal::sat(.42f*glow+.18f*(1-v)));
            sky=sky.interpolatedWith(Colour(0xff9ab3c4),metal::smooth(.42f,.72f,fbm(u*.72f+4.f,v*.78f+3.f))*.55f);
            auto lit=Colour(0xff536e87).interpolatedWith(Colour(0xfff7e8cc),metal::sat(.22f+.57f*light+.25f*glow));
            auto colour=sky.interpolatedWith(lit,metal::sat(cloud*.96f+beam));
            pixels.setPixelColour(x,y,colour);}}
        return im;
    }();return image;
}
inline void paintAirDetail(Graphics& g,juce::Rectangle<float> r,int type,float phase=0.f,float level=0.f,bool frame=true){
    if(frame)paintHardwarePlate(g,r,Colour(0xff394956),true,8.f,3.f);
    const auto glass=r.reduced(5.f);
    {juce::Graphics::ScopedSaveState save(g);juce::Path clip;clip.addRoundedRectangle(glass.getX(),glass.getY(),glass.getWidth(),glass.getHeight(),5.f);g.reduceClipRegion(clip);
    g.setOpacity(1.f);g.drawImage(airAtmosphere(),glass.expanded(13.f,6.f).translated(10.f*std::sin(phase),4.f*std::sin(phase*2.f)));
    // Luminous haze moves independently of the cached sky. Glass remains fixed.
    const float hx=glass.getX()+glass.getWidth()*(.32f+.09f*std::sin(phase));
    g.setGradientFill(juce::ColourGradient(Colour(0xfff3e6cb).withAlpha(.08f+.19f*level),hx,glass.getY()+26.f,juce::Colours::transparentWhite,hx+glass.getWidth()*.44f,glass.getCentreY(),true));g.fillRect(glass);
    g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::transparentBlack,glass.getCentreY(),Colour(0xff10202e).withAlpha(.88f),glass.getBottom()));g.fillRect(glass);
    // Original drifting skeleton silhouette within the cloud artwork.
    paintEngravedFigure(g,{glass.getX()+glass.getWidth()*.64f,glass.getY()+9,glass.getWidth()*.19f,glass.getHeight()*.59f},Colour(0xffeae5d1).withAlpha(.76f),BonePose::floatUp,phase);
    // A small atmospheric halo links the engraved figure to the sky rather than the bezel.
    g.setGradientFill(juce::ColourGradient(Colour(0xffdce6e7).withAlpha(.13f),glass.getX()+glass.getWidth()*.74f,glass.getY()+glass.getHeight()*.38f,juce::Colours::transparentWhite,glass.getX()+glass.getWidth()*.90f,glass.getY()+glass.getHeight()*.70f,true));g.fillRect(glass);
    for(int i=0;i<7;++i){const float xx=glass.getX()+14.f+i*glass.getWidth()*.11f,yy=glass.getY()+12.f+float((i*17)%29)+3.f*std::sin(phase+i);g.setColour(Colour(0xfff4e8cd).withAlpha(.19f+.10f*std::sin(phase*2.f+i)+.09f*level));g.drawLine(xx-2,yy,xx+2,yy,.65f);g.drawLine(xx,yy-2,xx,yy+2,.65f);}
    // Fine rising curves are symbolic: the actual processing controls remain below.
    juce::Path lift;lift.startNewSubPath(glass.getX()+15,glass.getBottom()-24);lift.cubicTo(glass.getX()+glass.getWidth()*.38f,glass.getBottom()-25,glass.getX()+glass.getWidth()*.55f,glass.getY()+31,glass.getX()+glass.getWidth()*.60f,glass.getY()+29);
    g.setColour(Colour(0xfff4e6c7).withAlpha(.10f));g.strokePath(lift,juce::PathStrokeType(5.f));g.setColour(Colour(0xfff4e6c7).withAlpha(.34f));g.strokePath(lift,juce::PathStrokeType(.65f));
    if(type==SkellAirFX){g.setColour(Colour(0xffc2e9ee).withAlpha(.58f));g.strokePath(lift,juce::PathStrokeType(.8f),juce::AffineTransform::translation(0,-8.f));}
    g.setColour(Colour(0xfff4e8d0));g.setFont(font(10.f,true).withExtraKerningFactor(.17f));g.drawText(type==Air?"HIGH SHELF":"PRESENCE / BRILLIANCE",glass.withY(glass.getBottom()-19).withHeight(14),juce::Justification::centred,false);
    g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::black.withAlpha(.44f),glass.getY(),juce::Colours::transparentBlack,glass.getY()+12));g.fillRect(glass.withHeight(12));}
    paintOpticalFinish(g,glass);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff7c8b8e),glass.getY(),Colour(0xffd8e5e6),glass.getBottom()));g.drawRoundedRectangle(glass.reduced(1.f),4.f,.7f);
}
// Decorative optical windows for space processors. All control values remain real.
inline void paintSpaceDetail(Graphics& g,juce::Rectangle<float> r,int type,float phase=0.f,float level=0.f,bool frame=true){
    if(frame)paintHardwarePlate(g,r,Colour(0xff363a40),true,8.f,3.f);
    auto glass=r.reduced(6.f);
    {juce::Graphics::ScopedSaveState save(g);juce::Path clip;clip.addRoundedRectangle(glass.getX(),glass.getY(),glass.getWidth(),glass.getHeight(),4.f);g.reduceClipRegion(clip);
    const float x=glass.getX(),y=glass.getY(),w=glass.getWidth(),h=glass.getHeight();
    g.setGradientFill(juce::ColourGradient(Colour(type==Reverb?0xff53404e:0xff264c5f),x+w*.24f,y+h*.20f,Colour(0xff10151d),x+w*.82f,y+h*.85f,true));g.fillRect(glass);
    if(type==Reverb){
        // Recessed arched chamber with converging floor and illuminated distant figure.
        for(int i=0;i<5;++i){float t=i/5.f;auto a=glass.reduced(9.f+w*.31f*t,5.f+h*.28f*t);juce::Path arch;arch.startNewSubPath(a.getX(),a.getBottom());arch.lineTo(a.getX(),a.getY()+a.getHeight()*.30f);arch.quadraticTo(a.getCentreX(),a.getY()-a.getHeight()*.22f,a.getRight(),a.getY()+a.getHeight()*.30f);arch.lineTo(a.getRight(),a.getBottom());g.setColour(Colour(0xff09070b).withAlpha(.57f));g.strokePath(arch,juce::PathStrokeType(6.f-3.f*t),juce::AffineTransform::translation(1.3f,1.1f));g.setColour(Colour(0xffa78b70).withAlpha(.58f-t*.26f));g.strokePath(arch,juce::PathStrokeType(2.8f-1.4f*t));g.setColour(Colour(0xffe1c2a2).withAlpha(.22f));g.strokePath(arch,juce::PathStrokeType(.7f));}
        // Receding room geometry and reflections: conceptual depth, no fake meter.
        juce::Path floor;floor.startNewSubPath(x,y+h);floor.lineTo(x+w*.43f,y+h*.51f);floor.lineTo(x+w*.57f,y+h*.51f);floor.lineTo(x+w,y+h);floor.closeSubPath();
        g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff69523d),y+h*.51f,Colour(0xff171319),y+h));g.fillPath(floor);
        for(int k=0;k<2;++k){const float xx=x+w*(k?.81f:.19f);g.setGradientFill(juce::ColourGradient(Colour(0xffe1ab6b).withAlpha(.25f+.15f*level+.06f*std::sin(phase+k)),xx,y+h*.36f,juce::Colours::transparentBlack,xx+24.f,y+h*.68f,true));g.fillEllipse(xx-32,y+h*.08f,64,h*.66f);g.setColour(Colour(0xffe3b57e).withAlpha(.8f));g.fillRoundedRectangle(xx-1.4f,y+h*.31f,2.8f,5.f,1.f);}
        for(int i=0;i<4;++i){float t=(i+1.f)/5.f;float yy=y+h*(.53f+.47f*t*t);g.setColour(Colour(0xffc8a88a).withAlpha(.16f));g.drawLine(x+w*.43f*(1-t),yy,x+w*(.57f+.43f*t),yy,.7f);}
        g.setGradientFill(juce::ColourGradient(Colour(0xffe0b075).withAlpha(.27f+.22f*level+.04f*std::sin(phase)),x+w*.5f,y+h*.36f,juce::Colours::transparentBlack,x+w*.65f,y+h*.68f,true));g.fillEllipse(x+w*.32f,y+h*.08f,w*.36f,h*.68f);
        // Soft lamp behind the figure separates the silhouette from the chamber linework.
        const float cx=x+w*.5f,cy=y+h*.42f;
        g.setGradientFill(juce::ColourGradient(Colour(0xff392b2c).withAlpha(.52f),cx,cy,juce::Colours::transparentBlack,cx+29.f,cy+18.f,true));g.fillEllipse(cx-36,cy-32,72,64);
        paintEngravedFigure(g,{x+w*.42f,y+h*.17f,w*.16f,h*.51f},Colour(0xffe7c5a1).withAlpha(.91f),BonePose::stand,phase);
    }else{
        for(int band=0;band<3;++band){juce::Path wave;for(int k=0;k<=100;++k){float t=float(k)/100.f;float yy=y+h*.44f+std::sin(t*juce::MathConstants<float>::twoPi*1.4f+band*.65f-phase)*h*.20f;float xx=x+w*.34f+w*.32f*t;if(k==0)wave.startNewSubPath(xx,yy);else wave.lineTo(xx,yy);}g.setColour(Colour(0xff6cc8cf).withAlpha(.08f+.14f*level));g.strokePath(wave,juce::PathStrokeType(7.f));g.setColour(Colour(0xffa2d9d7).withAlpha(.28f));g.strokePath(wave,juce::PathStrokeType(2.6f));g.setColour(Colour(0xffe2e9d6).withAlpha(.8f-.18f*band));g.strokePath(wave,juce::PathStrokeType(.9f));}
        paintEngravedFigure(g,{x+w*.12f,y+8,w*.20f,h*.60f},Colour(0xffcfe8dc).withAlpha(.88f),BonePose::mirror,phase);
        paintEngravedFigure(g,{x+w*.68f,y+8,w*.20f,h*.60f},Colour(0xffb6d0ce).withAlpha(.81f),BonePose::reverse,-phase);
    }
    g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::transparentBlack,y+h*.54f,Colour(0xff080b12).withAlpha(.83f),y+h));g.fillRect(glass);
    g.setColour(Colour(0xffe9dfcb));g.setFont(font(10.f,true).withExtraKerningFactor(.16f));g.drawText(type==Reverb?"REFLECTION CHAMBER":"TWIN MODULATION",glass.withY(y+h-18.f).withHeight(14),juce::Justification::centred,false);}
    paintOpticalFinish(g,glass);g.setColour(Colour(0xffbcb5a5).withAlpha(.36f));g.drawRoundedRectangle(glass.reduced(1.f),3.f,.7f);
}
inline const juce::Image& reelMetal(){
    static const auto image=[] {
        constexpr int n=256;juce::Image im(juce::Image::ARGB,n,n,true,juce::SoftwareImageType());
        {juce::Image::BitmapData pixels(im,juce::Image::BitmapData::writeOnly);
        for(int y=0;y<n;++y)for(int x=0;x<n;++x){
            const float u=(x+.5f-n*.5f)/(n*.5f),v=(y+.5f-n*.5f)/(n*.5f),r=std::sqrt(u*u+v*v);
            if(r>=1.f)continue;
            const float angle=std::atan2(v,u),light=-.58f*u-.42f*v;
            const float grain=.027f*std::sin(r*780.f)+.012f*std::sin(r*1283.f);
            const float spec=.12f*std::cos(2.f*(angle+1.8f+.18f*r))*metal::smooth(.08f,.5f,r);
            const float edge=metal::smooth(.91f,.99f,r);
            const float t=metal::sat((.58f+light*.25f+spec+grain)*(1.f-edge)+edge*(.29f+.24f*light));
            pixels.setPixelColour(x,y,Colour(0xff594735).interpolatedWith(Colour(0xfff1ddbc),t).withAlpha(metal::sat((1.f-r)*n*.5f)));}}
        return im;
    }();return image;
}
// Photographed reel faces rotate; housing, rim and spindle stay fixed.
// A bounded, lazy 120-angle cache preserves the original photo's metal texture.
// Directional shading was fitted from the bright flange pixels of the supplied
// 250x142 reference. Remove the rotating low-frequency component and restore
// the same light in screen space; dark cutouts are not relit like metal.
inline const juce::Image& referenceReelFrame(int reel,float angle){
    constexpr int count=120,n=104;constexpr float c=52.f,cy=68.4f;
    static std::array<std::array<juce::Image,count>,2> frames;
    const int k=juce::jlimit(0,count-1,int(angle*count/juce::MathConstants<float>::twoPi+.5f)%count);
    auto& im=frames[(size_t)reel][(size_t)k];if(im.isValid())return im;
    const float cx=reel?188.8f:62.8f,a=float(k)*juce::MathConstants<float>::twoPi/count;
    im=juce::Image(juce::Image::RGB,n,n,true,juce::SoftwareImageType());
    {juce::Graphics rg(im);rg.setOpacity(1.f);rg.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
     rg.drawImageTransformed(referenceTapeArtwork(),juce::AffineTransform::rotation(a,cx,cy).translated(c-cx,c-cy));}
    const float ca=std::cos(a),sa=std::sin(a),base=reel?.547f:.567f,sx=reel?-.122f:.088f,sy=reel?.071f:.064f;
    {juce::Image::BitmapData px(im,juce::Image::BitmapData::readWrite);
    for(int y=0;y<n;++y)for(int x=0;x<n;++x){
        const float u=(x+.5f-c)/51.f,v=(y+.5f-c)/51.f;
        if(u*u+v*v>1.f)continue;
        const auto col=px.getPixelColour(x,y);
        const float lum=.299f*col.getFloatRed()+.587f*col.getFloatGreen()+.114f*col.getFloatBlue();
        const float src=base+sx*(u*ca+v*sa)+sy*(-u*sa+v*ca),dst=base+sx*u+sy*v;
        const float f=1.f+metal::smooth(.30f,.50f,lum)*(juce::jlimit(.80f,1.25f,dst/std::max(.25f,src))-1.f);
        px.setPixelColour(x,y,Colour::fromFloatRGBA(metal::sat(col.getFloatRed()*f),metal::sat(col.getFloatGreen()*f),metal::sat(col.getFloatBlue()*f),1.f));
    }}return im;
}
inline void paintReferenceTapeMotion(Graphics& g,juce::Rectangle<float> r,float left,float right){
    const auto& art=referenceTapeArtwork();if(!art.isValid())return;
    juce::Graphics::ScopedSaveState window(g);
    g.addTransform(juce::AffineTransform::scale(r.getWidth()/250.f,r.getHeight()/142.f).translated(r.getX(),r.getY()));
    const float centres[]={62.8f,188.8f},angles[]={left,right};constexpr float cy=68.4f,faceRadius=51.f,hubRadius=19.f;
    for(int i=0;i<2;++i){
        if(std::abs(angles[i])<.0001f)continue;
        const float cx=centres[i];
        {juce::Graphics::ScopedSaveState face(g);juce::Path clip;clip.addEllipse(cx-faceRadius,cy-faceRadius,2*faceRadius,2*faceRadius);g.reduceClipRegion(clip);
        g.setOpacity(1.f);g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.drawImage(referenceReelFrame(i,angles[i]),juce::Rectangle<float>(cx-52.f,cy-52.f,104.f,104.f));}
        {juce::Graphics::ScopedSaveState hub(g);juce::Path clip;clip.addEllipse(cx-hubRadius,cy-hubRadius,2*hubRadius,2*hubRadius);g.reduceClipRegion(clip);g.setOpacity(1.f);g.drawImageAt(art,0,0);}
    }
}
inline void paintTapeDetail(Graphics& g,juce::Rectangle<float> r,bool useReference=true,float left=0.f,float right=0.f,float phase=0.f) {
    // Only the decorative compartment is supplied artwork. Actual delay parameters
    // remain native, interactive controls. Other finishes retain the procedural renderer.
    const auto& art=referenceTapeArtwork();
    if(useReference&&art.isValid()){
        g.setColour(juce::Colours::black.withAlpha(.65f));g.fillRoundedRectangle(r.expanded(.6f).translated(0,2.5f),7.f);
        juce::Graphics::ScopedSaveState save(g);juce::Path clip;clip.addRoundedRectangle(r.getX(),r.getY(),r.getWidth(),r.getHeight(),6.f);g.reduceClipRegion(clip);
        g.setOpacity(1.f);g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);g.drawImage(art,r);
        paintReferenceTapeMotion(g,r,left,right);
        // Retain the small original Skell mechanic on the transport plate.
        paintEngravedFigure(g,{r.getX()+15.f,r.getBottom()-15.f,7.f,12.f},Colour(0xffd8c19e).withAlpha(.65f),BonePose::stand,phase);
        return;
    }
    // Decorative tape-inspired mechanical illustration. It is not a signal meter and does not
    // show tape physics; the real Time / Feedback / Tone / Mix controls are the knobs below.
    g.setColour(juce::Colours::black.withAlpha(.68f));g.fillRoundedRectangle(r.expanded(.7f).translated(0,2.4f),7);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff3b362d),r.getY(),Colour(0xff0f0e0c),r.getBottom()));
    g.fillRoundedRectangle(r,7);                                                 // machined bezel
    g.setGradientFill(juce::ColourGradient(Colour(0xffd8bb8a).withAlpha(.75f),r.getX(),r.getY(),Colour(0xff3c291a),r.getRight(),r.getBottom(),false));g.drawRoundedRectangle(r.reduced(.9f),6.f,1.3f);
    g.setColour(Colour(0xff000000).withAlpha(.6f));g.drawRoundedRectangle(r.reduced(2.5f),5,1);
    for(int i=0;i<4;++i)paintScrew(g,i%2?r.getRight()-5.f:r.getX()+5.f,i<2?r.getY()+5.f:r.getBottom()-5.f,3.35f,.9f*i);
    const auto win=r.reduced(6.f,5.f);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff30251b),win.getY(),Colour(0xff0b0907),win.getBottom()));
    g.fillRoundedRectangle(win,4);                                                // dark cavity
    // Actual enclosure lip casts a narrow shadow into the tape window.
    g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::black.withAlpha(.65f),win.getY(),juce::Colours::transparentBlack,win.getY()+10.f));g.fillRect(win.withHeight(10.f));
    g.setColour(juce::Colours::black.withAlpha(.55f));g.drawRoundedRectangle(win,4,1.2f);

    // A warm cavity lamp and metal transport bed beneath the reels.
    g.setGradientFill(juce::ColourGradient(Colour(0xffd5a15e).withAlpha(.34f),win.getCentreX(),win.getY()+8.f,juce::Colours::transparentBlack,win.getCentreX(),win.getBottom(),true));g.fillRoundedRectangle(win.reduced(1.f),3.f);
    const auto bed=win.reduced(3.f).removeFromBottom(20.f);
    const auto back=win.reduced(6.f).withTrimmedBottom(17.f);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff19130e),back.getY(),Colour(0xff50412c),back.getBottom()));g.fillRoundedRectangle(back,3.f);
    g.setColour(Colour(0xff9e7f52).withAlpha(.30f));g.drawHorizontalLine(int(back.getY())+1,back.getX()+4,back.getRight()-4);
    g.setColour(Colour(0xffb78e57).withAlpha(.28f));g.drawVerticalLine(int(win.getCentreX()),win.getY()+4.f,bed.getY());
    g.setColour(Colour(0xff20170f));g.fillRect(win.withY(bed.getY()-3.f).withHeight(3.f));
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff8a6337),bed.getY(),Colour(0xff2e2113),bed.getBottom()));g.fillRoundedRectangle(bed,2.f);
    g.setColour(Colour(0xffd8b17d).withAlpha(.55f));g.drawHorizontalLine(int(bed.getY()),bed.getX()+2,bed.getRight()-2);
    juce::Path recess;recess.startNewSubPath(bed.getX()+22.f,bed.getBottom());recess.lineTo(bed.getX()+32.f,bed.getY()+4.f);
    recess.lineTo(bed.getRight()-32.f,bed.getY()+4.f);recess.lineTo(bed.getRight()-22.f,bed.getBottom());recess.closeSubPath();
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff16110b),bed.getY(),Colour(0xff48341f),bed.getBottom()));g.fillPath(recess);
    g.setColour(Colour(0xffc9a477).withAlpha(.48f));g.strokePath(recess,juce::PathStrokeType(.8f));
    const float radius=std::min(win.getWidth()*.234f,win.getHeight()*.445f);
    const float cy=win.getCentreY()-5.f;
    const float cxs[2]={win.getX()+win.getWidth()*.255f,win.getX()+win.getWidth()*.745f};
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
        for(int k=0;k<3;++k){float xx=head.getX()+3+k*(head.getWidth()-6)/2.f;g.setColour(Colour(0xff231c13));g.fillRoundedRectangle(xx-1.5f,head.getY()+.8f,3.f,3.5f,.5f);g.setColour(Colour(0xffe2c79c).withAlpha(.7f));g.drawHorizontalLine(int(head.getY()+1),xx-1,xx+1);}
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
        for(int i=6;i>=0;--i){g.setColour(juce::Colours::black.withAlpha(.09f+float(6-i)*.045f));g.fillEllipse(cx-radius-i*.55f+1.5f,cy-radius-i*.55f+4.f,2*radius+i*1.1f,2*radius+i*1.1f);}
        g.setGradientFill(juce::ColourGradient(Colour(0xff4a3524),cx-radius*.5f,cy-radius*.6f,Colour(0xff150f0a),cx+radius*.8f,cy+radius*.9f,false));
        g.fillEllipse(cx-radius,cy-radius,2*radius,2*radius);                         // wound tape
        g.setColour(Colour(0xff000000).withAlpha(.5f));g.drawEllipse(cx-radius,cy-radius,2*radius,2*radius,1.2f);
        const float fr=radius*.98f;
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.setOpacity(1.f);
        // Flat reel flange with shared turned-metal light, not a grooved knob.
        g.setGradientFill(juce::ColourGradient(Colour(0xfff0d7ac),cx-fr*.70f,cy-fr*.68f,Colour(0xff655445),cx+fr*.76f,cy+fr*.80f,false));
        g.fillEllipse(cx-fr,cy-fr,2*fr,2*fr);
        // Independent reel metal: flatter and broader than the knob cap.
        const auto flangeFace=juce::Rectangle<float>(cx-fr*.975f,cy-fr*.975f,fr*1.95f,fr*1.95f);
        g.setOpacity(1.f);g.drawImage(reelMetal(),flangeFace);
        {juce::Graphics::ScopedSaveState save(g);
         juce::Path disc;disc.addEllipse(cx-fr*.94f,cy-fr*.94f,fr*1.88f,fr*1.88f);g.reduceClipRegion(disc);
         juce::ColourGradient reflect(Colour(0xfff6dcb4).withAlpha(.17f),cx-fr*.62f,cy-fr*.62f,Colour(0xff100d09).withAlpha(.19f),cx+fr*.82f,cy+fr*.83f,false);
         reflect.addColour(.58,juce::Colours::transparentBlack);g.setGradientFill(reflect);g.fillPath(disc);}
        // Fine machining rings on the reel flange, behind the cutouts.
        for(int ring=0;ring<3;++ring){const float rr=fr*(.91f+ring*.022f);g.setColour(Colour(0xfff4dfb9).withAlpha(.10f));g.drawEllipse(cx-rr,cy-rr,2*rr,2*rr,.5f);}
        const float turn=k==0?-1.11f+left:.49f+right;                                                // reels deliberately out of phase
        for(int spoke=0;spoke<3;++spoke){
            const float from=turn+spoke*juce::MathConstants<float>::twoPi/3+.53f;
            const float to=turn+(spoke+1)*juce::MathConstants<float>::twoPi/3-.53f;
            // Rounded tapered triangular ports rather than broad pie slices.
            auto point=[&](float a,float rr){return juce::Point<float>(cx+std::sin(a)*rr,cy-std::cos(a)*rr);};
            const float mid=(from+to)*.5f;
            juce::Path opening;opening.startNewSubPath(point(from,fr*.84f));
            opening.quadraticTo(point(mid,fr*.93f),point(to,fr*.84f));
            opening.lineTo(point(mid+.32f,fr*.42f));opening.quadraticTo(point(mid,fr*.39f),point(mid-.32f,fr*.42f));opening.closeSubPath();
            opening=opening.createPathWithRoundedCorners(2.6f);
            g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff47382a),cy-fr*.8f,Colour(0xff080706),cy+fr*.8f));
            g.fillPath(opening);
            {juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(opening);
             // Windings visible through each recessed port.
             for(int band=0;band<8;++band){const float rr=fr*(.40f+band*.053f);g.setColour(Colour(0xffc4a77b).withAlpha(.10f));g.drawEllipse(cx-rr,cy-rr,2*rr,2*rr,.55f);}}
            g.setColour(Colour(0xff080604).withAlpha(.72f));g.strokePath(opening,juce::PathStrokeType(1.7f),juce::AffineTransform::translation(.1f,-.45f));
            g.setColour(Colour(0xffe9dcc0).withAlpha(.30f));                          // lit rim of the opening
            g.strokePath(opening,juce::PathStrokeType(1.3f),juce::AffineTransform::translation(.3f,1.f));
            g.setColour(Colour(0xff000000).withAlpha(.80f));g.strokePath(opening,juce::PathStrokeType(.65f));
            g.setColour(Colour(0xffcdb48b).withAlpha(.22f));g.strokePath(opening,juce::PathStrokeType(.55f),juce::AffineTransform::translation(-.45f,-.35f));
        }
        juce::Path flange;flange.addCentredArc(cx,cy,fr*.96f,fr*.96f,0.f,-1.7f,.65f,true);g.setColour(Colour(0xfff9e6c6).withAlpha(.65f));g.strokePath(flange,juce::PathStrokeType(.85f));
        g.setColour(juce::Colours::black.withAlpha(.65f));g.drawEllipse(cx-fr*.99f,cy-fr*.99f,fr*1.98f,fr*1.98f,.8f);   // rim highlight
        g.setColour(juce::Colours::black.withAlpha(.65f));g.fillEllipse(cx-fr*.37f+1.f,cy-fr*.37f+1.5f,fr*.74f,fr*.74f);
        g.setGradientFill(juce::ColourGradient(Colour(0xffd9c4a1),cx-fr*.28f,cy-fr*.28f,Colour(0xff241b11),cx+fr*.32f,cy+fr*.32f,false));g.fillEllipse(cx-fr*.36f,cy-fr*.36f,fr*.72f,fr*.72f);
        g.setGradientFill(juce::ColourGradient(Colour(0xfff1e2bf),cx-fr*.16f,cy-fr*.2f,Colour(0xff7c6a48),cx+fr*.2f,cy+fr*.24f,false));
        g.fillEllipse(cx-fr*.26f,cy-fr*.26f,fr*.52f,fr*.52f);                           // brass hub
        // Round the hub shoulder into the dark spindle bore with a directional return.
        juce::Path hubLip;hubLip.addCentredArc(cx,cy,fr*.26f,fr*.26f,0.f,-1.8f,.6f,true);
        g.setColour(Colour(0xffffead0).withAlpha(.72f));g.strokePath(hubLip,juce::PathStrokeType(1.15f));
        g.setColour(Colour(0xff17120d).withAlpha(.78f));g.drawEllipse(cx-fr*.30f,cy-fr*.30f,fr*.60f,fr*.60f,.8f);
        g.setGradientFill(juce::ColourGradient(Colour(0xff0b0907),cx-fr*.11f,cy-fr*.13f,Colour(0xff645344),cx+fr*.14f,cy+fr*.17f,false));g.fillEllipse(cx-fr*.18f,cy-fr*.18f,fr*.36f,fr*.36f);
        g.setColour(Colour(0xfff4e3c6).withAlpha(.55f));g.drawEllipse(cx-fr*.254f,cy-fr*.254f,fr*.508f,fr*.508f,.7f);
        g.setGradientFill(juce::ColourGradient(Colour(0xffb6a78e),cx-fr*.10f,cy-fr*.10f,Colour(0xff19140e),cx+fr*.12f,cy+fr*.13f,false));g.fillEllipse(cx-fr*.125f,cy-fr*.125f,fr*.25f,fr*.25f);
        g.setColour(Colour(0xff0b0a08));g.fillEllipse(cx-fr*.065f,cy-fr*.065f,fr*.13f,fr*.13f);
        g.setColour(Colour(0xffffe4bc).withAlpha(.65f));g.fillEllipse(cx-fr*.095f,cy-fr*.10f,fr*.07f,fr*.05f);
    }
    // Tiny original mechanic etched on the transport bed, not on the reel faces.
    paintEngravedFigure(g,{bed.getX()+5,bed.getY()+1,12,bed.getHeight()-2},Colour(0xffddc093).withAlpha(.72f),BonePose::stand,phase);
    paintOpticalFinish(g,win);
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
        g.setColour(Colour(0xfff5dbad).withAlpha(.22f));g.drawRoundedRectangle(win.reduced(.9f),3.f,.65f);
        g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::black.withAlpha(.55f),win.getY(),juce::Colours::transparentBlack,win.getY()+10.f));g.fillRect(win.withHeight(10.f));
        g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::transparentWhite,win.getBottom()-7.f,Colour(0xffe5ba76).withAlpha(.26f),win.getBottom()));g.fillRoundedRectangle(win.withY(win.getBottom()-7.f).withHeight(7.f),2.f);
    }
    // The tape mechanism is decorative; its live controls are below the window.
}
inline void paintDemonVectorFallback(Graphics& g,juce::Rectangle<float> r,float phase=0,float level=0,bool frame=true){
    // Raise the lip to the lower chest rather than lengthening the rib cage.
    const auto win=r.withTrimmedBottom(43.f).withTrimmedTop(4.f);
    if(frame)paintHardwarePlate(g,win,Colour(0xff341218),false,6.f,3.f);
    auto a=win.reduced(5.f);juce::Graphics::ScopedSaveState clip(g);g.reduceClipRegion(r.toNearestInt());
    g.setGradientFill(juce::ColourGradient(Colour(0xff69120d),a.getCentreX(),a.getY()+a.getHeight()*.38f,Colour(0xff130407),a.getRight(),a.getBottom(),true));g.fillRoundedRectangle(a,4);
    const float scale=std::min(r.getWidth()/200.f,r.getHeight()/228.f);
    g.addTransform(juce::AffineTransform::scale(scale).translated(r.getCentreX()-100*scale,r.getY()+2.f));
    auto glow=[&](float x,float y,float rad,Colour c,float alpha){g.setGradientFill(juce::ColourGradient(c.withAlpha(alpha),x,y,juce::Colours::transparentBlack,x+rad,y,true));g.fillEllipse(x-rad,y-rad,2*rad,2*rad);};
    {juce::Graphics::ScopedSaveState bg(g);g.reduceClipRegion(juce::Rectangle<int>(5,5,190,207));
    glow(100,118,91,Colour(0xffff3e17),.17f+.17f*level);glow(30,159,60,Colour(0xffc62016),.19f);glow(176,51,56,Colour(0xffb21712),.20f);
    for(int i=0;i<24;++i)glow(8.f+float((i*47)%184),11.f+float((i*37)%190),9.f+(i%5)*6.f,Colour(0xff9c170e),.09f);}
    const float roar=3.f+9.f*(.5f+.5f*std::sin(phase*2.7f-.7f))+5.f*level;
    const auto bright=Colour(0xffef7954),shade=Colour(0xff300409),mid=Colour(0xffa12526),black=Colour(0xff080306);
    // Layered bony forms with dark undercuts, scarlet rim light and asymmetric highlights.
    auto sculpt=[&](const juce::Path& p,float ox,float oy,float dx,float dy){
        g.setColour(black.withAlpha(.92f));g.fillPath(p,juce::AffineTransform::translation(1.5f,2.4f));
        juce::ColourGradient gr(bright,ox,oy,shade,dx,dy,false);gr.addColour(.18,Colour(0xffc34733));gr.addColour(.43,mid);gr.addColour(.73,mid.darker(.74f));g.setGradientFill(gr);g.fillPath(p);
        g.setColour(Colour(0xfff6754e).withAlpha(.43f));g.strokePath(p,juce::PathStrokeType(.65f));
        juce::Graphics::ScopedSaveState sub(g);g.reduceClipRegion(p);g.setGradientFill(juce::ColourGradient(Colour(0xffffb886).withAlpha(.24f),ox,oy,juce::Colours::transparentWhite,dx,dy,true));g.fillPath(p);
    };
    auto bone=[&](float x1,float y1,float x2,float y2,float w){juce::Path p;p.startNewSubPath(x1,y1);p.lineTo(x2,y2);g.setColour(black);g.strokePath(p,juce::PathStrokeType(w+3,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));g.setGradientFill(juce::ColourGradient(bright,x1,y1,shade,x2+3,y2,false));g.strokePath(p,juce::PathStrokeType(w,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));g.setColour(bright.withAlpha(.65f));g.drawLine(x1-1,y1,x2-1,y2,w*.16f);};
    for(int i=0;i<33;++i){float x=5.f+float((i*43)%190),y=204.f-std::fmod(i*23.f+phase*12.f,202.f);g.setColour(Colour(0xffff6334).withAlpha(.12f+.17f*level));g.fillEllipse(x,y,1.4f,3.2f);}
    // Spinal column and rib cage, visible through the open black cavities.
    for(int i=0;i<12;++i){float y=109+i*7.3f;juce::Path v;v.addRoundedRectangle(94.f-(i%2),y,13,6,2);sculpt(v,94,y,108,y+7);}
    for(int side:{-1,1}){
        auto X=[&](float x){return 100+side*x;};
        // Blade-like scapula and jagged shoulder ridges.
        juce::Path shoulder;shoulder.startNewSubPath(X(10),118);shoulder.cubicTo(X(24),102,X(43),99,X(62),92);shoulder.lineTo(X(59),107);shoulder.lineTo(X(79),99);shoulder.lineTo(X(70),120);shoulder.cubicTo(X(49),140,X(35),130,X(10),125);shoulder.closeSubPath();sculpt(shoulder,X(20),100,X(60),139);
        for(int k=0;k<7;++k){float y=124+k*10.2f;juce::Path rib;rib.startNewSubPath(X(5),y);rib.cubicTo(X(25),y-8,X(50-k*2.f),y-5,X(45-k*2.f),y+9);rib.cubicTo(X(35-k*2.f),y+18,X(17),y+14,X(10),y+8);g.setColour(black);g.strokePath(rib,juce::PathStrokeType(8.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));g.setGradientFill(juce::ColourGradient(bright,X(20),y-3,shade,X(43),y+14,false));g.strokePath(rib,juce::PathStrokeType(4.8f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
        // Reach forward: exposed arm bones, articulated knuckles and hooked claws.
        bone(X(67),119,X(86),156,13);bone(X(79),153,X(61),190,8);bone(X(90),155,X(71),191,6);
        // Long swept segmented horns, red bone rather than smooth beige cones.
        juce::Path horn;horn.startNewSubPath(X(20),60);horn.cubicTo(X(50),68,X(87),39,X(72),5);horn.cubicTo(X(97),23,X(96),62,X(72),85);horn.cubicTo(X(47),97,X(25),85,X(20),75);horn.closeSubPath();sculpt(horn,X(68),16,X(23),91);
        for(int i=0;i<7;++i){float t=i/7.f,xx=X(35+33*t),yy=77-42*t*t;juce::Path grooves;grooves.startNewSubPath(xx-side*5,yy-5);grooves.quadraticTo(xx,yy+3,xx+side*7,yy+2);g.setColour(black.withAlpha(.67f));g.strokePath(grooves,juce::PathStrokeType(1.6f));g.setColour(bright.withAlpha(.43f));g.strokePath(grooves,juce::PathStrokeType(.55f),juce::AffineTransform::translation(0,-1.3f));}
    }
    // The dominant skeletal face: cheek hollows, nasal aperture, separated jaw and fangs.
    juce::Path skull;skull.startNewSubPath(100,35);skull.cubicTo(64,25,46,44,48,72);skull.lineTo(64,91);skull.lineTo(76,94);skull.lineTo(79,110);skull.lineTo(88,117);skull.lineTo(112,117);skull.lineTo(121,110);skull.lineTo(124,94);skull.lineTo(136,91);skull.lineTo(152,72);skull.cubicTo(154,44,136,25,100,35);skull.closeSubPath();sculpt(skull,69,34,128,111);
    {juce::Graphics::ScopedSaveState face(g);g.reduceClipRegion(skull);glow(75,40,36,Colour(0xffffb27b),.26f);glow(131,74,34,Colour(0xff200309),.31f);
        for(int i=0;i<100;++i){const float x=56.f+float((i*53)%88),y=34.f+float((i*37)%77);g.setColour((i%3?shade:bright).withAlpha(.19f));g.fillEllipse(x,y,.55f+float(i%3)*.4f,.8f+float(i%4)*.35f);}}
    juce::Path mouth;mouth.startNewSubPath(73,97);mouth.quadraticTo(100,88,127,97);mouth.cubicTo(127,131+roar,115,143+roar,100,145+roar);mouth.cubicTo(85,143+roar,73,131+roar,73,97);mouth.closeSubPath();g.setColour(black);g.fillPath(mouth);glow(100,121,18,Colour(0xff8b140d),.22f);
    juce::Path jaw;jaw.startNewSubPath(67,93);jaw.cubicTo(68,124+roar,75,140+roar,86,147+roar);jaw.quadraticTo(100,157+roar,114,147+roar);jaw.cubicTo(125,140+roar,132,124+roar,133,93);jaw.lineTo(124,99);jaw.cubicTo(122,123+roar,116,136+roar,100,138+roar);jaw.cubicTo(84,136+roar,78,123+roar,76,99);jaw.closeSubPath();sculpt(jaw,74,110,126,155+roar);
    for(int i=0;i<9;++i){float x=76.8f+i*5.2f,y=98.f+std::abs(i-4)*.5f;juce::Path tooth;tooth.startNewSubPath(x,y);tooth.quadraticTo(x+2.5f,y-2,x+4.9f,y);tooth.cubicTo(x+4.9f,y+3,x+3.9f,y+6,x+2.2f,y+(i==0||i==8?15.f:9.f));tooth.quadraticTo(x+.1f,y+6,x,y);tooth.closeSubPath();sculpt(tooth,x,y,x+5,y+13);
        const float yy=139+roar-std::abs(i-4)*1.3f;juce::Path low;low.startNewSubPath(x,yy);low.quadraticTo(x+1,yy-7,x+2.6f,yy-(i==0||i==8?12.f:7.f));low.quadraticTo(x+5,yy-4,x+5,yy);low.closeSubPath();sculpt(low,x,yy-8,x+5,yy);}
    for(int side:{-1,1}){auto X=[&](float x){return 100+side*x;};juce::Path socket;socket.startNewSubPath(X(5),70);socket.lineTo(X(32),58);socket.quadraticTo(X(39),79,X(25),84);socket.quadraticTo(X(11),85,X(5),70);socket.closeSubPath();g.setColour(black);g.fillPath(socket);g.setColour(shade);g.strokePath(socket,juce::PathStrokeType(2.2f));
        float ex=X(21);g.setGradientFill(juce::ColourGradient(Colour(0xffff5a22).withAlpha(.85f),ex,74,juce::Colours::transparentBlack,ex+13,85,true));g.fillEllipse(ex-15,61,30,27);g.setColour(Colour(0xffffa350).withAlpha(.76f+.23f*std::sin(phase*2)));g.fillEllipse(ex-5,71,10,4);g.setColour(Colour(0xfffff0bf));g.fillEllipse(ex-1.5f,71.6f,3,2.8f);
        juce::Path brow;brow.startNewSubPath(X(3),66);brow.quadraticTo(X(22),48,X(37),56);brow.lineTo(X(31),62);brow.lineTo(X(8),72);brow.closeSubPath();sculpt(brow,X(15),52,X(27),72);
        juce::Path hollow;hollow.startNewSubPath(X(33),83);hollow.lineTo(X(15),90);hollow.lineTo(X(20),101);hollow.lineTo(X(34),91);hollow.closeSubPath();g.setColour(shade.darker(.6f));g.fillPath(hollow);
    }
    juce::Path nose;nose.startNewSubPath(100,76);nose.lineTo(94,93);nose.lineTo(100,90);nose.lineTo(106,93);nose.closeSubPath();g.setColour(black);g.fillPath(nose);
    // Engraved irregular cranial sutures and pitted facial ridges break the smooth icon look.
    juce::Path cracks;cracks.startNewSubPath(99,36);cracks.lineTo(103,46);cracks.lineTo(97,54);cracks.lineTo(101,63);cracks.startNewSubPath(73,42);cracks.lineTo(80,49);cracks.lineTo(77,54);cracks.lineTo(86,57);cracks.startNewSubPath(131,49);cracks.lineTo(125,53);cracks.lineTo(128,59);cracks.startNewSubPath(66,83);cracks.lineTo(73,88);cracks.lineTo(69,92);
    g.setColour(black.withAlpha(.72f));g.strokePath(cracks,juce::PathStrokeType(1.25f));g.setColour(bright.withAlpha(.38f));g.strokePath(cracks,juce::PathStrokeType(.65f),juce::AffineTransform::translation(.7f,-.65f));
    for(int i=0;i<33;++i){const float x=68.f+float((i*31)%65),y=39.f+float((i*13)%15);g.setColour(shade.withAlpha(.48f));g.fillEllipse(x,y,.9f,1.4f);}
    // Reaching hands: in front of the rim, not flat inside the window.
    for(int side:{-1,1}){auto X=[&](float x){return 100+side*x;};const float reach=2.f*std::sin(phase*1.65f+side*.8f);
        juce::Path palm;palm.startNewSubPath(X(81),184+reach);palm.cubicTo(X(74),180+reach,X(51),185+reach,X(45),195+reach);palm.cubicTo(X(49),208+reach,X(71),216+reach,X(83),204+reach);palm.closeSubPath();sculpt(palm,X(50),184+reach,X(79),213+reach);
        for(int f=0;f<4;++f){const float xx=X(48+f*11.f),yy=194+reach+std::abs(f-1)*1.4f;bone(xx,yy,xx+side*3,yy+10,7.f);bone(xx+side*3,yy+10,xx-side*3,yy+19,5.2f);juce::Path claw;claw.startNewSubPath(xx-side*6,yy+17);claw.cubicTo(xx+side*2,yy+15,xx+side*3,yy+24,xx-side*7,yy+31-f*1.4f);claw.quadraticTo(xx-side*3,yy+25,xx-side*9,yy+21);claw.closeSubPath();sculpt(claw,xx,yy+17,xx-side*4,yy+31);}
    }
    // A diffuse red reflected light across the chamber, never obscuring the eyes.
    g.setGradientFill(juce::ColourGradient(Colour(0xffff3d19).withAlpha(.10f+.06f*level),100,189,juce::Colours::transparentBlack,100,126,true));g.fillEllipse(43,139,114,65);
}
// Original baked perspective relief. The nine frames share one decoded atlas.
inline const juce::Image& demonReliefAtlas(){
    static const auto atlas=juce::ImageFileFormat::loadFrom(SkelliereKnobData::demon_relief_atlas_png,SkelliereKnobData::demon_relief_atlas_pngSize);
    return atlas;
}
inline bool demonReliefIsValid(){const auto& a=demonReliefAtlas();return a.isValid()&&a.hasAlphaChannel()&&a.getWidth()==1296&&a.getHeight()==1488;}
inline const juce::Image& infernalBackdrop();
inline void paintDemonDetail(Graphics& g,juce::Rectangle<float> r,float phase=0,float level=0,bool frame=true){
    if(!demonReliefIsValid()){paintDemonVectorFallback(g,r,phase,level,frame);return;}
    // Raise the lip to the lower chest rather than lengthening the rib cage.
    const auto win=r.withTrimmedBottom(43.f).withTrimmedTop(4.f);const auto a=win.reduced(5.f);
    if(frame){
        paintHardwarePlate(g,win,Colour(0xff381c16),false,6.f,3.f);
        g.setColour(Colour(0xffbd8150).withAlpha(.78f));g.drawRoundedRectangle(win.reduced(2.f),4.f,.9f);
        g.setColour(Colour(0xff080505));g.drawRoundedRectangle(win.reduced(4.f),3.5f,1.2f);
        for(int k=0;k<4;++k)paintScrew(g,k%2?win.getRight()-4.5f:win.getX()+4.5f,k<2?win.getY()+4.5f:win.getBottom()-4.5f,2.7f,.7f*k);
        return;
    }
    juce::Graphics::ScopedSaveState clip(g);g.reduceClipRegion(r.toNearestInt());
    g.setGradientFill(juce::ColourGradient(Colour(0xff5b140f),a.getCentreX(),a.getY()+a.getHeight()*.53f,Colour(0xff090306),a.getRight(),a.getBottom(),true));g.fillRoundedRectangle(a,4.f);
    {juce::Graphics::ScopedSaveState bg(g);g.reduceClipRegion(a.toNearestInt());
        g.setOpacity(1.f);g.drawImage(infernalBackdrop(),a);
        for(int i=0;i<21;++i){float x=a.getX()+a.getWidth()*float((i*47)%100)/100.f,y=a.getY()+a.getHeight()*float((i*37)%100)/100.f,rad=15.f+(i%5)*7.f;g.setGradientFill(juce::ColourGradient(Colour(0xffc32613).withAlpha(.07f+.025f*level),x,y,juce::Colours::transparentBlack,x+rad,y,true));g.fillEllipse(x-rad,y-rad,rad*2,rad*2);}
        for(int i=0;i<28;++i){const float x=a.getX()+float((i*43)%100)*a.getWidth()/100.f,y=a.getBottom()-std::fmod(i*23.f+phase*12.f,a.getHeight());g.setColour(Colour(0xffff7031).withAlpha(.15f+.19f*level));g.fillEllipse(x,y,1.1f,2.8f);}
    }
    const int index=juce::jlimit(0,8,int(std::round(8.f*juce::jlimit(0.f,1.f,.5f+.43f*std::sin(phase*2.7f-.7f)+.12f*level))));
    const auto& atlas=demonReliefAtlas();const float dw=r.getWidth()+25.f,dh=dw*496.f/432.f;const auto dst=juce::Rectangle<float>(r.getCentreX()-dw*.5f,r.getBottom()-dh-10.f,dw,dh);g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);g.setOpacity(1.f);
    g.drawImage(atlas, dst.getX(),dst.getY(),dst.getWidth(),dst.getHeight(),(index%3)*432,(index/3)*496,432,496);
}
// Distinct patina/finish per module. Static faceplate cache only, no audio dependency.
inline void paintIdentityFinish(Graphics& g,juce::Rectangle<float> r,int type,bool light){
    juce::Graphics::ScopedSaveState save(g);juce::Path clip;clip.addRoundedRectangle(r.getX(),r.getY(),r.getWidth(),r.getHeight(),4.f);g.reduceClipRegion(clip);
    const bool red=type==SkellDemon, ruby=type==SkellRubyQ, tape=type==SkellMixHead;
    const auto tint=red?Colour(0xffe44319):ruby?Colour(0xffe94e75):tape?Colour(0xffe0a47c):Colour(0xffe3d8bb);
    // Layered casting grain / lacquer clouding, generated with a local seed.
    juce::Random rng(0x7f124+type*937);
    for(int i=0;i<(red||ruby?240:90);++i){float x=r.getX()+rng.nextFloat()*r.getWidth(),y=r.getY()+rng.nextFloat()*r.getHeight(),rr=6.f+rng.nextFloat()*(red?56.f:32.f);auto c=(i%3==0?tint:juce::Colours::black).withAlpha(red||ruby?.055f:.028f);g.setGradientFill(juce::ColourGradient(c,x,y,c.withAlpha(0.f),x+rr,y,true));g.fillEllipse(x-rr,y-rr,2*rr,2*rr);}
    for(int i=0;i<(red?1750:ruby?1050:420);++i){float x=r.getX()+rng.nextFloat()*r.getWidth(),y=r.getY()+rng.nextFloat()*r.getHeight();g.setColour((i%3==0?tint:juce::Colours::black).withAlpha(red?.19f:ruby?.13f:light?.09f:.08f));g.fillEllipse(x,y,.35f+rng.nextFloat()*1.5f,.35f+rng.nextFloat()*1.3f);}
    // Give each effect family a distinct manufacturing finish. Static cached
    // detail; the seed and pitch differ for every individual processor.
    const int cat=info(type).cat;
    if(!red&&!ruby&&!tape&&type!=SkellDope&&type!=SkellSSL){
        auto pigment=originalSpec(type,1).accent;
        const int pitch=3+type%5;
        if(cat==CatTone){
            for(int x=4;x<int(r.getWidth())-4;x+=pitch){float xx=r.getX()+float(x);g.setColour(pigment.withAlpha(.027f));g.drawVerticalLine(int(xx),r.getY()+4,r.getBottom()-4);g.setColour(juce::Colours::black.withAlpha(.042f));g.drawVerticalLine(int(xx+1),r.getY()+4,r.getBottom()-4);}
        }else if(cat==CatDynamics){
            for(int i=0;i<850;++i){float x=r.getX()+rng.nextFloat()*r.getWidth(),y=r.getY()+rng.nextFloat()*r.getHeight(),sz=.5f+rng.nextFloat();g.setColour(juce::Colours::black.withAlpha(.11f));g.fillEllipse(x,y,sz,sz);g.setColour(pigment.withAlpha(.085f));g.fillEllipse(x-.35f,y-.40f,sz*.70f,sz*.70f);}
        }else{
            for(int i=0;i<210;++i){float x=r.getX()+rng.nextFloat()*r.getWidth(),y=r.getY()+rng.nextFloat()*r.getHeight(),len=4.f+rng.nextFloat()*18.f;g.setColour((i%3?pigment:juce::Colours::black).withAlpha(cat==CatSpace?.075f:.044f));g.drawLine(x,y,juce::jmin(r.getRight()-2,x+len),y+.13f,.45f);}
        }
        g.setColour(pigment.withAlpha(light?.16f:.28f));g.drawRoundedRectangle(r.reduced(2.f),4.f,.6f);
    }
    if(ruby){
        // Deep lacquer with a single sculpted ruby cartouche; leave the legends clear.
        const float cx=r.getCentreX(),cy=r.getY()+107.f;const float rw=25.f,rh=14.f;
        juce::Path jewel;jewel.startNewSubPath(cx-rw,cy);jewel.lineTo(cx-rw*.57f,cy-rh);jewel.lineTo(cx+rw*.57f,cy-rh);jewel.lineTo(cx+rw,cy);jewel.lineTo(cx+rw*.57f,cy+rh);jewel.lineTo(cx-rw*.57f,cy+rh);jewel.closeSubPath();
        g.setColour(juce::Colours::black.withAlpha(.7f));g.fillPath(jewel,juce::AffineTransform::translation(0,2.f));
        g.setGradientFill(juce::ColourGradient(Colour(0xfff48b97),cx-rw,cy-rh,Colour(0xff2c0015),cx+rw,cy+rh,false));g.fillPath(jewel);
        for(int k=0;k<6;++k){const juce::Point<float> v[]={{cx-rw,cy},{cx-rw*.57f,cy-rh},{cx+rw*.57f,cy-rh},{cx+rw,cy},{cx+rw*.57f,cy+rh},{cx-rw*.57f,cy+rh}};juce::Path facet;facet.startNewSubPath(cx+3,cy-3);facet.lineTo(v[k]);facet.lineTo(v[(k+1)%6]);facet.closeSubPath();g.setColour((k%2==0?Colour(0xfffcbac0):Colour(0xff1b0610)).withAlpha(k==1?.26f:.13f));g.fillPath(facet);}
        g.setColour(Colour(0xffedc58d).withAlpha(.9f));g.strokePath(jewel,juce::PathStrokeType(.9f));g.setColour(Colour(0xffe2ba82).withAlpha(.37f));g.drawHorizontalLine(int(cy),r.getX()+15,cx-rw-8);g.drawHorizontalLine(int(cy),cx+rw+8,r.getRight()-15);
        // Sparse engraved medallion pattern, away from the hardware controls.
        for(int i=0;i<3;++i){float y=r.getY()+r.getHeight()*(.33f+i*.247f);juce::Path f;f.startNewSubPath(r.getX()+13,y+25);f.quadraticTo(cx,y+53,r.getRight()-13,y+25);g.setColour(Colour(0xffd9b27c).withAlpha(.17f));g.strokePath(f,juce::PathStrokeType(.6f));}
        g.setColour(Colour(0xffd0a267).withAlpha(.65f));g.drawRoundedRectangle(r.reduced(5.f),3.f,.8f);g.setColour(Colour(0xff1f060e).withAlpha(.8f));g.drawRoundedRectangle(r.reduced(7.f),2.f,1.1f);
    }else if(tape){
        auto band=r.reduced(8.f,0.f).withTrimmedTop(100.f).withTrimmedBottom(21.f);g.setGradientFill(juce::ColourGradient(Colour(0xff32120c).withAlpha(.28f),band.getX(),band.getY(),Colour(0xffc05b36).withAlpha(.16f),band.getRight(),band.getY(),false));g.fillRoundedRectangle(band,3.f);g.setColour(Colour(0xffd7af79).withAlpha(.28f));g.drawRoundedRectangle(band,3.f,.9f);
    }else if(red){g.setColour(Colour(0xffba7750).withAlpha(.48f));g.drawRoundedRectangle(r.reduced(4.f),4.f,.8f);g.setColour(Colour(0xff270509));g.drawRoundedRectangle(r.reduced(6.f),3.f,1.f);}
}
// Infernal background, baked once into a software image; animation only composites it.
inline const juce::Image& infernalBackdrop(){
    static const auto image=[] {constexpr int w=384,h=512;juce::Image im(juce::Image::ARGB,w,h,true,juce::SoftwareImageType());
        auto hash=[](int x,int y){unsigned v=unsigned(x)*0x9e3779b9u+unsigned(y)*0x85ebca6bu+0xf41e2u;v^=v>>16;v*=0x7feb352du;v^=v>>15;return float(v&65535u)/65535.f;};
        auto field=[&](float x,float y){const int i=int(std::floor(x)),j=int(std::floor(y));float a=x-i,b=y-j;a=a*a*(3-2*a);b=b*b*(3-2*b);return (hash(i,j)*(1-a)+hash(i+1,j)*a)*(1-b)+(hash(i,j+1)*(1-a)+hash(i+1,j+1)*a)*b;};
        auto fbm=[&](float x,float y){float n=0,amp=.54f;for(int k=0;k<7;++k){n+=amp*field(x,y);x=x*2.04f+17.f;y=y*2.03f+5.f;amp*=.49f;}return n;};
        juce::Image::BitmapData px(im,juce::Image::BitmapData::writeOnly);for(int y=0;y<h;++y)for(int x=0;x<w;++x){const float u=float(x)/w,v=float(y)/h,warp=fbm(u*4+12,v*5+3);const float n=fbm(u*6+warp*2,v*8+warp*3),n2=fbm(u*17+warp*4,v*19+warp*4);float flame=metal::sat((fbm(u*10+warp*4,v*2.3f+warp*2)-.36f)*3.5f);float light=metal::sat((n-.25f)*1.9f+.36f*flame);float fissure=std::exp(-105.f*std::abs(n2-.51f));float halo=std::exp(-4.2f*((u-.5f)*(u-.5f)+(v-.5f)*(v-.5f)));auto c=Colour(0xff090103).interpolatedWith(Colour(0xffad160b),metal::sat(light*halo+.18f*fissure));c=c.interpolatedWith(Colour(0xffffa139),metal::sat(fissure*.24f*halo+flame*.34f*(.35f+.65f*v)));px.setPixelColour(x,y,c);}return im;}();return image;
}
inline void paintOriginalSurface(Graphics& g,juce::Rectangle<float> r,int type,int finish) {
    const auto p=originalSpec(type,finish);
    // The reference's dark anodised panels are substantially darker and more
    // saturated than the previous washed-out capture. Avoid additive white
    // gradients that lift the entire blue/olive/burgundy material.
    const Colour base=p.light?p.metal.interpolatedWith(Colour(0xffead6b8),.08f)
        :p.metal.interpolatedWith(Colour(0xff0e0d0b),.21f);
    paintHardwarePlate(g,r.reduced(.3f),base,p.light,7.f,4.3f);
    auto f=r.reduced(6.f);g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(.055f),f.getX(),f.getY(),juce::Colours::black.withAlpha(.20f),f.getRight(),f.getBottom(),false));g.fillRoundedRectangle(f,6.f);
    g.setColour(juce::Colours::black.withAlpha(.55f));g.drawRoundedRectangle(r.reduced(4.f),4.f,.85f);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xffdfc8ab).withAlpha(p.light?.32f:.15f),f.getY(),juce::Colours::transparentWhite,f.getY()+5.f));g.fillRect(f.withHeight(4.f));
    g.setGradientFill(juce::ColourGradient::vertical(juce::Colours::transparentBlack,f.getBottom()-8.f,juce::Colours::black.withAlpha(.30f),f.getBottom()));g.fillRect(f.withY(f.getBottom()-8.f).withHeight(8.f));
    paintIdentityFinish(g,f,type,p.light);
    paintAnalogWear(g,f,p.light,0x521d+type*179+finish*11,1.30f);
    g.setColour((p.light?Colour(0xfffcf2d7):Colour(0xffc2b49a)).withAlpha(.25f));g.drawRoundedRectangle(r.reduced(3.f),5.f,.55f);
    g.setColour(juce::Colours::black.withAlpha(.62f));g.drawRoundedRectangle(r.reduced(5.f),4.f,.65f);
    g.setColour(p.accent.withAlpha(p.light?.20f:.32f));g.drawRoundedRectangle(r.reduced(7.5f),3.5f,.55f);
    for(int i=0;i<4;++i)paintScrew(g,i%2?r.getRight()-15:r.getX()+15,i<2?r.getY()+15:r.getBottom()-15,6.8f,.8f*i);
}
inline void paintOriginalMini(Graphics& g,juce::Rectangle<float> r,int type) {
    const auto p=originalSpec(type,1);
    // Small library symbols use a single hardware badge, not tiny versions
    // of the entire faceplate (which produced four distracting screw dots).
    g.setGradientFill(juce::ColourGradient(p.metal.interpolatedWith(Colour(0xff3b3833),.65f),r.getX(),r.getY(),Colour(0xff211e1b),r.getRight(),r.getBottom(),false));
    g.fillRoundedRectangle(r.reduced(1.f),4.f);
    g.setColour(Colour(0xffd0bda1).withAlpha(.56f));g.drawRoundedRectangle(r.reduced(1.4f),4.f,.75f);
    g.setColour(juce::Colours::black.withAlpha(.36f));g.drawRoundedRectangle(r.reduced(2.8f),3.f,.7f);
    auto f=r.reduced(6);
    if(type==Reverb||type==Chorus){g.setColour(Colour(0xff151e25));g.fillRoundedRectangle(f,2.f);paintBoneFigure(g,f.reduced(1.f),Colour(0xffd4c8b0),type==Reverb?BonePose::stand:BonePose::mirror);return;}
    if(isAirPanel(type)){juce::Graphics::ScopedSaveState save(g);juce::Path clip;clip.addRoundedRectangle(f.getX(),f.getY(),f.getWidth(),f.getHeight(),2.f);g.reduceClipRegion(clip);g.setOpacity(1.f);g.drawImage(airAtmosphere(),f);g.setColour(Colour(0xfff2e9d0));g.drawLine(f.getX()+2,f.getBottom()-3,f.getRight()-2,f.getY()+3,.9f);return;}
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
    if(info(type).hasVU||hasOutputPeak(type)){
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
        const bool dark=true; // all miniature badges now use dark metal
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
        case Eq:case Bell:case Tilt:case DualFilter:case SkellRubyQ:case SkellDope:{
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
