#pragma once
#include "Dsp.h"
#include "AlignedTape4x.h"
#include <SkellCore/backends/vocal/SkellAir.h>
#include <SkellCore/backends/vocal/ClassicDeEsser.h>
namespace skl {
// New identities only. Existing modules and their normalized mappings remain untouched.
class ConsolidatedModule final : public ModuleBase {
public:
    explicit ConsolidatedModule(int t):type(t){}
    void prepare(double sr,int,int) override {
        rate=std::clamp(std::isfinite(sr)?sr:48000.,8000.,384000.);
        if(type==Doubler||type==EchoUnified||type==RoomUnified)for(auto& d:delay)d.prepare(int(rate*(type==Doubler?.12:type==RoomUnified?.12:4.05))+8);
        if(type==RoomUnified)for(int j=0;j<3;++j)rooms[j].prepare(rate,roomSize[j]);
        if(type==ClipUnified)clip.prepare();
        if(type==AirUnified)air.prepare(rate);
        if(type==DeEsserUnified){deess.prepare(rate);phrase.prepare(rate);}
        reset();
    }
    int latency() const override {return type==ClipUnified?AlignedTape4x::delay:0;}
    void setTempo(double bpm) noexcept override {tempo=std::clamp(std::isfinite(bpm)?bpm:120.,30.,300.);}
    void reset() override {
        for(auto& f:filters)f.reset();
        for(auto& f:detect)f.reset();
        for(auto& d:delay)d.clear();
        energy.fill(0);gain.fill(0);lp.fill(0);phase=0;tick=0;wet=1;seed=false;curDelay=-1;meterDb.store(0);
        if(type==RoomUnified){for(auto& f:rooms)f.reset();}
        if(type==ClipUnified)clip.reset();
        if(type==AirUnified)air.reset();
        if(type==DeEsserUnified){deess.reset();phrase.reset();}
    }
    float meter() const override {return meterDb.load(std::memory_order_relaxed);}
    void process(juce::AudioBuffer<float>& b,const float* k,bool active) override {
        const int ch=std::min(2,b.getNumChannels());if(!ch||!b.getNumSamples())return;
        const int count=info(type).numKnobs;float targets[6]{};for(int j=0;j<count;++j)targets[j]=std::isfinite(k[j])?std::clamp(k[j],0.f,1.f):defaultNorm(type,j);
        if(!seed){std::copy(targets,targets+6,p);wet=active?1.f:0.f;update();seed=true;}
        float lastMeter=0;
        const float step=float(1/(.025*rate)),fade=float(1/(.005*rate));auto* l=b.getWritePointer(0);auto* r=ch==2?b.getWritePointer(1):nullptr;
        for(int i=0;i<b.getNumSamples();++i){
            bool changed=false;for(int j=0;j<count;++j){const float next=p[j]+std::clamp(targets[j]-p[j],-step,step);changed|=next!=p[j];p[j]=next;}dirty|=changed;
            if((tick++&31)==0){if(dirty){update();dirty=false;}if(type==DynamicEQ)filters[0].peaking(rate,v[0],v[1],gain[0]);if(type==Resonance)for(int j=0;j<8;++j)filters[j].peaking(rate,freq[j],v[3],gain[j]);}
            wet+=std::clamp((active?1.f:0.f)-wet,-fade,fade);
            float x=sane(l[i]),y=r?sane(r[i]):x;float out[2]={x,y};const float in[2]={x,y};
            const float peak=std::max(std::abs(x),std::abs(y));float gr=0;
            switch(type){
            case VoxRider:{
                energy[0]=smooth(energy[0],peak*peak,a,bp);const float level=g2db(std::sqrt(energy[0]));
                const float eligibility=std::clamp((level-v[3])/6.f,0.f,1.f);
                const float want=eligibility*std::clamp(v[0]-level,-v[1],v[1]);
                gain[0]=smooth(gain[0],want,cp,cp);for(int c=0;c<ch;++c)out[c]=in[c]*db2g(gain[0]*v[4]);gr=gain[0];break;}
            case DynamicEQ:{
                const float d=bandLevel(0,in,ch);energy[0]=smooth(energy[0],d*d,a,bp);
                const float amount=std::clamp((g2db(std::sqrt(energy[0]))-v[2])/12.f,0.f,1.f);
                const float want=v[3]*amount;gain[0]=smooth(gain[0],want,std::abs(want)>std::abs(gain[0])?a:bp,std::abs(want)>std::abs(gain[0])?a:bp);for(int c=0;c<ch;++c)out[c]=filters[0].process(c,in[c]);gr=std::abs(gain[0]);break;}
            case Multiband:{
                float bands[2][6]{};for(int c=0;c<ch;++c){float prev=0;for(int j=0;j<5;++j){const float next=filters[j].process(c,in[c]);bands[c][j]=next-prev;prev=next;}bands[c][5]=in[c]-prev;}
                float correction[2]{};for(int j=0;j<6;++j){const float d=std::max(std::abs(bands[0][j]),ch>1?std::abs(bands[1][j]):0.f);energy[j]=smooth(energy[j],d*d,a,bp);const float want=-std::min(v[2],std::max(0.f,g2db(std::sqrt(energy[j]))-v[0])*(1-1/v[1]));gain[j]=smooth(gain[j],want,dp,cp);gr=std::max(gr,-gain[j]);const float g=db2g(gain[j])-1;for(int c=0;c<ch;++c)correction[c]+=bands[c][j]*g;}for(int c=0;c<ch;++c)out[c]=in[c]+correction[c]*v[5];break;}
            case Resonance:{
                for(int j=0;j<8;++j){const float d=bandLevel(j,in,ch);energy[j]=smooth(energy[j],d*d,a,bp);}
                for(int j=0;j<8;++j){const float around=(energy[std::max(0,j-1)]+energy[std::min(7,j+1)])*.5f;const float prominence=10.f*std::log10((energy[j]+1e-9f)/(around+1e-9f));const float want=energy[j]>1e-7f?-std::min(v[1],std::max(0.f,prominence-3.f)*v[0]):0.f;gain[j]=smooth(gain[j],want,dp,cp);gr=std::max(gr,-gain[j]);}for(int c=0;c<ch;++c){float z=in[c];for(int j=0;j<8;++j)z=filters[j].process(c,z);out[c]=in[c]+v[5]*(z-in[c]);}break;}
            case Doubler:{
                const float mono=.5f*(x+y);delay[0].write(mono);delay[1].write(mono);
                const float rateHz=v[2],depth=std::min(v[0]*.45f,1000.f*(std::pow(2.f,v[1]/1200.f)-1.f)/(float(2*kPi)*rateHz));
                const float w0=delay[0].read((v[0]+depth*std::sin(phase))*float(rate*.001));const float w1=delay[1].read((v[0]*1.28f+depth*.72f*std::sin(phase*1.19f+1.7f))*float(rate*.001));
                phase+=float(2*kPi*rateHz/rate);if(phase>float(2*kPi*100))phase-=float(2*kPi*100);
                const float mid=.5f*(w0+w1),side=.5f*(w0-w1)*v[3];out[0]=x*(1-v[4])+v[4]*(mid+side);out[1]=y*(1-v[4])+v[4]*(mid-side);if(ch==1)out[0]=x*(1-v[4])+v[4]*mid;break;}
            case Exciter:{
                for(int c=0;c<ch;++c){const float z=detect[0].process(c,in[c])*db2g(v[1]);const float residual=std::tanh(z)-z;out[c]=in[c]+v[2]*filters[0].process(c,residual)/db2g(v[1]);}break;}
            case AirUnified:{
                const auto d=air.process(x,y);for(int c=0;c<ch;++c){const float st=filters[1].process(c,filters[0].process(c,in[c]))*db2g(v[3]);out[c]=st+p[5]*(d[c]-st);}break;}
            case DeEsserUnified:{
                const float d=bandLevel(0,in,ch);const auto dec=phrase.process(peak,d,v[1],6.0);
                const float threshold=v[5]>.5f?(dec.eligible?float(dec.thresholdDb):(dec.ready?0.f:v[1])):v[1];
                if((tick&31)==1)deess.set(v[0],threshold,v[2],v[3],v[4]>.5f,false,false);
                const auto result=deess.process(x,y);out[0]=result[0];out[1]=result[1];gr=deess.reductionDb();break;}
            case EchoUnified:{
                const int mode=int(v[5]);const bool ping=mode==1||mode==4;constexpr float beats[]={.25f,.5f,.75f,1.f,1.5f,2.f};const int division=std::clamp(int(std::lround(p[0]*5)),0,5);const float time=mode>=3?float(60.*rate/tempo)*beats[division]:(mode==2?std::min(v[0],120.f):v[0])*float(rate*.001);if(curDelay<0)curDelay=time;curDelay+=std::clamp(time-curDelay,-.25f,.25f);
                const float echo0=delay[0].read(curDelay-1.f),echo1=delay[1].read(curDelay-1.f);lp[0]=smooth(lp[0],echo0,a,a);lp[1]=smooth(lp[1],echo1,a,a);
                const float fb=mode==2?0.f:v[1];delay[0].write(sane((ping&&ch==2?.5f*(x+y):x)+fb*(ping&&ch==2?lp[1]:lp[0])));delay[1].write(sane((ping&&ch==2?0.f:y)+fb*(ping&&ch==2?lp[0]:lp[1])));
                energy[0]=smooth(energy[0],peak*peak,bp,cp);const float duck=1.f/(1.f+v[4]*30.f*std::sqrt(energy[0]));for(int c=0;c<ch;++c)out[c]=in[c]+lp[c]*v[3]*duck;break;}
            case RoomUnified:{
                const float mode=p[5]*2.f;delay[0].write(x);delay[1].write(y);const float pd=v[1]*float(rate*.001);const float dl=pd<1?x:delay[0].read(pd),dr=pd<1?y:delay[1].read(pd);
                std::array<float,2> result{};for(int j=0;j<3;++j){const auto f=rooms[j].processFrame(dl,dr);const float weight=std::max(0.f,1.f-std::abs(float(j)-mode));for(int c=0;c<2;++c)result[c]+=weight*f[c];}
                energy[0]=smooth(energy[0],peak*peak,a,bp);const float duck=1.f/(1.f+v[4]*30.f*std::sqrt(energy[0]));for(int c=0;c<ch;++c)out[c]=in[c]*(1-v[3])+result[c]*v[3]*duck*1.7f;break;}
            case DriveUnified:{
                const int mode=int(v[3]);for(int c=0;c<ch;++c){const float input=in[c],pre=db2g(v[0]),z=input*pre;float color=0;
                if(mode==0){const float h=std::tanh(z+.2f)-std::tanh(.2f);color=detect[0].process(c,h/pre);}
                else if(mode==1)color=z/(1.f+std::abs(z))/pre;
                else{const float h=detect[1].process(c,z);color=input+(std::tanh(h)-h)/pre;}
                out[c]=input+v[2]*(filters[0].process(c,color)-input);}break;}
            case ClipUnified:{
                const int mode=int(v[3]);const bool hard=(mode&1)!=0,oversample=mode>=2;const float ceiling=db2g(v[1]),pre=db2g(v[0]);
                for(int c=0;c<ch;++c){const auto f=clip.process(c,in[c],pre/ceiling,oversample?1.f:0.f,1.f,0,0,hard);const float u=f.dry*pre/ceiling;
                    const float processed=oversample?std::clamp(u+f.color,-1.f,1.f):(hard?std::clamp(u,-1.f,1.f):std::tanh(u));out[c]=f.dry+v[2]*(processed*ceiling-f.dry);if(c==0)x=f.dry;else y=f.dry;}break;}
            case StereoUnified:{
                if(ch==2){const float mid=.5f*(x+y),side=filters[0].process(0,.5f*(x-y))*v[0];out[0]=(mid+side)*db2g(v[2])*std::min(1.f,1.f-v[3]);out[1]=(mid-side)*db2g(v[2])*std::min(1.f,1.f+v[3]);}else out[0]=x*db2g(v[2]);
                const float right=ch==2?out[1]:out[0];energy[0]=smooth(energy[0],out[0]*out[0],a,a);energy[1]=smooth(energy[1],right*right,a,a);energy[2]=smooth(energy[2],out[0]*right,a,a);gr=std::clamp(energy[2]/std::sqrt(std::max(1e-15f,energy[0]*energy[1])),-1.f,1.f);break;}
            default:break;
            }
            l[i]=wet==0?x:sane(x+wet*(out[0]-x));if(r)r[i]=wet==0?y:sane(y+wet*(out[1]-y));lastMeter=wet*gr;
        }
        meterDb.store(lastMeter,std::memory_order_relaxed);
    }
private:
    static float smooth(float old,float target,float rise,float fall){const float pole=target>old?rise:fall;return target+pole*(old-target);}
    void bandpass(Biquad& f,float hz,float q){const double w=2*kPi*Biquad::clampF(rate,hz)/rate,alpha=std::sin(w)/(2*q);f.set(alpha,0,-alpha,1+alpha,-2*std::cos(w),1-alpha);}
    float bandLevel(int j,const float* in,int ch){const float x=std::abs(detect[j].process(0,in[0]));return ch==2?std::max(x,std::abs(detect[j].process(1,in[1]))):x;}
    void update(){for(int j=0;j<info(type).numKnobs;++j)v[j]=realOf(type,j,p);a=onePole(rate,5);bp=onePole(rate,80);cp=onePole(rate,100);dp=onePole(rate,10);
        switch(type){
        case VoxRider:cp=onePole(rate,v[2]);a=onePole(rate,30);bp=onePole(rate,160);break;
        case DynamicEQ:bandpass(detect[0],v[0],v[1]);a=onePole(rate,v[4]);bp=onePole(rate,v[5]);cp=a;dp=a;break;
        case Multiband:{constexpr float hz[]={120,350,1000,3000,7500};for(int j=0;j<5;++j)filters[j].lowPass(rate,hz[j]);a=onePole(rate,v[3]);bp=onePole(rate,v[4]);cp=bp;dp=a;break;}
        case Resonance:for(int j=0;j<8;++j){freq[j]=float(std::clamp(v[2]*std::pow(2.f,(j-3.f)*.5f),100.f,float(rate*.42)));bandpass(detect[j],freq[j],v[3]);}cp=onePole(rate,v[4]);break;
        case Exciter:detect[0].highPass(rate,v[0]);filters[0].lowPass(rate,v[3]);break;
        case AirUnified:air.set(v[0],v[1],v[2],p[3]);filters[0].highShelf(rate,4000,v[0]*8);filters[1].highShelf(rate,v[4],v[1]*12);break;
        case DeEsserUnified:detect[0].highPass(rate,v[0]);break;
        case EchoUnified:a=float(std::exp(-2*kPi*Biquad::clampF(rate,v[2])/rate));bp=onePole(rate,5);cp=onePole(rate,180);break;
        case RoomUnified:for(int j=0;j<3;++j)rooms[j].setParameters(v[0],std::clamp(.9f*(1.f-std::log(v[2]/2000.f)/std::log(8.f)),0.f,.95f),1.f);a=onePole(rate,10);bp=onePole(rate,200);break;
        case DriveUnified:detect[0].highPass(rate,20);detect[1].highPass(rate,3000);filters[0].lowPass(rate,v[1]);break;
        case StereoUnified:filters[0].highPass(rate,v[1]);a=onePole(rate,100);break;
        default:break;}
    }
    static constexpr double roomSize[3]={1.,1.65,.55};skellcore::FDNReverb rooms[3];AlignedTape4x clip;
    int type;double rate=48000,tempo=120;float p[6]{},v[6]{},freq[8]{},a=0,bp=0,cp=0,dp=0,phase=0,wet=1,curDelay=-1;unsigned tick=0;bool seed=false,dirty=true;
    std::array<float,8>energy{},gain{};std::array<float,2>lp{};Biquad filters[8],detect[8];DelayLine delay[2];
    skell::research::SkellAir air;skell::research::ClassicDeEsser deess;skellcore::deesser::PhraseAwareDetector phrase;std::atomic<float> meterDb{0};
};
}
