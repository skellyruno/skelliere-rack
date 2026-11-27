#pragma once
#include "Dsp.h"
namespace skl {
// Original linked dynamics; supplied artwork never selects an algorithm.
class RackCompressor final : public ModuleBase {
public:
    explicit RackCompressor(int type) : type_(type) {}
    void prepare(double rate, int, int) override {
        sr_ = std::isfinite(rate) && rate >= 8000 ? rate : 48000;
        step_ = (float) (1.0 / (.020 * sr_));
        fadeStep_ = (float) (1.0 / (.005 * sr_));
        detectorCoeff_ = onePole(sr_, type_ == BusComp ? 10.0f : 1.0f);
        reset();
    }
    void reset() override { seeded_=false; tick_=0; env_=gr_=0; wet_=1; meter_.store(0); }
    float meter() const override { return meter_.load(); }
    void process(juce::AudioBuffer<float>& b, const float* k, bool active) override {
        const int ch=std::min(2,b.getNumChannels()), n=b.getNumSamples();
        if(ch==0 || n==0) return;
        float targets[5];
        for(int j=0;j<5;++j) targets[j]=std::isfinite(k[j]) ? std::clamp(k[j],0.0f,1.0f) : defaultNorm(type_,j);
        if(!seeded_) { for(int j=0;j<5;++j) current_[j]=targets[j]; wet_=active ? 1.0f : 0.0f; seeded_=true; dirty_=true; }
        float* l=b.getWritePointer(0); float* r=ch>1 ? b.getWritePointer(1) : nullptr;
        for(int i=0;i<n;++i) {
            for(int j=0;j<5;++j) {
                const float next=approach(current_[j],targets[j],step_);
                dirty_=dirty_ || next!=current_[j]; current_[j]=next;
            }
            if(tick_==0) { if(dirty_) { update(); dirty_=false; } tick_=16; } --tick_;
            wet_=approach(wet_,active ? 1.0f : 0.0f,fadeStep_);
            const float dl=sane(l[i]), dr=r ? sane(r[i]) : dl;
            const float peak=std::max(std::fabs(dl),std::fabs(dr));
            // Double precision detector prevents overflow for finite float inputs.
            const double power=std::max((double) dl*dl,(double) dr*dr);
            env_=type_==BusComp ? detectorCoeff_*env_+(1-detectorCoeff_)*power : std::max((double)peak,detectorCoeff_*env_);
            const float level=(float) (type_==BusComp ? std::sqrt(env_) : env_);
            const float over=g2db(level)-threshold_, knee=type_==BusComp ? 9.0f : 3.0f;
            float target=0;
            if(over>=knee*.5f) target=over*slope_;
            else if(over>-knee*.5f) { const float x=over+knee*.5f; target=slope_*x*x/(2*knee); }
            const float coeff=target>gr_ ? attack_ : release_;
            gr_=coeff*gr_+(1-coeff)*target;
            const float gain=db2g(makeup_-gr_);
            l[i]=wet_==0 ? dl : dl+wet_*(dl*gain-dl);
            if(r) r[i]=wet_==0 ? dr : dr+wet_*(dr*gain-dr);
        }
        meter_.store(gr_*wet_);
    }
private:
    static float approach(float a,float b,float step) { return a+std::clamp(b-a,-step,step); }
    void update() {
        threshold_=realOf(type_,0,current_); slope_=1-1/realOf(type_,1,current_);
        attack_=onePole(sr_,realOf(type_,2,current_)); release_=onePole(sr_,realOf(type_,3,current_));
        makeup_=realOf(type_,4,current_);
    }
    int type_,tick_=0; double sr_=48000,env_=0;
    float current_[5]{},step_=1.0f/960,fadeStep_=1.0f/240,wet_=1,gr_=0;
    float detectorCoeff_=0,threshold_=0,slope_=0,attack_=0,release_=0,makeup_=0;
    bool seeded_=false,dirty_=true; std::atomic<float> meter_{0};
};
// First-order antiderivative antialiasing of the *color residual* only.
// The unshifted compressed path remains direct. Independent of vendor code.
inline float fetColorResidual(float v,float previous,float drive) noexcept {
    const double a=double(v)*drive,b=double(previous)*drive,d=a-b;
    auto primitive=[](double z){const double q=std::abs(z);return q+std::log1p(std::exp(-2*q))-std::log(2.)-.5*z*z;};
    const double m=.5*(a+b);
    return float((std::abs(d)<1e-4?std::tanh(m)-m:(primitive(a)-primitive(b))/d)/drive);
}
class FetCompressor final : public ModuleBase {
public:
    void prepare(double sr,int,int) override {rate=std::isfinite(sr)?std::clamp(sr,8000.,384000.):48000.;slew=onePole(rate,12);bypassSlew=onePole(rate,3);reset();}
    void reset() override {env=memory=gr=0;mix=1;seeded=false;shapeSeeded=false;previous.fill(0);tick=0;meterValue.store(0);}
    float meter() const override {return meterValue.load(std::memory_order_relaxed);}
    void process(juce::AudioBuffer<float>& b,const float* k,bool active) override {
        const int ch=std::min(2,b.getNumChannels());if(ch<1)return;
        std::array<float,6> targets{};for(int j=0;j<6;++j)targets[j]=std::isfinite(k[j])?std::clamp(k[j],0.f,1.f):defaultNorm(FastComp,j);
        targets[1]=std::round(targets[1]*4)*.25f;targets[5]=targets[5]>=.5f?1.f:0.f;
        if(!seeded){cur=targets;mix=active?1.f:0.f;seeded=true;}
        auto* l=b.getWritePointer(0);auto* r=ch==2?b.getWritePointer(1):nullptr;
        for(int i=0;i<b.getNumSamples();++i){
            for(int j=0;j<6;++j)cur[j]=targets[j]+slew*(cur[j]-targets[j]);
            if((tick++&15u)==0){
                input=db2g(realOf(FastComp,0,cur.data()));output=db2g(realOf(FastComp,4,cur.data()));
                const float rev=cur[5],ratioIndex=cur[1]*4;const int ri=std::min(3,int(ratioIndex));static constexpr float ratios[]={4,8,12,20,32};
                const float ratio=ratios[ri]+(ratios[ri+1]-ratios[ri])*(ratioIndex-ri);slope=1-1/ratio;slam=std::clamp(ratioIndex-3.f,0.f,1.f);
                threshold=-18.f+rev*2.f-slam*4.f;knee=2.f+rev*2.f+slam*3.f;
                atk=float(std::exp(-1000.0/(rate*realOf(FastComp,2,cur.data())*(1.f+.25f*rev+.65f*slam))));
                const float release=realOf(FastComp,3,cur.data());rel=onePole(rate,release);slow=onePole(rate,release*(3.f+rev*1.6f));
                color=.12f-.07f*rev+.1f*slam;drive=1.5f-.5f*rev+.65f*slam;
            }
            const float x=sane(l[i]),y=r?sane(r[i]):x;
            const float a=std::clamp(x,-64.f,64.f)*input,bb=std::clamp(y,-64.f,64.f)*input;
            const float peak=std::max(std::abs(a),std::abs(bb));env=std::max(peak,env*rel);
            const float over=g2db(env)-threshold;float want=0;
            if(over>=knee*.5f)want=over*slope;else if(over>-.5f*knee){const float t=over+.5f*knee;want=slope*t*t/(2*knee);}
            want=std::min(want,60.f);memory=want+slow*(memory-want);
            const float recovery=rel+(slow-rel)*std::clamp(memory/24.f,0.f,1.f);
            gr=want+(want>gr?atk:recovery)*(gr-want);
            const float gain=db2g(-gr);const float m=active?1.f:0.f;mix=m+bypassSlew*(mix-m);if(std::abs(mix-m)<1e-5f)mix=m;
            auto shape=[&](float v,int channel){v*=gain;const float old=shapeSeeded?previous[channel]:v;previous[channel]=v;return output*(v+color*fetColorResidual(v,old,drive));};
            // Keep color history current even when bypassed, avoiding stale-state pops.
            const float outL=shape(a,0),outR=r?shape(bb,1):outL;if(!r)previous[1]=previous[0];shapeSeeded=true;
            l[i]=mix==0?x:sane(x+mix*(outL-x));if(r)r[i]=mix==0?y:sane(y+mix*(outR-y));
        }
        meterValue.store(gr*mix,std::memory_order_relaxed);
    }
private:
    double rate=48000;std::array<float,6> cur{};std::array<float,2> previous{};unsigned tick=0;bool seeded=false,shapeSeeded=false;
    float env=0,memory=0,gr=0,mix=1,slew=0,bypassSlew=0,input=1,output=1,slope=.75f,slam=0,threshold=-18,knee=2,atk=0,rel=0,slow=0,color=.12f,drive=1.5f;
    std::atomic<float> meterValue{0};
};

}
