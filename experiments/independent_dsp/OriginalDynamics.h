#pragma once
// Original offline research prototypes; no vendor constants/tables or JUCE dependency.
// Not installed in Skelliere Rack. All construction/allocations occur off the sample path.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace skell_lab {
using Frame=std::array<double,2>;
inline double db(double d) { return std::pow(10.0,d/20.0); }
inline double level(double v) {return 20.0*std::log10(std::max(v,1e-12));}
inline double clean(double x) {return std::isfinite(x)?std::clamp(x,-64.0,64.0):0.;}
inline double decay(double fs,double ms) {return std::exp(-1000.0/(fs*ms));}
inline double peak(Frame f){return std::max(std::abs(f[0]),std::abs(f[1]));}
inline void validRate(double fs){if(!std::isfinite(fs)||fs<8000||fs>384000)throw std::invalid_argument("sample rate");}
// Sample-peak limiter. The smoothed gain is an average of N+1 causal peak-window
// gain decisions, each of which includes the delayed output sample in its own
// lookahead interval. Therefore the average never exceeds its safe gain bound.
// A release-only follower is constrained by the immediate peak demand.
class LookaheadLimiter {
    struct Node {std::uint64_t sample; double p;};
    std::vector<Node> deque;
    std::vector<Frame> delayLine;
    std::vector<double> gains;
    std::size_t head=0,used=0,write=0,N;
    std::uint64_t index=0;
    double ceil,release,env=1,sum;
public:
    explicit LookaheadLimiter(double fs,double ceilingDb=-1.,double ms=3.0,double releaseMs=75.) {
        validRate(fs); if(!std::isfinite(ms)||ms<0||ms>20||!std::isfinite(ceilingDb)||ceilingDb< -24||ceilingDb>0||!std::isfinite(releaseMs)||releaseMs<1||releaseMs>2000)throw std::invalid_argument("limiter settings");
        N=static_cast<std::size_t>(std::llround(fs*ms*.001)); ceil=db(ceilingDb); release=decay(fs,releaseMs);
        deque.resize(N+2);delayLine.resize(N+1);gains.assign(N+1,1.0);sum=double(N+1);
    }
    std::size_t latency()const{return N;}
    Frame process(Frame x) {
        for(auto& s:x)s=clean(s);
        const double p=peak(x); const auto oldest=index>=N?index-N:0;
        while(used && deque[head].sample<oldest){head=(head+1)%deque.size();--used;}
        while(used && deque[(head+used-1)%deque.size()].p<=p)--used;
        deque[(head+used)%deque.size()]={index,p};++used;
        const double demand=std::min(1.,ceil/std::max(deque[head].p,1e-15));
        env=std::min(demand,1.+release*(env-1.));
        sum+=env-gains[write];gains[write]=env;
        delayLine[write]=x;const auto read=(write+1)%delayLine.size();
        Frame y=delayLine[read];
        if(index<N)y={};
        else {const double g=std::clamp(sum/double(N+1),0.,std::min(1.,ceil/std::max(peak(y),1e-15))); for(auto& s:y)s*=g;}
        write=read;++index;return y;
    }
};
// Straightforward instantaneous sample-peak baseline, not a claim about a vendor.
class BaselineLimiter {
    double ceil,rel,g=1;
public:
    BaselineLimiter(double fs,double c=-1.,double r=75.) {validRate(fs);ceil=db(c);rel=decay(fs,r);}
    Frame process(Frame x){for(auto& s:x)s=clean(s);const double t=std::min(1.,ceil/std::max(peak(x),1e-15));g=std::min(t,1.+rel*(g-1.));for(auto& s:x)s*=g;return x;}
};
// Fixed-frequency state-variable detector and equalizer. Adaptive range uses a
// separately chosen smoothstep law; original calibration, no vendor tables.
struct Svf {
    double g,k,a1,a2,a3,s1=0,s2=0;
    void configure(double G,double K) {g=G;k=K;a1=1/(1+g*(g+k));a2=g*a1;a3=g*a2;}
    std::array<double,2> process(double x){const double v3=x-s2,v1=a1*s1+a2*v3,v2=s2+a2*s1+a3*v3;s1=2*v1-s1;s2=2*v2-s2;return {v1,v2};}
};
class StaticBell {
    std::array<Svf,2> eq{}; double m1;
public:
    explicit StaticBell(double fs) {validRate(fs);const double A=std::pow(10.,-5./40.),k=1/1.6/A,g=std::tan(3.14159265358979323846*250./fs);m1=k*(A*A-1);for(auto& f:eq)f.configure(g,k);}
    Frame process(Frame x){for(int c=0;c<2;++c){x[c]=clean(x[c]);x[c]+=m1*eq[c].process(x[c])[0];}return x;}
};
class DynamicBell {
    std::array<Svf,2> det{},eq{};
    double baseG,k,threshold,range,fast,slow,gainSmooth,env=0,cut=0,m1=0;
    int countdown=0;
public:
    DynamicBell(double fs,double freq=250.,double q=1.6,double thresholdDb=-24.,double rangeDb=-5.) {
        validRate(fs); if(!std::isfinite(freq)||freq<20||freq>fs*.4||!std::isfinite(q)||q<.3||q>10||!std::isfinite(thresholdDb)||thresholdDb< -90||thresholdDb>0||!std::isfinite(rangeDb)||rangeDb< -12||rangeDb>0)throw std::invalid_argument("bell settings");
        baseG=std::tan(3.14159265358979323846*freq/fs);k=1/q;threshold=thresholdDb;range=rangeDb;fast=decay(fs,12);slow=decay(fs,135);gainSmooth=decay(fs,8);
        for(auto& s:det)s.configure(baseG,k);
        for(auto& s:eq)s.configure(baseG,k);
    }
    double reductionDb()const{return cut;}
    Frame process(Frame x) {
        double p=0;for(int c=0;c<2;++c){x[c]=clean(x[c]);p=std::max(p,std::abs(det[c].process(x[c])[0]*k));}
        const double power=p*p;const double a=power>env?fast:slow;env=a*env+(1-a)*power;
        const double t=std::clamp((level(std::sqrt(env))-threshold)/12.,0.,1.);const double goal=range*t*t*(3-2*t);
        cut=gainSmooth*cut+(1-gainSmooth)*goal;
        if(countdown==0){const double A=std::pow(10.,cut/40.),K=k/A;m1=K*(A*A-1);for(auto& s:eq)s.configure(baseG,K);countdown=16;}--countdown;
        for(int c=0;c<2;++c)x[c]+=m1*eq[c].process(x[c])[0];
        return x;
    }
};
}
