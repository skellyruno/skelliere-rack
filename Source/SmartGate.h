#pragma once
#include "Dsp.h"
namespace skl {
// Bounded adaptive gate. Tracks quiet stationary audio, not a speech classifier.
// Threshold is minimum opening level; adaptive threshold is at most +10 dB.
class SmartGateModule final : public ModuleBase {
public:
    explicit SmartGateModule(int t=SmartGate):type(t){}
    void prepare(double sr,int,int) override {
        rate=std::clamp(std::isfinite(sr)?sr:48000.,8000.,384000.);
        hp.highPass(rate,85);lp.lowPass(rate,8000);
        rmsPole=onePole(rate,3);peakPole=onePole(rate,6);openPole=onePole(rate,.45f);
        frameLength=std::max(1,int(std::round(rate*.01)));reset();
    }
    void reset() override {
        hp.reset();lp.reset();energy=peak=frameEnergy=framePeak=0;floor=db2g(-78);gainDb=0;wet=1;
        counter=holdLeft=stable=0;previousFrame=0;seed=false;open=false;updateCounter=0;dirty=true;
        gr.store(0,std::memory_order_relaxed);
    }
    float meter()const override{return gr.load(std::memory_order_relaxed);}
    void process(juce::AudioBuffer<float>& b,const float* k,bool active) override {
        const int ch=std::min(2,b.getNumChannels()),n=b.getNumSamples();if(!ch||!n)return;
        const int count=info(type).numKnobs;float target[6]{};for(int j=0;j<count;++j)target[j]=std::isfinite(k[j])?std::clamp(k[j],0.f,1.f):defaultNorm(type,j);
        if(!seed){std::copy(target,target+count,p);update();gainDb=-range;wet=active?1.f:0.f;seed=true;}
        const float step=float(1/(.025*rate)),fade=float(1/(.005*rate));
        float* l=b.getWritePointer(0);float* r=ch==2?b.getWritePointer(1):nullptr;
        for(int i=0;i<n;++i){
            for(int j=0;j<count;++j){const float next=p[j]+std::clamp(target[j]-p[j],-step,step);dirty|=next!=p[j];p[j]=next;}
            if((updateCounter++&31)==0&&dirty){update();dirty=false;}
            const float x=sane(l[i]),y=r?sane(r[i]):x;
            const float a=lp.process(0,hp.process(0,std::clamp(x,-8.f,8.f)));
            const float d=r?lp.process(1,hp.process(1,std::clamp(y,-8.f,8.f))):a;
            const float square=std::max(a*a,d*d);
            energy=square+rmsPole*(energy-square);peak=std::max(std::max(std::abs(a),std::abs(d)),peak*peakPole);
            frameEnergy+=square;framePeak=std::max(framePeak,std::max(std::abs(a),std::abs(d)));
            if(++counter>=frameLength){
                const float level=std::sqrt(std::max(0.f,frameEnergy/float(counter)));counter=0;frameEnergy=0;
                const bool noiseLike=framePeak>2.6f*level;framePeak=0;
                const bool steady=previousFrame>0&&level>previousFrame*.72f&&level<previousFrame*1.39f;
                stable=steady?std::min(stable+1,1000):0;previousFrame=level;
                if(level>db2g(-85)&&level<base*2.f&&(!open||(stable>=24&&noiseLike))){
                    const float bounded=std::min(level,base*2.f);
                    floor+=(bounded-floor)*(bounded>floor?.045f:.20f);
                }
                floor=std::clamp(floor,db2g(-85),base*2.f);
                trigger=type==NoiseGateUnified&&p[5]<.5f?base:std::clamp(floor*3.16f,base,base*3.16f);
            }
            const float detected=std::max(std::sqrt(std::max(energy,0.f)),peak*.55f);
            if(detected>=trigger){open=true;holdLeft=holdSamples;}
            else if(open&&detected>=trigger*.70f)holdLeft=holdSamples;
            else if(holdLeft>0)--holdLeft;
            else open=false;
            // A short soft opening knee preserves low-level consonant onsets
            // without requiring lookahead. Fully closed noise still receives Range.
            const float near=std::clamp((detected/trigger-.45f)/.55f,0.f,1.f);
            const float desired=open?0.f:-range*(1.f-near)*(1.f-near),coef=desired>gainDb?openPole:closePole;
            gainDb=desired+coef*(gainDb-desired);
            wet+=std::clamp((active?1.f:0.f)-wet,-fade,fade);
            const float g=1.f+wet*(db2g(gainDb)-1.f);l[i]=g==1.f?x:sane(x*g);if(r)r[i]=g==1.f?y:sane(y*g);
        }
        gr.store(-g2db(std::max(1e-6f,1.f+wet*(db2g(gainDb)-1.f))),std::memory_order_relaxed);
    }
private:
    void update(){base=db2g(realOf(SmartGate,0,p));range=realOf(SmartGate,1,p);holdSamples=int(rate*.001*realOf(SmartGate,2,p));closePole=onePole(rate,realOf(SmartGate,3,p));if(type==NoiseGateUnified)openPole=onePole(rate,realOf(type,4,p));trigger=type==NoiseGateUnified&&p[5]<.5f?base:std::clamp(floor*3.16f,base,base*3.16f);}
    int type;Biquad hp,lp;double rate=48000;float p[6]{},energy=0,peak=0,frameEnergy=0,framePeak=0,floor=.000125f,gainDb=0,wet=1,previousFrame=0;
    float rmsPole=0,peakPole=0,openPole=0,closePole=0,base=.004f,trigger=.004f,range=42.f;
    int frameLength=480,counter=0,holdLeft=0,holdSamples=0,stable=0;unsigned updateCounter=0;bool seed=false,open=false,dirty=true;std::atomic<float> gr{0};
};
}
