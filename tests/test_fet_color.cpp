#include "../Source/RackCompressor.h"
#include <cstdio>
#include <cstdlib>
using namespace skl;
static void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"%s\n",why);std::exit(1);}}
static double bin(const std::vector<float>& y,double sr,double freq){double re=0,im=0;for(int i=0;i<int(sr);++i){double t=2*kPi*freq*i/sr;re+=y[i+int(sr)]*std::cos(t);im+=y[i+int(sr)]*std::sin(t);}return 2*std::hypot(re,im)/sr;}
static std::vector<float> render(double sr,double hz,bool adaa){std::vector<float> y(int(2*sr));float prev=0;for(int i=0;i<int(y.size());++i){const float x=.7f*std::sin(2*kPi*hz*i/sr);const float residual=adaa?fetColorResidual(x,prev,2.f):std::tanh(x*2.f)/2.f-x;prev=x;y[i]=x+.22f*residual;check(std::isfinite(y[i]),"Nonfinite color");}return y;}
// Exercise the exact production class at a demanding ALL-mode setting.
static std::vector<float> renderModule(double hz,int revision){
    constexpr double sr=48000;std::vector<float> l(96000),r(96000);for(int i=0;i<int(l.size());++i){l[i]=.7f*std::sin(2*kPi*hz*i/sr);r[i]=-l[i];}
    FetCompressor m;m.prepare(sr,257,2);float k[6];for(int j=0;j<6;++j)k[j]=defaultNorm(FastComp,j);k[0]=toNorm(info(FastComp).knobs[0],18.f);k[1]=1.f;k[5]=float(revision);
    float* pp[]={l.data(),r.data()};for(int i=0;i<int(l.size());i+=257){juce::AudioBuffer<float>b(pp,2,i,std::min(257,int(l.size())-i));m.process(b,k,true);}
    check(m.latency()==0,"FET latency changed");for(size_t i=0;i<l.size();++i)check(std::isfinite(l[i])&&std::abs(l[i]+r[i])<1e-6f,"Production FET stereo link");return l;
}
int main(){
    for(double sr:{44100.,48000.,96000.}){const double f=sr*17./48.,alias=3*f-sr;auto a=render(sr,f,false),b=render(sr,f,true);double old=bin(a,sr,alias),now=bin(b,sr,alias);double low=20*std::log10(bin(render(sr,1000,true),sr,3000)/bin(render(sr,1000,false),sr,3000));double fundamental=20*std::log10(bin(b,sr,f)/bin(a,sr,f));
        printf("%.0f Hz color residual: folded third %.2f -> %.2f dBFS; 1k desired third delta %.3f dB; HF fundamental delta %.3f dB\n",sr,20*std::log10(old),20*std::log10(now),low,fundamental);check(now<old*.55,"Insufficient alias reduction");check(std::abs(low)<.25,"Low harmonic preservation");check(std::abs(fundamental)<1.5,"HF fundamental change too large");}
    for(float v:{-1024.f,-1.f,-.01f,0.f,.01f,1.f,1024.f})for(float p:{-1024.f,-1.f,0.f,1.f,1024.f}){float a=fetColorResidual(v,p,1.5f),b=fetColorResidual(-v,-p,1.5f);check(std::isfinite(a)&&std::abs(a+b)<1e-6f,"Odd symmetry/finite");}
    for(int rev:{0,1}){const auto low=renderModule(1000,rev),high=renderModule(17000,rev);const double desired=20*std::log10(bin(low,48000,3000)/bin(low,48000,1000)),alias=20*std::log10(bin(high,48000,3000)/bin(high,48000,17000));
        printf("Production FET rev %d desired third %.3f dBc; folded third %.3f dBc\n",rev,desired,alias);
        check(alias<desired-8.,"Full-module alias reduction insufficient");
        check(std::abs(desired-(rev?-67.322:-63.277))<.5,"Desired harmonic changed from measured baseline");}
    puts("FET antialiased color checks passed");
}
