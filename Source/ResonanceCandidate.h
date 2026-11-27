#pragma once
#include "Dsp.h"
namespace skl {
// Experimental narrow dynamic-notch bank. Local prominence is measured
// against distinct neighbours, and only local maxima are eligible for cuts.
class ResonanceCandidate {
public:
    static constexpr int bands=13;
    void prepare(double sr) noexcept {rate=std::clamp(sr,8000.,384000.);reset();set(.35f,6,2500,4,100,1);}
    void reset() noexcept {for(auto& f:detectors)f.reset();for(auto& f:cuts)f.reset();energy.fill(0);gain.fill(0);wide=0;tick=0;}
    void set(float a,float r,float focus,float q,float releaseMs,float m) noexcept {
        amount=std::clamp(a,0.f,1.f);range=std::clamp(r,0.f,12.f);mix=std::clamp(m,0.f,1.f);sharp=std::clamp(q,1.f,8.f);
        const double center=std::clamp(double(focus),100.,rate*.42/std::pow(2.,1.5));
        for(int j=0;j<bands;++j){freq[j]=float(center*std::pow(2.,(j-6)*.25));const double w=2*kPi*freq[j]/rate,alpha=std::sin(w)/(2*std::max(6.f,sharp));detectors[j].set(alpha,0,-alpha,1+alpha,-2*std::cos(w),1-alpha);}
        attack=onePole(rate,12);release=onePole(rate,releaseMs);detectAttack=onePole(rate,8);detectRelease=onePole(rate,65);
    }
    std::array<float,2> process(float x,float y,int channels=2) noexcept {
        const int ch=channels==1?1:2;const float in[2]={sane(x),sane(y)};float out[2]={in[0],in[1]};
        const float p=std::max(in[0]*in[0],ch==2?in[1]*in[1]:0.f);wide=smooth(wide,p,detectAttack,detectRelease);
        for(int j=0;j<bands;++j){const float a=detectors[j].process(0,in[0]),b=ch==2?detectors[j].process(1,in[1]):0;energy[j]=smooth(energy[j],std::max(a*a,b*b),detectAttack,detectRelease);}
        for(int j=0;j<bands;++j){const float e=energy[j];const bool local=(j==0||e>=energy[j-1])&&(j==bands-1||e>=energy[j+1]);float neighbours=0;int n=0;if(j>=2){neighbours+=energy[j-2];++n;}if(j+2<bands){neighbours+=energy[j+2];++n;}neighbours/=float(n);
            const float prominence=10*std::log10((e+1e-10f)/(neighbours+1e-10f));const float relative=e/std::max(wide,1e-10f);const float eligibility=std::clamp((relative-.012f)/.04f,0.f,1.f);
            const float want=local&&e>1e-7f?std::min(range,std::max(0.f,prominence-(4.f+(1-amount)*6.f))*amount*eligibility):0;gain[j]=smooth(gain[j],want,attack,release);}
        if((tick++&31u)==0){float total=0;for(float g:gain)total+=g;const float scale=total>range&&total>0?range/total:1.f;for(int j=0;j<bands;++j)cuts[j].peaking(rate,freq[j],std::max(3.f,sharp*1.5f),-gain[j]*scale);}
        for(int c=0;c<ch;++c){for(auto& f:cuts)out[c]=f.process(c,out[c]);out[c]=sane(in[c]+mix*(out[c]-in[c]));}return {out[0],ch==2?out[1]:0};
    }
private:
    static float smooth(float old,float target,float a,float b) noexcept {return target+(target>old?a:b)*(old-target);}
    double rate=48000;float amount=.35f,range=6,mix=1,sharp=4,wide=0,attack=0,release=0,detectAttack=0,detectRelease=0;unsigned tick=0;
    Biquad detectors[bands],cuts[bands];std::array<float,bands> freq{},energy{},gain{};
};
}
