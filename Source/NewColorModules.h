#pragma once
#include "Dsp.h"
#include "Recovered/core/Spectral4.h"
namespace skl {
// Symmetric 49-tap interpolation and decimation; polyphase interpolation avoids
// multiplying inserted zeros. 24+24 high-rate samples = 12 host samples.
class AlignedTape4x {
public:
    static constexpr int delay=12;
    void prepare() noexcept {
        double sum=0;for(int i=0;i<49;++i){const double t=i-24,fc=.1125;const double w=.42-.5*std::cos(2*kPi*i/48)+.08*std::cos(4*kPi*i/48);const double v=w*(t==0?2*fc:std::sin(2*kPi*fc*t)/(kPi*t));h[i]=float(v);sum+=v;}
        for(auto& v:h)v=float(v/sum);
        reset();
    }
    void reset() noexcept {for(auto& c:channels)c={};}
    struct Result {float dry,color;};
    Result process(int c,float x,float input,float amount,float drive,float bias,float biasZero) noexcept {
        auto& s=channels[c];s.history[s.w]=x;const float dry=s.history[(s.w-delay)&15];float color=0;
        for(int j=0;j<4;++j){float residual=0;if(amount>0){float v=0;for(int k=j,m=0;k<49;k+=4,++m)v+=h[k]*s.history[(s.w-m)&15];v*=4*input;residual=(std::tanh(drive*v+bias)-biasZero)/drive-v;}s.residual[s.r]=residual;
            if(j==0&&amount>0){for(int k=0;k<49;++k)color+=h[k]*s.residual[(s.r-k)&63];}
            s.r=(s.r+1)&63;
        }
        s.w=(s.w+1)&15;return {dry,color};
    }
private:
    struct Channel {float history[16]{},residual[64]{};int w=0,r=0;};Channel channels[2];float h[49]{};
};
class NewColorModule final : public ModuleBase {
public:
    explicit NewColorModule(int t):type(t){}
    void prepare(double rate,int,int) override {
        sr=std::isfinite(rate)&&rate>=8000?rate:48000;step=float(1/(.02*sr));fade=float(1/(.005*sr));
        if(type==SkellDope){spectral[0].prepare(sr,{230,2200,7400},{0,0,3.5,5.5},-20);spectral[1].prepare(sr,{320,4700,9300},{0,0,2,8},-23);for(auto& m:spectral)m.setDepthWeights({0,0,.7,1});}
        if(type==SkellMixHead)tape.prepare();
        reset();
    }
    int latency() const override{return type==SkellMixHead?AlignedTape4x::delay:0;}
    void reset() override {for(auto& f:filters)f.reset();for(auto& m:spectral)m.reset();tape.reset();wet=1;seed=false;dirty=true;countdown=0;for(auto& d:dc)d=0;}
    void process(juce::AudioBuffer<float>& b,const float* knobs,bool active) override {
        const int ch=std::min(2,b.getNumChannels()),n=b.getNumSamples();if(ch==0||n==0)return;
        const int nk=info(type).numKnobs;float targets[kMaxKnobs]{};
        for(int j=0;j<nk;++j)targets[j]=std::isfinite(knobs[j])?std::clamp(knobs[j],0.f,1.f):defaultNorm(type,j);
        if(!seed){for(int j=0;j<nk;++j)current[j]=targets[j];wet=active?1.f:0.f;seed=true;dirty=true;countdown=0;}
        float* data[2]={b.getWritePointer(0),ch==2?b.getWritePointer(1):nullptr};
        for(int i=0;i<n;++i){
            for(int j=0;j<nk;++j){const float next=current[j]+std::clamp(targets[j]-current[j],-step,step);dirty=dirty||next!=current[j];current[j]=next;}
            if(countdown==0){if(dirty){update();dirty=false;}countdown=16;}--countdown;
            wet+=std::clamp((active?1.f:0.f)-wet,-fade,fade);
            float x[2]={sane(data[0][i]),ch==2?sane(data[1][i]):sane(data[0][i])},y[2]={x[0],x[1]};
            if(type==SkellDope){
                const auto a=spectral[0].process({x[0],x[1]}),b4=spectral[1].process({x[0],x[1]});
                const double c[2]={a.l+current[0]*(b4.l-a.l),a.r+current[0]*(b4.r-a.r)};
                for(int cidx=0;cidx<ch;++cidx)y[cidx]=(v[1]*x[cidx]+v[2]*float(c[cidx]-x[cidx]))*out;
            }else if(type==SkellRubyQ){
                for(int c=0;c<ch;++c){for(auto& f:filters)y[c]=f.process(c,y[c]);}
            }else{
                for(int c=0;c<ch;++c){const auto z=tape.process(c,x[c],input,v[1],drive,bias,biasZero);x[c]=z.dry;dc[c]+=(z.color-dc[c])*dcStep;const float residual=z.color-dc[c];y[c]=filters[0].process(c,z.dry*input+v[1]*residual)*out;}
            }
            for(int c=0;c<ch;++c)data[c][i]=wet==0.f?x[c]:sane(x[c]+wet*(y[c]-x[c]));
        }
    }
private:
    void update(){
        for(int j=0;j<info(type).numKnobs;++j)v[j]=realOf(type,j,current);
        if(type==SkellDope){for(auto& m:spectral)m.setDepth(2.5*v[3]);out=db2g(v[4]);}
        else if(type==SkellRubyQ){
            // Smooth broad tube-EQ style curves; shape varies with cut vs boost.
            filters[0].lowShelf(sr,v[1],v[0]);filters[1].peaking(sr,v[3],.62+.20*std::abs(v[2])/12.,v[2]);filters[2].highShelf(sr,v[5],v[4]);
        }else{input=db2g(v[0]);out=db2g(v[3]);drive=1.f+5.f*v[1];bias=.12f*(1.f-current[5])*v[1];biasZero=std::tanh(bias);dcStep=1.f-onePole(sr,12);filters[0].highShelf(sr,3500.f+2500.f*current[4],v[2]-2.f*(1.f-current[4])*v[1]);}
    }
    const int type;double sr=48000;int countdown=0;bool seed=false,dirty=true;
    float step=1.f/960,fade=1.f/240,current[kMaxKnobs]{},v[kMaxKnobs]{},wet=1,input=1,out=1,drive=1,bias=0,biasZero=0,dcStep=0,dc[2]{};
    Biquad filters[3];AlignedTape4x tape;skell_lab::Spectral4 spectral[2];
};
}
