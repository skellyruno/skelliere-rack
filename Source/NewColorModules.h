#pragma once
#include "Dsp.h"
namespace skl {
// Original experimental processors. No imported calibration coefficients or
// reference-plugin fidelity claims. Fixed storage, stereo-linked detection,
// sample-cadence smoothing; the tape uses causal IIR resampling (no fixed PDC).
class NewColorModule final : public ModuleBase {
public:
    explicit NewColorModule(int t):type(t){}
    void prepare(double rate,int,int) override {
        sr=std::isfinite(rate)&&rate>=8000?rate:48000;
        step=float(1/(.02*sr)); fade=float(1/(.005*sr));
        for(auto& f:up)f.lowPass(sr*4,sr*.43,.70710678);
        for(auto& f:down)f.lowPass(sr*4,sr*.43,.70710678);
        reset();
    }
    void reset() override {
        for(auto& f:filters)f.reset();
        for(auto& f:up)f.reset();
        for(auto& f:down)f.reset();
        envelope=0;wet=1;seed=false;dirty=true;countdown=0;
        for(auto& d:dc)d=0;
    }
    void process(juce::AudioBuffer<float>& b,const float* knobs,bool active) override {
        const int ch=std::min(2,b.getNumChannels()),n=b.getNumSamples();if(ch==0||n==0)return;
        const int nk=info(type).numKnobs;
        float targets[kMaxKnobs]{};
        for(int j=0;j<nk;++j)targets[j]=std::isfinite(knobs[j])?std::clamp(knobs[j],0.f,1.f):defaultNorm(type,j);
        if(!seed){for(int j=0;j<nk;++j)current[j]=targets[j];wet=active?1.f:0.f;seed=true;dirty=true;countdown=0;}
        float* data[2]={b.getWritePointer(0),ch==2?b.getWritePointer(1):nullptr};
        for(int i=0;i<n;++i){
            for(int j=0;j<nk;++j){const float next=current[j]+std::clamp(targets[j]-current[j],-step,step);dirty=dirty||next!=current[j];current[j]=next;}
            if(countdown==0){if(dirty){update();dirty=false;}countdown=16;}--countdown;
            wet+=std::clamp((active?1.f:0.f)-wet,-fade,fade);
            float x[2]={sane(data[0][i]),ch==2?sane(data[1][i]):0.f},y[2]={x[0],x[1]};
            if(type==SkellDope){
                float hi[2]{};float peak=0;
                for(int c=0;c<ch;++c){hi[c]=filters[0].process(c,x[c]);peak=std::max(peak,std::abs(hi[c]));}
                envelope=peak+(envelope-peak)*(peak>envelope?attack:release);
                // Linked soft-knee HF compression with bounded compensating color.
                const float colored=(1.f+1.5f*v[3])/(1.f+6.f*v[3]*envelope);
                for(int c=0;c<ch;++c)y[c]=(v[1]*x[c]+v[2]*colored*hi[c])*out;
            }else if(type==SkellRubyQ){
                for(int c=0;c<ch;++c){for(auto& f:filters)y[c]=f.process(c,y[c]);}
            }else{
                for(int c=0;c<ch;++c){
                    const float in=x[c]*input;float sample=0;
                    // Zero insertion + reconstruction, waveshaping, anti-alias
                    // filtering and decimation. No fractional-delay fiction.
                    for(int j=0;j<4;++j){
                        float z=j==0?4.f*in:0.f;
                        for(auto& f:up)z=f.process(c,z);
                        const float shaped=std::tanh(drive*z+bias)-biasZero;
                        z=shaped/drive-z;
                        for(auto& f:down)z=f.process(c,z);
                        if(j==0)sample=z;
                    }
                    dc[c]+=(sample-dc[c])*dcStep;
                    const float color=sample-dc[c];
                    // Filter the nonlinear residual, not a delayed copy of the dry signal.
                    // This avoids linear dry/wet phase cancellation at small levels.
                    // At zero drive
                    // and flat tone the level-only path is sample-exact.
                    y[c]=filters[0].process(c,in+v[1]*color)*out;
                }
            }
            for(int c=0;c<ch;++c)data[c][i]=wet==0.f?x[c]:sane(x[c]+wet*(y[c]-x[c]));
        }
    }
private:
    void update(){
        for(int j=0;j<info(type).numKnobs;++j)v[j]=realOf(type,j,current);
        if(type==SkellDope){
            // Continuous interpolation of the two voiced settings avoids a
            // discontinuous filter change while automating the mode.
            const float mode=current[0];filters[0].highPass(sr,2200.f+2800.f*mode);
            attack=onePole(sr,3.f+6.f*mode);release=onePole(sr,75.f+90.f*mode);out=db2g(v[4]);
        }else if(type==SkellRubyQ){
            filters[0].lowShelf(sr,v[1],v[0]);filters[1].peaking(sr,v[3],.65,v[2]);filters[2].highShelf(sr,v[5],v[4]);
        }else{
            input=db2g(v[0]);out=db2g(v[3]);drive=1.f+5.f*v[1];
            bias=.12f*(1.f-current[5])*v[1];biasZero=std::tanh(bias);
            dcStep=1.f-onePole(sr,80);filters[0].highShelf(sr,3500.f+2500.f*current[4],v[2]-2.f*(1.f-current[4])*v[1]);
        }
    }
    const int type;double sr=48000;int countdown=0;bool seed=false,dirty=true;
    float step=1.f/960,fade=1.f/240,current[kMaxKnobs]{},v[kMaxKnobs]{},wet=1;
    float envelope=0,attack=0,release=0,input=1,out=1,drive=1,bias=0,biasZero=0,dcStep=0,dc[2]{};
    Biquad filters[3],up[2],down[2];
};
}
