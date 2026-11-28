#pragma once
#include "Modules.h"
#include <vector>
#include <utility>

namespace skl {
struct FSlot { int type; std::vector<std::pair<int, float>> set; };   // knob index -> real value
struct FPreset { const char* name; std::vector<FSlot> slots; };

inline const std::vector<FPreset>& factory() {
    static const std::vector<FPreset> list = {
        { "Silk Lead", {
            { Tune, {{0, 25}} },
            { Eq, {{0, 90}, {2, -2}, {3, 1.5f}, {4, 1}} },
            { Comp, {{0, -26}, {1, 3.5f}, {2, 12}, {3, 140}, {4, 5}} },
            { DeEss, {} },
            { Sat, {{0, 5}, {2, 0.35f}} },
            { Air, {{0, 3.5f}} },
            { Limiter, {} } } },
        { "Warm Vocal", {
            { Comp, {{0, -22}, {1, 2.5f}, {2, 20}, {3, 200}, {4, 3}} },
            { Sat, {{0, 9}, {1, 8000}, {2, 0.5f}} },
            { DeEss, {} },
            { Chorus, {{2, 0.18f}} },
            { Delay, {{3, 0.18f}} },
            { skl::Reverb, {{3, 0.22f}} } } },
        { "Glue Bus", {
            { Trim, {{0, -2}} },
            { Eq, {{0, 30}, {1, 1}, {4, 1.5f}} },
            { Sat, {{0, 4}, {2, 0.4f}} },
            { Comp, {{0, -18}, {1, 2}, {2, 30}, {3, 250}, {4, 2}} },
            { Limiter, {{0, 2}} } } },
        { "Empty rack", {} },
        { "Velvet Lead", {
            {NoiseGateUnified,{{0,-52},{1,24},{2,95},{3,145}}},
            {LevelUnified,{{2,-23},{3,4},{4,300},{5,1}}},
            {DualFilter,{{0,75},{1,18000}}},
            {FastComp,{{0,4},{1,0},{2,.45f},{3,180},{4,-1},{5,1}}},
            {DeEsserUnified,{{0,7000},{1,-28},{2,4},{3,90},{5,1}}},
            {SkellRubyQ,{{0,-1},{1,120},{2,-1.5f},{3,550},{4,1.2f},{5,10000}}},
            {DriveUnified,{{0,4},{1,14000},{2,.16f},{3,0}}},
            {Limiter,{{0,0},{1,-1.5f}}} } },
        { "Front Row", {
            {DualFilter,{{0,85},{1,19000}}},
            {FastComp,{{0,6},{1,1},{2,.35f},{3,110},{4,0},{5,0}}},
            {ProgramOpto,{{0,-24},{1,2},{2,25},{3,280},{4,100},{5,1}}},
            {DeEsserUnified,{{0,7500},{1,-30},{2,5},{3,80},{5,1}}},
            {SkellRubyQ,{{0,-1},{1,140},{2,1},{3,2000},{4,1.5f},{5,11000}}},
            {AirUnified,{{0,.1f},{1,.12f},{2,1},{3,-1}}},
            {EchoUnified,{{0,95},{1,0},{2,6500},{3,.12f},{4,.3f},{5,2}}},
            {Limiter,{{0,0},{1,-1.5f}}} } },
        { "Analog Bus", {
            {LevelUnified,{{0,-3}}},
            {SkellRubyQ,{{0,.5f},{1,100},{2,-.5f},{3,350},{4,.8f},{5,10000}}},
            {BusComp,{{0,-18},{1,2},{2,30},{3,250},{4,0}}},
            {SkellMixHead,{{0,0},{1,.15f},{2,0},{3,-1},{4,1},{5,0}}},
            {StereoUnified,{{0,1},{1,100},{2,0}}},
            {ClipUnified,{{0,0},{1,-1},{2,.35f},{3,2}}},
            {Limiter,{{0,0},{1,-1.5f}}} } }
    };
    return list;
}
struct ModulePreset { const char* name; std::vector<float> values; };
inline const std::vector<ModulePreset>& rackModulePresets(int type) {
    // Original real-unit starting points, not copies of vendor presets.
    static const std::vector<ModulePreset> echo={{"Vocal Slap",{115,0,7000,.22f,0,2}}, {"Wide Echo",{320,.28f,6000,.25f,.30f,1}}, {"Quarter Note",{320,.35f,5500,.24f,.40f,3}}};
    static const std::vector<ModulePreset> room={{"Short Plate",{1.2f,20,9000,.20f,.35f,0}}, {"Dark Hall",{3.2f,35,4500,.28f,.35f,1}}, {"Small Room",{.65f,8,11000,.17f,.15f,2}}};
    static const std::vector<ModulePreset> drive={{"Warm Tube",{6,12000,.45f,0}}, {"Console Edge",{9,10000,.38f,1}}, {"Air Lift",{5,16000,.25f,2}}};
    static const std::vector<ModulePreset> fast={{"Vocal Catch",{8,0,.16f,135,0,0}}, {"Black Control",{5,1,.35f,240,-1,1}}, {"Drum Slam",{12,4,.08f,90,-5,0}}};
    static const std::vector<ModulePreset> bus={{"Gentle Glue",{-18,2,30,250,0}}, {"Vocal Level",{-24,3,12,180,2}}, {"Tight Bus",{-18,4,10,120,0}}};
    static const std::vector<ModulePreset> opto={{"Gentle Level",{-24,2,25,280,100,1}}, {"Close Vocal",{-30,3,14,220,90,2}}, {"Bass Hold",{-22,3,30,350,40,0}}};
    static const std::vector<ModulePreset> ruby={{"Vocal Clarity",{-1,120,-1.5f,550,1.5f,10000}}, {"Velvet",{1,100,-.7f,1600,-1.5f,8000}}, {"Presence",{-1,180,1.5f,2500,.8f,12000}}};
    static const std::vector<ModulePreset> soothe={{"Gentle Vocal",{.25f,3,2500,4,120,.8f}}, {"Harsh Upper Mid",{.55f,6,3500,6,90,1}}, {"Low Mid Cleanup",{.4f,4,650,3,150,.9f}}};
    static const std::vector<ModulePreset> air={{"Soft Lift",{.08f,.12f,1,-.5f,12000,1}}, {"Open Vocal",{.14f,.24f,1,-1,10000,1}}, {"Static Silk",{.06f,.12f,1,-.5f,14000,0}}};
    static const std::vector<ModulePreset> mixhead={{"Nickel Tape",{0,.15f,0,-.5f,1,0}}, {"Slow Warmth",{1,.25f,-1,-1.5f,0,0}}, {"Bright Edge",{0,.3f,1,-1,1,1}}};
    static const std::vector<ModulePreset> flang={{"Velvet Chorus",{0,.32f,.4f,.42f,.6f,.23f}}, {"Slow Flange",{1,.48f,.3f,.38f,.5f,.28f}}, {"Wide Dimension",{3,.3f,.3f,.5f,.75f,.25f}}};
    static const std::vector<ModulePreset> delay={{"Short Slap",{48,.12f,.65f,0,0,.13f}}, {"Quarter Ping",{192,.3f,.65f,0,2,.2f}}, {"Dotted Echo",{144,.32f,.7f,.12f,0,.18f}}};
    static const std::vector<ModulePreset> glitch={{"Soft Gate",{1,2,.5f,.35f,0,.5f}}, {"Reverse Accent",{2,2,.65f,.3f,0,.45f}}, {"Octave Fragments",{0,1,.6f,.4f,-12,.4f}}};
    static const std::vector<ModulePreset> empty;
    switch(type){case EchoUnified:return echo;case RoomUnified:return room;case DriveUnified:return drive;case FastComp:return fast;case BusComp:return bus;case ProgramOpto:return opto;case SkellRubyQ:return ruby;case Resonance:return soothe;case AirUnified:return air;case SkellMixHead:return mixhead;case FlangNative:return flang;case DelayNative:return delay;case GlitchNative:return glitch;default:return empty;}
}
} // namespace
