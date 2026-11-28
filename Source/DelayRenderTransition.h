#pragma once
#include <algorithm>
#include <array>

namespace skl::native_delay {
// Audio-thread-only rendering policy. A pending/failed request does not erase
// an already playing Classic model, and a stale advanced model cannot replace it.
constexpr int chooseRenderModel(int requested, bool ready, int actual, int previous) noexcept {
    if (requested == 0) return 0;
    if (previous == 0 && (!ready || actual != requested)) return 0;
    if (ready && actual > 0 && (previous > 0 || actual == requested)) return actual;
    return -1; // no valid wet engine yet: preserve dry input
}
class RenderTransition {
public:
    void reset() noexcept { mode_=-1; fade_=1.f; fromClassic_=false; last_={}; held_={}; }
    int begin(int requested, bool ready, int actual) noexcept {
        const int next=chooseRenderModel(requested,ready,actual,mode_);
        if(next!=mode_){
            fromClassic_=mode_==0&&next>0&&fade_>=1.f;
            held_=last_; fade_=mode_<0?1.f:0.f; mode_=next;
        }
        return mode_;
    }
    std::array<float,2> process(float classicL,float classicR,float targetL,float targetR,float step) noexcept {
        fade_=std::min(1.f,fade_+step);
        if(fade_>=1.f){last_={targetL,targetR};return last_;}
        const float l=fromClassic_?classicL:held_[0],r=fromClassic_?classicR:held_[1];
        last_={l+fade_*(targetL-l),r+fade_*(targetR-r)};
        return last_;
    }
private:
    int mode_=-1; float fade_=1.f; bool fromClassic_=false;
    std::array<float,2> last_{},held_{};
};
}
