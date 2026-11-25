#pragma once
#include "Dsp.h"
#include "ExtraRackModules.h"
#include "RackCompressor.h"
#include "RackProgramOpto.h"
#include "AnalogRackModules.h"
#include <SkellCore/backends/vocal/SkellAir.h>
#include <SkellCore/backends/vocal/ClassicDeEsser.h>
namespace skl {
inline float slewToward(float value,float target,float step) noexcept {
    return value+std::clamp(target-value,-step,step);
}
// Twenty millisecond full control sweep, five millisecond slot bypass fade.
class SmoothedTone final : public ModuleBase {
public:
    explicit SmoothedTone(int type) : type_(type) {}
    void prepare(double rate,int,int) override {
        sr_=std::isfinite(rate) && rate>=8000 ? rate : 48000;
        controlStep_=static_cast<float>(1/(.020*sr_));
        bypassStep_=static_cast<float>(1/(.005*sr_)); reset();
    }
    void reset() override {
        for(auto& f:filters_) f.reset();
        seeded_=false; dirty_=true; countdown_=0; wet_=1;
    }
    void process(juce::AudioBuffer<float>& buffer,const float* knobs,bool active) override {
        const int channels=std::min(2,buffer.getNumChannels());
        if(channels==0 || buffer.getNumSamples()==0) return;
        const int count=info(type_).numKnobs;
        float targets[kMaxKnobs]{};
        for(int j=0;j<count;++j)
            targets[j]=std::isfinite(knobs[j]) ? std::clamp(knobs[j],0.0f,1.0f) : defaultNorm(type_,j);
        if(!seeded_) {
            for(int j=0;j<count;++j) current_[j]=targets[j];
            wet_=active ? 1.0f : 0.0f; seeded_=true;
        }
        float* audio[2]={buffer.getWritePointer(0),channels>1 ? buffer.getWritePointer(1) : nullptr};
        for(int i=0;i<buffer.getNumSamples();++i) {
            for(int j=0;j<count;++j) {
                float next=slewToward(current_[j],targets[j],controlStep_);
                dirty_=dirty_ || next!=current_[j]; current_[j]=next;
            }
            if(countdown_==0) {
                if(dirty_) { updateFilters(); dirty_=false; }
                countdown_=16;
            }
            --countdown_;
            wet_=slewToward(wet_,active ? 1.0f : 0.0f,bypassStep_);
            for(int c=0;c<channels;++c) {
                const float dry=sane(audio[c][i]); float processed=dry;
                if(type_==Eq) {
                    const float hp=filters_[0].process(c,processed);
                    processed+=hpMix_*(hp-processed);
                    for(int band=1;band<5;++band) processed=filters_[band].process(c,processed);
                } else processed=filters_[0].process(c,processed);
                audio[c][i]=wet_==0 ? dry : dry+wet_*(processed-dry);
            }
        }
    }
private:
    void updateFilters() {
        if(type_==Eq) {
            float hp=realOf(Eq,0,current_);
            filters_[0].highPass(sr_,hp);
            hpMix_=std::clamp((hp-20)*.5f,0.0f,1.0f);
            filters_[1].lowShelf(sr_,100,realOf(Eq,1,current_));
            filters_[2].peaking(sr_,400,1,realOf(Eq,2,current_));
            filters_[3].peaking(sr_,3000,1,realOf(Eq,3,current_));
            filters_[4].highShelf(sr_,10000,realOf(Eq,4,current_));
        } else filters_[0].highShelf(sr_,realOf(Air,1,current_),realOf(Air,0,current_));
    }
    int type_,countdown_=0;
    double sr_=48000;
    float controlStep_=1.0f/960,bypassStep_=1.0f/240,current_[kMaxKnobs]{},wet_=1,hpMix_=0;
    bool seeded_=false,dirty_=true;
    Biquad filters_[5];
};
class SkellAirModule final : public ModuleBase {
public:
    void prepare(double rate,int,int) override { core.prepare(rate); }
    void reset() override { core.reset(); }
    void process(juce::AudioBuffer<float>& b,const float* k,bool active) override {
        const int channels=std::min(2,b.getNumChannels());
        if(channels==0)return;
        core.set(active?k[0]:0.f,active?k[1]:0.f,k[2],active?k[3]:.5f);
        auto* l=b.getWritePointer(0);
        auto* r=channels>1?b.getWritePointer(1):nullptr;
        for(int i=0;i<b.getNumSamples();++i) {
            const auto y=core.process(l[i],r?r[i]:l[i]);
            l[i]=y[0];if(r)r[i]=y[1];
        }
    }
private:
    skell::research::SkellAir core;
};
class SkellClassicDeEsserModule final : public ModuleBase {
public:
    void prepare(double rate,int,int) override { core.prepare(rate); }
    void reset() override { core.reset(); reduction.store(0,std::memory_order_relaxed); }
    float meter() const override { return reduction.load(std::memory_order_relaxed); }
    void process(juce::AudioBuffer<float>& b,const float* k,bool active) override {
        const int channels=std::min(2,b.getNumChannels());
        if(channels==0)return;
        const int det=int(realOf(SkellDeEsserFX,5,k));
        core.set(realOf(SkellDeEsserFX,0,k),realOf(SkellDeEsserFX,1,k),
                 active?realOf(SkellDeEsserFX,2,k):0.f,realOf(SkellDeEsserFX,3,k),
                 realOf(SkellDeEsserFX,4,k)>.5f,(det&1)!=0,active && det>=2);
        auto* l=b.getWritePointer(0);
        auto* r=channels>1?b.getWritePointer(1):nullptr;
        for(int i=0;i<b.getNumSamples();++i) {
            const auto y=core.process(l[i],r?r[i]:l[i]);
            l[i]=y[0];if(r)r[i]=y[1];
        }
        reduction.store(core.reductionDb(),std::memory_order_relaxed);
    }
private:
    skell::research::ClassicDeEsser core;
    std::atomic<float> reduction{0};
};
inline std::unique_ptr<ModuleBase> makeRackModule(int type) {
    if(type==SkellDeEsserFX)return std::make_unique<SkellClassicDeEsserModule>();
    if(type==SkellAirFX)return std::make_unique<SkellAirModule>();
    if(type==Eq || type==Air) return std::make_unique<SmoothedTone>(type);
    if(type==Tilt || type==Stereo) return std::make_unique<ExtraRackModule>(type);
    if(type==FastComp || type==BusComp) return std::make_unique<RackCompressor>(type);
    if(type==ProgramOpto) return std::make_unique<RackProgramOpto>();
    if(type>=Gate && type<kNumRackTypes) return std::make_unique<AnalogRackModule>(type);
    return makeModule(type);
}
} // namespace skl

