#pragma once
#include "Dsp.h"
namespace skl {
// Lightweight zero-latency additions. Filters and gain are recalculated only
// when controls move, on a persistent 16-sample cadence. No callback allocation.
class ExtraRackModule final : public ModuleBase {
public:
    explicit ExtraRackModule(int type) : type_(type) {}
    void prepare(double sr,int,int) override {
        sr_=std::isfinite(sr) && sr>=8000 ? sr : 48000;
        step_=static_cast<float>(1/(.020*sr_));
        fadeStep_=static_cast<float>(1/(.005*sr_));
        reset();
    }
    void reset() override {
        low_.reset(); high_.reset();
        seeded_=false; dirty_=true; tick_=0;
        gain_=1; width_=1; wet_=1;
    }
    void process(juce::AudioBuffer<float>& b,const float* k,bool active) override {
        const int ch=std::min(2,b.getNumChannels()), n=b.getNumSamples();
        if(ch==0 || n==0) return;
        float targets[3];
        for(int j=0;j<3;++j) targets[j]=std::isfinite(k[j]) ? std::clamp(k[j],0.0f,1.0f) : defaultNorm(type_,j);
        if(!seeded_) {
            for(int j=0;j<3;++j) current_[j]=targets[j];
            wet_=active ? 1.0f : 0.0f; seeded_=true;
        }
        float* l=b.getWritePointer(0);
        float* r=ch>1 ? b.getWritePointer(1) : nullptr;
        for(int i=0;i<n;++i) {
            for(int j=0;j<3;++j) {
                float next=approach(current_[j],targets[j],step_);
                dirty_=dirty_ || next!=current_[j]; current_[j]=next;
            }
            if(tick_==0) { if(dirty_) { update(); dirty_=false; } tick_=16; }
            --tick_;
            wet_=approach(wet_,active ? 1.0f : 0.0f,fadeStep_);
            const float dl=sane(l[i]), dr=r ? sane(r[i]) : dl;
            float wl=dl,wr=dr;
            if(type_==Tilt) {
                wl=high_.process(0,low_.process(0,dl))*gain_;
                if(r) wr=high_.process(1,low_.process(1,dr))*gain_;
            } else if(r) {
                const float mid=.5f*(dl+dr);
                const float side=low_.process(0,.5f*(dl-dr))*width_;
                wl=(mid+side)*gain_; wr=(mid-side)*gain_;
            } else wl=dl*gain_;
            l[i]=wet_==0 ? dl : dl+wet_*(wl-dl);
            if(r) r[i]=wet_==0 ? dr : dr+wet_*(wr-dr);
        }
    }
private:
    static float approach(float a,float b,float step) { return a+std::clamp(b-a,-step,step); }
    void update() {
        gain_=db2g(realOf(type_,2,current_));
        if(type_==Tilt) {
            const float tilt=realOf(Tilt,0,current_), pivot=realOf(Tilt,1,current_);
            low_.lowShelf(sr_,pivot,-tilt); high_.highShelf(sr_,pivot,tilt);
        } else {
            width_=realOf(Stereo,0,current_);
            low_.highPass(sr_,realOf(Stereo,1,current_));
        }
    }
    int type_,tick_=0;
    double sr_=48000;
    float step_=1.0f/960,fadeStep_=1.0f/240,current_[3]{},wet_=1,gain_=1,width_=1;
    bool seeded_=false,dirty_=true;
    Biquad low_,high_;
};
} // namespace skl
