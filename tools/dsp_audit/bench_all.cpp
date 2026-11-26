#include "../../Source/RackModuleFactory.h"
#include <chrono>
#include <cstdio>
#include <vector>
using namespace skl;
int main(){
    const int sr=48000, frames=sr*10, block=512;
    std::vector<float> L(frames),R(frames);
    for(int i=0;i<frames;++i){L[i]=0.25f*std::sin(2*kPi*220.0*i/sr)+0.1f*std::sin(2*kPi*3100.0*i/sr);R[i]=L[i];}
    for(int t=1;t<kNumRackTypes;++t){
        auto m=makeRackModule(t); if(!m){std::printf("%2d  <no factory module>\n",t);continue;}
        m->prepare(sr,block,2);
        float k[8]{};for(int j=0;j<info(t).numKnobs;++j)k[j]=defaultNorm(t,j);
        auto l=L,r=R; double best=1e18;
        for(int rep=0;rep<3;++rep){
            auto t0=std::chrono::steady_clock::now();
            for(int i=0;i<frames;i+=block){float* ch[]={l.data()+i,r.data()+i};juce::AudioBuffer<float> b(ch,2,0,std::min(block,frames-i));m->process(b,k,true);}
            best=std::min(best,std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-t0).count()/frames);
            l=L;r=R;
        }
        std::printf("%2d %-18s %8.1f ns/frame  (%.4f%% of one core @48k)\n",t,displayName(t),best,best*sr/1e9*100);
    }
}
