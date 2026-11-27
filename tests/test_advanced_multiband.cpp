#include "../Source/AdvancedMultiband.h"
#include <cstdio>
#include <cstdlib>
using namespace skl;
static void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"%s\n",why);std::exit(1);}}
static double measure(AdvancedMultiband::Settings s,float f,double sr=48000){AdvancedMultiband m;m.prepare(sr);m.set(s);double out=0,in=0;for(int i=0;i<int(sr*2);++i){float x=.25f*std::sin(2*kPi*f*i/sr);auto y=m.process(x,-x);check(std::isfinite(y[0])&&std::abs(y[0]+y[1])<1e-6,"Stereo link/finite");if(i>=sr){out+=y[0]*y[0];in+=x*x;}}return 10*std::log10(out/in);}
int main(){AdvancedMultiband::Settings s;for(auto& b:s.bands)b.ratio=1;
 for(double sr:{8000.,44100.,48000.,96000.,192000.})for(float f:{55.f,500.f,2000.f})check(std::abs(measure(s,f,sr))<1e-6,"Neutral reconstruction");
 auto low=s;low.bands[0].ratio=10;low.bands[0].threshold=-50;low.bands[0].range=12;
 auto a=measure(low,55),b=measure(low,3000);std::printf("Low band: 55 Hz %.3f dB; 3 kHz %.3f dB\n",a,b);check(a< -7&&std::abs(b)<.1,"Low band selectivity");
 auto moved=low;moved.crossover[0]=300;const auto a1=measure(low,230),a2=measure(moved,230);std::printf("Moved crossover at 230 Hz: %.3f -> %.3f dB\n",a1,a2);check(a2<a1-2,"Movable crossover ineffective");
 auto floating=s;auto& f=floating.floating[0];f.enabled=true;f.frequency=2500;f.q=8;f.dynamics.threshold=-50;f.dynamics.ratio=10;f.dynamics.range=12;auto center=measure(floating,2500),off=measure(floating,800);std::printf("Floating: 2.5 kHz %.3f dB; 800 Hz %.3f dB\n",center,off);check(center< -10&&std::abs(off)<.3,"Floating band selectivity");
 AdvancedMultiband m;m.prepare(8000);s.crossover={99999,-10,0,NAN,1};m.set(s);auto hz=m.crossovers();for(int j=1;j<5;++j)check(hz[j]>=hz[j-1]*1.19f,"Unordered crossover");check(hz[4]<=3200,"Nyquist guard");
 for(int i=0;i<40000;++i){if(i%257==0){s.crossover[0]=i%514?200.f:2000.f;s.floating[0].frequency=i%514?1000.f:3000.f;m.set(s);}auto y=m.process(.2f*std::sin(i*.1f),0,1);check(std::isfinite(y[0])&&std::abs(y[0])<1,"Automation stability");}
 std::puts("Advanced multiband candidate: independent gain, crossovers, floating band, neutral, mono/stereo automation passed");}
