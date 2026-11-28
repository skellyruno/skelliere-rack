#pragma once
#include "Dsp.h"
namespace skl {
// Experimental narrow dynamic-notch bank. Local prominence is measured
// against distinct neighbours, and only local maxima are eligible for cuts.
class ResonanceCandidate {
public:
    static constexpr int bands=13;
    void prepare(double sr) noexcept {rate=std::clamp(std::isfinite(sr)?sr:48000.,8000.,384000.);reset();set(.35f,6,2500,4,100,1);}
    void reset() noexcept {for(auto& f:detectors)f.reset();for(auto& f:cuts)f.reset();energy.fill(0);gain.fill(0);desired.fill(0);offset.fill(0);tracked.fill(0);cutActive.fill(false);wide=0;tick=0;}
    void set(float a,float r,float focus,float q,float releaseMs,float m) noexcept {
        amount=bounded(a,0.f,1.f,.35f);range=bounded(r,0.f,12.f,6);mix=bounded(m,0.f,1.f,1);sharp=bounded(q,1.f,8.f,4);
        const double center=std::clamp(double(std::isfinite(focus)?focus:2500),100.,rate*.42/std::pow(2.,1.5));
        for(int j=0;j<bands;++j){freq[j]=float(center*std::pow(2.,(j-6)*.25));const double w=2*kPi*freq[j]/rate,alpha=std::sin(w)/(2*std::max(6.f,sharp));detectors[j].set(alpha,0,-alpha,1+alpha,-2*std::cos(w),1-alpha);}
        attack=onePole(rate,12);release=onePole(rate,bounded(releaseMs,5,2000,100));detectAttack=onePole(rate,8);detectRelease=onePole(rate,65);trackSlew=std::pow(onePole(rate,30),32.f);
    }
    float gainReduction() const noexcept {float total=0;for(float g:gain)total+=g;return std::min(total,range);}
    std::array<float,2> process(float x,float y,int channels=2) noexcept {
        const int ch=channels==1?1:2;const float in[2]={sane(x),sane(y)};float out[2]={in[0],in[1]};
        const float p=std::max(in[0]*in[0],ch==2?in[1]*in[1]:0.f);wide=smooth(wide,p,detectAttack,detectRelease);
        for(int j=0;j<bands;++j){const float a=detectors[j].process(0,in[0]),b=ch==2?detectors[j].process(1,in[1]):0;energy[j]=smooth(energy[j],std::max(a*a,b*b),detectAttack,detectRelease);}
        if((tick&7u)==0)for(int j=0;j<bands;++j){const float e=energy[j];const bool local=(j==0||e>=energy[j-1])&&(j==bands-1||e>=energy[j+1]);float neighbours=0;int n=0;if(j>=2){neighbours+=energy[j-2];++n;}if(j+2<bands){neighbours+=energy[j+2];++n;}neighbours/=float(n);
            const float prominence=10*std::log10((e+1e-10f)/(neighbours+1e-10f));const float relative=e/std::max(wide,1e-10f);const float eligibility=std::clamp((relative-.012f)/.04f,0.f,1.f);
            const float strength=amount*(1+.3f*std::max(0.f,2*amount-1));desired[j]=local&&e>1e-7f?std::min(range,std::max(0.f,prominence-(4.f+(1-amount)*6.f))*strength*eligibility):0;
            // Refine an eligible local maximum between detector centers using log-power curvature.
            // At the edges or for flat/noisy peaks, keep the detector center.
            if(desired[j]>.15f){float delta=0;if(j>0&&j+1<bands){const float l=std::log(energy[j-1]+1e-10f),c=std::log(e+1e-10f),r=std::log(energy[j+1]+1e-10f),curvature=l-2*c+r;if(curvature<-.2f)delta=std::clamp(.5f*(l-r)/curvature,-.5f,.5f);}offset[j]=delta;}}
        for(int j=0;j<bands;++j)gain[j]=smooth(gain[j],desired[j],attack,release);
        if((tick++&31u)==0){float total=0;for(float g:gain)total+=g;const float scale=total>range&&total>0?range/total:1.f;for(int j=0;j<bands;++j){tracked[j]=offset[j]+trackSlew*(tracked[j]-offset[j]);const float cut=gain[j]*scale;if(cut<1e-5f){if(cutActive[j])cuts[j].reset();cutActive[j]=false;continue;}const float center=freq[j]*std::exp2(tracked[j]*.25f);cuts[j].peaking(rate,center,std::max(3.f,sharp*1.5f),-cut);cutActive[j]=true;}}
        for(int c=0;c<ch;++c){for(int j=0;j<bands;++j)if(cutActive[j])out[c]=cuts[j].process(c,out[c]);out[c]=sane(in[c]+mix*(out[c]-in[c]));}return {out[0],ch==2?out[1]:0};
    }
private:
    static float bounded(float v,float lo,float hi,float def) noexcept{return std::clamp(std::isfinite(v)?v:def,lo,hi);}
    static float smooth(float old,float target,float a,float b) noexcept {return target+(target>old?a:b)*(old-target);}
    double rate=48000;float amount=.35f,range=6,mix=1,sharp=4,wide=0,attack=0,release=0,detectAttack=0,detectRelease=0,trackSlew=0;unsigned tick=0;
    Biquad detectors[bands],cuts[bands];std::array<float,bands> freq{},energy{},gain{},desired{},offset{},tracked{};std::array<bool,bands> cutActive{};
};
}
