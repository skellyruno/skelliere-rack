#include "../research/NormalizedSaturation.h"
#include <cassert>
#include <cstdio>
#include <cmath>
using skl::research::NormalizedSaturation;
constexpr double pi=3.141592653589793;
struct Stats {double fundamental,third,dc;};
static Stats render(int sr,int hz,int bin,bool asymmetric,bool aa){
    NormalizedSaturation s;s.prepare(sr,4,asymmetric);double re=0,im=0,r3=0,i3=0,dc=0;
    for(int i=0;i<2*sr;i++){const double y=s.process(.65*std::sin(2*pi*hz*i/sr),aa);assert(std::isfinite(y));if(i>=sr){const double a=2*pi*hz*i/sr,b=2*pi*bin*i/sr;re+=y*std::cos(a);im+=y*std::sin(a);r3+=y*std::cos(b);i3+=y*std::sin(b);dc+=y;}}
    return {2*std::hypot(re,im)/sr,2*std::hypot(r3,i3)/sr,dc/sr};
}
int main(){for(int sr:{44100,48000,96000,192000})for(bool b:{false,true}){
    NormalizedSaturation s;s.prepare(sr,4,b);assert(std::abs((s.transfer(.25)-s.transfer(-.25))*.5-.25)<1e-12);
    double out=0;for(int n=0;n<2*sr;n++)out=s.process(.5);assert(std::abs(out)<1e-12);
    auto a=render(sr,1000,3000,b,false),c=render(sr,1000,3000,b,true);assert(std::abs(c.dc)<1e-6);assert(std::abs(20*std::log10(c.third/a.third))<.5);
    printf("%d Hz asymmetric=%d desired H3 change %.3f dB\n",sr,b,20*std::log10(c.third/a.third));
    const int f=sr*35/100,alias=3*f-sr;auto x=render(sr,f,alias,b,false),y=render(sr,f,alias,b,true);printf("  folded third harmonic change %.3f dB (absolute), %.3f dB (relative to fundamental)\n",20*std::log10(y.third/x.third),20*std::log10((y.third/y.fundamental)/(x.third/x.fundamental)));assert(y.third<x.third*.8);
}puts("Normalized saturation candidate: span, DC, harmonic and folded alias tests passed");}
