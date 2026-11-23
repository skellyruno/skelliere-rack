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
}
