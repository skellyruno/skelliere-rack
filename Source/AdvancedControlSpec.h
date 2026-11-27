#pragma once
#include "Modules.h"
namespace skl {
inline constexpr bool hasAdvancedEngine(int t) noexcept {return t==Multiband||t==Resonance||t==Doubler||t==Exciter;}
constexpr int advMode=0, advCross=1, advBand=6, advFloating=62, advVoice=66, kAdvancedCount=74;
constexpr int bandControl(int band,int field) noexcept{return advBand+band*7+field;}
constexpr int floatingControl(int band,int field) noexcept{return advFloating+band*2+field;}
constexpr int voiceControl(int voice,int field) noexcept{return advVoice+voice*4+field;}
inline Knob advancedSpec(int i) noexcept {
    if(i==advMode)return {"Engine",0,1,false,0,Unit::Steps,2,"Legacy,Advanced"};
    if(i>=advCross&&i<advBand){constexpr float f[]={120,350,1000,3000,7500};return {"Crossover",30,16000,true,f[i-advCross],Unit::Hz,0,nullptr};}
    if(i>=advBand&&i<advFloating){constexpr Knob k[]={{"Threshold",-80,0,false,-24,Unit::Db,0,nullptr},{"Ratio",1,20,true,2,Unit::Ratio,0,nullptr},{"Range",0,24,false,6,Unit::Db,0,nullptr},{"Attack",.1f,200,true,15,Unit::Ms,0,nullptr},{"Release",5,2000,true,150,Unit::Ms,0,nullptr},{"Makeup",-12,12,false,0,Unit::Db,0,nullptr},{"Enabled",0,1,false,1,Unit::Steps,2,"Off,On"}};auto v=k[(i-advBand)%7];if((i-advBand)/7>=6&&((i-advBand)%7)==6)v.def=0;return v;}
    if(i>=advFloating&&i<advVoice){if((i-advFloating)%2==0)return {"Frequency",40,16000,true,i==advFloating?2500.f:5000.f,Unit::Hz,0,nullptr};return {"Q",.5f,12,true,4,Unit::Scalar,0,nullptr};}
    if(i>=advVoice&&i<kAdvancedCount){const bool right=(i-advVoice)/4==1;switch((i-advVoice)%4){case 0:return {"Delay",8,65,false,18,Unit::Ms,0,nullptr};case 1:return {"Pitch",-35,35,false,right?-8.f:8.f,Unit::Scalar,0,nullptr};case 2:return {"Pan",-1,1,false,right?.8f:-.8f,Unit::Pan,0,nullptr};default:return {"Level",0,1,false,1,Unit::Pct,0,nullptr};}}
    return {"Unknown",0,1,false,0,Unit::Scalar,0,nullptr};
}
inline float advancedDefault(int i){const auto k=advancedSpec(i);return toNorm(k,k.def);}
inline float advancedReal(int i,float n){const auto k=advancedSpec(i);return std::abs(n-advancedDefault(i))<1e-6f?k.def:toReal(k,n);}
using AdvancedValues=std::array<float,kAdvancedCount>;
inline AdvancedValues advancedDefaults(){AdvancedValues v{};for(int i=0;i<kAdvancedCount;++i)v[i]=advancedDefault(i);return v;}
}
