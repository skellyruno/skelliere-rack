#pragma once
#include "Dsp.h"
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
        skellflang::Controls controls;controls.depth=importedControl(FlangNative,1,k);controls.speed=importedControl(FlangNative,2,k);controls.delay=importedControl(FlangNative,3,k);controls.spread=importedControl(FlangNative,4,k);
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
    void prepare(double sr,int,int) override {
        rate=sr;engine.reset();if(!std::isfinite(sr)||sr<32000||sr>192000||std::abs(sr-std::round(sr))>1.e-6)return;
        float p[6]{};for(int i=0;i<6;++i)p[i]=defaultNorm(DelayNative,i);
        state=skellayRackState(p);engine=std::make_unique<SKELLdelay::Delay2Engine>(state,rate,tempo,1,true);engine->reserve(30.);reset();
    }
    void reset() override {seeded=false;wet=0;if(engine)engine->restart(state,tempo,true);}
    void setTempo(double bpm) noexcept override {tempo=std::clamp(std::isfinite(bpm)?bpm:120.,30.,300.);}
    void process(juce::AudioBuffer<float>& b,const float* k,bool active) override {
        if(!engine||!b.getNumChannels()||!b.getNumSamples())return;
        float p[6]{};for(int j=0;j<6;++j)p[j]=importedControl(DelayNative,j,k);
        const auto next=skellayRackState(p);if(!seeded){engine->restart(next,tempo,true);state=next;previousTempo=tempo;wet=active?p[5]:0;seeded=true;}
        if(state.v!=next.v||previousTempo!=tempo){engine->set(next,tempo);state=next;previousTempo=tempo;}
        auto* l=b.getWritePointer(0);auto* r=b.getNumChannels()>1?b.getWritePointer(1):nullptr;const float step=float(1/(.005*rate));
        for(int i=0;i<b.getNumSamples();++i){const float x=sane(l[i]),y=r?sane(r[i]):x;float a=x,c=y;engine->process(&a,&c,1);wet+=std::clamp((active?p[5]:0.f)-wet,-step,step);l[i]=wet==0?x:sane(x+wet*(sane(a)-x));if(r)r[i]=wet==0?y:sane(y+wet*(sane(c)-y));}
    }
private:
    double rate=48000,tempo=120,previousTempo=120;float wet=0;bool seeded=false;SKELLdelay::Delay2State state{};std::unique_ptr<SKELLdelay::Delay2Engine> engine;
};
// Original Glitch kernels, with an AUTO/full-band/zero-PDC rack control layer.
// MIDI/sidechain, aligned PDC and full step editor are not exposed here.
class NativeGlitchRack final:public ModuleBase {
public:
    void prepare(double sr,int,int channels) override {rate=std::clamp(std::isfinite(sr)?sr:48000.,8000.,192000.);capture.prepare(std::clamp(channels,1,2),int(rate*8)+16);clock.prepare(rate);for(auto& s:shapers)s.prepare(rate);reset();}
    void reset() override {capture.clear();clock.prepare(rate);for(auto& s:shapers)s.reset();lastSlice=INT64_MIN;lastWet={};heldWet={};seeded=false;wetMix=0;hostValid=false;haveEnd=false;}
    void setTempo(double bpm) noexcept override {tempo=std::clamp(std::isfinite(bpm)?bpm:120.,30.,300.);}
    void setTransport(double bpm,double ppq,int num,int den,bool valid) noexcept override {
        setTempo(bpm);const bool next=valid&&std::isfinite(ppq)&&std::abs(ppq)<1.e10;
        if(next&&(!hostValid||(haveEnd&&std::abs(ppq-expectedEnd)>2.*tempo/(60.*rate)))){
            capture.clear();clock.resetTracking();lastSlice=INT64_MIN;heldWet={};lastWet={};
        }
        hostPpq=ppq;numerator=num;denominator=den;hostValid=next;
    }
    void process(juce::AudioBuffer<float>& b,const float* k,bool active) override {
        const int ch=std::min(2,b.getNumChannels()),n=b.getNumSamples();if(ch<1||n<=0)return;
        float p[6]{};for(int j=0;j<6;++j)p[j]=importedControl(GlitchNative,j,k);
        const int mode=int(std::lround(p[0]*2)),division=int(std::lround(p[1]*5));
        const float closed=p[2]>=.999f?0.f:std::pow(10.f,-96.f*p[2]/20.f);
        const float smooth=p[3],pitch=(p[4]*2.f-1.f)*12.f;const auto pc=skell::dsp::Reverser::makePitchContext(pitch,int(rate));
        clock.setHost(tempo,numerator,denominator,hostPpq,hostValid,true,division,500);
        for(int j=0;j<16;++j)sequencer.setStep(j,j%4==0?2:(j%2?0:1),100.f);
        for(auto& s:shapers)s.setTimes(3.f,15.f);
        if(!seeded){wetMix=active?p[5]:0;seeded=true;}
        auto* l=b.getWritePointer(0);auto* r=ch==2?b.getWritePointer(1):nullptr;const float fade=float(1/(.005*rate));
        for(int i=0;i<n;++i){
            auto pos=clock.tick(i);if(pos.boundary){const bool jump=lastSlice!=INT64_MIN&&pos.sliceIndex!=lastSlice+1;heldWet=jump?std::array<float,2>{}:lastWet;capture.onBoundary();if(jump)capture.clear();lastSlice=pos.sliceIndex;}
            const double cycle=pos.beat/4.;const auto step=sequencer.stepAt(cycle-std::floor(cycle),16,0.);const auto decision=sequencer.evaluate(step,int64_t(std::floor(cycle)),1,closed);
            float dry[2]={sane(l[i]),r?sane(r[i]):sane(l[i])};const bool needsReverse=mode!=1||decision.reverse;
            const double transition=std::min(double(smooth*.025f*rate),clock.getSamplesPerSlice()*.5);
            const double transitionPhase=transition>0?pos.phase*clock.getSamplesPerSlice()/transition:1.;
            wetMix+=std::clamp((active?p[5]:0.f)-wetMix,-fade,fade);auto mix=skell::dsp::Crossfader::unityGains(wetMix);
            for(int c=0;c<ch;++c){const float shaped=shapers[c].next(decision.gain,1);const float liveGate=(mode==1||mode==2)?dry[c]*shaped:dry[c];float wet=mode==1?liveGate:dry[c];
                if(needsReverse&&capture.length()>0){wet=reverser.samplePrepared(capture,c,pos.phase,0,pos.sliceIndex,pc);if(mode==2||mode==1)wet*=shaped;if(transitionPhase<1.)wet=skell::dsp::Crossfader::spliceHold(heldWet[c],wet,float(transitionPhase));}else if(mode!=1)wet=liveGate;
                lastWet[c]=wet;float out=wetMix==0?dry[c]:sane(dry[c]*mix.dry+wet*mix.wet);if(c==0)l[i]=out;else r[i]=out;}
            capture.write(dry[0],dry[1]);
        }clock.advance(n);expectedEnd=hostPpq+n*tempo/(60.*rate);haveEnd=hostValid;
    }
private:
    double rate=48000,tempo=120,hostPpq=0,expectedEnd=0;bool haveEnd=false;int numerator=4,denominator=4;bool hostValid=false,seeded=false;float wetMix=0;int64_t lastSlice=INT64_MIN;
    std::array<float,2> lastWet{},heldWet{};skell::dsp::TempoClock clock;skell::dsp::SliceCapture capture;skell::dsp::Reverser reverser;skell::dsp::GateSequencer sequencer;std::array<skell::dsp::GateShaper,2> shapers;
};
}
#endif
