#pragma once
#include "Dsp.h"

namespace skl {
// Normalised, sample-timed slew: independent of the host's block length.
// A complete control sweep takes 20 ms; slot bypass takes 5 ms.
inline float slewToward(float value, float target, float step) noexcept {
    return value + std::clamp(target - value, -step, step);
}

class SmoothedTone final : public ModuleBase {
public:
    explicit SmoothedTone(int type) : type_(type) {}
    void prepare(double rate, int, int) override {
        sr_ = std::isfinite(rate) && rate >= 8000.0 ? rate : 48000.0;
        controlStep_ = static_cast<float>(1.0 / (0.020 * sr_));
        bypassStep_ = static_cast<float>(1.0 / (0.005 * sr_));
        reset();
    }
    void reset() override {
        for (auto& f : filters_) f.reset();
        seeded_ = false;
        dirty_ = true;
        countdown_ = 0;
        wet_ = 1.0f;
    }
    void process(juce::AudioBuffer<float>& buffer, const float* knobs, bool active) override {
        const int channels = std::min(2, buffer.getNumChannels());
        if (channels == 0 || buffer.getNumSamples() == 0) return;
        const int count = info(type_).numKnobs;
        float targets[kMaxKnobs]{};
        for (int j = 0; j < count; ++j)
            targets[j] = std::isfinite(knobs[j]) ? std::clamp(knobs[j], 0.0f, 1.0f) : defaultNorm(type_, j);
        if (!seeded_) {
            for (int j = 0; j < count; ++j) current_[j] = targets[j];
            wet_ = active ? 1.0f : 0.0f;
            seeded_ = true;
        }
        float* audio[2] = {buffer.getWritePointer(0), channels > 1 ? buffer.getWritePointer(1) : nullptr};
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            for (int j = 0; j < count; ++j) {
                const float next = slewToward(current_[j], targets[j], controlStep_);
                dirty_ = dirty_ || next != current_[j];
                current_[j] = next;
            }
            // Fixed control cadence persists across callbacks, including 1-sample blocks.
            if (countdown_ == 0) {
                if (dirty_) { updateFilters(); dirty_ = false; }
                countdown_ = 16;
            }
            --countdown_;
            wet_ = slewToward(wet_, active ? 1.0f : 0.0f, bypassStep_);
            for (int c = 0; c < channels; ++c) {
                const float dry = sane(audio[c][i]);
                float processed = dry;
                if (type_ == Eq) {
                    const float highPassed = filters_[0].process(c, processed);
                    processed += hpMix_ * (highPassed - processed);
                    for (int band = 1; band < 5; ++band) processed = filters_[band].process(c, processed);
                } else {
                    processed = filters_[0].process(c, processed);
                }
                // The filter stays warm while bypassed; no stale-state burst on re-enable.
                audio[c][i] = wet_ == 0.0f ? dry : dry + wet_ * (processed - dry);
            }
        }
    }
private:
    void updateFilters() {
        if (type_ == Eq) {
            const float hp = realOf(Eq, 0, current_);
            filters_[0].highPass(sr_, hp);
            // Continuous transition out of the original 20 Hz off position.
            hpMix_ = std::clamp((hp - 20.0f) * 0.5f, 0.0f, 1.0f);
            filters_[1].lowShelf(sr_, 100.0, realOf(Eq, 1, current_));
            filters_[2].peaking(sr_, 400.0, 1.0, realOf(Eq, 2, current_));
            filters_[3].peaking(sr_, 3000.0, 1.0, realOf(Eq, 3, current_));
            filters_[4].highShelf(sr_, 10000.0, realOf(Eq, 4, current_));
        } else {
            filters_[0].highShelf(sr_, realOf(Air, 1, current_), realOf(Air, 0, current_));
        }
    }
    int type_, countdown_ = 0;
    double sr_ = 48000.0;
    float controlStep_ = 1.0f / 960.0f, bypassStep_ = 1.0f / 240.0f;
    float current_[kMaxKnobs]{}, wet_ = 1.0f, hpMix_ = 0.0f;
    bool seeded_ = false, dirty_ = true;
    Biquad filters_[5];
};

// The processor and regression tests use this exact factory. All other effects
// retain their existing algorithms, latency and parameter mapping.
inline std::unique_ptr<ModuleBase> makeRackModule(int type) {
    if (type == Eq || type == Air) return std::make_unique<SmoothedTone>(type);
    return makeModule(type);
}
} // namespace skl
