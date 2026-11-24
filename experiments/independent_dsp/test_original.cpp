#include "OriginalDynamics.h"
#include <iostream>
#include <limits>
#include <random>
using namespace skell_lab;
void check(bool v){if(!v)throw std::runtime_error("DSP regression");}
int main(){
 std::mt19937 rng(42);std::uniform_real_distribution<double> u(-2,2);
 for(double fs:{44100.,48000.,96000.,192000.}){
   LookaheadLimiter l(fs);LookaheadLimiter margin(fs,-1.25);StaticBell fixed(fs);BaselineLimiter b(fs);DynamicBell d(fs);DynamicBell unity(fs,250,1.6,-24,0);
   const auto n=l.latency();for(std::size_t i=0;i<n;++i)check(l.process({})==Frame{});
   for(int i=0;i<24000;++i){Frame f{u(rng),u(rng)};auto y=l.process(f);auto z=b.process(f);auto q=d.process(f);auto m=margin.process(f);auto v=fixed.process(f);check(peak(m)<=db(-1.25)+1e-12);check(std::isfinite(v[0])&&std::isfinite(v[1]));check(peak(y)<=db(-1)+1e-12&&peak(z)<=db(-1)+1e-12);check(std::isfinite(q[0])&&std::isfinite(q[1]));check(d.reductionDb()>=-5.0000001&&d.reductionDb()<=1e-12);check(unity.process(f)==f);}
   LookaheadLimiter impulse(fs);auto p=impulse.process({.3,-.3});check(p==Frame{});for(std::size_t i=1;i<n;++i)check(impulse.process({})==Frame{});auto out=impulse.process({});check(std::abs(out[0]-.3)<1e-10&&out[1]==-out[0]);
   auto bad=d.process({std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()});check(std::isfinite(bad[0])&&std::isfinite(bad[1]));
   DynamicBell sine(fs);double en=0,wet=0;for(int i=0;i<int(fs*2);++i){double x=.6*std::sin(2*3.141592653589793*250*i/fs);auto v=sine.process({x,x});check(v[0]==v[1]);if(i>=fs){en+=x*x;wet+=v[0]*v[0];}}check(wet/en<.5&&wet/en>.2);
 }
 LookaheadLimiter zero(48000,-1,0);check(std::abs(zero.process({.4,.2})[0]-.4)<1e-12);
 bool bad=false;try{LookaheadLimiter invalid(0);}catch(const std::invalid_argument&){bad=true;}check(bad);
 std::cout<<"Original prototype regression passed: 4 sample rates, sample ceiling, latency, stereo linkage, nonfinite input, bell range and unity\n";
}
