#include "../Source/ResonanceCandidate.h"
#include "../Source/RackModuleFactory.h"
#include <cstdio>
#include <cstdlib>
using namespace skl;
static void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"%s\n",why);std::exit(1);}}
static double bin(const std::vector<float>& x,float f){double a=0,b=0;for(int i=48000;i<96000;++i){a+=x[i]*std::cos(2*kPi*f*i/48000);b+=x[i]*std::sin(2*kPi*f*i/48000);}return 2*std::sqrt(a*a+b*b)/48000;}
int main(){std::vector<float> dry(96000),old(96000),now(96000);for(int i=0;i<96000;++i)dry[i]=.20f*std::sin(2*kPi*2500*i/48000)+.04f*std::sin(2*kPi*800*i/48000)+.04f*std::sin(2*kPi*4000*i/48000);old=dry;
 auto m=makeRackModule(Resonance);m->prepare(48000,257,1);float k[6]{};const float real[]={1,10,2500,6,100,1};for(int j=0;j<6;++j)k[j]=toNorm(info(Resonance).knobs[j],real[j]);float* p[]={old.data()};for(int i=0;i<96000;i+=257){juce::AudioBuffer<float>b(p,1,i,std::min(257,96000-i));m->process(b,k,true);}
 ResonanceCandidate c;c.prepare(48000);c.set(1,10,2500,6,100,1);for(int i=0;i<96000;++i)now[i]=c.process(dry[i],0,1)[0];
 for(float f:{800.f,2500.f,4000.f}){double a=20*std::log10(bin(old,f)/bin(dry,f)),b=20*std::log10(bin(now,f)/bin(dry,f));std::printf("Resonance %.0f Hz: legacy %.3f dB, candidate %.3f dB\n",f,a,b);if(f==2500)check(b< -5,"Insufficient center cut");else check(std::abs(b)<.7,"Unrelated tone cut");}
 for(double sr:{8000.,44100.,96000.,192000.}){c.prepare(sr);c.set(0,10,8000,8,100,1);for(int i=0;i<20000;++i){float x=.25f*std::sin(i*.3f);auto y=c.process(x,-x);check(std::abs(y[0]-x)<1e-6&&std::abs(y[1]+x)<1e-6,"Neutral/stereo");}}
 std::puts("Candidate resonance center, off-band and neutral tests passed");}
