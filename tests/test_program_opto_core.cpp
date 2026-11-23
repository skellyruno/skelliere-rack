#include "../Source/ProgramOptoCore.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

static constexpr double pi=3.14159265358979323846;
static void check(bool ok,const char* what) {
    if(!ok) { std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1); }
}
static double db(double a){return 20.0*std::log10(std::max(a,1.e-15));}
static double steadyGain(float inAmp,float sidechainHz,float thresholdDb) {
    skl::ProgramOptoCore c;c.prepare(48000.0);
    c.configure(thresholdDb,4.0f,5.0f,150.0f,sidechainHz,0.0f);
    double inputE=0,outputE=0;
    for(int i=0;i<48000;++i){
        float x=inAmp*std::sin(2*pi*70*i/48000.0),l,r;
        c.processPair(x,x,true,l,r);
        if(i>=24000){inputE+=x*x;outputE+=l*l;}
        check(std::isfinite(l)&&std::isfinite(r),"finite output during compression");
        check(std::fabs(l-r)<1.e-7,"linked channels produce same gain");
    }
    return db(std::sqrt(outputE/inputE));
}
static double releaseAfter(float priorAmp) {
    skl::ProgramOptoCore c;c.prepare(48000.0);
    c.configure(-36.0f,5.0f,5.0f,120.0f,50.0f,0.0f);
    for(int i=0;i<48000;++i) {float x=priorAmp*std::sin(2*pi*1000*i/48000.0),l,r;c.processPair(x,x,true,l,r);}
    double before=c.gainReductionDb();
    for(int i=0;i<4800;++i) {float l,r;c.processPair(0,0,true,l,r);}
    check(before > 0.5,"prior compression achieved gain reduction");
    return c.gainReductionDb();
}
int main(){
    skl::ProgramOptoCore c;c.prepare(48000.0);
    check(c.sampleRate()==48000.0,"prepare sample rate");
    c.configure(-24,4,5,150,80,0);
    for(int i=0;i<48000;++i) {float l,r;c.processPair(0,0,true,l,r);check(l==0&&r==0,"zero silence");}
    check(c.gainReductionDb()==0,"zero reduction on silence");
    const double lowCut=steadyGain(0.4f,20,-30), highCut=steadyGain(0.4f,350,-30);
    std::printf("70 Hz sidechain test: HP20 %.2f dB, HP350 %.2f dB\n",lowCut,highCut);
    check(highCut > lowCut+7.0,"sidechain HP should reduce low-frequency pumping");
    check(lowCut < -6.0,"compression should actually reduce loud input");
    const auto afterDeep=releaseAfter(1.0f),afterShallow=releaseAfter(0.08f);
    std::printf("reduction remaining after 100 ms: deep %.2f dB, shallow %.2f dB\n",afterDeep,afterShallow);
    check(afterDeep>afterShallow+1.0,"program-dependent release memory affects recovery");
    c.reset(); float l=0,r=0;
    c.processPair(std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),true,l,r);
    check(std::isfinite(l)&&std::isfinite(r),"nonfinite input sanitization");
    for(double rate:{44100.,48000.,96000.,192000.}) {
        c.prepare(rate);c.configure(-30,4,1,100,100,0);
        for(int i=0;i<4096;++i) {float x=std::sin(2*pi*1000*i/rate),a,b;c.processPair(x,-x,true,a,b);
            check(std::isfinite(a)&&std::isfinite(b),"rate-stable processing");
            check(std::fabs(a+b)<1.e-6,"stereo polarity maintained");
        }
    }
    std::puts("Original ProgramOptoCore DSP checks passed");
}
