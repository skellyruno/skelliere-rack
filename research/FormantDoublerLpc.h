#pragma once
#include "../Source/AdvancedColorSpace.h"
namespace skl::research {
// Experimental adaptive source/filter separation. This path is not wired to
// published controls: prove formant retention on independent vocals first.
class LpcEnvelope {
public:
    static constexpr int order=16;
    void prepare(double sr) noexcept {rate=std::clamp(sr,8000.,192000.);memory=std::exp(-1./(.045*rate));slew=1-std::exp(-1./(.025*rate));interval=std::max(64,int(rate*.005));reset();}
    void reset() noexcept {history={};corr={};reflection={};target={};analysis={};synthesis={};counter=0;}
    float whiten(float x) noexcept {
        x=sane(x);const double w=1-memory;corr[0]=memory*corr[0]+w*x*x;
        for(int j=1;j<=order;++j)corr[j]=memory*corr[j]+w*x*history[j-1];
        for(int j=order-1;j>0;--j)history[j]=history[j-1];history[0]=x;
        if(++counter>=interval){counter=0;estimate();}
        double f=x;std::array<double,order> next{};next[0]=x;
        for(int j=0;j<order;++j){reflection[j]+=slew*(target[j]-reflection[j]);const double b=analysis[j];if(j+1<order)next[j+1]=b+reflection[j]*f;f+=reflection[j]*b;}
        analysis=next;return sane(float(f));
    }
    float restore(float excitation) noexcept {
        double f=sane(excitation);std::array<double,order> next{};
        for(int j=order-1;j>=0;--j){f-=reflection[j]*synthesis[j];if(j+1<order)next[j+1]=synthesis[j]+reflection[j]*f;}
        // Research safety: reject pathological synthesis rather than hiding it
        // behind a limiter. Count failures for the comparison harness.
        if(!std::isfinite(f)||std::abs(f)>16){++failures;synthesis={};return 0;}
        next[0]=f;synthesis=next;return float(f);
    }
    unsigned faultCount() const noexcept {return failures;}
private:
    void estimate() noexcept {
        if(corr[0]<1e-10){target={};return;}
        std::array<double,order+1>a{};a[0]=1;double e=corr[0]*1.002;
        for(int m=1;m<=order;++m){double v=corr[m];for(int j=1;j<m;++j)v+=a[j]*corr[m-j];const double k=std::clamp(-v/std::max(e,1e-12),-.94,.94);auto next=a;for(int j=1;j<m;++j)next[j]=a[j]+k*a[m-j];next[m]=k;a=next;target[m-1]=k;e*=1-k*k;}
    }
    double rate=48000,memory=.999,slew=.001;int interval=240,counter=0;unsigned failures=0;
    std::array<double,order>history{},reflection{},target{},analysis{},synthesis{};std::array<double,order+1>corr{};
};
class FormantDoublerLpc {
public:
    void prepare(double sr){doubler.prepare(sr);for(auto& e:envelope)e.prepare(sr);set(18,35,1);}
    void set(float delay,float cents,float amount) noexcept {wet=std::clamp(amount,0.f,1.f);doubler.setVoices({PitchDoublerCandidate::Voice{delay,cents,-1,1},PitchDoublerCandidate::Voice{delay,-cents,1,1}},1);}
    std::array<float,2> process(float l,float r) noexcept {l=sane(l);r=sane(r);auto shifted=doubler.process(envelope[0].whiten(l),envelope[1].whiten(r));const float a=envelope[0].restore(shifted[0]),b=envelope[1].restore(shifted[1]);return {l+wet*(a-l),r+wet*(b-r)};}
    unsigned faults() const noexcept {return envelope[0].faultCount()+envelope[1].faultCount();}
private:PitchDoublerCandidate doubler;std::array<LpcEnvelope,2> envelope;float wet=1;
};
}
