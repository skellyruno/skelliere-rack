#include "../Source/AlignedTape4x.h"
#include "AlignedTapeReference.h"
#include <cmath>
template<class T> static void render(const float* x,float* y,unsigned n,unsigned ch,float input,float amount,float biasControl){
    T tape;tape.prepare();const float drive=1+5*amount,bias=.12f*(1-biasControl)*amount,bz=std::tanh(bias);
    for(unsigned i=0;i<n;++i)for(unsigned c=0;c<ch;++c){auto z=tape.process(c,x[i*ch+c],input,amount,drive,bias,bz);y[2*(i*ch+c)]=z.dry;y[2*(i*ch+c)+1]=z.color;}
}
extern "C" int tape_render(int fast,const float* x,float* y,unsigned n,unsigned ch,float input,float amount,float bias){
    if(!x||!y||!n||ch<1||ch>2||(fast!=0&&fast!=1)||!std::isfinite(input)||!std::isfinite(amount)||!std::isfinite(bias)||input<0||amount<0||amount>1||bias<0||bias>1)return 0;
    if(fast)render<skl::AlignedTape4x>(x,y,n,ch,input,amount,bias);else render<skl_test::AlignedTapeReference>(x,y,n,ch,input,amount,bias);return 1;
}
