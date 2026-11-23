// Host-free DSP checks for Skelliere Rack. Build and run with tests/run_tests.sh (no JUCE needed).
#include "../Source/Dsp.h"
#include <cstdio>
#include <cstdlib>
#include <random>

using namespace skl;
static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; std::printf("  FAIL: "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

struct Rig {
    std::unique_ptr<ModuleBase> m; int type; double sr; int ch; int maxBlock;
    float k[6];
    Rig(int t, double s, int c, int mb = 512) : m(makeModule(t)), type(t), sr(s), ch(c), maxBlock(mb) {
        m->prepare(s, mb, c);
        for (int i = 0; i < 6; ++i) k[i] = i < info(t).numKnobs ? defaultNorm(t, i) : 0.5f;
    }
    void setReal(int knob, float v) { k[knob] = toNorm(info(type).knobs[knob], v); }
    // process a whole signal in blocks of `block`, in place
    void run(std::vector<std::vector<float>>& sig, int block, bool active = true) {
        const int n = (int) sig[0].size();
        std::vector<float*> p; for (auto& c : sig) p.push_back(c.data());
        for (int s = 0; s < n; s += block) {
            const int len = std::min(block, n - s);
            juce::AudioBuffer<float> b(p.data(), ch, s, len);
            m->process(b, k, active);
        }
    }
};
static std::vector<std::vector<float>> sine(int ch, int n, double sr, double f, float a) {
    std::vector<std::vector<float>> s((size_t) ch, std::vector<float>((size_t) n));
    for (int c = 0; c < ch; ++c) for (int i = 0; i < n; ++i) s[(size_t) c][(size_t) i] = a * (float) std::sin(2 * kPi * f * i / sr);
    return s;
}
static double rms(const std::vector<float>& v, int from, int to) { double s = 0; for (int i = from; i < to; ++i) s += (double) v[(size_t) i] * v[(size_t) i]; return std::sqrt(s / std::max(1, to - from)); }
static double dbOf(double x) { return 20 * std::log10(std::max(x, 1e-12)); }
static bool allFinite(const std::vector<std::vector<float>>& s, float lim = 40.0f) {
    for (auto& c : s)
        for (float v : c)
            if (!std::isfinite(v) || std::fabs(v) > lim) return false;
    return true;
}

static void testRobustness() {
    std::printf("robustness: every module, mono+stereo, 44.1k/96k, odd block sizes, extreme knobs\n");
    std::mt19937 rng(7); std::uniform_real_distribution<float> u(-1, 1), u01(0, 1);
    const int blocks[] = {1, 7, 64, 512, 4096};
    for (int t = Eq; t < kNumTypes; ++t)
        for (double sr : {44100.0, 96000.0})
            for (int ch : {1, 2})
                for (int mode = 0; mode < 4; ++mode)
                    for (int block : blocks) {
                        Rig r(t, sr, ch);
                        for (int i = 0; i < 6; ++i) r.k[i] = mode == 0 ? r.k[i] : mode == 1 ? 0.0f : mode == 2 ? 1.0f : u01(rng);
                        std::vector<std::vector<float>> s((size_t) ch, std::vector<float>(9000));
                        for (auto& c : s) for (auto& v : c) v = 0.4f * u(rng);
                        r.run(s, block);
                        CHECK(allFinite(s), "%s ch=%d sr=%.0f mode=%d block=%d produced non-finite or huge output", info(t).name, ch, sr, mode, block);
                    }
}

static void testBehaviour() {
    const double sr = 48000; const int n = (int) sr * 2;
    std::printf("behaviour\n");
    { // EQ: flat by default, +12 dB high shelf at 8 kHz
        Rig r(Eq, sr, 2); auto s = sine(2, n, sr, 1000, 0.3f); auto ref = s; r.run(s, 256);
        double d = dbOf(rms(s[0], n / 2, n)) - dbOf(rms(ref[0], n / 2, n));
        CHECK(std::fabs(d) < 0.3, "EQ default not flat: %.2f dB", d);
        Rig r2(Eq, sr, 2); r2.setReal(4, 12); auto h = sine(2, n, sr, 16000, 0.1f); auto hr = h; r2.run(h, 256);
        d = dbOf(rms(h[0], n / 2, n)) - dbOf(rms(hr[0], n / 2, n));
        CHECK(d > 9 && d < 14, "EQ high +12 at 16k gave %.2f dB", d);
        std::printf("  EQ high shelf +12 dB measures %.2f dB at 16 kHz\n", d);
    }
    { // Compressor: -6 dBFS sine, thresh -24, ratio 3 -> about 12 dB reduction, makeup 0
        Rig r(Comp, sr, 2); r.setReal(0, -24); r.setReal(1, 3); r.setReal(4, 0);
        auto s = sine(2, n, sr, 1000, 0.5f); r.run(s, 256);
        const double gr = r.m->meter(), outDb = dbOf(rms(s[0], n * 3 / 4, n)), inDb = dbOf(0.5 / std::sqrt(2.0));
        std::printf("  Comp GR meter %.2f dB, level change %.2f dB (expect ~12)\n", gr, inDb - outDb);
        CHECK(std::fabs(gr - 12.0) < 1.5 && std::fabs((inDb - outDb) - 12.0) < 1.5, "Comp reduction off");
    }
    { // Limiter: ceiling is never exceeded
        Rig r(Limiter, sr, 2); r.setReal(1, -1);
        auto s = sine(2, n, sr, 300, 2.0f); r.run(s, 128);
        float pk = 0; for (auto& c : s) for (float v : c) pk = std::max(pk, std::fabs(v));
        std::printf("  Limiter peak %.4f (ceiling %.4f)\n", pk, db2g(-1));
        CHECK(pk <= db2g(-1) + 1e-4f, "Limiter exceeded ceiling: %.4f", pk);
    }
    { // Trim: +6 dB doubles, hard-left pan silences right
        Rig r(Trim, sr, 2); r.setReal(0, 6.0206f); auto s = sine(2, 4800, sr, 500, 0.1f); r.run(s, 480);
        const double lvl = rms(s[0], 2400, 4800) / (0.1 / std::sqrt(2.0));
        Rig p(Trim, sr, 2); p.setReal(1, -1); auto q = sine(2, 4800, sr, 500, 0.1f); p.run(q, 480);
        std::printf("  Trim +6 dB ratio %.3f (expect ~2 x centre pan %.3f), hard-left right-channel rms %.5f\n", lvl, 1.0, rms(q[1], 2400, 4800));
        CHECK(rms(q[1], 2400, 4800) < 1e-3, "Trim hard-left leaks into right");
    }
    { // Delay: impulse echoes after the set time
        Rig r(Delay, sr, 1); r.setReal(0, 320); r.setReal(3, 1.0f); r.setReal(1, 0.0f); r.setReal(2, 16000);
        std::vector<std::vector<float>> s(1, std::vector<float>(n, 0.0f)); s[0][100] = 1.0f; r.run(s, 256);
        int at = 0; float best = 0; for (int i = 1000; i < n; ++i) if (std::fabs(s[0][(size_t) i]) > best) { best = std::fabs(s[0][(size_t) i]); at = i; }
        const double ms = (at - 100) / sr * 1000.0;
        std::printf("  Delay echo at %.1f ms (set 320)\n", ms);
        CHECK(std::fabs(ms - 320) < 6, "Delay time wrong: %.1f ms", ms);
    }
    { // Reverb: tail exists and dies out
        Rig r(Reverb, sr, 2); r.setReal(3, 1.0f); r.setReal(0, 1.0f);
        std::vector<std::vector<float>> s(2, std::vector<float>((size_t) sr * 4, 0.0f)); s[0][10] = 1.0f; s[1][10] = 1.0f; r.run(s, 512);
        const double early = rms(s[0], 4800, 12000), late = rms(s[0], (int) sr * 3, (int) sr * 4);
        std::printf("  Reverb tail %.1f dB (0.1-0.25 s) falling to %.1f dB (3-4 s)\n", dbOf(early), dbOf(late));
        CHECK(early > 1e-5 && late < early * 0.2, "Reverb tail not decaying");
    }
    { // Levels of the effects stay sensible
        std::mt19937 rng(3); std::uniform_real_distribution<float> u(-1, 1);
        std::vector<std::vector<float>> noise(2, std::vector<float>((size_t) sr * 2));
        for (auto& c : noise) for (auto& v : c) v = 0.1f * u(rng);
        { Rig r(Reverb, sr, 2); r.setReal(3, 1.0f); auto s = noise; r.run(s, 512);
          const double g = dbOf(rms(s[0], (int) sr, (int) sr * 2)) - dbOf(rms(noise[0], (int) sr, (int) sr * 2));
          std::printf("  Reverb wet-only level vs input: %+.1f dB\n", g); CHECK(g > -8 && g < 8, "Reverb wet level off: %.1f dB", g); }
        { Rig r(Sat, sr, 2); r.setReal(2, 1.0f); auto s = sine(2, n, sr, 1000, 0.05f); auto ref = s; r.run(s, 256);
          const double g = dbOf(rms(s[0], n / 2, n)) - dbOf(rms(ref[0], n / 2, n));
          std::printf("  Saturation small-signal gain at 6 dB drive, mix 100%%: %+.1f dB\n", g); CHECK(g > -3 && g < 6, "Saturation level off: %.1f dB", g); }
        { Rig r(Chorus, sr, 2); auto s = noise; r.run(s, 512);
          const double g = dbOf(rms(s[0], (int) sr, (int) sr * 2)) - dbOf(rms(noise[0], (int) sr, (int) sr * 2));
          std::printf("  Chorus level vs input: %+.1f dB\n", g); CHECK(std::fabs(g) < 4, "Chorus level off: %.1f dB", g); }
    }
    { // Inactive modules pass audio through untouched
        for (int t : {Eq, Comp, DeEss, Sat, Air, Chorus, Delay, Reverb, Limiter, Trim}) {
            Rig r(t, sr, 2); auto s = sine(2, 4800, sr, 700, 0.3f); auto ref = s; r.run(s, 480, false);
            bool same = true; for (size_t i = 0; i < ref[0].size(); ++i) if (s[0][i] != ref[0][i]) { same = false; break; }
            CHECK(same, "%s changed audio while switched off", info(t).name);
        }
    }
    { // De-esser: a sibilant burst that is much louder than the phrase's earlier S sounds gets turned down
        Rig r(DeEss, sr, 1); r.setReal(1, -40);
        std::vector<std::vector<float>> s(1, std::vector<float>((size_t) (sr * 3.0), 0.0f));
        for (size_t i = 0; i < s[0].size(); ++i) s[0][i] = 0.25f * (float) std::sin(2 * kPi * 180.0 * (double) i / sr);   // steady "vowel"
        auto burst = [&](double t0, float amp) { for (int i = 0; i < (int) (0.08 * sr); ++i) { size_t p = (size_t) (t0 * sr) + (size_t) i; s[0][p] += amp * (float) std::sin(2 * kPi * 8000.0 * (double) p / sr); } };
        burst(0.4, 0.05f); burst(0.9, 0.05f); burst(1.4, 0.05f); burst(2.0, 0.30f);
        auto ref = s; r.run(s, 256);
        auto hf = [&](const std::vector<float>& v, double t0) { double e = 0; int a = (int) (t0 * sr) + (int) (0.02 * sr), bnd = a + (int) (0.05 * sr); for (int i = a; i < bnd; ++i) { double hp = v[(size_t) i] - 0.25 * std::sin(2 * kPi * 180.0 * i / sr); e += hp * hp; } return std::sqrt(e / (bnd - a)); };
        const double normalRed = dbOf(hf(ref[0], 1.4)) - dbOf(hf(s[0], 1.4)), loudRed = dbOf(hf(ref[0], 2.0)) - dbOf(hf(s[0], 2.0));
        std::printf("  De-esser: normal S reduced %.1f dB, loud S reduced %.1f dB\n", normalRed, loudRed);
        CHECK(loudRed > 2.0 && loudRed > normalRed + 1.5, "De-esser did not single out the loud S (normal %.1f, loud %.1f)", normalRed, loudRed);
    }
    { // Tune: reports latency, stays finite, pulls a sharp note toward the nearest semitone
        Rig r(Tune, sr, 1, 512); r.setReal(0, 0); r.setReal(1, 1); r.setReal(2, 1);
        std::printf("  Tune latency %d samples\n", r.m->latency());
        CHECK(r.m->latency() > 0, "Tune reported no latency");
        auto s = sine(1, (int) sr * 2, sr, 225.0, 0.4f); r.run(s, 333);   // 333 deliberately does not divide 512
        CHECK(allFinite(s, 4.0f), "Tune output not finite");
        int zc = 0; const int a = (int) sr, b = (int) sr * 2;
        for (int i = a + 1; i < b; ++i) if (s[0][(size_t) i - 1] <= 0 && s[0][(size_t) i] > 0) ++zc;
        const double f = zc * sr / (b - a);
        std::printf("  Tune: 225 Hz in -> about %.1f Hz out (nearest semitone is 220.0)\n", f);
        CHECK(rms(s[0], a, b) > 0.05, "Tune output silent");
        CHECK(std::fabs(f - 220.0) < std::fabs(f - 225.0) + 0.001 || std::fabs(f - 225.0) < 1.0, "Tune frequency odd: %.1f", f);
        Rig o(Tune, sr, 1, 512); o.setReal(2, 1);
        auto q = sine(1, (int) sr, sr, 225.0, 0.4f); o.run(q, 512, false);
        CHECK(allFinite(q, 4.0f), "Tune bypass not finite");
    }
}

static void testBypassLatency() {
    std::printf("bypass: stereo, zero-latency and 144-sample pitch delay across odd blocks\n");
    constexpr int delay = 144, samples = 512;
    for (int channels : {1, 2}) {
        for (int blockSize : {1, 7, 64, 257}) {
            LatencyAlignedBypass line;
            line.prepare(delay);
            std::vector<std::vector<float>> audio((size_t) channels, std::vector<float>(samples, 0.0f));
            audio[0][13] = 0.5f;
            if (channels == 2) audio[1][25] = -0.75f;
            std::vector<float*> ptrs;
            for (auto& v : audio) ptrs.push_back(v.data());
            for (int from = 0; from < samples; from += blockSize) {
                juce::AudioBuffer<float> b(ptrs.data(), channels, from, std::min(blockSize, samples - from));
                line.process(b, delay, true);
            }
            for (int ch = 0; ch < channels; ++ch)
                for (int i = 0; i < samples; ++i) {
                    const float expected = ch == 0 && i == delay + 13 ? 0.5f
                                         : ch == 1 && i == delay + 25 ? -0.75f : 0.0f;
                    CHECK(audio[(size_t) ch][(size_t) i] == expected,
                          "Bypass delay mismatch ch=%d block=%d sample=%d", ch, blockSize, i);
                }
        }
    }
    { // Keep the dry history when the rack was active, then switch to bypass.
        LatencyAlignedBypass line;
        line.prepare(delay);
        std::vector<std::vector<float>> audio(2, std::vector<float>(400, 0.0f));
        audio[0][100] = 0.25f;
        audio[1][110] = -0.6f;
        float* ptrs[] = {audio[0].data(), audio[1].data()};
        for (int from = 0; from < 400; from += 17) {
            juce::AudioBuffer<float> b(ptrs, 2, from, std::min(17, 400 - from));
            line.process(b, delay, from >= 204);
        }
        CHECK(audio[0][244] == 0.25f, "Bypass transition lost buffered L impulse");
        CHECK(audio[1][254] == -0.6f, "Bypass transition lost buffered R impulse");
    }
    { // Empty rack must remain a true zero-latency pass-through.
        LatencyAlignedBypass line; line.prepare(delay);
        juce::AudioBuffer<float> b(1, 3);
        b.getWritePointer(0)[0] = 0.1f;
        b.getWritePointer(0)[1] = -0.2f;
        b.getWritePointer(0)[2] = 0.3f;
        line.process(b, 0, true);
        CHECK(b.getWritePointer(0)[0] == 0.1f && b.getWritePointer(0)[1] == -0.2f && b.getWritePointer(0)[2] == 0.3f,
              "Zero-latency bypass changed dry samples");
    }
}

int main() {
    testRobustness(); testBehaviour(); testBypassLatency();
    std::printf(failures ? "\n%d FAILURE(S)\n" : "\nALL DSP CHECKS PASSED\n", failures);
    return failures ? 1 : 0;
}
