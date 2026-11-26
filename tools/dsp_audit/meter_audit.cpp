#include "../../Source/RackModuleFactory.h"
#include <cstdio>
#include <vector>
using namespace skl;
static void run(ModuleBase* m,const float* k,bool active,float amp,double sec,int block=512){
    const int sr=48000,n=(int)(sec*sr);std::vector<float> x(n);
    for(int i=0;i<n;++i)x[i]=amp*std::sin(2*kPi*1000.0*i/sr);
    for(int i=0;i<n;i+=block){float* ch[]={x.data()+i,x.data()+i};juce::AudioBuffer<float> b(ch,2,0,std::min(block,n-i));m->process(b,k,active);}
}
int main(){
    std::printf("%-18s %9s | %12s %12s %12s\n","module","loud GR","inactive 0.1s","inactive 1s","silence 1s");
    for(int t=1;t<kNumRackTypes;++t){
        if(!info(t).hasVU) continue;
        float k[8]{};
        for(int j=0;j<info(t).numKnobs;++j)k[j]=defaultNorm(t,j);
        if(t!=Gate) k[0]=0.15f;   // push threshold down on dynamics modules so they engage hard
        struct Case{const char* name;};
        auto fresh=[&]{auto m=makeRackModule(t);m->prepare(48000,512,2);return m;};
        auto a=fresh(); run(a.get(),k,true,.8f,1.0);   float loud=a->meter();
        run(a.get(),k,false,.8f,.1);                    float i01=a->meter();
        run(a.get(),k,false,.8f,1.0);                   float i1 =a->meter();
        auto b=fresh(); run(b.get(),k,true,.8f,1.0); run(b.get(),k,true,0.f,1.0); float sil=b->meter();
        std::printf("%-18s %9.2f | %12.2f %12.2f %12.2f\n",displayName(t),loud,i01,i1,sil);
    }
}
