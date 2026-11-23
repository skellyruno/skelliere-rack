#include "../Source/RackModuleFactory.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace skl;
static void check(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
static float signal(int i, double sr) {
    return 0.1f * static_cast<float>(std::sin(2*kPi*137*i/sr) + std::sin(2*kPi*7831*i/sr));
}
static std::vector<float> run(int type, double sr, int block, int ch) {
    auto module = makeRackModule(type);
    module->prepare(sr, 4096, ch);
    std::vector<float> a(24576), b(24576);
    for (int i=0; i<24576; ++i) a[i]=b[i]=signal(i,sr);
    float* p[2]={a.data(),b.data()};
    const int edges[]={4096,8192,10240,12288,16384,24576};
    for (int event=0,pos=0; pos<24576;) {
        while (pos==edges[event]) ++event;
        float k[6]{};
        for (int j=0;j<info(type).numKnobs;++j)
            k[j]=event==0 ? defaultNorm(type,j) : event<4 ? 1.0f : event==4 ? 0.0f : std::numeric_limits<float>::quiet_NaN();
        int n=std::min(block,edges[event]-pos);
        juce::AudioBuffer<float> view(p,ch,pos,n);
        module->process(view,k,event!=2);
        pos+=n;
    }
    for (int i=0;i<24576;++i) {
        check(std::isfinite(a[i]) && std::fabs(a[i])<8,"Invalid automated output");
        if (ch==2) check(a[i]==b[i],"Stereo imbalance");
        if (i>=8192+static_cast<int>(std::ceil(.005*sr))+4 && i<10240)
            check(a[i]==signal(i,sr),"Bypass did not settle to dry");
    }
    check(module->latency()==0,"Added latency");
    return a;
}
int main() {
    for (int type : {Eq,Air}) {
        for (double sr : {44100.0,48000.0,96000.0,192000.0}) {
            float v=0, step=static_cast<float>(1/(.020*sr));
            int n=static_cast<int>(std::round(.020*sr));
            for(int i=0;i<n/2;++i) v=slewToward(v,1,step);
            check(std::fabs(v-.5f)<.002f,"Incorrect slew time");
            for(int i=n/2;i<n+4;++i) v=slewToward(v,1,step);
            check(v==1,"Slew did not settle");
            for (int ch : {1,2}) {
                const auto ref=run(type,sr,1,ch);
                for (int bs : {7,63,512,4096}) check(ref==run(type,sr,bs,ch),"Block-dependent automation");
            }
        }
        auto old=makeModule(type), fresh=makeRackModule(type);
        old->prepare(48000,512,1); fresh->prepare(48000,512,1);
        float k[6]{}; for(int j=0;j<info(type).numKnobs;++j) k[j]=.7f;
        juce::AudioBuffer<float> a(1,8192), b(1,8192);
        for(int i=0;i<8192;++i) a.getWritePointer(0)[i]=b.getWritePointer(0)[i]=signal(i,48000);
        old->process(a,k,true); fresh->process(b,k,true);
        for(int i=0;i<8192;++i) check(std::fabs(a.getWritePointer(0)[i]-b.getWritePointer(0)[i])<2e-6f,"Static response changed");
        fresh->reset();
        b.getWritePointer(0)[3]=std::numeric_limits<float>::infinity();
        b.getWritePointer(0)[19]=std::numeric_limits<float>::quiet_NaN();
        fresh->process(b,k,true);
        for(int i=0;i<8192;++i) check(std::isfinite(b.getWritePointer(0)[i]),"Filter poisoned by invalid input");
        juce::AudioBuffer<float> empty(0,0); fresh->process(empty,k,false);
    }
    std::puts("Tone regression tests passed");
}
