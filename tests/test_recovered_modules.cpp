#include "../Source/RackModuleFactory.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
using namespace skl;
static void check(bool ok,const char* m){if(!ok){std::fprintf(stderr,"%s\n",m);std::exit(1);}}
static std::vector<float> run(int type,double sr,int ch,int block,const std::vector<float>& x,const float* k,bool on){
    auto m=makeRackModule(type);m->prepare(sr,block,ch);auto a=x,b=x;float* p[]={a.data(),b.data()};
    for(int i=0;i<(int)a.size();i+=block){juce::AudioBuffer<float> v(p,ch,i,std::min(block,(int)a.size()-i));m->process(v,k,on);}for(float s:a)check(std::isfinite(s),"Nonfinite recovered module output");return a;
}
static double rms(const std::vector<float>& x,int first){double sum=0;for(int i=first;i<(int)x.size();++i)sum+=double(x[i])*x[i];return std::sqrt(sum/(x.size()-first));}
int main(){
    for(double sr:{44100.,48000.,96000.,192000.}) for(int ch:{1,2}){
        std::vector<float> x((int)sr/4);x[31]=.5f;
        for(int type:{SkellMixHead,SkellDemon,SkellSSL,SkellNoise,SkellDope,SkellRubyQ}){
            float k[6]{};for(int j=0;j<info(type).numKnobs;++j)k[j]=defaultNorm(type,j);
            auto m=makeRackModule(type);m->prepare(sr,257,ch);const auto d=m->latency();
            const auto y=run(type,sr,ch,257,x,k,false);for(int i=0;i<(int)y.size();++i)check(std::abs(y[i]-(i>=d?x[i-d]:0.f))<2e-6f,"Bypass impulse latency mismatch");
        }
    }
    const int n=48000;std::vector<float> noise(n),tone(n);unsigned rng=13;for(int i=0;i<n;++i){rng=rng*1664525u+1013904223u;noise[i]=.0006f*(float((rng>>8)&65535)/32768.f-1.f);tone[i]=.2f*std::sin(float(2*kPi*220*i/48000.));}
    float k[6]{};for(int i=0;i<3;++i)k[i]=defaultNorm(SkellNoise,i);k[0]=1.f;auto y=run(SkellNoise,48000,2,257,noise,k,true);const double reduction=20*std::log10(rms(y,n/2)/rms(noise,n/2));check(reduction<-6,"Noise reduction ineffective");
    k[0]=0;auto z=run(SkellNoise,48000,2,257,noise,k,true);check(z==noise,"Amount 0 not transparent");
    for(int i=0;i<6;++i){k[i]=defaultNorm(SkellSSL,i);}
    k[4]=1.f;std::vector<float> hf(n);for(int i=0;i<n;++i)hf[i]=.02f*std::sin(float(2*kPi*12000*i/48000.));auto e=run(SkellSSL,48000,2,257,hf,k,true);k[0]=1;auto g=run(SkellSSL,48000,2,257,hf,k,true);check(rms(e,n/2)>rms(hf,n/2)*1.3,"SSL E high shelf inactive");check(rms(g,n/2)>rms(hf,n/2)*1.3,"SSL G high shelf inactive");
    for(int i=0;i<6;++i){k[i]=defaultNorm(SkellDemon,i);}
    k[0]=.75f;auto shifted=run(SkellDemon,48000,1,257,tone,k,true);int crossings=0;for(int i=n/2+1;i<n;++i)crossings+=shifted[i-1]<=0&&shifted[i]>0;const float hz=2.f*crossings;check(hz>285&&hz<340,"Demon +6 semitone pitch not working");
    std::printf("Recovered modules: 48 bypass cases; quiet noise %.2f dB; E/G high shelf active; Demon +6: %.1f Hz (220 Hz source)\n",reduction,hz);
}
