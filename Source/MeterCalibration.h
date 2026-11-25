#pragma once
#include <array>
#include <algorithm>
#include <cmath>
namespace skl::ui {
enum class MeterScale { Custom, Optical, Fet, Bus };
struct MeterPoint { float x,y; };
inline constexpr std::array<float,7> meterDb{0,1,3,5,7,10,20};
// Measured tick-centres in the original GR scale resources (1x canvas).
// Needle is newly drawn; the source packs have scale/bezel/glass, not a needle strip.
inline constexpr std::array<MeterPoint,7> meterTicks(MeterScale scale) {
    switch(scale) {
    case MeterScale::Custom: return {{{108,32},{100,28},{83,23},{66,21},{54,21},{40,24},{16,32}}};
    case MeterScale::Optical:return {{{109,41},{100,34},{83,28},{67,24},{54,24},{40,27},{17,39}}};
    case MeterScale::Fet:return {{{119,43},{112,35},{93,29},{77,28},{64,29},{50,32},{28,44}}};
    default:return {{{110,58},{103,53},{87,46},{71,42},{58,44},{44,48},{27,59}}};
    }
}
inline MeterPoint meterTick(MeterScale scale,float reduction) {
    const auto ticks=meterTicks(scale);
    reduction=std::isfinite(reduction)?std::clamp(reduction,0.0f,20.0f):0.0f;
    for(size_t i=1;i<meterDb.size();++i)if(reduction<=meterDb[i]) {
        const auto t=(reduction-meterDb[i-1])/(meterDb[i]-meterDb[i-1]);
        return {ticks[i-1].x+(ticks[i].x-ticks[i-1].x)*t,ticks[i-1].y+(ticks[i].y-ticks[i-1].y)*t};
    }
    return ticks.back();
}
inline float smoothReduction(float shown,float gr,float seconds) {
    gr=std::isfinite(gr)?std::clamp(gr,0.0f,20.0f):0.0f;
    if(!std::isfinite(shown))shown=0;
    const float tau=gr>shown?.045f:.24f;
    return shown+(gr-shown)*(1-std::exp(-std::clamp(seconds,0.0f,.25f)/tau));
}
}
