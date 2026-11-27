#include "../Source/RackModuleFactory.h"
#include <cstdio>
#include <cstdlib>
#include <random>
using namespace skl;
static void req(bool b,const char* s){if(!b){std::fprintf(stderr,"Smart Gate: %s\n",s);std::exit(1);}}
static double rms(const std::vector<float>& a,int start,int end){double s=0;for(int i=start;i<end;++i)s+=double(a[i])*a[i];return std::sqrt(s/(end-start));}
static double ratioDb(const std::vector<float>& a,const std::vector<float>& b,int start,int end){return 20*std::log10((rms(a,start,end)+1e-15)/(rms(b,start,end)+1e-15));}
static std::array<std::vector<float>,2> run(const std::array<std::vector<float>,2>& x,int sr,int block,bool active=true,float range=48,int channels=2){
    SmartGateModule gate;gate.prepare(sr,block,channels);req(gate.latency()==0,"Added latency");float k[6]{};for(int j=0;j<4;++j)k[j]=defaultNorm(SmartGate,j);k[1]=toNorm(info(SmartGate).knobs[1],range);k[2]=toNorm(info(SmartGate).knobs[2],25);k[3]=toNorm(info(SmartGate).knobs[3],25);
    auto y=x;float* d[]={y[0].data(),y[1].data()};for(int i=0;i<int(y[0].size());i+=block){juce::AudioBuffer<float>b(d,channels,i,std::min(block,int(y[0].size())-i));gate.process(b,k,active);req(std::isfinite(gate.meter()),"Nonfinite meter");}
    for(auto& ch:y)for(float v:ch)req(std::isfinite(v),"Nonfinite output");
    return y;
}
int main(){
    for(int sr:{44100,48000,96000}){
        const int n=8*sr;std::array<std::vector<float>,2>x{std::vector<float>(n),std::vector<float>(n)};std::mt19937 rng(123);std::normal_distribution<float> noise(0.f,1.f);
        for(int i=0;i<n;++i){const float t=float(i)/sr;float floor=db2g(t>=1&&t<5?-42.8f:-64.f);float voice=0;
            if(t>=3&&t<4)voice=.09f*std::sin(2*kPi*170*t)+.035f*std::sin(2*kPi*340*t);
            if(t>=6&&t<7)voice=.016f*std::sin(2*kPi*230*t);
            x[0][i]=voice+floor*noise(rng);x[1][i]=.7f*x[0][i];}
        const auto a=run(x,sr,257),b=run(x,sr,1),c=run(x,sr,511);
        req(a==b&&a==c,"Block-size invariance failed");req(run(x,sr,257,false)==x,"Inactive not sample-exact");req(run(x,sr,257,true,0)==x,"Zero range not sample-exact");
        const double low=ratioDb(a[0],x[0],sr/2,sr),high=ratioDb(a[0],x[0],int(2.6*sr),3*sr),speech=ratioDb(a[0],x[0],int(3.05*sr),4*sr),quiet=ratioDb(a[0],x[0],int(6.05*sr),7*sr);
        std::printf("Smart Gate %d Hz: low noise %.2f dB, rising noise %.2f dB, speech %.3f dB, quiet voiced signal %.3f dB\n",sr,low,high,speech,quiet);
        req(low<-35&&high<-15,"Noise attenuation / adaptation inadequate");req(speech>-.3&&quiet>-.6,"Voiced signal attenuated");
        for(int i=0;i<n;++i)req(std::abs(a[1][i]-.7f*a[0][i])<1e-6,"Stereo image shifted");
        auto m=run(x,sr,257,true,48,1);req(ratioDb(m[0],x[0],int(3.05*sr),4*sr)>-.3,"Mono speech attenuated");
    }
    std::puts("Smart Gate: adaptive floor, voice preservation, linked stereo, mono, block sizes, bypass and zero latency passed.");
}
