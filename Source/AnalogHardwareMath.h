#pragma once
#include <algorithm>
#include <cmath>
namespace skl::ui::metal {
inline float sat(float x){return std::clamp(x,0.f,1.f);}
inline float smooth(float a,float b,float x){const float t=sat((x-a)/(b-a));return t*t*(3.f-2.f*t);}
struct CapSample { float brightness,edge; };
// A turned-metal face with anisotropic reflections, a wide curved chamfer,
// and narrow occlusion at the physical edge. No separate decorative ring.
inline CapSample cap(float r,float angle,float original){
    const float dir=std::cos(2.f*(angle+1.85f+.24f*(r-.55f)));
    const float spec=std::max(0.f,dir); // broad softbox reflection
    const float az=std::cos(angle+2.20f);
    const float fade=smooth(0.f,.34f,r);
    const float grain=fade*(.014f*std::sin(r*810.f)+.008f*std::sin(r*1337.f));
    float v=.37f+.25f*original+.055f*(1.f-r*r)+fade*(.19f*dir+.16f*spec+.09f*az)+grain;
    const float bevel=smooth(.865f,.93f,r);
    v=v*(1.f-bevel)+bevel*(.47f+.32f*az+.10f*spec);
    const float edge=smooth(.958f,.995f,r);
    v=v*(1.f-edge)+edge*(.14f+.12f*std::max(0.f,az));
    return {sat(v),edge};
}
inline float engravingScale(float r){return .72f+.28f*smooth(.29f,.52f,r);}
inline float engravingWeight(float r,float alpha){return (1.f-smooth(.36f,.46f,r))*(1.f-smooth(.25f,.93f,alpha));}
}
