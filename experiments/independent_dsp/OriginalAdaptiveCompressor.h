#pragma once
// Independent original transient-aware dynamics experiment. Not a vendor model.
#include "OriginalDynamics.h"
namespace skell_lab {
class AdaptiveCompressor {
    double threshold, slope, knee, amount;
    double fastPole, slowPole, attackFast, attackSlow, releaseFast, releaseSlow;
    double fast=0., slow=0., reduction=0.;
public:
    AdaptiveCompressor(double fs,double thresholdDb=-18.,double ratio=4.,double kneeDb=6.,double adapt=1.) {
        validRate(fs);
        if(!std::isfinite(thresholdDb)||thresholdDb< -90.||thresholdDb>0.||
           !std::isfinite(ratio)||ratio<1.||ratio>30.||
           !std::isfinite(kneeDb)||kneeDb<0.||kneeDb>24.||
           !std::isfinite(adapt)||adapt<0.||adapt>1.)throw std::invalid_argument("compressor settings");
        threshold=thresholdDb;slope=1.-1./ratio;knee=kneeDb;amount=adapt;
        fastPole=decay(fs,1.);slowPole=decay(fs,45.);
        attackFast=decay(fs,2.);attackSlow=decay(fs,18.);
        releaseFast=decay(fs,55.);releaseSlow=decay(fs,240.);
    }
    void reset(){fast=slow=reduction=0.;}
    double reductionDb()const{return reduction;}
    static double curve(double over,double width,double s){
        if(width==0.)return -s*std::max(0.,over);
        if(over<=-width*.5)return 0.;
        if(over>=width*.5)return -s*over;
        double x=over+width*.5;
        return -s*x*x/(2.*width);
    }
    Frame process(Frame x){
        for(auto& v:x)v=clean(v);
        const double p=peak(x),power=p*p;
        fast=fastPole*fast+(1.-fastPole)*power;
        slow=slowPole*slow+(1.-slowPole)*power;
        if(fast<1e-24)fast=0.;
        if(slow<1e-24)slow=0.;
        // Positive fast/slow energy contrast flags onset; a slower attack
        // deliberately preserves more onset energy. Sustained energy prolongs release.
        const double onset=std::clamp((fast-slow)/(fast+slow+1e-12),0.,1.)*amount;
        const double sustain=std::clamp(slow/(fast+slow+1e-12),0.,1.)*amount;
        const double detector=std::sqrt(std::max(0.,.65*fast+.35*slow));
        const double goal=curve(level(detector)-threshold,knee,slope);
        const double a=goal<reduction
            ? attackFast+onset*(attackSlow-attackFast)
            : releaseFast+sustain*(releaseSlow-releaseFast);
        reduction=a*reduction+(1.-a)*goal;
        if(std::abs(reduction)<1e-12)reduction=0.;
        const double g=db(reduction);
        for(auto& v:x)v*=g;
        return x;
    }
};
}
