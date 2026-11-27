#pragma once
// Skelliere Rack DSP. One class per module, all real-time safe: every allocation happens
// in prepare(), nothing allocates, locks or logs inside process().
// Knob values arrive normalised 0..1 and are converted with the table in Modules.h.
#include <JuceHeader.h>
#include <SkellCore/Core.h>
#include <SkellCore/backends/SkellyTuneBackend.h>
#include <SkellCore/backends/deesser/PhraseAwareDetector.h>
#include "Modules.h"
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

namespace skl {

constexpr double kPi = 3.14159265358979323846;
inline float db2g(float d) { return std::exp(d * 0.11512925465f); }
inline float g2db(float g) { return 20.0f * std::log10(std::max(g, 1.0e-6f)); }
inline float sane(float v) { return std::isfinite(v) ? v : 0.0f; }
inline float onePole(double sr, float ms) { return (float) std::exp(-1.0 / (std::max(0.05, (double) ms) * 0.001 * sr)); }

// ---------------------------------------------------------------- filters
struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1[2] = {0, 0}, z2[2] = {0, 0};
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    void set(double nb0, double nb1, double nb2, double a0, double na1, double na2) {
        b0 = nb0 / a0; b1 = nb1 / a0; b2 = nb2 / a0; a1 = na1 / a0; a2 = na2 / a0;
    }
    static double clampF(double sr, double f) { return std::clamp(f, 10.0, sr * 0.45); }
    void lowPass(double sr, double f, double q = 0.70710678) {
        const double w = 2 * kPi * clampF(sr, f) / sr, c = std::cos(w), s = std::sin(w), al = s / (2 * q);
        set((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void highPass(double sr, double f, double q = 0.70710678) {
        const double w = 2 * kPi * clampF(sr, f) / sr, c = std::cos(w), s = std::sin(w), al = s / (2 * q);
        set((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void peaking(double sr, double f, double q, double dB) {
        const double A = std::pow(10.0, dB / 40), w = 2 * kPi * clampF(sr, f) / sr, c = std::cos(w), s = std::sin(w), al = s / (2 * q);
        set(1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void lowShelf(double sr, double f, double dB) {
        const double A = std::pow(10.0, dB / 40), w = 2 * kPi * clampF(sr, f) / sr, c = std::cos(w), s = std::sin(w);
        const double beta = 2 * std::sqrt(A) * (s / std::sqrt(2.0));
        set(A * ((A + 1) - (A - 1) * c + beta), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - beta),
            (A + 1) + (A - 1) * c + beta, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - beta);
    }
    void highShelf(double sr, double f, double dB) {
        const double A = std::pow(10.0, dB / 40), w = 2 * kPi * clampF(sr, f) / sr, c = std::cos(w), s = std::sin(w);
        const double beta = 2 * std::sqrt(A) * (s / std::sqrt(2.0));
        set(A * ((A + 1) + (A - 1) * c + beta), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - beta),
            (A + 1) - (A - 1) * c + beta, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - beta);
    }
    inline float process(int ch, float x) {
        const double in = x, y = b0 * in + z1[ch];
        z1[ch] = b1 * in - a1 * y + z2[ch];
        z2[ch] = b2 * in - a2 * y;
        return (float) y;
    }
};

struct DelayLine {
    std::vector<float> buf; int w = 0;
    void prepare(int size) { buf.assign((size_t) std::max(8, size), 0.0f); w = 0; }
    void clear() { std::fill(buf.begin(), buf.end(), 0.0f); w = 0; }
    inline void write(float x) { buf[(size_t) w] = x; if (++w >= (int) buf.size()) w = 0; }
    inline float read(float d) const {  // d samples behind the last written sample, linear interpolation
        const int n = (int) buf.size();
        d = std::clamp(d, 1.0f, (float) (n - 2));
        const int i = (int) d; const float fr = d - (float) i;
        int p0 = w - 1 - i; if (p0 < 0) p0 += n;
        int p1 = p0 - 1; if (p1 < 0) p1 += n;
        return buf[(size_t) p0] * (1 - fr) + buf[(size_t) p1] * fr;
    }
};

// ---------------------------------------------------------------- latency-aligned rack bypass
// Keeps a shadow copy of the untouched input running even while the rack is active.
// Bypass All can then output the same host-compensated latency as the Tune path,
// without allocations or reconfiguring the delay line inside the audio callback.
class LatencyAlignedBypass {
public:
    void prepare(int maximumDelaySamples) {
        maxDelay = std::max(0, maximumDelaySamples);
        for (auto& line : lines) line.prepare(maxDelay + 8);
    }
    void reset() { for (auto& line : lines) line.clear(); }
    void process(juce::AudioBuffer<float>& b, int delaySamples, bool bypass) {
        const int delay = std::clamp(delaySamples, 0, maxDelay);
        const int channels = std::min(2, b.getNumChannels());
        for (int ch = 0; ch < channels; ++ch) {
            auto& line = lines[ch];
            float* data = b.getWritePointer(ch);
            for (int i = 0; i < b.getNumSamples(); ++i) {
                const float input = data[i];
                line.write(input);
                if (bypass)
                    data[i] = delay == 0 ? input : line.read((float) delay);
            }
        }
    }
private:
    DelayLine lines[2];
    int maxDelay = 0;
};

// ---------------------------------------------------------------- base
class ModuleBase {
public:
    virtual ~ModuleBase() = default;
    virtual void prepare(double sr, int maxBlock, int channels) = 0;
    virtual void reset() = 0;
    // k = six normalised knob values. active=false means the slot is switched off.
    virtual void process(juce::AudioBuffer<float>& b, const float* k, bool active) = 0;
    virtual float meter() const { return 0.0f; }   // gain reduction in dB, read by the UI thread
    virtual int latency() const { return 0; }
    virtual void setTempo(double) noexcept {}
};

// ---------------------------------------------------------------- Quad EQ
class QuadEq final : public ModuleBase {
public:
    void prepare(double s, int, int) override { sr = s; reset(); }
    void reset() override { for (auto& f : f_) f.reset(); }
    void process(juce::AudioBuffer<float>& b, const float* k, bool active) override {
        if (!active) return;
        const float hpf = realOf(Eq, 0, k);
        useHp = hpf > 22.0f;
        if (useHp) f_[0].highPass(sr, hpf);
        f_[1].lowShelf(sr, 100.0, realOf(Eq, 1, k));
        f_[2].peaking(sr, 400.0, 1.0, realOf(Eq, 2, k));
        f_[3].peaking(sr, 3000.0, 1.0, realOf(Eq, 3, k));
        f_[4].highShelf(sr, 10000.0, realOf(Eq, 4, k));
        const int n = b.getNumSamples();
        for (int c = 0; c < std::min(2, b.getNumChannels()); ++c) {
            float* d = b.getWritePointer(c);
            for (int i = 0; i < n; ++i) {
                float x = d[i];
                if (useHp) x = f_[0].process(c, x);
                for (int s = 1; s < 5; ++s) x = f_[s].process(c, x);
                d[i] = x;
            }
        }
    }
private:
    double sr = 48000; bool useHp = false; Biquad f_[5];
};

// ---------------------------------------------------------------- Compressor
class Compressor final : public ModuleBase {
public:
    void prepare(double s, int, int) override { sr = s; reset(); }
    void reset() override { gr = 0; pk = 0; meterV.store(0); }
    void process(juce::AudioBuffer<float>& b, const float* k, bool active) override {
        if (!active) { meterV.store(0); return; }
        const float thresh = realOf(Comp, 0, k), ratio = realOf(Comp, 1, k);
        const float att = onePole(sr, realOf(Comp, 2, k)), rel = onePole(sr, realOf(Comp, 3, k));
        const float makeup = realOf(Comp, 4, k), slope = 1.0f - 1.0f / std::max(1.0f, ratio);
        const float follow = onePole(sr, 8.0f);   // peak follower: instant attack, 8 ms fall, so low notes don't ripple the gain
        const int n = b.getNumSamples(), ch = std::min(2, b.getNumChannels());
        float* d[2] = {b.getWritePointer(0), ch > 1 ? b.getWritePointer(1) : nullptr};
        for (int i = 0; i < n; ++i) {
            float in = std::fabs(d[0][i]);
            if (ch > 1) in = std::max(in, std::fabs(d[1][i]));
            pk = std::max(in, pk * follow);
            const float over = g2db(pk) - thresh;
            float tgt;
            if (over <= -3.0f) tgt = 0.0f;
            else if (over >= 3.0f) tgt = over * slope;
            else { const float x = over + 3.0f; tgt = slope * x * x / 12.0f; }
            gr = tgt > gr ? att * gr + (1 - att) * tgt : rel * gr + (1 - rel) * tgt;
            const float g = db2g(makeup - gr);
            for (int c = 0; c < ch; ++c) d[c][i] *= g;
        }
        meterV.store(gr);
    }
    float meter() const override { return meterV.load(); }
private:
    double sr = 48000; float gr = 0, pk = 0; std::atomic<float> meterV{0};
};

// ---------------------------------------------------------------- De-esser
// Split-band (Linkwitz-Riley 4th order). The high band is turned down only when its level
// exceeds what the phrase-aware detector learned as "normal" sibilance for the current phrase.
class DeEsser final : public ModuleBase {
public:
    void prepare(double s, int, int) override { sr = s; det.prepare(s); reset(); }
    void reset() override { for (auto& f : lp) f.reset(); for (auto& f : hp) f.reset(); det.reset(); env = 0; gr = 0; meterV.store(0); }
    void process(juce::AudioBuffer<float>& b, const float* k, bool active) override {
        if (!active) { meterV.store(0); return; }
        const float freq = realOf(DeEss, 0, k), thr = realOf(DeEss, 1, k), ratio = realOf(DeEss, 2, k);
        for (int s = 0; s < 2; ++s) { lp[s].lowPass(sr, freq); hp[s].highPass(sr, freq); }
        const float slope = 1.0f - 1.0f / std::max(1.0f, ratio);
        const float envA = onePole(sr, 2.0f), envR = onePole(sr, 16.0f), grA = onePole(sr, 1.0f), grR = onePole(sr, 40.0f);
        const int n = b.getNumSamples(), ch = std::min(2, b.getNumChannels());
        float* d[2] = {b.getWritePointer(0), ch > 1 ? b.getWritePointer(1) : nullptr};
        for (int i = 0; i < n; ++i) {
            float low[2] = {0, 0}, high[2] = {0, 0};
            double broad = 0, det_ = 0;
            for (int c = 0; c < ch; ++c) {
                const float x = d[c][i];
                low[c] = lp[1].process(c, lp[0].process(c, x));
                high[c] = hp[1].process(c, hp[0].process(c, x));
                broad = std::max(broad, (double) std::fabs(x));
                det_ = std::max(det_, (double) std::fabs(high[c]));
            }
            const float da = (float) det_;
            env = da > env ? envA * env + (1 - envA) * da : envR * env + (1 - envR) * da;
            const auto dec = det.process(broad, det_, thr, 6.0);
            const float level = g2db(env);
            // Before the first S of a phrase is learned, fall back to the Thresh knob as an absolute level.
            const float limit = dec.eligible ? (float) dec.thresholdDb : (dec.ready ? 1.0e9f : thr);
            const float tgt = std::max(0.0f, (level - limit) * slope);
            gr = tgt > gr ? grA * gr + (1 - grA) * tgt : grR * gr + (1 - grR) * tgt;
            const float g = db2g(-gr);
            for (int c = 0; c < ch; ++c) d[c][i] = low[c] + high[c] * g;
        }
        meterV.store(gr);
    }
    float meter() const override { return meterV.load(); }
private:
    double sr = 48000; Biquad lp[2], hp[2]; skellcore::deesser::PhraseAwareDetector det;
    float env = 0, gr = 0; std::atomic<float> meterV{0};
};

// ---------------------------------------------------------------- Saturation
class Saturator final : public ModuleBase {
public:
    void prepare(double s, int, int) override { sr = s; reset(); }
    void reset() override { lp[0] = lp[1] = 0; xp[0] = xp[1] = yp[0] = yp[1] = 0; }
    void process(juce::AudioBuffer<float>& b, const float* k, bool active) override {
        if (!active) return;
        const float pre = db2g(realOf(Sat, 0, k)), tone = realOf(Sat, 1, k), mix = realOf(Sat, 2, k);
        const float a = (float) std::exp(-2.0 * kPi * tone / sr), wetG = 0.5f / std::sqrt(pre), off = std::tanh(0.2f);
        const int n = b.getNumSamples();
        for (int c = 0; c < std::min(2, b.getNumChannels()); ++c) {
            float* d = b.getWritePointer(c);
            for (int i = 0; i < n; ++i) {
                const float x = d[i];
                float y = (std::tanh(2.0f * x * pre + 0.2f) - off) / 0.97f;
                // remove the DC the asymmetric curve creates
                const float hpOut = y - xp[c] + 0.995f * yp[c]; xp[c] = y; yp[c] = hpOut; y = hpOut;
                lp[c] = (1 - a) * y + a * lp[c];
                d[i] = x * (1 - mix) + lp[c] * wetG * mix;
            }
        }
    }
private:
    double sr = 48000; float lp[2] = {0, 0}, xp[2] = {0, 0}, yp[2] = {0, 0};
};

// ---------------------------------------------------------------- Air
class AirShelf final : public ModuleBase {
public:
    void prepare(double s, int, int) override { sr = s; reset(); }
    void reset() override { f.reset(); }
    void process(juce::AudioBuffer<float>& b, const float* k, bool active) override {
        if (!active) return;
        f.highShelf(sr, realOf(Air, 1, k), realOf(Air, 0, k));
        const int n = b.getNumSamples();
        for (int c = 0; c < std::min(2, b.getNumChannels()); ++c) { float* d = b.getWritePointer(c); for (int i = 0; i < n; ++i) d[i] = f.process(c, d[i]); }
    }
private:
    double sr = 48000; Biquad f;
};

// ---------------------------------------------------------------- Chorus
class TwinChorus final : public ModuleBase {
public:
    void prepare(double s, int, int) override { sr = s; for (auto& d : dl) d.prepare((int) (s * 0.06) + 8); reset(); }
    void reset() override { for (auto& d : dl) d.clear(); ph[0] = 0.0f; ph[1] = 0.33f; }
    void process(juce::AudioBuffer<float>& b, const float* k, bool active) override {
        if (!active) return;
        const float rate = realOf(Chorus, 0, k), depth = realOf(Chorus, 1, k), mix = realOf(Chorus, 2, k);
        const int n = b.getNumSamples(), ch = std::min(2, b.getNumChannels());
        const float inc = (float) (rate / sr), sec = (float) sr;
        for (int c = 0; c < ch; ++c) {
            float* d = b.getWritePointer(c);
            float p0 = ph[0] + (c ? 0.25f : 0.0f), p1 = ph[1] + (c ? 0.25f : 0.0f);
            for (int i = 0; i < n; ++i) {
                const float x = d[i];
                dl[c].write(x);
                const float d0 = (0.018f + depth * 0.006f * std::sin(6.2831853f * p0)) * sec;
                const float d1 = (0.024f + depth * 0.006f * std::sin(6.2831853f * p1)) * sec;
                const float wet = 0.7f * (dl[c].read(d0) + dl[c].read(d1));
                d[i] = x * (1 - mix) + wet * mix;
                p0 += inc; if (p0 >= 1) p0 -= 1;
                p1 += inc * 1.17f; if (p1 >= 1) p1 -= 1;
            }
        }
        ph[0] = std::fmod(ph[0] + inc * (float) n, 1.0f);
        ph[1] = std::fmod(ph[1] + inc * 1.17f * (float) n, 1.0f);
    }
private:
    double sr = 48000; DelayLine dl[2]; float ph[2] = {0, 0.33f};
};

// ---------------------------------------------------------------- Delay
class TapeEcho final : public ModuleBase {
public:
    void prepare(double s, int, int) override { sr = s; for (auto& d : dl) d.prepare((int) (s * 1.05) + 8); reset(); }
    void reset() override { for (auto& d : dl) d.clear(); lp[0] = lp[1] = 0; cur = -1; }
    void process(juce::AudioBuffer<float>& b, const float* k, bool active) override {
        if (!active) return;
        const float target = realOf(Delay, 0, k) * 0.001f * (float) sr, fb = realOf(Delay, 1, k), tone = realOf(Delay, 2, k), mix = realOf(Delay, 3, k);
        if (cur < 0) cur = target;
        const float a = (float) std::exp(-2.0 * kPi * tone / sr);
        const int n = b.getNumSamples(), ch = std::min(2, b.getNumChannels());
        for (int c = 0; c < ch; ++c) {
            float* d = b.getWritePointer(c); float t = cur;
            for (int i = 0; i < n; ++i) {
                t += (target - t) * 0.0008f;
                const float x = d[i], echo = dl[c].read(t);
                lp[c] = (1 - a) * echo + a * lp[c];
                dl[c].write(sane(x + lp[c] * fb));
                d[i] = x * (1 - mix) + lp[c] * mix;
            }
            if (c == ch - 1) cur = t;
        }
    }
private:
    double sr = 48000; DelayLine dl[2]; float lp[2] = {0, 0}, cur = -1;
};

// ---------------------------------------------------------------- Reverb (SkellCore FDN)
class PlateRoom final : public ModuleBase {
public:
    void prepare(double s, int, int) override { sr = s; fdn.prepare(s); for (auto& d : pre) d.prepare((int) (s * 0.11) + 8); reset(); }
    void reset() override { fdn.reset(); for (auto& d : pre) d.clear(); }
    void process(juce::AudioBuffer<float>& b, const float* k, bool active) override {
        if (!active) return;
        const float decay = realOf(Reverb, 0, k), preMs = realOf(Reverb, 1, k), dampHz = realOf(Reverb, 2, k), mix = realOf(Reverb, 3, k);
        const float damping = std::clamp(0.9f * (1.0f - std::log(dampHz / 2000.0f) / std::log(8.0f)), 0.0f, 0.95f);
        fdn.setParameters(decay, damping, 1.0f);  // wet only; dry/wet is mixed here so pre-delay never delays the dry
        const float pd = preMs * 0.001f * (float) sr;
        const int n = b.getNumSamples(), ch = std::min(2, b.getNumChannels());
        float* d0 = b.getWritePointer(0); float* d1 = ch > 1 ? b.getWritePointer(1) : nullptr;
        for (int i = 0; i < n; ++i) {
            const float l = d0[i], r = d1 ? d1[i] : l;
            pre[0].write(l); pre[1].write(r);
            const auto w = fdn.processFrame(pre[0].read(pd + 1.0f), pre[1].read(pd + 1.0f));
            d0[i] = l * (1 - mix) + w[0] * mix * 2.4f;
            if (d1) d1[i] = r * (1 - mix) + w[1] * mix * 2.4f;
        }
    }
private:
    double sr = 48000; skellcore::FDNReverb fdn; DelayLine pre[2];
};

// ---------------------------------------------------------------- Limiter (zero latency)
class Brick final : public ModuleBase {
public:
    void prepare(double s, int, int) override { sr = s; reset(); }
    void reset() override { g = 1; meterV.store(0); }
    void process(juce::AudioBuffer<float>& b, const float* k, bool active) override {
        if (!active) { meterV.store(0); return; }
        const float in = db2g(realOf(Limiter, 0, k)), ceil = db2g(realOf(Limiter, 1, k)), rel = onePole(sr, realOf(Limiter, 2, k));
        const int n = b.getNumSamples(), ch = std::min(2, b.getNumChannels());
        float* d[2] = {b.getWritePointer(0), ch > 1 ? b.getWritePointer(1) : nullptr};
        float worst = 1.0f;
        for (int i = 0; i < n; ++i) {
            float pk = 0;
            for (int c = 0; c < ch; ++c) { d[c][i] *= in; pk = std::max(pk, std::fabs(d[c][i])); }
            const float tgt = pk > ceil ? ceil / pk : 1.0f;
            g = tgt < g ? tgt : rel * g + (1 - rel) * tgt;
            for (int c = 0; c < ch; ++c) d[c][i] *= g;
            worst = std::min(worst, g);
        }
        meterV.store(-g2db(worst));
    }
    float meter() const override { return meterV.load(); }
private:
    double sr = 48000; float g = 1; std::atomic<float> meterV{0};
};

// ---------------------------------------------------------------- Trim
class TrimGain final : public ModuleBase {
public:
    void prepare(double, int, int) override { reset(); }
    void reset() override { pl = pr = -1; }
    void process(juce::AudioBuffer<float>& b, const float* k, bool active) override {
        if (!active) return;
        const float g = db2g(realOf(Trim, 0, k)), pan = realOf(Trim, 1, k);
        const int ch = std::min(2, b.getNumChannels()), n = b.getNumSamples();
        const float ang = (pan + 1.0f) * 0.25f * 3.14159265f;
        const float gl = ch > 1 ? g * std::cos(ang) * 1.41421356f : g, gr = ch > 1 ? g * std::sin(ang) * 1.41421356f : g;
        if (pl < 0) { pl = gl; pr = gr; }
        for (int c = 0; c < ch; ++c) {
            float* d = b.getWritePointer(c); const float from = c ? pr : pl, to = c ? gr : gl;
            for (int i = 0; i < n; ++i) d[i] *= from + (to - from) * (float) (i + 1) / (float) n;
        }
        pl = gl; pr = gr;
    }
private:
    float pl = -1, pr = -1;
};

// ---------------------------------------------------------------- Tune (SkellyTune backend)
class SkellyTune final : public ModuleBase {
public:
    void prepare(double s, int maxBlock, int channels) override {
        maxBlock_ = std::max(1, maxBlock); ch_ = std::clamp(channels, 1, 2);
        tuner.prepare(s, ch_);
        scratch.assign((size_t) maxBlock_ * 2, 0.0f);
        lat = tuner.latencySamples(); lastValid = false;
    }
    void reset() override { tuner.reset(); lastValid = false; }
    int latency() const override { return lat; }
    void setHumanize(float v) noexcept { humanize=std::isfinite(v)?std::clamp(v,0.f,1.f):0.f; }
    void process(juce::AudioBuffer<float>& b, const float* k, bool active) override {
        const int ch = std::min(ch_, b.getNumChannels());
        skelly::Parameters p = tuner.parameters();
        p.speedMs = realOf(Tune, 0, k);
        p.amount = realOf(Tune, 1, k);
        p.vibratoPreservation=humanize;
        p.flexCents=35.f*humanize;
        p.mix = active ? realOf(Tune, 2, k) : 0.0f;   // off = fixed dry path, so latency never changes
        const int key = (int) realOf(Tune, 3, k), scale = (int) realOf(Tune, 4, k);
        p.pitchClasses = mask(key, scale);
        if (!lastValid || p.speedMs != last.speedMs || p.amount != last.amount || p.mix != last.mix || p.pitchClasses != last.pitchClasses || p.vibratoPreservation != last.vibratoPreservation || p.flexCents != last.flexCents) {
            tuner.setParameters(p); last = p; lastValid = true;
        }
        const int total = b.getNumSamples();
        for (int start = 0; start < total; start += maxBlock_) {
            const int n = std::min(maxBlock_, total - start);
            const float* in[2] = {scratch.data(), scratch.data() + maxBlock_};
            float* out[2] = {b.getWritePointer(0) + start, ch > 1 ? b.getWritePointer(1) + start : nullptr};
            for (int c = 0; c < ch; ++c) std::copy(out[c], out[c] + n, scratch.data() + (size_t) c * (size_t) maxBlock_);
            tuner.processBlock(in, out, n, ch);
        }
    }
    static std::uint16_t mask(int key, int scale) {
        static const int major[7] = {0, 2, 4, 5, 7, 9, 11}, minor[7] = {0, 2, 3, 5, 7, 8, 10};
        if (scale <= 0) return 0x0fff;
        std::uint16_t m = 0; const int* iv = scale == 1 ? major : minor;
        for (int i = 0; i < 7; ++i) m = (std::uint16_t) (m | (1u << ((key + iv[i]) % 12)));
        return m;
    }
private:
    skellcore::backends::SkellyTuneBackend tuner; std::vector<float> scratch;
    float humanize=0.f;
    skelly::Parameters last{}; bool lastValid = false; int maxBlock_ = 512, ch_ = 2, lat = 0;
};

// ---------------------------------------------------------------- factory
inline std::unique_ptr<ModuleBase> makeModule(int type) {
    switch (type) {
        case Eq: return std::make_unique<QuadEq>();
        case Comp: return std::make_unique<Compressor>();
        case DeEss: return std::make_unique<DeEsser>();
        case Sat: return std::make_unique<Saturator>();
        case Air: return std::make_unique<AirShelf>();
        case Chorus: return std::make_unique<TwinChorus>();
        case Delay: return std::make_unique<TapeEcho>();
        case Reverb: return std::make_unique<PlateRoom>();
        case Limiter: return std::make_unique<Brick>();
        case Trim: return std::make_unique<TrimGain>();
        case Tune: return std::make_unique<SkellyTune>();
        default: return nullptr;
    }
}

} // namespace skl
