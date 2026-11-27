#pragma once
#include <array>
#include <cmath>
struct Biquad {double b0=1,b1=0,b2=0,a1=0,a2=0;std::array<double,2>z1{},z2{};void set(double b,double c,double d,double a,double e,double f) noexcept {b0=b/a;b1=c/a;b2=d/a;a1=e/a;a2=f/a;}void filter(double sr,int,double hz,double q) noexcept {double w=6.283185307179586*hz/sr,c=std::cos(w),a=std::sin(w)/(2*q);set((1+c)/2,-(1+c),(1+c)/2,1+a,-2*c,1-a);}void reset() noexcept {z1={};z2={};}float process(float x,int c) noexcept {const double y=b0*x+z1[c];z1[c]=b1*x-a1*y+z2[c];z2[c]=b2*x-a2*y;return float(y);}};
