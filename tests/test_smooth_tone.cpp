// Exercise the exact factory used by SkellierProcessor.
#include "../Source/RackModuleFactory.h"
#include <cstdio>
#include <limits>
#include <vector>
using namespace skl;
static int failures = 0;
#define VERIFY(c, m) do { if (!(c)) { std::fprintf(stderr, "%s\n", m); ++failures; } } while (0)
static float input(int i, double rate) {
    return 0.1f * static_cast<float>(std::sin(2.0 * kPi * 137.0 * i / rate) + std::sin(2.0 * kPi * 7831.0 * i / rate));
}
static std::vector<float> automated(int type, double rate, int block, int channels) {
    auto module = makeRackModule(type);
    module->prepare(rate, 4096, channels);
    const int total = 24576;
    std::vector<float> a(total), b(total);
    for (int i = 0; i < total; ++i) a[i] = b[i] = input(i, rate);
    float* pointers[2] = {a.data(), b.data()};
    const int boundaries[] = {4096, 8192, 10240, 12288, 16384, total};
    for (int event = 0, pos = 0; pos < total;) {
        while (pos == boundaries[event]) ++event;
        float knobs[kMaxKnobs]{};
        for (int j = 0; j < info(type).numKnobs; ++j)
            knobs[j] = event == 0 ? defaultNorm(type, j) : event < 4 ? 1.0f : event == 4 ? 0.0f : std::numeric_limits<float>::quiet_NaN();
        const int length = std::min(block, boundaries[event] - pos);
        juce::AudioBuffer<float> view(pointers, channels, pos, length);
        module->process(view, knobs, event != 2);
        pos += length;
    }
    for (int i = 0; i < total; ++i) {
        VERIFY(std::isfinite(a[i]) && std::fabs(a[i]) < 8.0f, "Invalid or excessive automated tone output");
        if (channels == 2) VERIFY(a[i] == b[i], "Stereo balance changed");
        if (i >= 8192 + static_cast<int>(std::ceil(0.005 * rate)) + 4 && i < 10240)
            VERIFY(a[i] == input(i, rate), "Bypass did not settle to exact dry audio");
    }
    VERIFY(module->latency() == 0, "Tone smoothing added latency");
    return a;
}
static void blockInvariance() {
    for (int type : {Eq, Air})
        for (double rate : {44100.0, 48000.0, 96000.0, 192000.0})
            for (int channels : {1, 2}) {
                const auto reference = automated(type, rate, 1, channels);
                for (int block : {7, 63, 512, 4096})
                    VERIFY(reference == automated(type, rate, block, channels), "Automation depends on host block size");
            }
}
static void staticResponse() {
    for (int type : {Eq, Air}) {
        auto oldModule = makeModule(type);
        auto newModule = makeRackModule(type);
        oldModule->prepare(48000.0, 512, 1); newModule->prepare(48000.0, 512, 1);
        float knobs[kMaxKnobs]{};
        for (int j = 0; j < info(type).numKnobs; ++j) knobs[j] = 0.7f;
        std::vector<float> a(8192), b(8192);
        for (int i = 0; i < 8192; ++i) a[i] = b[i] = input(i, 48000.0);
        float* ap[] = {a.data()}; float* bp[] = {b.data()};
        for (int offset = 0; offset < 8192; offset += 512) {
            juce::AudioBuffer<float> av(ap, 1, offset, 512), bv(bp, 1, offset, 512);
            oldModule->process(av, knobs, true); newModule->process(bv, knobs, true);
        }
        for (int i = 0; i < 8192; ++i)
            VERIFY(std::fabs(a[i] - b[i]) < 2.0e-6f, "Settled tone response changed unexpectedly");
    }
}
static void poisonRecovery() {
    for (int type : {Eq, Air}) {
        auto module = makeRackModule(type); module->prepare(48000.0, 64, 1);
        float knobs[kMaxKnobs]{};
        for (int j = 0; j < info(type).numKnobs; ++j) knobs[j] = defaultNorm(type, j);
        std::vector<float> audio(256, 0.1f);
        audio[3] = std::numeric_limits<float>::infinity();
        audio[19] = std::numeric_limits<float>::quiet_NaN();
        float* p[] = {audio.data()};