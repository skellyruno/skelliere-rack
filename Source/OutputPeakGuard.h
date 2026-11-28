#pragma once
#include <algorithm>
#include <cmath>
namespace skl {
// Explicitly opt-in, stereo-linked, zero-lookahead sample-peak protection.
// This is not a true-peak limiter. Old sessions retain their original output.
class OutputPeakGuard {
public:
    static constexpr float ceiling=0.891250938f; // -1 dBFS
    void prepare(double sr) noexcept {release=1-std::exp(-1/(.100*std::max(1000.,sr)));reset();}
    void reset() noexcept {gain=1;}
    void process(float& l,float& r,bool enabled) noexcept {
        if(!enabled&&gain==1.f)return;
        const float peak=std::max(std::abs(l),std::abs(r));
        const double target=enabled&&peak>ceiling?double(ceiling)/peak:1.;
        if(target<gain)gain=target;else {gain+=release*(target-gain);if(gain>1.f-1e-5f)gain=1;}
        l*=float(gain);r*=float(gain);
        if(enabled){l=std::clamp(l,-ceiling,ceiling);r=std::clamp(r,-ceiling,ceiling);}
    }
private:double gain=1,release=.0002083;
};
}
