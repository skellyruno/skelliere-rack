#pragma once
#include "../Source/AdvancedColorSpace.h"
namespace skl::research {
// Research-only: retain the phase match of the previous candidate while
// widening the usable passband and sharpening the decimator transition.
// Host-latency contract not yet wired; no factory sound change.
template<int Taps=129> class ExciterLongFir {
    static_assert((Taps-1)%8==0 && Taps>=49 && Taps<=257);
    static constexpr int ring=512, historySize=128;
public:
    static constexpr int latency=(Taps-1)/4;
    void prepare(double sr) noexcept {rate=sr;constexpr double fc=.119;double sum=0;for(int i=0;i<Taps;++i){const double t=i-(Taps-1)*.5,w=.42-.5*std::cos(2*kPi*i/(Taps-1))+.08*std::cos(4*kPi*i/(Taps-1));const double v=w*(t==0?2*fc:std::sin(2*kPi*fc*t)/(kPi*t));h[i]=float(v);sum+=v;}for(auto& v:h)v=float(v/sum);channels={};hp.reset();lp.reset();}
    void set(float frequency,float drive,float amount,float tone) noexcept {hp.highPass(rate,frequency);lp.lowPass(rate,tone);pre=db2g(drive);mix=amount;}
    float process(int c,float x) noexcept {
        auto& s=channels[c];x=sane(x);s.dry[s.w]=x;s.history[s.w]=hp.process(c,x);const float dry=s.dry[(s.w-latency)&(historySize-1)];float color=0;
        for(int j=0;j<4;++j){float v=0;for(int k=j,m=0;k<Taps;k+=4,++m)v+=h[k]*s.history[(s.w-m)&(historySize-1)];v*=4*pre;s.residual[s.r]=(std::tanh(v)-v)/pre;if(j==0)for(int k=0;k<Taps;++k)color+=h[k]*s.residual[(s.r-k)&(ring-1)];s.r=(s.r+1)&(ring-1);}
        s.w=(s.w+1)&(historySize-1);const float matched=.5f*(color+s.previous);s.previous=color;return sane(dry+mix*lp.process(c,matched));
    }
private:struct Channel{std::array<float,historySize> history{},dry{};std::array<float,ring> residual{};int w=0,r=0;float previous=0;};std::array<Channel,2>channels{};std::array<float,Taps> h{};double rate=48000;float pre=1,mix=1;Biquad hp,lp;
};
}
