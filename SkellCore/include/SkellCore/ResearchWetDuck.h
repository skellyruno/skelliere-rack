#pragma once
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace skellcore::research {
// Independent, opt-in wet gain controller. Construct off the audio thread.
// Smooth gain, not the detector amplitude, to avoid a long nonlinear release.
class WetDuck {
    double minimum_, attack_, release_, gain_=1.0;
public:
    explicit WetDuck(double rate, double depthDb=9., double attackMs=14., double releaseMs=260.) {
        if (!std::isfinite(rate) || rate<8000. || rate>384000. ||
            !std::isfinite(depthDb) || depthDb<0. || depthDb>18. ||
            !std::isfinite(attackMs) || attackMs<1. || attackMs>2000. ||
            !std::isfinite(releaseMs) || releaseMs<1. || releaseMs>2000.)
            throw std::invalid_argument("WetDuck configuration");
        minimum_=std::pow(10.,-depthDb/20.);
        attack_= -std::expm1(-1000./(rate*attackMs));
        release_= -std::expm1(-1000./(rate*releaseMs));
    }
    void reset() noexcept { gain_=1.; }
    double gain() const noexcept { return gain_; }
    double process(double left, double right) noexcept {
        const auto magnitude=[](double x) noexcept { return std::isfinite(x)?std::min(std::abs(x),64.):0.; };
        const double peak=std::max(magnitude(left),magnitude(right));
        const double target=1.-(1.-minimum_)*peak/(peak+.10);
        gain_+=(target<gain_?attack_:release_)*(target-gain_);
        if (1.-gain_<1e-12) gain_=1.;
        return gain_;
    }
};
}
