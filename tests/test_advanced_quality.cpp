#include "../Source/AdvancedMultiband.h"
#include "../Source/AdvancedColorSpace.h"
#include "../Source/AdvancedControlSpec.h"
#include "../Source/ResonanceCandidate.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace skl;
static void check(bool ok,const char* msg){if(!ok){std::fprintf(stderr,"FAIL: %s\n",msg);std::exit(1);}}
static double db(double x){return 20*std::log10(std::max(1e-15,x));}
static double magnitude(double a,double b,int n){return 2*std::hypot(a,b)/n;}
int main(){
    for(int i=0;i<kAdvancedCount;++i){check(advancedReal(i,NAN)==advancedSpec(i).def,"NaN parameter fallback");check(std::isfinite(advancedReal(i,INFINITY)),"infinite parameter fallback");}
    for(double sr:{44100.,48000.,96000.,192000.}){
        // Deliberately between the nominal 2.5/2.973 kHz detector centers.
        ResonanceCandidate res;res.prepare(sr);res.set(1,10,2500,6,100,1);
        constexpr double f=2710,lo=800,hi=4000;double a[3]{},b[3]{};const double fs[]={lo,f,hi};const double amps[]={.04,.2,.04};
        const int n=int(sr);for(int i=0;i<2*n;++i){float x=0;for(int j=0;j<3;++j)x+=float(amps[j]*std::sin(2*kPi*fs[j]*i/sr));const auto y=res.process(x,-x);check(std::isfinite(y[0])&&std::abs(y[0]+y[1])<1e-6,"resonance stereo/finiteness");if(i>=n)for(int j=0;j<3;++j){a[j]+=y[0]*std::cos(2*kPi*fs[j]*i/sr);b[j]+=y[0]*std::sin(2*kPi*fs[j]*i/sr);}}
        const double center=db(magnitude(a[1],b[1],n)/amps[1]),low=db(magnitude(a[0],b[0],n)/amps[0]),high=db(magnitude(a[2],b[2],n)/amps[2]);
        check(center<-5&&std::abs(low)<.8&&std::abs(high)<.8,"off-grid cut/selectivity");
        // Preserve stereo information that previously cancelled at the shared mono input.
        PitchDoublerCandidate d;d.prepare(sr);d.set(18,8,1,1);double in=0,out=0;for(int i=0;i<2*n;++i){const float x=.35f*std::sin(2*kPi*440*i/sr);const auto y=d.process(x,-x);check(std::isfinite(y[0])&&std::isfinite(y[1])&&std::abs(y[0])<=.351f&&std::abs(y[1])<=.351f,"doubler headroom");if(i>=n){in+=2*x*x;out+=y[0]*y[0]+y[1]*y[1];}}check(out/in>.1,"antiphase wet signal collapsed");
        // An undelayed dry sample must remain exact with the advanced effects neutral.
        d.reset();d.set(18,8,1,0);ExciterCandidate e;e.prepare(sr);e.set(3500,18,0,12000);AdvancedMultiband m;m.prepare(sr);auto s=AdvancedMultiband::Settings{};for(auto& x:s.bands)x.ratio=1;m.set(s);res.reset();res.set(0,10,2500,6,100,1);
        for(int i=0;i<2048;++i){const float x=i==0?.7f:0;check(d.process(x,-x)[0]==x,"neutral doubler impulse changed");check(e.process(0,x)==x,"neutral exciter impulse changed");check(m.process(x,-x)[0]==x,"neutral multiband impulse changed");check(res.process(x,-x)[0]==x,"neutral resonance impulse changed");}
        std::printf("%.0f Hz: off-grid %.2f dB, neighbours %.2f/%.2f dB, antiphase wet %.2f dB, neutral dry exact\n",sr,center,low,high,10*std::log10(out/in));
    }
    std::puts("Advanced quality regressions passed");
}
