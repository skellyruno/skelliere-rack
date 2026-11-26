#pragma once
#include <algorithm>
#include <cmath>
namespace skl::ui {
// Message-thread-only decorative motion. No DSP state or host parameters.
struct ArtworkMotion {
    static constexpr float cycle=6.28318530718f;
    float phase=0.f,left=0.f,right=0.f,level=0.f;
    bool advance(float seconds,bool visible,bool enabled,float input) noexcept {
        if(!visible||!enabled||!std::isfinite(seconds)||seconds<=0.f)return false;
        const float dt=std::min(seconds,.1f);
        phase=std::fmod(phase+dt*1.35f,cycle);
        left=std::fmod(left+dt*1.60f,cycle);
        right=std::fmod(right+dt*2.15f,cycle);
        const float target=std::isfinite(input)?std::clamp(input,0.f,1.f):0.f;
        level+=(target-level)*(1.f-std::exp(-dt*(target>level?9.f:3.f)));
        return true;
    }
};
}
