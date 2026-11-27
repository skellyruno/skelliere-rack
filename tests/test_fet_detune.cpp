#include "../Source/RackModuleFactory.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <chrono>
using namespace skl;
static void check(bool ok,const char* what){if(!ok){std::fprintf(stderr,"%s\n",what);std::exit(1);}}
static std::vector<float> fet(const std::vector<float>& input,double sr,int rev,int ratio,float drive,int block,bool enabled=true){
    auto m=makeRackModule(FastComp);m->prepare(sr,257,2);float k[6]{};for(int j=0;j<6;++j)k[j]=defaultNorm(FastComp,j);k[0]=toNorm(info(FastComp).knobs[0],drive);k[1]=ratio*.25f;k[5]=float(rev);
    auto l=input,r=input;for(float& v:r)v=-v;float* ptr[]={l.data(),r.data()};
    for(int i=0;i<int(l.size());i+=block){juce::AudioBuffer<float>b(ptr,2,i,std::min(block,int(l.size())-i));m->process(b,k,enabled);}
    check(m->latency()==0,"FET added latency");for(size_t i=0;i<l.size();++i)check(std::isfinite(l[i])&&std::abs(l[i]+r[i])<1e-6f,"FET finite/link failure");return l;
}
static double rms(const std::vector<float>& x,size_t start=0){double v=0;for(size_t i=start;i<x.size();++i)v+=double(x[i])*x[i];return std::sqrt(v/(x.size()-start));}
static double relative(const std::vector<float>& a,const std::vector<float>& b){double p=0;for(size_t i=0;i<a.size();++i){double d=a[i]-b[i];p+=d*d;}return std::sqrt(p/a.size());}
static std::vector<float> tune(const std::vector<float>& x,double sr,float detune,bool active){SkellyTune m;m.prepare(sr,257,1);m.setDetune(detune);float k[6]{};for(int j=0;j<5;++j)k[j]=defaultNorm(Tune,j);k[0]=0;auto y=x;float* p[]={y.data()};for(int i=0;i<int(y.size());i+=257){juce::AudioBuffer<float>b(p,1,i,std::min(257,int(y.size())-i));m.process(b,k,active);}for(float v:y)check(std::isfinite(v),"Detune nonfinite");return y;}
int main(int argc,char** argv){
    for(double sr:{44100.,48000.,96000.}){std::vector<float> x(size_t(sr*2));for(size_t i=0;i<x.size();++i)x[i]=.5f*std::sin(2*kPi*440*i/sr);auto blue=fet(x,sr,0,0,6,257),black=fet(x,sr,1,0,6,257),slam=fet(x,sr,0,4,6,257);check(blue==fet(x,sr,0,0,6,63),"FET block dependency");check(x==fet(x,sr,0,0,6,257,false),"FET bypass not exact");check(relative(blue,black)>.005,"FET revisions indistinguishable");check(rms(slam,x.size()/2)<rms(blue,x.size()/2)*.85,"Slam not stronger");printf("%.0f Hz: Blue %.2f dBFS, Black %.2f dBFS, ALL %.2f dBFS\n",sr,20*std::log10(rms(blue,x.size()/2)),20*std::log10(rms(black,x.size()/2)),20*std::log10(rms(slam,x.size()/2)));
    auto a=tune(x,sr,0,true),b=tune(x,sr,30,true);check(relative(a,b)>.005,"Detune did not change correction");check(tune(x,sr,0,false)==tune(x,sr,30,false),"Detune changed bypass");}
    if(argc>1){std::ifstream file(argv[1],std::ios::binary);std::vector<float> dry(441000);file.read(reinterpret_cast<char*>(dry.data()),dry.size()*4);check(bool(file),"Missing private vocal");for(int rev:{0,1}){auto t=std::chrono::steady_clock::now();auto y=fet(dry,44100,rev,0,6,257);auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count();float peak=0;for(float v:y)peak=std::max(peak,std::abs(v));printf("Vocal revision %d: RMS %.2f dBFS, peak %.2f dBFS, render %.1f ms (10 s stereo core)\n",rev,20*std::log10(rms(y)),20*std::log10(peak),ms);check(peak<1,"Vocal clipped");}}
    puts("FET revisions, ratio, block independence, bypass and Tune detune passed");
}
