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
static std::vector<float> tune(const std::vector<float>& x,double sr,float hz,bool active,float knob=1.f){SkellyTune m;m.prepare(sr,257,1);m.setReferenceA(hz);float k[6]{};for(int j=0;j<5;++j)k[j]=defaultNorm(Tune,j);k[0]=knob;auto y=x;float* p[]={y.data()};for(int i=0;i<int(y.size());i+=257){juce::AudioBuffer<float>b(p,1,i,std::min(257,int(y.size())-i));m.process(b,k,active);}for(float v:y)check(std::isfinite(v),"Detune nonfinite");return y;}
int main(int argc,char** argv){
    const auto& retune=info(Tune).knobs[0];
    check(toReal(retune,0)==100.f&&toReal(retune,1)==0.f,"Retune direction/endpoints");
    check(toReal(retune,.5f)==50.f&&defaultNorm(Tune,0)==.75f,"Retune midpoint/default");
    for(float ms:{0.f,25.f,50.f,100.f})check(std::abs(toReal(retune,toNorm(retune,ms))-ms)<1e-5f,"Retune inverse mapping");
    check(tuneReferenceHz(.5f)==440.f&&tuneReferenceHz(0)==400.f&&tuneReferenceHz(1)==480.f,"A4 endpoint/default");
    check(migrateTuneRetune(.125f)==.75f&&migrateTuneRetune(.75f)==0.f,"Retune migration");
    check(migrateTuneReference(.5f)==.5f&&std::abs(tuneReferenceHz(migrateTuneReference(1))-440.f*std::exp2(50.f/1200.f))<.001f,"A4 migration");
    for(double sr:{44100.,48000.,96000.}){std::vector<float> x(size_t(sr*2));for(size_t i=0;i<x.size();++i)x[i]=.5f*std::sin(2*kPi*440*i/sr);auto blue=fet(x,sr,0,0,6,257),black=fet(x,sr,1,0,6,257),slam=fet(x,sr,0,4,6,257);check(blue==fet(x,sr,0,0,6,63),"FET block dependency");check(x==fet(x,sr,0,0,6,257,false),"FET bypass not exact");check(relative(blue,black)>.005,"FET revisions indistinguishable");check(rms(slam,x.size()/2)<rms(blue,x.size()/2)*.85,"Slam not stronger");printf("%.0f Hz: Blue %.2f dBFS, Black %.2f dBFS, ALL %.2f dBFS\n",sr,20*std::log10(rms(blue,x.size()/2)),20*std::log10(rms(black,x.size()/2)),20*std::log10(rms(slam,x.size()/2)));
    auto a=tune(x,sr,440,true),b=tune(x,sr,432,true),c=tune(x,sr,448,true);
    check(relative(a,b)>.005,"A4 did not change correction");check(tune(x,sr,440,false)==tune(x,sr,432,false),"A4 changed bypass");
    auto frequency=[&](const std::vector<float>& y){int zc=0;for(int i=int(sr)+1;i<int(2*sr);++i)if(y[i-1]<=0&&y[i]>0)++zc;return double(zc);};
    const double fa=frequency(a),fb=frequency(b),fc=frequency(c);
    printf("A4 reference %.0f Hz sample rate: %.1f / %.1f / %.1f Hz output (440 / 432 / 448 Hz targets)\n",sr,fa,fb,fc);
    check(std::abs(fa-440)<2&&std::abs(fb-432)<2&&std::abs(fc-448)<2,"A4 reference target not reached");
    check(relative(tune(x,sr,432,true,0),b)>.0001,"Slow and fast retune responses indistinguishable");}
    if(argc>1){std::ifstream file(argv[1],std::ios::binary);std::vector<float> dry(441000);file.read(reinterpret_cast<char*>(dry.data()),dry.size()*4);check(bool(file),"Missing private vocal");for(int rev:{0,1}){auto t=std::chrono::steady_clock::now();auto y=fet(dry,44100,rev,0,6,257);auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count();float peak=0;for(float v:y)peak=std::max(peak,std::abs(v));printf("Vocal revision %d: RMS %.2f dBFS, peak %.2f dBFS, render %.1f ms (10 s stereo core)\n",rev,20*std::log10(rms(y)),20*std::log10(peak),ms);check(peak<1,"Vocal clipped");}}
    puts("FET revisions, ratio, block independence, bypass and Tune detune passed");
}
