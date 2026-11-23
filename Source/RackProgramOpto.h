#pragma once
#include "Dsp.h"
#include "ProgramOptoCore.h"

namespace skl {
// Optional original program-dependent linked compressor; zero added latency.
// Existing Opto C and legacy slot parameters remain untouched.
class RackProgramOpto final : public ModuleBase {
public:
    void prepare(double sampleRate, int, int) override {
        sampleRate_ = std::isfinite(sampleRate) && sampleRate >= 8000.0 ? sampleRate : 48000.0;
        core_.prepare(sampleRate_);
        controlStep_ = static_cast<float>(1.0 / (0.020 * sampleRate_));
        bypassStep_ = static_cast<float>(1.0 / (0.005 * sampleRate_));
        reset();
    }
    void reset() override {
        core_.reset();
        seeded_ = false;
        dirty_ = true;
        countdown_ = 0;
        wet_ = 1.0f;
        meter_.store(0.0f);
    }
    void process(juce::AudioBuffer<float>& buffer, const float* knobs, bool active) override {
        const int channels = std::min(2,buffer.getNumChannels());
        const int n = buffer.getNumSamples();
        if(channels == 0 || n == 0) return;
        float targets[6];
        for(int j=0;j<6;++j)
            targets[j] = std::isfinite(knobs[j]) ? std::clamp(knobs[j],0.0f,1.0f) : defaultNorm(ProgramOpto,j);
        if(!seeded_) {
            for(int j=0;j<6;++j) current_[j] = targets[j];
            wet_ = active ? 1.0f : 0.0f;
            seeded_ = true;
            dirty_ = true;
        }
        float* const left = buffer.getWritePointer(0);
        float* const right = channels > 1 ? buffer.getWritePointer(1) : nullptr;
        for(int i=0;i<n;++i) {
            for(int j=0;j<6;++j) {
                const float delta = std::clamp(targets[j]-current_[j],-controlStep_,controlStep_);
                if(delta!=0.0f) dirty_=true;
                current_[j]+=delta;
            }
            if(countdown_==0) {
                if(dirty_) { updateCoefficients(); dirty_=false; }
                countdown_=16;
            }
            --countdown_;
            wet_ += std::clamp((active ? 1.0f : 0.0f)-wet_,-bypassStep_,bypassStep_);
            const float dl=sane(left[i]), dr=right ? sane(right[i]) : dl;
            float wetLeft=0.0f,wetRight=0.0f;
            core_.processPair(dl,dr,right!=nullptr,wetLeft,wetRight);
            left[i]=dl+wet_*(wetLeft-dl);
            if(right) right[i]=dr+wet_*(wetRight-dr);
        }
        meter_.store(active ? core_.gainReductionDb()*wet_ : 0.0f);
    }
    float meter() const override { return meter_.load(); }
private:
    void updateCoefficients() noexcept {
        core_.configure(realOf(ProgramOpto,0,current_), realOf(ProgramOpto,1,current_),
                        realOf(ProgramOpto,2,current_), realOf(ProgramOpto,3,current_),
                        realOf(ProgramOpto,4,current_), realOf(ProgramOpto,5,current_));
    }
    ProgramOptoCore core_;
    double sampleRate_=48000.0;
    float current_[6]{},controlStep_=1.0f/960.0f,bypassStep_=1.0f/240.0f,wet_=1.0f;
    int countdown_=0;
    bool seeded_=false,dirty_=true;
    std::atomic<float> meter_{0.0f};
};
} // namespace skl
