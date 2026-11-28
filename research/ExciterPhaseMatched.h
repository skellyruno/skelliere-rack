#pragma once
#include "../Source/AdvancedColorSpace.h"
namespace skl::research {
// Research: emulate the fractional-sample phase and droop of first-order
// ADAA on the oversampled residual, keeping dry aligned to FIR latency.
class ExciterPhaseMatched {
public:
    void prepare(double sr){rate=sr;os.prepare();hp.reset();lp.reset();history={};positions={};prev={};}
    void set(float freq,float drive,float amount,float tone){hp.highPass(rate,freq);lp.lowPass(rate,tone);pre=db2g(drive);mix=amount;}
    float process(int c,float x){auto& w=positions[c];history[c][w]=x;const float dry=history[c][(w-AlignedTape4x::delay)&15];w=(w+1)&15;const float r=os.process(c,hp.process(c,x),pre,1,1,0,0).color/pre;const float matched=.5f*(prev[c]+r);prev[c]=r;return sane(dry+mix*lp.process(c,matched));}
private:double rate=48000;float pre=1,mix=1;Biquad hp,lp;AlignedTape4x os;std::array<std::array<float,16>,2> history{};std::array<int,2>positions{};std::array<float,2>prev{};
};
}
