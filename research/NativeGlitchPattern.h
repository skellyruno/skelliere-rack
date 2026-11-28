#pragma once
// Owner-source-only prototype, using exact SkellGlitch kernels. Settings are
// copied by the caller on the audio thread; no mutable UI-shared arrays.
#include <dsp/TempoClock.h>
#include <dsp/SliceCapture.h>
#include <dsp/Reverser.h>
#include <dsp/GateSequencer.h>
#include <dsp/GateShaper.h>
#include <dsp/TriggerRouter.h>
#include <array>
#include <cmath>
namespace skl::native_glitch {
using namespace skell::dsp;
struct Settings {
    int mode=2, division=2, style=0, count=16, gateRate=2, seed=1, source=TriggerRouter::Auto;
    float depth=.8f, smooth=.2f, pitch=0, chaos=0, swing=0, mix=.65f, wetTrimDb=-1.3f;
    bool seedLock=true;
    std::array<int,32> states=[] {std::array<int,32> a{};for(int i=0;i<32;++i)a[i]=i%4==0?2:i%2?0:1;return a;}();
    std::array<float,32> probability=[] {std::array<float,32> a{};a.fill(100.f);return a;}();
};
inline float bound(float x,float lo,float hi,float fallback=0.f) noexcept {return std::clamp(std::isfinite(x)?x:fallback,lo,hi);}
class PatternEngine {
public:
    void prepare(double sampleRate,int channels){rate=std::isfinite(sampleRate)?std::clamp(sampleRate,8000.,192000.):48000.;capture.prepare(std::clamp(channels,1,2),int(rate*8)+16);for(auto& s:shapers)s.prepare(rate);reset();}
    void reset() noexcept {clock.prepare(rate);capture.clear();triggers.prepare(rate);for(auto& s:shapers)s.reset();lastSlice=INT64_MIN;last={};held={};event=GlitchRandomizer::Normal;seeded=false;mix=0;eventMix=0;wasHost=false;}
    void trigger(float velocity=1.f,int note=60) noexcept {triggers.trigger(velocity,note);}
    void process(float* l,float* r,int n,const Settings& p,double bpm=120.,double ppq=0.,int num=4,int den=4,bool host=false,const float* side=nullptr,bool active=true) noexcept {
        if(!l||n<=0||r==l)return;
        bpm=std::isfinite(bpm)?std::clamp(bpm,30.,300.):120.;host=host&&std::isfinite(ppq)&&std::abs(ppq)<1.e10;
        if(host&&(!wasHost||std::abs(ppq-expected)>2.*bpm/(60.*rate))){capture.clear();clock.resetTracking();lastSlice=INT64_MIN;last={};held={};event=GlitchRandomizer::Normal;}
        wasHost=host;expected=ppq+n*bpm/(60.*rate);
        const int count=std::clamp(p.count,1,32),mode=std::clamp(p.mode,0,2),style=std::clamp(p.style,0,3),source=std::clamp(p.source,0,3);
        clock.setHost(bpm,num,den,ppq,host,true,std::clamp(p.division,0,5),500);
        const double slice=clock.getSamplesPerSlice();
        for(int j=0;j<count;++j)seq.setStep(j,p.states[j],bound(p.probability[j],0,100,100));
        for(auto& s:shapers)s.setTimes(3,15);
        constexpr double steps[]={1.,.5,.25,.125,1./3.,1./6.};const double cycleQ=steps[std::clamp(p.gateRate,0,5)]*count;
        const auto pitch=Reverser::makePitchContext(bound(p.pitch,-12,12),int(rate));
        const float closed=bound(p.depth,0,1)>=.999f?0.f:std::pow(10.f,-4.8f*bound(p.depth,0,1));
        const float trim=std::pow(10.f,bound(p.wetTrimDb,-24,0,-1.3f)/20.f),target=active?bound(p.mix,0,1):0.f,fade=float(1/(.005*rate));
        const double transition=std::min(double(bound(p.smooth,0,1)*.025f*rate),slice*.5);
        if(!seeded){mix=target;eventMix=source==TriggerRouter::Auto?1.f:0.f;seeded=true;}
        int prior=-1;int64_t priorCycle=INT64_MIN;GateSequencer::Decision decision{};
        for(int i=0;i<n;++i){const auto pos=clock.tick(i);if(pos.boundary){const bool jump=lastSlice!=INT64_MIN&&pos.sliceIndex!=lastSlice+1;held=jump?std::array<float,2>{}:last;capture.onBoundary();if(jump)capture.clear();lastSlice=pos.sliceIndex;event=random.choose(pos.sliceIndex,p.seed,bound(p.chaos,0,100),p.seedLock,int64_t(std::floor(pos.beat/std::max(1.,clock.getBarLength()))));}
            if(source==TriggerRouter::Sidechain&&side)triggers.processSidechain(side[i]);const bool on=source==TriggerRouter::Auto||triggers.active();
            const double cycle=pos.beat/cycleQ;const int64_t ci=int64_t(std::floor(cycle));const int step=seq.stepAt(cycle-std::floor(cycle),count,bound(p.swing,0,.75f));if(step!=prior||ci!=priorCycle){decision=seq.evaluate(step,ci,p.seed,closed);prior=step;priorCycle=ci;}
            const float gate=on?(source==TriggerRouter::MIDI?closed+(decision.gain-closed)*triggers.velocity():decision.gain):1.f;
            const float x=std::isfinite(l[i])?l[i]:0.f,y=r?(std::isfinite(r[i])?r[i]:0.f):x;const float dry[]={x,y};
            const bool reverse=on&&(mode!=1||decision.reverse||event==GlitchRandomizer::Flip||event==GlitchRandomizer::HalfTime||event==GlitchRandomizer::Stutter);
            const double phase=event==GlitchRandomizer::HalfTime?pos.phase*.5:pos.phase;
            mix+=std::clamp(target-mix,-fade,fade);eventMix+=std::clamp((on?1.f:0.f)-eventMix,-fade,fade);const auto gains=Crossfader::unityGains(mix*eventMix);
            for(int c=0;c<(r?2:1);++c){const float shaped=shapers[c].next(gate,1);float wet=(mode==1||mode==2)?dry[c]*shaped:dry[c];if(reverse&&capture.length()>0){wet=reverser.samplePrepared(capture,c,phase,event==GlitchRandomizer::Stutter?3:style,pos.sliceIndex,pitch);if(event==GlitchRandomizer::Flip)wet=capture.readFinished(c,phase*std::max(0,capture.length()-1));if(mode!=0)wet*=shaped;if(transition>0&&pos.phase*slice<transition)wet=Crossfader::spliceHold(held[c],wet,float(pos.phase*slice/transition));}if(on&&event==GlitchRandomizer::Skip)wet=0;if(on)last[c]=wet;else wet=last[c];
                // The trim is explicit, never a limiter. Trigger expiry fades the
                // last wet sample into dry instead of an abrupt bypass.
                const float out=(gains.wet==0)?dry[c]:dry[c]*gains.dry+wet*trim*gains.wet;if(c==0)l[i]=out;else r[i]=out;}
            capture.write(x,y);triggers.tick();}
        clock.advance(n);
    }
private:
    double rate=48000,expected=0;bool wasHost=false,seeded=false;float mix=0,eventMix=0;int64_t lastSlice=INT64_MIN;std::array<float,2> last{},held{};
    TempoClock clock;SliceCapture capture;Reverser reverser;GateSequencer seq;std::array<GateShaper,2> shapers;TriggerRouter triggers;GlitchRandomizer random;GlitchRandomizer::Event event=GlitchRandomizer::Normal;
};
}
