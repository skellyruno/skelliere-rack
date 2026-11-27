#pragma once
#include <cmath>
namespace skl {
// Original Rack tape residual path; zero-amount fast path advances the exact same state.
class AlignedTape4x {
public:
    static constexpr int delay=12;
    void prepare() noexcept {
        constexpr double kPi=3.14159265358979323846;
        double sum=0;for(int i=0;i<49;++i){const double t=i-24,fc=.1125;const double w=.42-.5*std::cos(2*kPi*i/48)+.08*std::cos(4*kPi*i/48);const double v=w*(t==0?2*fc:std::sin(2*kPi*fc*t)/(kPi*t));h[i]=float(v);sum+=v;}
        for(auto& v:h)v=float(v/sum);
        reset();
    }
    void reset() noexcept {for(auto& c:channels)c={};}
    struct Result {float dry,color;};
    Result process(int c,float x,float input,float amount,float drive,float bias,float biasZero) noexcept {
        auto& s=channels[c];s.history[s.w]=x;const float dry=s.history[(s.w-delay)&15];float color=0;
        // r starts at zero and advances four entries per call, so this group
        // cannot wrap within the four stores. Preserve history for reactivation.
        if(!(amount>0)){
            for(int j=0;j<4;++j)s.residual[s.r+j]=0;
            s.r=(s.r+4)&63;s.w=(s.w+1)&15;return {dry,0};
        }
        for(int j=0;j<4;++j){float residual=0;if(amount>0){float v=0;for(int k=j,m=0;k<49;k+=4,++m)v+=h[k]*s.history[(s.w-m)&15];v*=4*input;residual=(std::tanh(drive*v+bias)-biasZero)/drive-v;}s.residual[s.r]=residual;
            if(j==0&&amount>0){for(int k=0;k<49;++k)color+=h[k]*s.residual[(s.r-k)&63];}
            s.r=(s.r+1)&63;
        }
        s.w=(s.w+1)&15;return {dry,color};
    }
private:
    struct Channel {float history[16]{},residual[64]{};int w=0,r=0;};Channel channels[2];float h[49]{};
};
}
