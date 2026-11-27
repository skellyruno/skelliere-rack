#pragma once
#include "Dsp.h"
namespace skl {
// Experimental, not bound to the existing Exciter or Doubler identity.
// ADAA residual: integrate tanh(z)-z analytically to suppress nonlinear aliases.
class ExciterCandidate {
public:
    void prepare(double sr) noexcept {rate=std::clamp(sr,8000.,384000.);reset();set(3500,6,.2f,12000);}
    void reset() noexcept {hp.reset();lp.reset();previous.fill(0);}
    void set(float freq,float drive,float amount,float tone) noexcept {
        hp.highPass(rate,freq);lp.lowPass(rate,tone);pre=db2g(std::clamp(drive,0.f,18.f));mix=std::clamp(amount,0.f,1.f);
    }
    float process(int ch,float x) noexcept {
        const double z=std::clamp(double(hp.process(ch,sane(x)))*pre,-64.,64.),p=previous[ch];previous[ch]=z;
        const double d=z-p,mid=(z+p)*.5;
        const double r=std::abs(d)<1e-5?std::tanh(mid)-mid:(primitive(z)-primitive(p))/d;
        return sane(x+mix*lp.process(ch,float(r/pre)));
    }
private:
    static double primitive(double x) noexcept {const double a=std::abs(x);return a+std::log1p(std::exp(-2*a))-std::log(2.)-.5*x*x;}
    double rate=48000;float pre=1,mix=.2f;Biquad hp,lp;std::array<double,2>previous{};
};
// Two independent constant-ratio wet voices. Dual delay heads crossfade over
// one complete sweep, preserving an uninterrupted, undelayed dry path.
class PitchDoublerCandidate {
public:
    void prepare(double sr){rate=std::clamp(sr,8000.,384000.);for(auto& d:lines)d.prepare(int(rate*.14)+8);reset();set(18,8,.8f,.25f);}
    void reset() noexcept {for(auto& d:lines)d.clear();phase={0.,.25};}
    void set(float delayMs,float cents,float width,float mix) noexcept {
        base=std::clamp(delayMs,8.f,45.f)*rate*.001;span=std::min(rate*.032,base*.85);spread=std::clamp(width,0.f,1.f);wet=std::clamp(mix,0.f,1.f);
        const double c=std::clamp(cents,0.f,20.f);increment[0]=(1-std::pow(2.,c/1200.))/span;increment[1]=(1-std::pow(2.,-c/1200.))/span;
    }
    std::array<float,2> process(float x,float y,int channels=2) noexcept {
        const float mono=channels==1?x:.5f*(x+y);float voice[2]{};
        for(int j=0;j<2;++j){auto& line=lines[j];line.write(mono);const double p=phase[j],q=p<.5?p+.5:p-.5;
            const float a=float(.5-.5*std::cos(2*kPi*p));const float b=1-a;
            voice[j]=a*line.read(float(base+span*p))+b*line.read(float(base+span*q));phase[j]+=increment[j];phase[j]-=std::floor(phase[j]);}
        const float mid=.5f*(voice[0]+voice[1]),side=.5f*(voice[0]-voice[1])*spread;
        return {sane(x*(1-wet)+wet*(mid+(channels==1?0:side))),channels==1?0.f:sane(y*(1-wet)+wet*(mid-side))};
    }
private:
    double rate=48000,base=864,span=768;float spread=.8f,wet=.25f;std::array<double,2>phase{},increment{};DelayLine lines[2];
};
}
