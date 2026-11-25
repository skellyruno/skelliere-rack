#pragma once
// Shared DSP/UI metadata. The original host choice remains twelve entries.
#include <algorithm>
#include <array>
#include <cmath>
namespace skl {
constexpr int kNumSlots = 8, kMaxKnobs = 6;
enum ModuleType : int {
    Empty = 0, Eq, Comp, DeEss, Sat, Air, Chorus, Delay, Reverb, Limiter, Trim, Tune,
    kNumTypes, // compatibility boundary: do not extend the legacy choice parameter
    Tilt = kNumTypes, Stereo, FastComp, BusComp, ProgramOpto, Gate, Transient, Bell, DualFilter, SoftClip, Tremolo, Phaser, Flanger, Slap, SkellAirFX, kNumRackTypes
};
// Pure selection policy shared by production and host-free regression tests.
inline int resolveRackType(float legacy, float extension) noexcept {
    const int type = std::isfinite(legacy) ? (int) std::lround(std::clamp(legacy, 0.0f, (float) kNumTypes - 1)) : Empty;
    if (type != Empty) return type;
    if (!std::isfinite(extension) || extension < 1 || extension > kNumRackTypes - kNumTypes) return Empty;
    return kNumTypes + (int) std::lround(extension) - 1;
}
enum class Unit { Db, Hz, Ms, Pct, Ratio, Sec, LfoHz, Pan, Steps, Scalar };
enum Category : int { CatNone=0, CatDynamics, CatTone, CatSpace, CatUtility };
struct Knob { const char* label; float lo,hi; bool log; float def; Unit unit; int steps; const char* names; };
struct Section { const char* legend; int first,count; };
struct ModuleInfo {
    const char* name; const char* sub; const char* desc;
    Category cat; int numKnobs; Knob knobs[kMaxKnobs];
    int numSections; Section sections[3]; bool hasVU;
};
inline const ModuleInfo& info(int type) {
    static const ModuleInfo table[kNumRackTypes] = {
        {"Empty","","",CatNone,0,{},0,{},false},
        {"Quad EQ","4-band EQ with high-pass","High-pass filter, low shelf, two peaking mids and a high shelf.",CatTone,5,
         {{"HPF",20,300,true,20,Unit::Hz,0,nullptr},{"Low",-12,12,false,0,Unit::Db,0,nullptr},
          {"Lo Mid",-12,12,false,0,Unit::Db,0,nullptr},{"Hi Mid",-12,12,false,0,Unit::Db,0,nullptr},
          {"High",-12,12,false,0,Unit::Db,0,nullptr}},2,{{"Filter",0,1},{"Gain",1,4}},false},
        {"Opto C","Smooth vocal compressor","Forgiving compressor with threshold, ratio, attack, release and makeup gain.",CatDynamics,5,
         {{"Thresh",-60,0,false,-24,Unit::Db,0,nullptr},{"Ratio",1,20,true,3,Unit::Ratio,0,nullptr},
          {"Attack",1,100,true,12,Unit::Ms,0,nullptr},{"Release",20,1000,true,150,Unit::Ms,0,nullptr},
          {"Makeup",0,24,false,3,Unit::Db,0,nullptr}},3,{{"Input",0,2},{"Time",2,2},{"Output",4,1}},true},
        {"Sibilant","Phrase-aware de-esser","Learns how loud S sounds are in each phrase and turns down only the ones that stick out.",CatDynamics,3,
         {{"Freq",3000,12000,true,6500,Unit::Hz,0,nullptr},{"Thresh",-60,-10,false,-30,Unit::Db,0,nullptr},
          {"Amount",1,12,false,6,Unit::Ratio,0,nullptr}},2,{{"Detect",0,2},{"Reduce",2,1}},true},
        {"Warm Drive","Tube-style saturator","Soft saturation with a touch of asymmetry, a tone control and a mix knob for parallel use.",CatTone,3,
         {{"Drive",0,24,false,6,Unit::Db,0,nullptr},{"Tone",2000,18000,true,12000,Unit::Hz,0,nullptr},
          {"Mix",0,1,false,.5f,Unit::Pct,0,nullptr}},2,{{"Drive",0,2},{"Blend",2,1}},false},
        {"Air Lift","High-shelf enhancer","A wide high shelf for opening the top end. Pick the corner, then add shine.",CatTone,2,
         {{"Air",0,12,false,4,Unit::Db,0,nullptr},{"Freq",6000,16000,true,12000,Unit::Hz,0,nullptr}},1,{{"Shelf",0,2}},false},
        {"Twin Chorus","Two-voice chorus","Two modulated delay voices at slightly different rates. Subtle thickens, deep sounds classic.",CatSpace,3,
         {{"Rate",.1f,4,false,.8f,Unit::LfoHz,0,nullptr},{"Depth",0,1,false,.5f,Unit::Pct,0,nullptr},
          {"Mix",0,1,false,.35f,Unit::Pct,0,nullptr}},2,{{"Mod",0,2},{"Blend",2,1}},false},
        {"Tape Echo","Tone-shaped feedback delay","One delay line with feedback and a low-pass tone control so repeats sit behind the vocal.",CatSpace,4,
         {{"Time",20,1000,true,320,Unit::Ms,0,nullptr},{"Feedback",0,.9f,false,.3f,Unit::Pct,0,nullptr},
          {"Tone",1500,16000,true,5000,Unit::Hz,0,nullptr},{"Mix",0,1,false,.25f,Unit::Pct,0,nullptr}},2,{{"Echo",0,2},{"Output",2,2}},false},
        {"Plate Room","Feedback-network reverb","A four-line feedback delay network with decay, pre-delay and damping.",CatSpace,4,
         {{"Decay",.3f,5,true,1.6f,Unit::Sec,0,nullptr},{"Pre",0,100,false,20,Unit::Ms,0,nullptr},
          {"Damp",2000,16000,true,9000,Unit::Hz,0,nullptr},{"Mix",0,1,false,.2f,Unit::Pct,0,nullptr}},2,{{"Room",0,2},{"Output",2,2}},false},
        {"Brick 1","Peak limiter","Pushes the level up and stops it at a ceiling. Put it last in the chain.",CatDynamics,3,
         {{"Gain",0,12,false,3,Unit::Db,0,nullptr},{"Ceiling",-12,0,false,-1,Unit::Db,0,nullptr},
          {"Release",20,500,true,100,Unit::Ms,0,nullptr}},2,{{"Level",0,2},{"Time",2,1}},true},
        {"Trim","Gain staging and pan","Clean gain and pan for balancing levels between modules.",CatUtility,2,
         {{"Gain",-24,24,false,0,Unit::Db,0,nullptr},{"Pan",-1,1,false,0,Unit::Pan,0,nullptr}},1,{{"Level",0,2}},false},
        {"Skelly Tune","Real-time pitch correction","SkellyTune pitch correction. Pick a key and scale, then set how fast notes snap. Adds a fixed latency the host compensates.",CatTone,5,
         {{"Speed",0,200,false,25,Unit::Ms,0,nullptr},{"Amount",0,1,false,1,Unit::Pct,0,nullptr},
          {"Mix",0,1,false,1,Unit::Pct,0,nullptr},{"Key",0,11,false,0,Unit::Steps,12,"C,C#,D,D#,E,F,F#,G,G#,A,A#,B"},
          {"Scale",0,2,false,0,Unit::Steps,3,"Chromatic,Major,Minor"}},2,{{"Correction",0,3},{"Scale",3,2}},false},
        {"Tilt Tone","Broad complementary shelves","Turn toward bright or warm around the pivot. Complementary shelves with smoothed controls and output trim.",CatTone,3,
         {{"Tilt",-6,6,false,0,Unit::Db,0,nullptr},{"Pivot",200,2000,true,800,Unit::Hz,0,nullptr},
          {"Output",-12,12,false,0,Unit::Db,0,nullptr}},2,{{"Balance",0,2},{"Output",2,1}},false},
        {"Stereo Stage","Width with mono bass","Mid/side width with a side-channel high-pass for a centered low end. Mono input remains mono; no synthetic stereo or added latency.",CatUtility,3,
         {{"Width",0,2,false,1,Unit::Pct,0,nullptr},{"Mono Bass",20,300,true,100,Unit::Hz,0,nullptr},
          {"Output",-12,12,false,0,Unit::Db,0,nullptr}},2,{{"Image",0,2},{"Output",2,1}},false},
        {"Fast C","Linked peak compressor","Original stereo-linked peak compressor with a 3 dB soft knee. No proprietary circuit emulation.",CatDynamics,5,
         {{"Thresh",-60,0,false,-18,Unit::Db,0,nullptr},{"Ratio",1,20,true,4,Unit::Ratio,0,nullptr},
          {"Attack",0.1f,30,true,2,Unit::Ms,0,nullptr},{"Release",20,1000,true,100,Unit::Ms,0,nullptr},
          {"Makeup",0,24,false,0,Unit::Db,0,nullptr}},3,{{"Input",0,2},{"Time",2,2},{"Output",4,1}},true},
        {"Bus C","Linked RMS compressor","Original stereo-linked RMS compressor with a 9 dB soft knee for broad level control. No proprietary circuit emulation.",CatDynamics,5,
         {{"Thresh",-60,0,false,-18,Unit::Db,0,nullptr},{"Ratio",1,10,true,2,Unit::Ratio,0,nullptr},
          {"Attack",1,100,true,20,Unit::Ms,0,nullptr},{"Release",40,1500,true,250,Unit::Ms,0,nullptr},
          {"Makeup",0,24,false,0,Unit::Db,0,nullptr}},3,{{"Input",0,2},{"Time",2,2},{"Output",4,1}},true},
        {"Program Opto","Adaptive linked vocal compressor",
         "Independent linked RMS compressor with slow gain-reduction memory and a frequency-sensitive sidechain. Experimental, no modeled circuit.",CatDynamics,6,
         {{"Thresh",-60,0,false,-24,Unit::Db,0,nullptr},{"Ratio",1,10,true,3,Unit::Ratio,0,nullptr},
          {"Attack",0.5f,60,true,12,Unit::Ms,0,nullptr},{"Release",40,1000,true,240,Unit::Ms,0,nullptr},
          {"SC HP",20,400,true,80,Unit::Hz,0,nullptr},{"Makeup",-12,18,false,0,Unit::Db,0,nullptr}},
         3,{{"Compression",0,4},{"Sidechain",4,1},{"Output",5,1}},true},
        {"Iron Gate","Downward expander","Linked envelope gate with smooth attack and release.",CatDynamics,4,
         {{"Thresh",-80,-10,false,-45,Unit::Db,0,nullptr},{"Range",0,60,false,36,Unit::Db,0,nullptr},{"Attack",0.2f,30,true,3,Unit::Ms,0,nullptr},{"Release",20,700,true,120,Unit::Ms,0,nullptr}},2,{{"Control",0,2},{"Shape",2,2}},true},
        {"Impact","Transient shaper","Linked fast and slow envelopes shape attack and sustain.",CatDynamics,4,
         {{"Attack",-12,12,false,3,Unit::Db,0,nullptr},{"Sustain",-12,12,false,0,Unit::Db,0,nullptr},{"Window",10,150,true,60,Unit::Ms,0,nullptr},{"Output",-12,12,false,0,Unit::Db,0,nullptr}},2,{{"Control",0,2},{"Shape",2,2}},false},
        {"Bell 73","Parametric bell EQ","Sweepable peaking filter with adjustable Q and output.",CatTone,4,
         {{"Freq",30,16000,true,1000,Unit::Hz,0,nullptr},{"Gain",-15,15,false,0,Unit::Db,0,nullptr},{"Q",0.3f,8,true,1,Unit::Scalar,0,nullptr},{"Output",-12,12,false,0,Unit::Db,0,nullptr}},2,{{"Control",0,2},{"Shape",2,2}},false},
        {"Crosscut","Dual filter","Independent high-pass and low-pass filters with resonance.",CatTone,4,
         {{"HPF",20,4000,true,20,Unit::Hz,0,nullptr},{"LPF",200,20000,true,20000,Unit::Hz,0,nullptr},{"Res",0.5f,3,true,0.707f,Unit::Scalar,0,nullptr},{"Mix",0,1,false,1,Unit::Pct,0,nullptr}},2,{{"Control",0,2},{"Shape",2,2}},false},
        {"Ember Clip","Soft peak shaping","Base-rate soft clipper with drive, ceiling and parallel blend.",CatTone,3,
         {{"Drive",0,24,false,6,Unit::Db,0,nullptr},{"Ceiling",-12,0,false,-1,Unit::Db,0,nullptr},{"Mix",0,1,false,1,Unit::Pct,0,nullptr}},2,{{"Control",0,2},{"Shape",2,1}},false},
        {"Pulse","Tremolo","Variable shape amplitude modulation.",CatSpace,3,
         {{"Rate",0.1f,12,true,3,Unit::LfoHz,0,nullptr},{"Depth",0,1,false,0.5f,Unit::Pct,0,nullptr},{"Shape",0,1,false,0.15f,Unit::Pct,0,nullptr}},2,{{"Control",0,2},{"Shape",2,1}},false},
        {"Phase Coil","Four-stage phaser","Swept all-pass stages with feedback and parallel dry blend.",CatSpace,4,
         {{"Rate",0.05f,4,true,0.4f,Unit::LfoHz,0,nullptr},{"Depth",0,1,false,0.65f,Unit::Pct,0,nullptr},{"Feedback",0,0.75f,false,0.2f,Unit::Pct,0,nullptr},{"Mix",0,1,false,0.5f,Unit::Pct,0,nullptr}},2,{{"Control",0,2},{"Shape",2,2}},false},
        {"Jet","Flanger","Short modulated delay with signed feedback and wet blend.",CatSpace,4,
         {{"Rate",0.05f,4,true,0.3f,Unit::LfoHz,0,nullptr},{"Depth",0,1,false,0.7f,Unit::Pct,0,nullptr},{"Feedback",-0.8f,0.8f,false,0.3f,Unit::Pct,0,nullptr},{"Mix",0,1,false,0.45f,Unit::Pct,0,nullptr}},2,{{"Control",0,2},{"Shape",2,2}},false},
        {"Slap 120","Single-repeat slapback","Short feed-forward echo with independent repeat tone and level.",CatSpace,3,
         {{"Time",25,120,false,85,Unit::Ms,0,nullptr},{"Tone",1000,16000,true,6000,Unit::Hz,0,nullptr},{"Mix",0,1,false,0.25f,Unit::Pct,0,nullptr}},2,{{"Control",0,2},{"Shape",2,1}},false},
        {"Air","Dynamic two-band air enhancement","Separate presence and brilliance bands with linked dynamic protection and output trim.",CatTone,4,
         {{"Mid Air",0,1,false,0,Unit::Pct,0,nullptr},{"High Air",0,1,false,0,Unit::Pct,0,nullptr},{"Protection",0,1,false,1,Unit::Pct,0,nullptr},{"Trim",-12,12,false,0,Unit::Db,0,nullptr}},2,{{"Air",0,2},{"Control",2,2}},false}
    };
    return table[std::clamp(type,0,static_cast<int>(kNumRackTypes)-1)];
}
// User-facing branding is separate from the legacy host choice strings.
// Preserve host choice names and all normalized automation/state identities.
inline const char* displayName(int type) noexcept {
    static constexpr const char* names[kNumRackTypes]={"Empty","Skell Quad EQ","Skell Opto C","Skell Sibilant","Skell Warm Drive","Skell Air Lift","Skell Twin Chorus","Skell Tape Echo","Skell Plate Room","Skell Brick 1","Skell Trim","Skell Tune","Skell Tilt Tone","Skell Stereo Stage","Skell Fast C","Skell Bus C","Skell Program Opto","Skell Iron Gate","Skell Impact","Skell Bell 73","Skell Crosscut","Skell Ember Clip","Skell Pulse","Skell Phase Coil","Skell Jet","Skell Slap 120","Skell Air"};
    return names[std::clamp(type,0,static_cast<int>(kNumRackTypes)-1)];
}
inline float toNorm(const Knob& k,float v) {
    if (!std::isfinite(v)) v=k.def;
    if (k.steps>0) return k.steps>1 ? std::clamp(v/static_cast<float>(k.steps-1),0.0f,1.0f) : 0.0f;
    v=std::clamp(v,k.lo,k.hi);
    if(k.log) return std::log(v/k.lo)/std::log(k.hi/k.lo);
    return (v-k.lo)/(k.hi-k.lo);
}
inline float toReal(const Knob& k,float n) {
    n=std::isfinite(n) ? std::clamp(n,0.0f,1.0f) : toNorm(k,k.def);
    if(k.steps>0) return static_cast<float>(std::lround(n*(k.steps-1)));
    if(k.log) return k.lo*std::pow(k.hi/k.lo,n);
    return k.lo+(k.hi-k.lo)*n;
}
inline float defaultNorm(int type,int knob) { return toNorm(info(type).knobs[knob],info(type).knobs[knob].def); }
inline float realOf(int type,int knob,const float* norm) { return toReal(info(type).knobs[knob],norm[knob]); }
} // namespace skl

