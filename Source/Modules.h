#pragma once
// Skelliere Rack: the single table describing every module. DSP and UI both read it,
// so a knob's range, default and printed legend are defined exactly once.
// Pure C++17, no JUCE, so it can be unit-tested off-host.
#include <algorithm>
#include <array>
#include <cmath>

namespace skl {

constexpr int kNumSlots = 8;
constexpr int kMaxKnobs = 6;

enum ModuleType : int {
    Empty = 0, Eq, Comp, DeEss, Sat, Air, Chorus, Delay, Reverb, Limiter, Trim, Tune,
    kNumTypes
};

enum class Unit { Db, Hz, Ms, Pct, Ratio, Sec, LfoHz, Pan, Steps };
enum Category : int { CatNone = 0, CatDynamics, CatTone, CatSpace, CatUtility };

struct Knob {
    const char* label;
    float lo, hi;
    bool log;
    float def;          // real-world default
    Unit unit;
    int steps;          // >0: stepped, value is the step index 0..steps-1
    const char* names;  // comma separated names for stepped knobs
};

struct Section { const char* legend; int first; int count; };

struct ModuleInfo {
    const char* name;
    const char* sub;
    const char* desc;
    Category cat;
    int numKnobs;
    Knob knobs[kMaxKnobs];
    int numSections;
    Section sections[3];
    bool hasVU;
};

inline const ModuleInfo& info(int type) {
    static const ModuleInfo table[kNumTypes] = {
        { "Empty", "", "", CatNone, 0, {}, 0, {}, false },
        { "Quad EQ", "4-band EQ with high-pass",
          "High-pass filter, low shelf, two peaking mids and a high shelf.", CatTone, 5,
          { {"HPF",20,300,true,20,Unit::Hz,0,nullptr}, {"Low",-12,12,false,0,Unit::Db,0,nullptr},
            {"Lo Mid",-12,12,false,0,Unit::Db,0,nullptr}, {"Hi Mid",-12,12,false,0,Unit::Db,0,nullptr},
            {"High",-12,12,false,0,Unit::Db,0,nullptr} },
          2, { {"Filter",0,1}, {"Gain",1,4} }, false },
        { "Opto C", "Smooth vocal compressor",
          "Forgiving compressor with threshold, ratio, attack, release and makeup gain.", CatDynamics, 5,
          { {"Thresh",-60,0,false,-24,Unit::Db,0,nullptr}, {"Ratio",1,20,true,3,Unit::Ratio,0,nullptr},
            {"Attack",1,100,true,12,Unit::Ms,0,nullptr}, {"Release",20,1000,true,150,Unit::Ms,0,nullptr},
            {"Makeup",0,24,false,3,Unit::Db,0,nullptr} },
          3, { {"Input",0,2}, {"Time",2,2}, {"Output",4,1} }, true },
        { "Sibilant", "Phrase-aware de-esser",
          "Learns how loud S sounds are in each phrase and turns down only the ones that stick out.", CatDynamics, 3,
          { {"Freq",3000,12000,true,6500,Unit::Hz,0,nullptr}, {"Thresh",-60,-10,false,-30,Unit::Db,0,nullptr},
            {"Amount",1,12,false,6,Unit::Ratio,0,nullptr} },
          2, { {"Detect",0,2}, {"Reduce",2,1} }, true },
        { "Warm Drive", "Tube-style saturator",
          "Soft saturation with a touch of asymmetry, a tone control and a mix knob for parallel use.", CatTone, 3,
          { {"Drive",0,24,false,6,Unit::Db,0,nullptr}, {"Tone",2000,18000,true,12000,Unit::Hz,0,nullptr},
            {"Mix",0,1,false,0.5f,Unit::Pct,0,nullptr} },
          2, { {"Drive",0,2}, {"Blend",2,1} }, false },
        { "Air Lift", "High-shelf enhancer",
          "A wide high shelf for opening the top end. Pick the corner, then add shine.", CatTone, 2,
          { {"Air",0,12,false,4,Unit::Db,0,nullptr}, {"Freq",6000,16000,true,12000,Unit::Hz,0,nullptr} },
          1, { {"Shelf",0,2} }, false },
        { "Twin Chorus", "Two-voice chorus",
          "Two modulated delay voices at slightly different rates. Subtle thickens, deep sounds classic.", CatSpace, 3,
          { {"Rate",0.1f,4,false,0.8f,Unit::LfoHz,0,nullptr}, {"Depth",0,1,false,0.5f,Unit::Pct,0,nullptr},
            {"Mix",0,1,false,0.35f,Unit::Pct,0,nullptr} },
          2, { {"Mod",0,2}, {"Blend",2,1} }, false },
        { "Tape Echo", "Tone-shaped feedback delay",
          "One delay line with feedback and a low-pass tone control so repeats sit behind the vocal.", CatSpace, 4,
          { {"Time",20,1000,true,320,Unit::Ms,0,nullptr}, {"Feedback",0,0.9f,false,0.3f,Unit::Pct,0,nullptr},
            {"Tone",1500,16000,true,5000,Unit::Hz,0,nullptr}, {"Mix",0,1,false,0.25f,Unit::Pct,0,nullptr} },
          2, { {"Echo",0,2}, {"Output",2,2} }, false },
        { "Plate Room", "Feedback-network reverb",
          "A four-line feedback delay network with decay, pre-delay and damping.", CatSpace, 4,
          { {"Decay",0.3f,5,true,1.6f,Unit::Sec,0,nullptr}, {"Pre",0,100,false,20,Unit::Ms,0,nullptr},
            {"Damp",2000,16000,true,9000,Unit::Hz,0,nullptr}, {"Mix",0,1,false,0.2f,Unit::Pct,0,nullptr} },
          2, { {"Room",0,2}, {"Output",2,2} }, false },
        { "Brick 1", "Peak limiter",
          "Pushes the level up and stops it at a ceiling. Put it last in the chain.", CatDynamics, 3,
          { {"Gain",0,12,false,3,Unit::Db,0,nullptr}, {"Ceiling",-12,0,false,-1,Unit::Db,0,nullptr},
            {"Release",20,500,true,100,Unit::Ms,0,nullptr} },
          2, { {"Level",0,2}, {"Time",2,1} }, true },
        { "Trim", "Gain staging and pan",
          "Clean gain and pan for balancing levels between modules.", CatUtility, 2,
          { {"Gain",-24,24,false,0,Unit::Db,0,nullptr}, {"Pan",-1,1,false,0,Unit::Pan,0,nullptr} },
          1, { {"Level",0,2} }, false },
        { "Skelly Tune", "Real-time pitch correction",
          "SkellyTune pitch correction. Pick a key and scale, then set how fast notes snap. Adds a fixed latency the host compensates.", CatTone, 5,
          { {"Speed",0,200,false,25,Unit::Ms,0,nullptr}, {"Amount",0,1,false,1,Unit::Pct,0,nullptr},
            {"Mix",0,1,false,1,Unit::Pct,0,nullptr},
            {"Key",0,11,false,0,Unit::Steps,12,"C,C#,D,D#,E,F,F#,G,G#,A,A#,B"},
            {"Scale",0,2,false,0,Unit::Steps,3,"Chromatic,Major,Minor"} },
          2, { {"Correction",0,3}, {"Scale",3,2} }, false },
    };
    return table[std::clamp(type, 0, (int) kNumTypes - 1)];
}

// Parameters are stored normalised 0..1. These convert to and from real values.
inline float toReal(const Knob& k, float n) {
    n = std::clamp(n, 0.0f, 1.0f);
    if (k.steps > 0) return (float) std::lround(n * (k.steps - 1));
    if (k.log) return k.lo * std::pow(k.hi / k.lo, n);
    return k.lo + (k.hi - k.lo) * n;
}
inline float toNorm(const Knob& k, float v) {
    if (k.steps > 0) return k.steps > 1 ? std::clamp(v / (float) (k.steps - 1), 0.0f, 1.0f) : 0.0f;
    v = std::clamp(v, k.lo, k.hi);
    if (k.log) return std::log(v / k.lo) / std::log(k.hi / k.lo);
    return (v - k.lo) / (k.hi - k.lo);
}
inline float defaultNorm(int type, int knob) { return toNorm(info(type).knobs[knob], info(type).knobs[knob].def); }
inline float realOf(int type, int knob, const float* norm) { return toReal(info(type).knobs[knob], norm[knob]); }

} // namespace skl
