#pragma once
#include <algorithm>
#include <cmath>
namespace skl::research {
// Independent research candidate, not a deployed rack module. Anchor a
// transfer to a quarter-scale differential span, smooth the nonlinear residual
// with first-order antiderivative antialiasing, and remove DC at 5 Hz.
class NormalizedSaturation {
public:
    void prepare(double rate, double drive, bool asymmetric) noexcept {
        rate=std::clamp(std::isfinite(rate)?rate:48000.,8000.,384000.);
        d=std::clamp(std::isfinite(drive)?drive:1.,1.,16.);
        b=asymmetric?.25:0.; bias=std::tanh(b);
        scale=.5/(std::tanh(b+.25*d)-std::tanh(b-.25*d));
        rho=std::exp(-6.283185307179586*5./rate); reset();
    }
    void reset() noexcept {valid=false;prev=prevPrimitive=lastInput=lastOutput=0.;}
    double transfer(double x) const noexcept {return scale*(std::tanh(d*x+b)-bias);}
    double process(double x,bool antialias=true) noexcept {
        if(!std::isfinite(x))x=0.;
        x=std::clamp(x,-16.,16.);
        const double p=primitive(x),delta=x-prev;
        const double y=(!valid||!antialias)?transfer(x):x+(std::abs(delta)<1e-6?transfer((x+prev)*.5)-(x+prev)*.5:(p-prevPrimitive)/delta);
        valid=true;prev=x;prevPrimitive=p;
        const double out=y-lastInput+rho*lastOutput;
        lastInput=y;lastOutput=std::abs(out)<1e-24?0.:out;return lastOutput;
    }
private:
    static double logcosh(double z) noexcept {const double a=std::abs(z);return a+std::log1p(std::exp(-2*a))-std::log(2.);}
    double primitive(double x) const noexcept {return scale*(logcosh(d*x+b)/d-bias*x)-.5*x*x;}
    double d=1.,b=0.,bias=0.,scale=1.,rho=.999,prev=0.,prevPrimitive=0.,lastInput=0.,lastOutput=0.;bool valid=false;
};
}
