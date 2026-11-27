#pragma once
#include "Dsp.h"
#include "AlignedTape4x.h"
namespace skl {
// Experimental, not bound to the existing Exciter or Doubler identity.
// ADAA residual: integrate tanh(z)-z analytically to suppress nonlinear aliases.
class ExciterCandidate {
public:
    void prepare(double sr) noexcept {rate=std::clamp(std::isfinite(sr)?sr:48000.,8000.,384000.);reset();set(3500,6,.2f,12000);}
    void reset() noexcept {hp.reset();lp.reset();previous.fill(0);}
    void set(float freq,float drive,float amount,float tone) noexcept {
        hp.highPass(rate,bounded(freq,1000,10000,3500));lp.lowPass(rate,bounded(tone,3000,18000,12000));pre=db2g(bounded(drive,0,18,6));mix=bounded(amount,0,1,.2f);
    }
    float process(int ch,float x) noexcept {
        const double z=std::clamp(double(hp.process(ch,sane(x)))*pre,-64.,64.),p=previous[ch];previous[ch]=z;
        const double d=z-p,mid=(z+p)*.5;
        const double r=std::abs(d)<1e-5?std::tanh(mid)-mid:(primitive(z)-primitive(p))/d;
        return sane(x+mix*lp.process(ch,float(r/pre)));
    }
private:
    static float bounded(float v,float lo,float hi,float def) noexcept{return std::clamp(std::isfinite(v)?v:def,lo,hi);}
    static double primitive(double x) noexcept {const double a=std::abs(x);return a+std::log1p(std::exp(-2*a))-std::log(2.)-.5*x*x;}
    double rate=48000;float pre=1,mix=.2f;Biquad hp,lp;std::array<double,2>previous{};
};
// Four-times FIR oversampled residual with explicitly aligned dry path.
// Not selectable in production until rack latency and bypass contracts are tested.
class ExciterOversampledCandidate {
public:
    static constexpr int latency=AlignedTape4x::delay;
    void prepare(double sr) noexcept {rate=std::clamp(std::isfinite(sr)?sr:48000.,8000.,384000.);oversample.prepare();reset();set(3500,6,.2f,12000);}
    void reset() noexcept {hp.reset();lp.reset();oversample.reset();history={};position={};}
    void set(float freq,float drive,float amount,float tone) noexcept {
        hp.highPass(rate,bounded(freq,1000,10000,3500));lp.lowPass(rate,bounded(tone,3000,18000,12000));pre=db2g(bounded(drive,0,18,6));mix=bounded(amount,0,1,.2f);
    }
    float process(int c,float x) noexcept {
        x=sane(x);auto& w=position[c];history[c][w]=x;const float dry=history[c][(w-latency)&15];w=(w+1)&15;
        const auto residual=oversample.process(c,hp.process(c,x),pre,1,1,0,0).color/pre;
        return sane(dry+mix*lp.process(c,residual));
    }
private:
    static float bounded(float v,float lo,float hi,float def) noexcept{return std::clamp(std::isfinite(v)?v:def,lo,hi);}
    double rate=48000;float pre=1,mix=.2f;Biquad hp,lp;AlignedTape4x oversample;
    std::array<std::array<float,16>,2> history{};std::array<int,2> position{};
};
// Two independent constant-ratio wet voices. Dual delay heads crossfade over
// one complete sweep, preserving an uninterrupted, undelayed dry path.
class PitchDoublerCandidate {
public:
    struct Voice { float delayMs=18,cents=8,pan=-1,gain=1; };
    void prepare(double sr){rate=std::clamp(std::isfinite(sr)?sr:48000.,8000.,384000.);line.prepare(int(rate*.14)+8);set(18,8,.8f,.25f);reset();}
    void reset() noexcept {line.clear();phase={0.,.25};seeded=false;}
    void set(float delayMs,float cents,float width,float mix) noexcept {
        const float c=bounded(cents,0,20,8),w=bounded(width,0,1,.8f);
        setVoices({Voice{delayMs,c,-w,1},Voice{delayMs,-c,w,1}},mix);
    }
    void setVoices(const std::array<Voice,2>& voices,float mix,float sweepMs=32) noexcept {
        targetWet=bounded(mix,0,1,.25f);
        const auto maxSpan=rate*.001*bounded(sweepMs,8,45,32);
        for(int j=0;j<2;++j){const auto& v=voices[j];auto& t=target[j];t.base=bounded(v.delayMs,8,65,18)*rate*.001;t.span=std::min(maxSpan,t.base*.85);t.slope=1-std::pow(2.,bounded(v.cents,-35,35,j? -8:8)/1200.);t.pan=bounded(v.pan,-1,1,j?.8f:-.8f);t.gain=bounded(v.gain,0,1,1);}
        slew=onePole(rate,20);
    }
    std::array<float,2> process(float x,float y,int channels=2) noexcept {
        x=sane(x);y=sane(y);const float mono=channels==1?x:.5f*(x+y);line.write(mono);
        if(!seeded){current=target;wet=targetWet;seeded=true;}
        wet=blend(wet,targetWet,slew);float l=0,r=0,lg=0,rg=0;
        for(int j=0;j<2;++j){auto& c=current[j];const auto& t=target[j];c.base=blend(c.base,t.base,slew);c.span=blend(c.span,t.span,slew);c.slope=blend(c.slope,t.slope,slew);c.pan=blend(c.pan,t.pan,slew);c.gain=blend(c.gain,t.gain,slew);
            const double p=phase[j],q=p<.5?p+.5:p-.5;const float a=float(.5-.5*std::cos(2*kPi*p));const float voice=(a*line.read(float(c.base+c.span*p))+(1-a)*line.read(float(c.base+c.span*q)))*float(c.gain);
            phase[j]+=c.slope/c.span;phase[j]-=std::floor(phase[j]);l+=voice*float((1-c.pan)*.5);r+=voice*float((1+c.pan)*.5);lg+=float(c.gain*(1-c.pan)*.5);rg+=float(c.gain*(1+c.pan)*.5);}
        const float normalizer=1/std::max(1.f,std::max(lg,rg));l*=normalizer;r*=normalizer;
        return {sane(x*(1-wet)+wet*(channels==1?.5f*(l+r):l)),channels==1?0.f:sane(y*(1-wet)+wet*r)};
    }
private:
    struct State {double base=864,span=768,slope=0,pan=0,gain=1;};
    static float bounded(float v,float lo,float hi,float def) noexcept{return std::clamp(std::isfinite(v)?v:def,lo,hi);}
    template<class T>static T blend(T old,T want,float p) noexcept{return std::abs(old-want)<T(1e-4)?want:want+T(p)*(old-want);}
    double rate=48000;float wet=.25f,targetWet=.25f,slew=0;bool seeded=false;std::array<double,2>phase{};std::array<State,2> target{},current{};DelayLine line;
};
}
