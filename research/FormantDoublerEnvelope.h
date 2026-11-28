#pragma once
#include "../Source/AdvancedColorSpace.h"
namespace skl::research {
// Research-only: low-order adaptive spectral-envelope matching of each
// shifted wet voice. Long averaging avoids chasing syllable transients;
// bounded peaking gains avoid amplifying narrow detector nulls.
class FormantDoublerEnvelope {
public:
    void prepare(double sr){rate=sr;voices.prepare(sr);set(35);for(int j=0;j<count;++j){const double f=350*std::pow(6000./350.,j/double(count-1)),w=2*kPi*f/rate,alpha=std::sin(w)/(2*1.5);for(auto& d:dryDetector)d[j].set(alpha,0,-alpha,1+alpha,-2*std::cos(w),1-alpha);for(auto& d:wetDetector)d[j].set(alpha,0,-alpha,1+alpha,-2*std::cos(w),1-alpha);freq[j]=float(f);}averaging=onePole(rate,300);step=0;dryPower={};wetPower={};gains={};}
    void set(float cents) noexcept {voices.setVoices({PitchDoublerCandidate::Voice{18,cents,-1,1},PitchDoublerCandidate::Voice{18,-cents,1,1}},1);}
    std::array<float,2> process(float l,float r) noexcept {
        const float in[]={sane(l),sane(r)};auto out=voices.process(in[0],in[1]);
        for(int c=0;c<2;++c){for(int j=0;j<count;++j){const float a=dryDetector[c][j].process(0,in[c]),b=wetDetector[c][j].process(0,out[c]);dryPower[c][j]=averaging*dryPower[c][j]+(1-averaging)*a*a;wetPower[c][j]=averaging*wetPower[c][j]+(1-averaging)*b*b;}}
        if((step++&255u)==0){for(int c=0;c<2;++c){float totalDry=0,totalWet=0;for(int j=0;j<count;++j){totalDry+=dryPower[c][j];totalWet+=wetPower[c][j];}for(int j=0;j<count;++j){const float x=dryPower[c][j],y=wetPower[c][j];const float correction=x>1e-7f&&y>1e-7f&&totalDry>1e-6f&&totalWet>1e-6f?std::clamp(5.f*std::log10((x/totalDry+1e-7f)/(y/totalWet+1e-7f)),-1.25f,1.25f):0.f;gains[c][j]+=.07f*(correction-gains[c][j]);eq[c][j].peaking(rate,freq[j],1.5,gains[c][j]);}}}
        for(int c=0;c<2;++c)for(auto& f:eq[c])out[c]=f.process(0,out[c]);return out;
    }
private:static constexpr int count=8;double rate=48000;float averaging=0;unsigned step=0;PitchDoublerCandidate voices;std::array<float,count>freq{};std::array<std::array<Biquad,count>,2>dryDetector{},wetDetector{},eq{};std::array<std::array<float,count>,2>dryPower{},wetPower{},gains{};
};
}
