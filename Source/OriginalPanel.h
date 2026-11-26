#pragma once
#include "Look.h"
namespace skl::ui {
#include "AtelierArt.h"
#include "ReferenceTapeArtwork.h"
struct PanelSpec { Colour metal,accent,cap; bool light; const char* series; };
inline PanelSpec originalSpec(int type,int finish) {
    static constexpr unsigned metals[]={0x25292c,0x9eaaa6,0xe1c4a6,0x62352e,0x5d4132,0xc4c0ad,0x293640,0x34382a,0x45433e,0x323235,0x6b716e,0x242d34,0x97876e,0x425c61,0x62403b,0x344c59,0xc1aa83,0x454f3e,0x71614d,0x303e4b,0x5e6354,0x784d3e,0x3c594d,0x4e4059,0x59322e,0x706449,0x496464,0x45484a};
    static constexpr unsigned accents[]={0x9a9a9a,0x39534b,0xaed5bf,0xe0b88e,0xe9be7b,0x365b55,0xa4bedc,0xe7c68c,0xaed0aa,0xe2ad9f,0xb5d3c4,0x9bddb7,0x3c514b,0xa4d4df,0xe5d9bf,0xb7d8cf,0x26392f,0xc0d58e,0xf0cc9b,0x405e50,0xe1d7ab,0xf3b397,0xc1d4ab,0xd5b7de,0xa7d5e5,0xf0cd98,0xc6e8da,0xc6c7b4};
    type=std::clamp(type,0,static_cast<int>(kNumRackTypes)-1);
    auto metal=Colour(0xff000000u|metals[type]);const auto accent=Colour(0xff000000u|accents[type]);
    if(finish==1&&(type==Air||type==SkellAirFX))metal=Colour(type==Air?0xff344e60:0xff284251);
    if(finish==1&&type==Sat)metal=Colour(0xff594033);
    if(finish==1&&type==Reverb)metal=Colour(0xff423b4a);
    if(finish==1&&type==Chorus)metal=Colour(0xff29454a);
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
    if(type==Delay||type==Slap||type==Air||type==SkellAirFX||type==Reverb||type==Chorus||type==Sat||type==SoftClip||type==Eq||type==Tremolo||type==Phaser||type==Stereo||type==Transient)l.detail={16,106,width-32,120};
    if(type==Eq)l.detail.setHeight(82);
    const bool window=!l.meter.isEmpty()||!l.detail.isEmpty();
    // Four-window controls leave a lower breathing area like real rack gear.
    // Five-control compressors need the bottom row for makeup gain.
    const int top=type==Eq?199:window?(n==5?(editing?248:253):242):108;
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
inline bool isAirPanel(int type) { return type==Air||type==SkellAirFX; }
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
inline void paintOriginalSurface(Graphics& g,juce::Rectangle<float> r,int type,int finish) {
    const auto p=originalSpec(type,finish);
    // The reference's dark anodised panels are substantially darker and more
    // saturated than the previous washed-out capture. Avoid additive white
    // gradients that lift the entire blue/olive/burgundy material.
    const Colour base=p.light?p.metal.interpolatedWith(Colour(0xffead6b8),.08f)
        :p.metal.interpolatedWith(Colour(0xff0e0d0b),.21f);
    paintHardwarePlate(g,r.reduced(.3f),base,p.light,10.f,4.9f);
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
