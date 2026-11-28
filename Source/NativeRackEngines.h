#pragma once
#include "Dsp.h"
#include "NativeControlSpec.h"
#if SKELLIERE_NATIVE_IMPORTS
#include <FlangDSP.h>
#include <NativeChorus/core/ChorusFx.h>
#include <NativeFlangus/SkellFlangusAdapter.h>
#include <ClassicChorusSettings.h>
#include <native/Delay.h>
#include <dsp/TempoClock.h>
#include <dsp/SliceCapture.h>
#include <dsp/Reverser.h>
#include <dsp/GateSequencer.h>
#include <dsp/GateShaper.h>
#include <dsp/Crossfader.h>
#include "NativeDelayModels.h"
#include "DelayRenderTransition.h"
#include "../research/NativeGlitchPattern.h"
namespace skl {
inline float importedControl(int t,int i,const float* k) noexcept {
    return std::isfinite(k[i])?std::clamp(k[i],0.f,1.f):defaultNorm(t,i);
}
inline auto flangChorusState(float depth,float rate,float delay,float width) noexcept {
    auto s=skellflang::classicchorus::migrate(depth,rate,delay,width);
    s[11]=1024; // native wet-only; the rack provides the only dry path
    return s;
}
inline auto flangusState(float depth,float rate,float delay,float width) noexcept {
    auto s=skell::flangusfx::defaults;
    s[1]=int(std::lround(depth*1024));s[2]=int(std::lround(rate*1024));
    s[3]=int(std::lround(delay*128));s[4]=int(std::lround(width*128));s[6]=0;s[7]=128;
    return s;
}
// Latest Flang resets only the selected engine. Archived checkpoints may use
// the original same-rate prepare; no replacement modulation algorithm.
template<class Bank> auto clearFlangMode(Bank& bank,int mode,double,int) -> decltype(bank.resetMode(mode),void()) {bank.resetMode(mode);}
template<class Bank> void clearFlangMode(Bank& bank,int,double rate,long) {bank.prepare(rate);}
class NativeFlangRack final:public ModuleBase {
public:
    void prepare(double sr,int,int) override {
        rate=sr;ready=std::isfinite(sr)&&sr>=32000&&sr<=192000&&std::abs(sr-std::round(sr))<1.e-6;
        if(!ready)return;
        chorus.setRate(int(sr));chorus.load(flangChorusState(.45f,.5f,.5f,.5f));chorus.reset();
        ready=flangus.prepare(int(sr),flangusState(.45f,.5f,.5f,.5f),true);
        bank.prepare(sr);smoother.prepare(sr);reset();
    }
    void reset() override {selected=-1;seeded=false;wet=0;transition=0;last={};held={};if(ready)smoother.prepare(rate);}
    void process(juce::AudioBuffer<float>& b,const float* k,bool active) override {
        if(!ready||!b.getNumChannels()||!b.getNumSamples())return;
        const int mode=int(std::lround(importedControl(FlangNative,0,k)*3));
        skellflang::Controls controls;controls.order=3;controls.depth=importedControl(FlangNative,1,k);controls.speed=importedControl(FlangNative,2,k);controls.delay=importedControl(FlangNative,3,k);controls.spread=importedControl(FlangNative,4,k);
        const float mix=importedControl(FlangNative,5,k),step=float(1./(.005*rate));
        if(!seeded){wet=active?mix:0;seeded=true;}
        if(selected!=mode){held=last;transition=selected<0?1.f:0.f;selected=mode;clock=0;
            if(mode==0){chorusState=flangChorusState(controls.depth,controls.speed,controls.delay,controls.spread);chorus.load(chorusState);chorus.reset();}
            else if(mode==2){flangus.setState(flangusState(controls.depth,controls.speed,controls.delay,controls.spread));flangus.reset();}
            else clearFlangMode(bank,mode==1?1:7,rate,0);
        }
        auto* l=b.getWritePointer(0);auto* r=b.getNumChannels()>1?b.getWritePointer(1):nullptr;
        for(int i=0;i<b.getNumSamples();++i){
            const float x=sane(l[i]),y=r?sane(r[i]):x;auto p=smoother.next(controls);std::array<float,2> v{x,y};
            if(mode==0){if((clock&15)==0){auto s=flangChorusState(p.depth,p.speed,p.delay,p.spread);for(int j=0;j<12;++j)if(s[j]!=chorusState[j])chorus.setParam(j,s[j]);chorusState=s;}float a[2]={x,y};chorus.render(a,a,1);v={a[0],a[1]};}
            else if(mode==2){if((clock&15)==0)flangus.setState(flangusState(p.depth,p.speed,p.delay,p.spread));float a=x,c=y;flangus.process(&a,&c,1);v={a,c};}
            else v=bank.process(mode==1?1:7,x,y,p);
            ++clock;transition=std::min(1.f,transition+step);wet+=std::clamp((active?mix:0.f)-wet,-step,step);
            for(int c=0;c<2;++c){v[c]=sane(v[c]);last[c]=held[c]+transition*(v[c]-held[c]);}
            l[i]=wet==0?x:sane(x+wet*(last[0]-x));if(r)r[i]=wet==0?y:sane(y+wet*(last[1]-y));
        }
    }
private:
    double rate=48000;bool ready=false,seeded=false;int selected=-1;unsigned clock=0;float wet=0,transition=1;
    std::array<float,2> last{},held{};skellflang::classicchorus::State chorusState{};
    skelly::fruitychorusfx::Engine chorus;skell::private_research::FlangusAdapter flangus;skellflang::EngineBank bank;skellflang::ControlSmoother smoother;
};
// Original Classic engine and its raw controls. Timing changes retain the
// upstream non-live behaviour (clear on time change); no hidden substitution.
inline SKELLdelay::Delay2State skellayRackState(const float* p) noexcept {
    SKELLdelay::Delay2State s{SKELLdelay::delay2Default};s.v[0]=0;s.v[1]=128;s.v[2]=0;
    s.v[3]=int(std::lround(p[1]*127));s.v[4]=std::clamp(int(std::lround(p[0]*767))+1,1,768);
    s.v[5]=int(std::lround((p[3]*2.f-1.f)*512));s.v[6]=int(std::lround(p[4]*2));s.v[7]=int(std::lround(p[2]*128));return s;
}
class NativeDelayRack final:public ModuleBase {
public:
    void setBudget(std::shared_ptr<native_delay::RackBudget> b){budget=std::move(b);}
    void prepare(double sr,int block,int) override {
        rate=sr;engine.reset();if(!std::isfinite(sr)||sr<32000||sr>192000||std::abs(sr-std::round(sr))>1.e-6)return;
        if(!budget)budget=std::make_shared<native_delay::RackBudget>();
        float p[6]{};for(int i=0;i<6;++i)p[i]=defaultNorm(DelayNative,i);
        state=skellayRackState(p);engine=std::make_unique<SKELLdelay::Delay2Engine>(state,rate,tempo,1,true);engine->reserve(30.);
        advanced.prepare(int(sr),block,budget);for(auto& x:scratch)x.resize(size_t(advanced.maxBlock()));reset();
    }
    void reset() override {advanced.invalidate();activateSlot();}
    // Leaving the slot already invalidates old advanced publications. Do not
    // invalidate the new request prepared before the audio thread sees entry.
    void activateSlot() noexcept {seeded=false;wet=0;renderTransition.reset();visibleModel.store(-1);if(engine)engine->restart(state,tempo,true);}
    void setTempo(double bpm) noexcept override {tempo=std::clamp(std::isfinite(bpm)?bpm:120.,30.,300.);serviceTempo.store(tempo,std::memory_order_relaxed);}
    void setExtended(const NativeValues& v) noexcept {ext=v;}
    // One message-thread producer. A budget rejection keeps the current model.
    bool service(const NativeValues& v,const float* knobs,bool selected){
        if(!engine)return false;
        float p[6]{};for(int i=0;i<6;++i)p[i]=importedControl(DelayNative,i,knobs);
        auto requested=native_delay::selection(selected?int(nativeReal(nDelayModel,v[nDelayModel])):0,int(nativeReal(nDelayBanks,v[nDelayBanks])),int(std::lround(p[4]*2)));
        if(requested.model==0){requested.banks=2;requested.route=0;}
        // Modern has native true stereo and ping-pong, but not a swapped-delay type.
        if(requested.model==1){requested.banks=2;requested.route=requested.route==2?2:0;}
        try{const bool ok=advanced.service(requested,skellayRackState(p).v,serviceTempo.load(std::memory_order_relaxed));status.store(ok?0:1);return ok;}catch(...){status.store(1);return false;}
    }
    int renderedModel()const noexcept{return visibleModel.load(std::memory_order_relaxed);}
    bool modelBlocked()const noexcept{return status.load(std::memory_order_relaxed)!=0;}
    void leaveSlot() noexcept {advanced.invalidate();draining=true;drainFrames=0;visibleModel.store(-1);}
    void drain(int n) noexcept {
        if(!draining||!engine)return;
        skellay::LiveControls c;c.model=skellay::Model::classic;std::copy(state.v.begin(),state.v.end(),c.values.begin());
        for(int i=0;i<n;i+=advanced.maxBlock()){const int count=std::min(advanced.maxBlock(),n-i);for(auto& x:scratch)std::fill_n(x.data(),count,0.f);advanced.process(scratch[0].data(),scratch[1].data(),count,tempo,c);}
        if(advanced.model()==0){drainFrames+=n;if(drainFrames>int(rate*.02)+2*advanced.maxBlock())draining=false;}else drainFrames=0;
    }
    void process(juce::AudioBuffer<float>& b,const float* k,bool active) override {
        if(!engine||!b.getNumChannels()||!b.getNumSamples())return;
        draining=false;
        float p[6]{};for(int j=0;j<6;++j)p[j]=importedControl(DelayNative,j,k);
        const auto next=skellayRackState(p);if(!seeded){engine->restart(next,tempo,true);state=next;previousTempo=tempo;wet=active?p[5]:0;seeded=true;}
        if(state.v!=next.v||previousTempo!=tempo){engine->set(next,tempo);state=next;previousTempo=tempo;}
        auto* l=b.getWritePointer(0);auto* r=b.getNumChannels()>1?b.getWritePointer(1):nullptr;const float step=float(1/(.005*rate));
        const int want=int(nativeReal(nDelayModel,ext[nDelayModel]));const float trim=std::pow(10.f,nativeReal(nDelayTrim,ext[nDelayTrim])/20.f);
        skellay::LiveControls controls;controls.model=skellay::Model::classic;std::copy(next.v.begin(),next.v.end(),controls.values.begin());
        for(int start=0;start<b.getNumSamples();start+=advanced.maxBlock()){
            const int n=std::min(advanced.maxBlock(),b.getNumSamples()-start);auto* a=scratch[0].data();auto* c=scratch[1].data();
            for(int i=0;i<n;++i){a[i]=sane(l[start+i]);c[i]=r?sane(r[start+i]):a[i];}
            // Consume pending/retired engines even when Classic is selected.
            const bool ok=advanced.process(a,c,n,tempo,controls);const int actual=advanced.model();
            const int mode=renderTransition.begin(want,ok,actual);
            visibleModel.store(mode,std::memory_order_relaxed);
            for(int i=0;i<n;++i){const float x=sane(l[start+i]),y=r?sane(r[start+i]):x;float oldL=x,oldR=y;engine->process(&oldL,&oldR,1);
                const bool swap=mode==1&&next.v[6]==1;
                const float vL=mode==0?oldL:mode>0?(swap?c[i]:a[i]):x,vR=mode==0?oldR:mode>0?(swap?a[i]:c[i]):y;
                const auto last=renderTransition.process(oldL,oldR,vL,vR,step);
                wet+=std::clamp((active?p[5]:0.f)-wet,-step,step);const float w=mode<0?0.f:wet;
                l[start+i]=w==0?x:sane(x+w*(last[0]*trim-x));if(r)r[start+i]=w==0?y:sane(y+w*(last[1]*trim-y));}
        }
    }
private:
    double rate=48000,tempo=120,previousTempo=120;float wet=0;bool seeded=false;SKELLdelay::Delay2State state{};std::unique_ptr<SKELLdelay::Delay2Engine> engine;
    NativeValues ext=nativeDefaults();std::shared_ptr<native_delay::RackBudget> budget;native_delay::PreparedSlot advanced;
    std::array<std::vector<float>,2> scratch;std::atomic<double> serviceTempo{120};std::atomic<int> visibleModel{-1},status{0};native_delay::RenderTransition renderTransition;bool draining=false;int drainFrames=0;
};
// Original gate/slice/reverse/trigger kernels from the tested candidate.
class NativeGlitchRack final:public ModuleBase {
public:
    void prepare(double sr,int,int channels) override {rate=std::clamp(std::isfinite(sr)?sr:48000.,8000.,192000.);pattern.prepare(rate,channels);reset();}
    void reset() override {pattern.reset();host=false;}
    void setTempo(double bpm) noexcept override {tempo=std::clamp(std::isfinite(bpm)?bpm:120.,30.,300.);}
    void setTransport(double bpm,double pos,int num,int den,bool valid) noexcept override {setTempo(bpm);ppq=pos;numerator=num;denominator=den;host=valid;}
    void setExtended(const NativeValues& v) noexcept {ext=v;}
    void trigger(float velocity=1.f,int note=60) noexcept {pattern.trigger(velocity,note);}
    void process(juce::AudioBuffer<float>& b,const float* k,bool active) override {
        if(!b.getNumChannels()||!b.getNumSamples())return;
        float p[6]{};for(int j=0;j<6;++j)p[j]=importedControl(GlitchNative,j,k);
        native_glitch::Settings v;v.mode=int(std::lround(p[0]*2));v.division=int(std::lround(p[1]*5));v.depth=p[2];v.smooth=p[3];v.pitch=(p[4]*2.f-1.f)*12.f;v.mix=p[5];
        auto real=[&](int i){return nativeReal(i,ext[i]);};
        v.style=int(real(nGlitchStyle));v.count=int(real(nGlitchCount));v.gateRate=int(real(nGlitchRate));v.chaos=100*real(nGlitchChaos);v.swing=real(nGlitchSwing);v.seed=int(real(nGlitchSeed));v.seedLock=real(nGlitchLock)>.5f;v.source=int(real(nGlitchSource))==2?skell::dsp::TriggerRouter::Manual:int(real(nGlitchSource));v.wetTrimDb=real(nGlitchTrim);
        for(int i=0;i<32;++i){v.states[i]=int(real(nGlitchState+i));v.probability[i]=100*real(nGlitchProbability+i);}
        pattern.process(b.getWritePointer(0),b.getNumChannels()>1?b.getWritePointer(1):nullptr,b.getNumSamples(),v,tempo,ppq,numerator,denominator,host,nullptr,active);
        if(host)ppq+=b.getNumSamples()*tempo/(60.*rate);
    }
private:
    native_glitch::PatternEngine pattern;NativeValues ext=nativeDefaults();double rate=48000,tempo=120,ppq=0;int numerator=4,denominator=4;bool host=false;
};
}
#endif
