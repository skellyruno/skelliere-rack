#pragma once
#include "../Source/AdvancedColorSpace.h"
namespace skl::research {
// Research: correct the expected spectral-envelope displacement caused by
// pitch shifting, using only the source envelope slope. Unlike wet/dry
// instantaneous matching this cannot chase timing differences between voices.
class FormantDoublerGradient {
public:
    void prepare(double sr){rate=sr;voices.prepare(sr);set(35);for(int j=0;j<count;++j){const double f=250*std::pow(8000./250.,j/double(count-1)),w=2*kPi*f/rate,alpha=std::sin(w)/(2*2.5);for(auto& d:detector)d[j].set(alpha,0,-alpha,1+alpha,-2*std::cos(w),1-alpha);freq[j]=float(f);}averaging=onePole(rate,200);step=0;power={};gains={};for(auto& e:eq)for(auto& f:e)f.reset();}
    void set(float value) noexcept {cents=std::clamp(value,-35.f,35.f);voices.setVoices({PitchDoublerCandidate::Voice{18,cents,-1,1},PitchDoublerCandidate::Voice{18,-cents,1,1}},1);}
    std::array<float,2> process(float l,float r) noexcept {
        const float in[]={sane(l),sane(r)};auto out=voices.process(in[0],in[1]);
        for(int c=0;c<2;++c)for(int j=0;j<count;++j){const float x=detector[c][j].process(0,in[c]);power[c][j]=averaging*power[c][j]+(1-averaging)*x*x;}
        if((step++&255u)==0){for(int c=0;c<2;++c){float maxPower=0;for(float p:power[c])maxPower=std::max(maxPower,p);const float offset=(c?-1:1)*cents*float(std::log(2.)/1200.)/float(std::log(8000./250.)/(count-1));for(int j=0;j<count;++j){const int lo=std::max(0,j-1),hi=std::min(count-1,j+1);const float floor=std::max(1e-9f,maxPower*1e-4f);const float slope=10*std::log10(std::max(power[c][hi],floor)/std::max(power[c][lo],floor))/float(hi-lo);const float want=maxPower>1e-7f?std::clamp(offset*slope,-1.5f,1.5f):0.f;gains[c][j]+=.10f*(want-gains[c][j]);eq[c][j].peaking(rate,freq[j],2.5,gains[c][j]);}}}
        for(int c=0;c<2;++c)for(auto& f:eq[c])out[c]=f.process(0,out[c]);return out;
    }
private:static constexpr int count=12;double rate=48000;float averaging=0,cents=35;unsigned step=0;PitchDoublerCandidate voices;std::array<float,count>freq{};std::array<std::array<Biquad,count>,2>detector{},eq{};std::array<std::array<float,count>,2>power{},gains{};
};
}
