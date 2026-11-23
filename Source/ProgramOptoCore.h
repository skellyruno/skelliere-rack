#pragma once
// Original, self-contained stereo-linked optical-style compressor core.
// No third-party DSP code, heap allocation, locking, or host dependencies.
#include <algorithm>
#include <cmath>

namespace skl {
class ProgramOptoCore final {
public:
    void prepare(double sampleRate) noexcept {
        sr_ = std::isfinite(sampleRate) && sampleRate >= 8000.0 && sampleRate <= 384000.0
            ? sampleRate : 48000.0;
        reset();
        configure(-24.0f, 3.0f, 12.0f, 240.0f, 80.0f, 0.0f);
    }
    void reset() noexcept {
        for (auto& s : hp_) s = {};
        detectorPower_ = reductionDb_ = longMemoryDb_ = 0.0;
    }
    void configure(float thresholdDb, float ratio, float attackMs, float releaseMs,
                   float sidechainHz, float makeupDb) noexcept {
        thresholdDb_ = finiteClamp(thresholdDb, -60.0f, 0.0f, -24.0f);
        ratio_ = finiteClamp(ratio, 1.0f, 10.0f, 3.0f);
        attackMs_ = finiteClamp(attackMs, 0.5f, 60.0f, 12.0f);
        releaseMs_ = finiteClamp(releaseMs, 40.0f, 1000.0f, 240.0f);
        makeupDb_ = finiteClamp(makeupDb, -12.0f, 18.0f, 0.0f);
        const float cutoff = finiteClamp(sidechainHz, 20.0f, 400.0f, 80.0f);
        hpCoeff_ = std::exp(-6.283185307179586 * cutoff / sr_);
        detectorAttack_ = coeff(2.0);
        detectorRelease_ = coeff(12.0);
        gainAttack_ = coeff(attackMs_);
        memoryRelease_ = coeff(std::max(300.0, static_cast<double>(releaseMs_)*4.0));
    }
    void processPair(float left, float right, bool stereo,
                     float& wetLeft, float& wetRight) noexcept {
        left = std::isfinite(left) ? left : 0.0f;
        right = stereo ? (std::isfinite(right) ? right : 0.0f) : left;
        const double scLeft = highPass(hp_[0],left);
        const double scRight = stereo ? highPass(hp_[1],right) : scLeft;
        const double power = stereo ? 0.5*(scLeft*scLeft+scRight*scRight) : scLeft*scLeft;
        const double coeffNow = power > detectorPower_ ? detectorAttack_ : detectorRelease_;
        detectorPower_ = coeffNow*detectorPower_ + (1.0-coeffNow)*power;
        if(detectorPower_ < 1.0e-28) detectorPower_=0.0;
        const double detectorDb = 10.0*std::log10(std::max(detectorPower_,1.0e-24));
        const double over = detectorDb - thresholdDb_;
        constexpr double knee = 6.0;
        const double slope = 1.0 - 1.0/ratio_;
        double demanded=0.0;
        if(over >= knee*0.5) demanded=over*slope;
        else if(over > -knee*0.5) {
            const double shifted = over + knee*0.5;
            demanded=slope*shifted*shifted/(2.0*knee);
        }
        // Slowly decaying gain-reduction memory gives deeper, sustained
        // compression a longer release without claiming a physical circuit model.
        longMemoryDb_ = std::max(demanded,longMemoryDb_*memoryRelease_);
        const double releaseMultiplier = 1.0 + 2.0*std::clamp(longMemoryDb_/18.0,0.0,1.0);
        const double gainCoeff = demanded > reductionDb_ ? gainAttack_ : coeff(releaseMs_*releaseMultiplier);
        reductionDb_ = gainCoeff*reductionDb_ + (1.0-gainCoeff)*demanded;
        if(reductionDb_<1.0e-12) reductionDb_=0.0;
        const double gain=std::exp((static_cast<double>(makeupDb_)-reductionDb_)*0.11512925464970229);
        wetLeft=static_cast<float>(left*gain);
        wetRight=static_cast<float>(right*gain);
    }
    float gainReductionDb() const noexcept { return static_cast<float>(reductionDb_); }
    double sampleRate() const noexcept { return sr_; }
private:
    struct HPState { double lastX=0.0, lastY=0.0; };
    static float finiteClamp(float x,float lo,float hi,float fallback) noexcept {
        return std::isfinite(x) ? std::clamp(x,lo,hi) : fallback;
    }
    double coeff(double milliseconds) const noexcept {
        return std::exp(-1.0 / (std::max(milliseconds,0.1)*0.001*sr_));
    }
    double highPass(HPState& s,double x) noexcept {
        const double y=hpCoeff_*(s.lastY + x - s.lastX);
        s.lastX=x;
        s.lastY=std::fabs(y)<1.0e-25 ? 0.0 : y;
        return s.lastY;
    }
    double sr_=48000.0,detectorPower_=0.0,reductionDb_=0.0,longMemoryDb_=0.0;
    double hpCoeff_=0.0,detectorAttack_=0.0,detectorRelease_=0.0,gainAttack_=0.0,memoryRelease_=0.0;
    float thresholdDb_=-24.0f,ratio_=3.0f,attackMs_=12.0f,releaseMs_=240.0f,makeupDb_=0.0f;
    HPState hp_[2];
};
} // namespace skl
