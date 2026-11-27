#include "../Source/Dsp.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
using namespace skl;
static void require(bool ok,const char* reason){if(!ok){std::fprintf(stderr,"%s\n",reason);std::exit(1);}}
static std::vector<float> render(const std::vector<float>& dry,double sr,float speed,float human,int block,bool active,bool reference){
    std::vector<float> y=dry;float k[6]{};for(int j=0;j<info(Tune).numKnobs;++j)k[j]=defaultNorm(Tune,j);k[0]=toNorm(info(Tune).knobs[0],speed);
    SkellyTune m;skellcore::backends::SkellyTuneBackend old;old.prepare(sr,1);m.prepare(sr,257,1);m.setHumanize(human);
    require(m.latency()==old.latencySamples(),"Latency changed");
    skelly::Parameters p=old.parameters();p.speedMs=realOf(Tune,0,k);p.amount=realOf(Tune,1,k);p.mix=active?realOf(Tune,2,k):0.f;p.pitchClasses=SkellyTune::mask(0,0);old.setParameters(p);
    for(int i=0;i<int(y.size());i+=block){int n=std::min(block,int(y.size())-i);if(reference){const float* in[]={dry.data()+i};float* out[]={y.data()+i};old.processBlock(in,out,n,1);}else{float* ptr[]={y.data()};juce::AudioBuffer<float>b(ptr,1,i,n);m.process(b,k,active);}}
    for(float x:y)require(std::isfinite(x)&&std::abs(x)<4,"Invalid output");
    return y;
}
int main(int argc,char** argv){
    for(double sr:{44100.,48000.,96000.}){std::vector<float> dry(int(sr*2));double phase=0;for(int i=0;i<int(dry.size());++i){phase+=2*kPi*(225+3*std::sin(2*kPi*5*i/sr))/sr;dry[i]=.22f*std::sin(phase)+.09f*std::sin(2*phase);}
        for(float speed:{0.f,25.f,100.f,200.f})for(bool active:{false,true}){auto ref=render(dry,sr,speed,0,257,active,true);auto now=render(dry,sr,speed,0,257,active,false);require(now==ref,"Humanize zero not sample-identical to old backend");}
        auto hard=render(dry,sr,0,0,333,true,false), human=render(dry,sr,0,1,333,true,false);double delta=0;for(size_t i=0;i<hard.size();++i)delta+=std::pow(double(hard[i])-human[i],2);require(delta>1e-5,"Humanize did not affect audio");
    }
    std::puts("24 sample-identical legacy Tune comparisons; Humanize active and latency unchanged across 3 sample rates");
    if(argc>1){std::ifstream f(argv[1],std::ios::binary);std::vector<float> dry(441000);f.read(reinterpret_cast<char*>(dry.data()),dry.size()*4);require(bool(f),"Missing private vocal excerpt");auto old=render(dry,44100,25,0,257,true,false),now=render(dry,44100,25,1,257,true,false);double delta=0,p0=0,p1=0,peak=0;for(size_t i=0;i<dry.size();++i){delta+=std::pow(double(old[i])-now[i],2);p0+=old[i]*old[i];p1+=now[i]*now[i];peak=std::max(peak,double(std::abs(now[i])));}require(peak<1,"Vocal peak clipped");printf("Private dry vocal: Humanize 100%% vs 0%% RMS change %+.3f dB, relative difference %.3f dB, peak %.3f dBFS\n",10*std::log10(p1/p0),10*std::log10(delta/p0),20*std::log10(peak));}
}
