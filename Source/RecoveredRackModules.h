#pragma once
#include "Dsp.h"
#include "Recovered/VoiceShiftCore.h"
#include "Recovered/core/VoiceMatch.h"
namespace skl {
// Fixed control cadence, not callback-dependent; old slot and knob IDs stay fixed.
class RecoveredDemon final : public ModuleBase {
public:
    void prepare(double sr,int,int) override {rate=std::clamp(sr,8000.,192000.);core=skelly::voiceshift::make();core->prepare(rate);reset();}
    void reset() override {if(core)core->reset();seed=false;tick=0;}
    int latency() const override {return core?core->latencySamples():0;}
    void process(juce::AudioBuffer<float>& b,const float* k,bool active) override {
        if(!core||b.getNumChannels()<1)return;
        float target[6];for(int j=0;j<6;++j)target[j]=std::isfinite(k[j])?std::clamp(k[j],0.f,1.f):defaultNorm(SkellDemon,j);
        if(!active)target[5]=0;
        if(!seed){std::copy(target,target+6,params);seed=true;}
        const float step=float(16./(.02*rate));auto* l=b.getWritePointer(0);auto* r=b.getNumChannels()>1?b.getWritePointer(1):nullptr;
        for(int i=0;i<b.getNumSamples();){if(tick==0){for(int j=0;j<6;++j)params[j]+=std::clamp(target[j]-params[j],-step,step);tick=16;}const int n=std::min(tick,b.getNumSamples()-i);core->process(l+i,r?r+i:nullptr,n,params);tick-=n;i+=n;}
    }
private:
    std::unique_ptr<skelly::voiceshift::VoiceShiftCore> core;double rate=48000;float params[6]{};bool seed=false;int tick=0;
};
class RecoveredSSL final : public ModuleBase {
public:
    void prepare(double sr,int,int) override {rate=std::clamp(sr,8000.,384000.);engine.prepare(rate);reset();}
    void reset() override {engine.reset();seed=false;tick=0;wet=1;gr.store(0,std::memory_order_relaxed);}
    float meter() const override{return gr.load(std::memory_order_relaxed);}
    void process(juce::AudioBuffer<float>& b,const float* k,bool active) override {
        if(b.getNumChannels()<1)return;
        float target[6];for(int j=0;j<6;++j)target[j]=std::isfinite(k[j])?std::clamp(k[j],0.f,1.f):defaultNorm(SkellSSL,j);
        if(!seed){std::copy(target,target+6,p);wet=active?1.f:0.f;seed=true;}
        const float step=float(32./(.02*rate)),fade=float(1./(.005*rate));auto* l=b.getWritePointer(0);auto* r=b.getNumChannels()>1?b.getWritePointer(1):nullptr;
        float reduction=0;
        for(int i=0;i<b.getNumSamples();){
            if(tick==0){for(int j=0;j<6;++j)p[j]+=std::clamp(target[j]-p[j],-step,step);settings.gLaw=p[0]>=.5f;settings.lfGain=realOf(SkellSSL,1,p);settings.lmfGain=realOf(SkellSSL,2,p);settings.hmfGain=realOf(SkellSSL,3,p);settings.hfGain=realOf(SkellSSL,4,p);settings.compThreshold=10.-30.*p[5];settings.compRatio=1.+5.*p[5];settings.compRelease=.4;settings.dynOn=p[5]>.0001f;tick=32;}
            const int n=std::min(tick,b.getNumSamples()-i);float a[32],z[32];for(int j=0;j<n;++j){a[j]=sane(l[i+j]);z[j]=r?sane(r[i+j]):a[j];}
            engine.process(a,z,n,settings);reduction=std::max(reduction,engine.compReductionDb());
            for(int j=0;j<n;++j){wet+=std::clamp((active?1.f:0.f)-wet,-fade,fade);const float x=sane(l[i+j]);l[i+j]=wet==0?x:sane(x+wet*(a[j]-x));if(r){const float y=sane(r[i+j]);r[i+j]=wet==0?y:sane(y+wet*(z[j]-y));}}
            tick-=n;i+=n;
        }
        gr.store(active?reduction:0.f,std::memory_order_relaxed);
    }
private:
    skelly::vmatch::echannel::Channel engine;skelly::vmatch::echannel::Settings settings;double rate=48000;float p[6]{},wet=1;int tick=0;bool seed=false;std::atomic<float> gr{0};
};
// Independent zero-lookahead four-band noise reducer. Not a reconstruction of NS1.
class RackNoiseReducer final : public ModuleBase {
public:
    void prepare(double sr,int,int) override {rate=std::clamp(sr,8000.,384000.);for(int j=0;j<3;++j)split[j].lowPass(rate,std::array<double,3>{250.,1800.,6500.}[j]);attack=onePole(rate,4);release=onePole(rate,120);reset();}
    void reset() override {for(auto& f:split)f.reset();env.fill(0);gain.fill(1);floor.fill(0);wet=1;seed=false;counter=0;last=0;gr.store(0,std::memory_order_relaxed);}
    float meter() const override{return gr.load(std::memory_order_relaxed);}
    void process(juce::AudioBuffer<float>& b,const float* k,bool active) override {
        if(b.getNumChannels()<1)return;
        const int ch=std::min(2,b.getNumChannels());float target[3];for(int j=0;j<3;++j)target[j]=std::isfinite(k[j])?std::clamp(k[j],0.f,1.f):defaultNorm(SkellNoise,j);
        if(!seed){std::copy(target,target+3,p);wet=active?1.f:0.f;seed=true;}
        const float step=float(1./(.025*rate)),fade=float(1./(.005*rate));auto* l=b.getWritePointer(0);auto* r=ch>1?b.getWritePointer(1):nullptr;
        for(int i=0;i<b.getNumSamples();++i){for(int j=0;j<3;++j)p[j]+=std::clamp(target[j]-p[j],-step,step);if((counter++&31)==0){threshold=db2g(realOf(SkellNoise,1,p));release=onePole(rate,realOf(SkellNoise,2,p));learnUp=onePole(rate,2500);learnDown=onePole(rate,150);}
            wet+=std::clamp((active?1.f:0.f)-wet,-fade,fade);float x[2]={sane(l[i]),r?sane(r[i]):sane(l[i])},bands[2][4]{};
            for(int c=0;c<ch;++c){float rem=x[c];for(int j=0;j<3;++j){bands[c][j]=split[j].process(c,rem);rem-=bands[c][j];}bands[c][3]=rem;}
            float y[2]={x[0],x[1]};float reduction=0;
            for(int j=0;j<4;++j){const float peak=std::max(std::abs(bands[0][j]),std::abs(bands[ch-1][j]));const float a=peak>env[j]?attack:release;env[j]=peak+a*(env[j]-peak);const float f=env[j]<floor[j]?learnDown:learnUp;floor[j]=env[j]+f*(floor[j]-env[j]);const float noise=std::clamp(floor[j],threshold*.25f,threshold);const float ratio=env[j]/(noise+1e-12f);const float confidence=std::clamp((ratio-1.f)/3.f,0.f,1.f);const float desired=1.f-.96f*p[0]*(1.f-confidence);const float g=desired>gain[j]?attack:release;gain[j]=desired+g*(gain[j]-desired);for(int c=0;c<ch;++c)y[c]+=(gain[j]-1.f)*bands[c][j];reduction=std::max(reduction,1.f-gain[j]);}
            l[i]=wet==0||p[0]==0?x[0]:sane(x[0]+wet*(y[0]-x[0]));if(r)r[i]=wet==0||p[0]==0?x[1]:sane(x[1]+wet*(y[1]-x[1]));last=std::max(last*.999f,reduction);
        }
        gr.store(active?-g2db(std::max(.04f,1.f-last)):0.f,std::memory_order_relaxed);
    }
private:
    Biquad split[3];std::array<float,4> env{},gain{},floor{};double rate=48000;float p[3]{},attack=0,release=0,learnUp=0,learnDown=0,threshold=.004f,wet=1,last=0;unsigned counter=0;bool seed=false;std::atomic<float> gr{0};
};
}
