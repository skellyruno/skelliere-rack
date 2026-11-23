#include "../Source/RackModuleFactory.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <chrono>
using namespace skl;
static void check(bool ok,const char* message) { if(!ok) { std::fprintf(stderr,"%s\n",message); std::exit(1); } }
static std::vector<float> render(int type,double rate,int block,int channels) {
    auto m=makeRackModule(type); check(m!=nullptr,"Production factory omitted module");
    m->prepare(rate,512,channels);
    std::vector<float> a(16384),b(16384);
    for(int i=0;i<16384;++i) a[i]=b[i]=.1f*(float)(std::sin(2*kPi*100*i/rate)+std::sin(2*kPi*8000*i/rate));
    float* p[2]={a.data(),b.data()};
    for(int event=0,pos=0;pos<16384;) {
        event=pos/4096;
        float k[6]{};
        for(int j=0;j<info(type).numKnobs;++j) k[j]=event==0 ? defaultNorm(type,j) : event==1 ? 1.0f : event==2 ? 0.0f : std::numeric_limits<float>::quiet_NaN();
        const int n=std::min(block,4096-pos%4096);
        juce::AudioBuffer<float> view(p,channels,pos,n); m->process(view,k,event!=2); pos+=n;
    }
    for(int i=0;i<16384;++i) {
        check(std::isfinite(a[i]),"Nonfinite extension left output");
        if(channels==2) {
            check(std::isfinite(b[i]),"Nonfinite extension right output");
            const float maxMagnitude=std::max(std::fabs(a[i]),std::fabs(b[i]));
            // Floating-point M/S roundoff is acceptable below -140 dBFS,
            // but real stereo divergence must still fail this regression.
            const float maxDifference=1.0e-7f + 1.0e-6f*maxMagnitude;
            if(std::fabs(a[i]-b[i])>maxDifference) {
                std::fprintf(stderr,"Linked channels diverged: module=%s type=%d rate=%.0f block=%d channel=2 sample=%d left=%.9g right=%.9g\\n",
                             info(type).name,type,rate,block,i,(double)a[i],(double)b[i]);
                std::exit(1);
            }
        }
    }
    check(m->latency()==0,"Unexpected latency");
    m->reset(); juce::AudioBuffer<float> silent(channels,512);
    float defaults[6]{}; for(int j=0;j<info(type).numKnobs;++j) defaults[j]=defaultNorm(type,j);
    m->process(silent,defaults,true);
    for(int c=0;c<channels;++c) for(int i=0;i<512;++i) check(silent.getWritePointer(c)[i]==0,"Reset/silence failure");
    silent.getWritePointer(0)[0]=std::numeric_limits<float>::infinity();
    silent.getWritePointer(0)[1]=std::numeric_limits<float>::quiet_NaN(); m->process(silent,defaults,true);
    for(int c=0;c<channels;++c) for(int i=0;i<512;++i) check(std::isfinite(silent.getWritePointer(c)[i]),"Invalid input poisoned extension");
    juce::AudioBuffer<float> empty(0,0); m->process(empty,defaults,false);
    return a;
}
static double energy(int type,float a,float b,double frequency,bool anti=false) {
    auto m=makeRackModule(type); m->prepare(48000,257,2);
    float k[6]{}; for(int j=0;j<info(type).numKnobs;++j) k[j]=defaultNorm(type,j);
    k[0]=toNorm(info(type).knobs[0],a); k[1]=toNorm(info(type).knobs[1],b);
    double sum=0;
    for(int pos=0;pos<48000;pos+=257) {
        juce::AudioBuffer<float> buffer(2,std::min(257,48000-pos));
        for(int i=0;i<buffer.getNumSamples();++i) { const float v=.2f*(float)std::sin(2*kPi*frequency*(pos+i)/48000); buffer.getWritePointer(0)[i]=v; buffer.getWritePointer(1)[i]=anti ? -v : v; }
        m->process(buffer,k,true);
        if(pos>24000) for(int i=0;i<buffer.getNumSamples();++i) sum+=(double)buffer.getWritePointer(0)[i]*buffer.getWritePointer(0)[i];
    }
    return sum;
}
int main() {
    check(kNumTypes==12,"Legacy choice count changed");
    for(int legacy=1;legacy<kNumTypes;++legacy) for(int ext=0;ext<128;++ext) check(resolveRackType((float)legacy,(float)ext)==legacy,"Extension overrides legacy automation");
    check(resolveRackType(0,0)==Empty && resolveRackType(0,127)==Empty,"Invalid extension not empty");
    for(int type=Tilt;type<kNumRackTypes;++type) {
        check(resolveRackType(0,(float)(type-kNumTypes+1))==type,"Extension selection mapping");
        for(double rate:{44100.,48000.,88200.,96000.,192000.}) for(int channels:{1,2}) {
            const auto reference=render(type,rate,1,channels);
            for(int block:{7,63,257,1024}) check(reference==render(type,rate,block,channels),"Block dependent extension automation");
        }
    }
    check(energy(Tilt,6,800,8000)>energy(Tilt,0,800,8000)*2,"Tilt high shelf inactive");
    check(energy(Tilt,6,800,80)<energy(Tilt,0,800,80)*.5,"Tilt low shelf inactive");
    check(energy(Stereo,0,100,1000,true)<1e-12,"Width zero not mono");
    check(energy(Stereo,1,300,40,true)<energy(Stereo,1,20,40,true)*.05,"Mono bass inactive");
    for(int type:{FastComp,BusComp}) check(energy(type,-36,4,1000)<energy(type,0,1,1000)*.1,"Compressor does not reduce gain");
    std::puts("Production extension DSP and selection tests passed");
}
