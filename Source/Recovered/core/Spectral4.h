#pragma once
// Adapted from skell-vst-decompiler experiments/engine_lab/Spectral4.h
// Original Skell research design, NOT a recovered proprietary DSP implementation.
// Real-time safe process: no heap allocations, locks, or file I/O.
#include <algorithm>
#include <array>
#include <cmath>
namespace skell_lab {
constexpr double pi=3.1415926535897932384626433832795;
inline double finite(double x) noexcept {return std::isfinite(x)?x:0.;}
inline double bound(double x,double lo,double hi,double fallback) noexcept {return std::clamp(std::isfinite(x)?x:fallback,lo,hi);}
inline double pole(double ms,double fs) noexcept {return std::exp(-1./(std::max(.01,ms)*.001*fs));}
inline double fromDb(double db) noexcept {return std::pow(10.,db/20.);}
struct Frame {double l,r;};
// Same DF2T Butterworth implementation as IndependentEngines.h.
struct Biquad {
    double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
    void setButterworth(double fs,double frequency,bool highpass) noexcept {
        const double g=std::tan(pi*std::min(frequency,.45*fs)/fs);
        const double d=1./(1.+std::sqrt(2.)*g+g*g),n=highpass?1.:g*g;
        b0=n*d;b1=(highpass?-2.:2.)*b0;b2=b0;a1=2.*(g*g-1.)*d;a2=(1.-std::sqrt(2.)*g+g*g)*d;
    }
    void reset() noexcept {z1=z2=0.;}
    double process(double input) noexcept {const double y=b0*input+z1;z1=b1*input-a1*y+z2;z2=b2*input-a2*y;if(std::abs(z1)<1e-30)z1=0.;if(std::abs(z2)<1e-30)z2=0.;return y;}
};
class Spectral4 {
    Biquad lpf[2][3][2]{};
    std::array<double,4> env{},gain{},base{},depthWeight{1,1,1,1};
    double wet=1,threshold=.2,depth=0,outputTrim=1,rise=0,fall=0,gainPole=0;
public:
    void prepare(double fsInput,std::array<double,3> crossovers={230,2400,7500},std::array<double,4> bandDb={0,0,0,0},double thresholdDb=-18,double depthInput=0,double wetInput=1,double outputDb=0) noexcept {
        const double fs=bound(fsInput,8000,384000,48000);double prev=35;
        for(int k=0;k<3;++k){const double lo=prev+30,hi=fs*.45-30*(2-k);const double f=std::clamp(std::isfinite(crossovers[k])?crossovers[k]:prev+500,lo,hi);prev=f;for(int c=0;c<2;++c)for(int j=0;j<2;++j)lpf[c][k][j].setButterworth(fs,f,false);}
        for(int k=0;k<4;++k)base[k]=fromDb(bound(bandDb[k],-18,18,0));
        threshold=fromDb(bound(thresholdDb,-72,0,-18));depth=bound(depthInput,0,3,0);wet=bound(wetInput,0,1,1);outputTrim=fromDb(bound(outputDb,-18,6,0));rise=pole(2,fs);fall=pole(85,fs);gainPole=pole(3,fs);reset();
    }
    void reset() noexcept {for(auto& c:lpf)for(auto& split:c)for(auto& f:split)f.reset();env.fill(0.);gain=base;}
    // Rack adapter: update depth without resetting filters or envelopes.
    void setDepth(double d) noexcept {depth=bound(d,0,3,0);}
    void setDepthWeights(std::array<double,4> w) noexcept {for(int k=0;k<4;++k)depthWeight[k]=bound(w[k],0,1,1);}
    Frame process(Frame x) noexcept {
        const double input[2]={finite(x.l),finite(x.r)};double bands[2][4];
        for(int c=0;c<2;++c){double remainder=input[c];for(int k=0;k<3;++k){const double low=lpf[c][k][1].process(lpf[c][k][0].process(remainder));bands[c][k]=low;remainder-=low;}bands[c][3]=remainder;}
        double result[2]={0,0};for(int k=0;k<4;++k){const double peak=std::max(std::abs(bands[0][k]),std::abs(bands[1][k]));const double p=peak>env[k]?rise:fall;env[k]=p*env[k]+(1-p)*peak;const double excess=std::max(0.,env[k]/threshold-1.);const double target=base[k]/(1.+depth*depthWeight[k]*excess);gain[k]=gainPole*gain[k]+(1.-gainPole)*target;for(int c=0;c<2;++c)result[c]+=bands[c][k]*gain[k];}
        return {(input[0]*(1.-wet)+result[0]*wet)*outputTrim,(input[1]*(1.-wet)+result[1]*wet)*outputTrim};
    }
};
}
