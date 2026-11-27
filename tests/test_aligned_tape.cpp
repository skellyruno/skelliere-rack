#include "../Source/AlignedTape4x.h"
#include "AlignedTapeReference.h"
#include <cassert>
#include <cstdio>
#include <random>
int main(){
    static_assert(skl::AlignedTape4x::delay==skl_test::AlignedTapeReference::delay);
    skl::AlignedTape4x a;skl_test::AlignedTapeReference b;
    std::mt19937 rng(20261126);std::uniform_real_distribution<float> u(-1,1);
    for(int channels:{1,2}){
        a.prepare();b.prepare();
        for(int i=0;i<100000;++i){
            if(i%8191==0){a.reset();b.reset();}
            const float amount=i%127<23?0.f:float(i%101)/100.f;
            const float input=.25f+float(i%77)/22.f,drive=1+5*amount,bias=.12f*amount,bz=std::tanh(bias);
            for(int c=0;c<channels;++c){const float x=u(rng);auto y=a.process(c,x,input,amount,drive,bias,bz);auto z=b.process(c,x,input,amount,drive,bias,bz);assert(y.dry==z.dry&&y.color==z.color);assert(std::isfinite(y.color));}
        }
        a.reset();for(int i=0;i<100;++i)for(int c=0;c<channels;++c){auto y=a.process(c,i==0?1.f:0.f,1,0,1,0,0);assert(y.dry==(i==12?1.f:0.f)&&y.color==0);}
    }
    std::puts("AlignedTape4x: exact original parity, reset, zero amount, mono/stereo, 12-sample delay passed");
}
