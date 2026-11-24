#include "OriginalAdaptiveCompressor.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <random>
#include <chrono>
using namespace skell_lab;
static void check(bool ok){if(!ok)throw std::runtime_error("adaptive compressor regression");}
int main(){
  std::cout<<std::setprecision(10);
  for(double fs:{44100.,48000.,96000.,192000.}){
    AdaptiveCompressor a(fs),fixed(fs,-18,4,6,0),unity(fs,-18,1);
    check(AdaptiveCompressor::curve(-3.,6.,.75)==0.);
    check(std::abs(AdaptiveCompressor::curve(3.,6.,.75)+2.25)<1e-12);
    check(std::abs(AdaptiveCompressor::curve(0.,6.,.75)+.5625)<1e-12);
    double adaptiveEnergy=0.,fixedEnergy=0.;
    for(int n=0;n<int(fs*.012);++n){
      auto x=a.process({.8,.4}),y=fixed.process({.8,.4});
      adaptiveEnergy+=x[0]*x[0];fixedEnergy+=y[0]*y[0];
      check(std::abs(x[0]-2*x[1])<1e-12);
    }
    check(adaptiveEnergy>fixedEnergy);
    const double transientDelta=10.*std::log10(adaptiveEnergy/fixedEnergy);
    for(int n=0;n<int(fs*2);++n)a.process({.8,.4});
    double expected=-.75*(level(.8)+18.);
    double settled=a.reductionDb();
    check(std::abs(settled-expected)<.03);
    for(int n=0;n<int(fs*.1);++n)a.process({});
    double recovery100=a.reductionDb();
    check(recovery100>settled&&recovery100<0.);
    for(int n=0;n<int(fs*3);++n)a.process({});
    check(std::abs(a.reductionDb())<.01);
    a.reset();check(a.reductionDb()==0.);
    std::mt19937 rng(42);std::uniform_real_distribution<double> u(-2.,2.);
    for(int n=0;n<20000;++n){
      Frame x{u(rng),u(rng)};
      auto y=a.process(x);
      check(std::isfinite(y[0])&&std::isfinite(y[1])&&peak(y)<=peak(x)+1e-12);
      check(unity.process(x)==x);
    }
    auto bad=a.process({std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()});
    check(bad==Frame{});
    std::cout<<"SKELL_ADAPTIVE_DSP rate="<<fs<<" transient_energy_advantage_db="<<transientDelta
             <<" settled_reduction_db="<<settled<<" recovery_100ms_db="<<recovery100<<"\n";
  }
  bool rejected=false;try{AdaptiveCompressor invalid(48000,-18,0);}catch(const std::invalid_argument&){rejected=true;}
  check(rejected);
  AdaptiveCompressor bench(48000);double sink=0.;auto start=std::chrono::steady_clock::now();
  for(int n=0;n<480000;++n)sink+=bench.process({.5*std::sin(n*.01),.3*std::sin(n*.013)})[0];
  auto elapsed=std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count();
  std::cout<<"SKELL_ADAPTIVE_BENCH ns_per_frame="<<elapsed/480000<<" sink="<<sink<<"\n";
}
