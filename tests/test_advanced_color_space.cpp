#include "../Source/AdvancedColorSpace.h"
#include "../Source/RackModuleFactory.h"
#include <cstdio>
#include <cstdlib>
using namespace skl;
static void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"%s\n",why);std::exit(1);}}
static double bin(const std::vector<float>& x,float f){double a=0,b=0;for(int i=48000;i<96000;++i){a+=x[i]*std::cos(2*kPi*f*i/48000);b+=x[i]*std::sin(2*kPi*f*i/48000);}return 2*std::sqrt(a*a+b*b)/48000;}
int main(){constexpr float frequency=9000;std::vector<float> dry(96000),legacy(96000),candidate(96000);for(int i=0;i<96000;++i)dry[i]=.5f*std::sin(2*kPi*frequency*i/48000);legacy=dry;
 auto m=makeRackModule(Exciter);m->prepare(48000,257,1);float k[6]{};k[0]=toNorm(info(Exciter).knobs[0],3500);k[1]=toNorm(info(Exciter).knobs[1],18);k[2]=1;k[3]=toNorm(info(Exciter).knobs[3],18000);float* p[]={legacy.data()};for(int i=0;i<96000;i+=257){juce::AudioBuffer<float>b(p,1,i,std::min(257,96000-i));m->process(b,k,true);}
 ExciterCandidate e;e.prepare(48000);e.set(3500,18,1,18000);for(int i=0;i<96000;++i)candidate[i]=e.process(0,dry[i]);
 const double old=bin(legacy,21000),now=bin(candidate,21000);std::printf("Exciter alias 21 kHz: %.2f -> %.2f dBFS (%.2f dB change)\n",20*std::log10(old),20*std::log10(now),20*std::log10(now/old));check(now<old*.7,"Alias reduction insufficient");
 for(double sr:{8000.,44100.,48000.,96000.,192000.}){PitchDoublerCandidate d;d.prepare(sr);d.set(18,8,1,0);for(int i=0;i<10000;++i){float x=.4f*std::sin(i*.1f);auto y=d.process(x,-x);check(y[0]==x&&y[1]==-x,"Doubler dry exact");}d.reset();d.set(18,12,1,1);double stereo=0;for(int i=0;i<int(sr);++i){float x=.4f*std::sin(2*kPi*440*i/sr);auto y=d.process(x,x);check(std::isfinite(y[0])&&std::abs(y[0])<=.41&&std::abs(y[1])<=.41,"Doubler finite/headroom");stereo+=std::abs(y[0]-y[1]);}check(stereo>10,"Independent voices inactive");}
 PitchDoublerCandidate d;d.prepare(48000);d.set(18,12,1,1);std::vector<float> l(96000),r(96000);for(int i=0;i<96000;++i){const float x=.4f*std::sin(2*kPi*440*i/48000);auto y=d.process(x,x);l[i]=y[0];r[i]=y[1];}const float up=440*std::pow(2.f,12.f/1200),down=440*std::pow(2.f,-12.f/1200);std::printf("Doubler nominal pitch %.2f/%.2f Hz; shifted bins L %.4f R %.4f\n",up,down,bin(l,up),bin(r,down));
 std::puts("Candidate exciter alias and doubler dry/stereo/headroom tests passed");}
