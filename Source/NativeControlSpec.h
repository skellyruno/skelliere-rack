#pragma once
#include "Modules.h"
namespace skl {
// Append-only parameter IDs, identical in public and owner-source builds.
// Values remain normalized even for integer choices. No existing automation is repurposed.
constexpr int nDelayModel=0,nDelayBanks=1,nDelayTrim=2,nGlitchStyle=3,nGlitchCount=4,nGlitchRate=5,nGlitchChaos=6,nGlitchSwing=7,nGlitchSeed=8,nGlitchLock=9,nGlitchSource=10,nGlitchTrim=11,nGlitchState=12,nGlitchProbability=44,kNativeControlCount=76;
using NativeValues=std::array<float,kNativeControlCount>;
inline Knob nativeSpec(int i) noexcept {
    switch(i){
    case nDelayModel:return {"Engine",0,2,false,0,Unit::Steps,3,"Classic,Modern,Bank"};
    case nDelayBanks:return {"Bank taps",1,8,false,2,Unit::Scalar,8,nullptr};
    case nDelayTrim:return {"Wet trim",-24,0,false,0,Unit::Db,0,nullptr};
    case nGlitchStyle:return {"Reverse style",0,3,false,0,Unit::Steps,4,"Classic,Tape,Ping Pong,Stutter"};
    case nGlitchCount:return {"Steps",1,32,false,16,Unit::Scalar,32,nullptr};
    case nGlitchRate:return {"Gate rate",0,5,false,2,Unit::Steps,6,"1/4,1/8,1/16,1/32,1/8T,1/16T"};
    case nGlitchChaos:return {"Chaos",0,1,false,0,Unit::Pct,0,nullptr};
    case nGlitchSwing:return {"Swing",0,.75f,false,0,Unit::Pct,0,nullptr};
    case nGlitchSeed:return {"Seed",1,1024,false,1,Unit::Scalar,1024,nullptr};
    case nGlitchLock:return {"Seed lock",0,1,false,1,Unit::Steps,2,"Off,On"};
    case nGlitchSource:return {"Trigger",0,2,false,0,Unit::Steps,3,"Auto,MIDI,Manual"};
    case nGlitchTrim:return {"Wet trim",-24,0,false,0,Unit::Db,0,nullptr};
    default:if(i>=nGlitchState&&i<nGlitchProbability){int j=i-nGlitchState;return {"Step",0,3,false,float(j%4==0?2:j%2?0:1),Unit::Steps,4,"Off,On,Accent,Reverse"};}
        return {"Probability",0,1,false,1,Unit::Pct,0,nullptr};
    }
}
inline float nativeDefault(int i) noexcept {auto s=nativeSpec(i);return (s.def-s.lo)/(s.hi-s.lo);}
inline NativeValues nativeDefaults() noexcept {NativeValues a{};for(int i=0;i<kNativeControlCount;++i)a[i]=nativeDefault(i);return a;}
inline float nativeReal(int i,float n) noexcept {auto s=nativeSpec(i);float v=s.lo+(s.hi-s.lo)*(std::isfinite(n)?std::clamp(n,0.f,1.f):nativeDefault(i));return s.steps>1?std::round(v):v;}
inline bool hasNativePanel(int t) noexcept {return importedAvailable(t)&&(t==DelayNative||t==GlitchNative);}
}
