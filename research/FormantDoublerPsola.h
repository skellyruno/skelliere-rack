#pragma once
#include "../Source/AdvancedColorSpace.h"
#include "../Source/Recovered/core/VocalPitch.h"
namespace skl::research {
// Separate pitch-synchronous voices, sharing only the mono pitch detector.
// Wet delay is intentional; the original dry path is not delayed. Research
// only: test low voiced notes, noisy breaths and stereo decorrelated input.
class FormantDoublerPsola {
public:
    void prepare(double sr) {
        rate=std::clamp(sr,32000.,192000.);detector.prepare(rate,65,900,int(rate*.010));detector.setHarmonicGuard(true);
        for(auto& v:voices)v.prepare(rate,rate/65.,int(rate*.040));set(35.f);
    }
    void set(float cents) noexcept {for(int j=0;j<2;++j)voices[j].setShift(std::exp2((j?-1:1)*std::clamp(cents,-35.f,35.f)/1200.),1.);}
    std::array<float,2> process(float l,float r) noexcept {
        l=sane(l);r=sane(r);if(detector.push(.5f*(l+r))){const auto e=detector.estimate();for(auto& v:voices)v.setPitch(e.period,e.voiced);}
        float a=0,b=0;voices[0].process(&l,&a,1);voices[1].process(&r,&b,1);return {sane(a),sane(b)};
    }
private:double rate=48000;skelly::vocal::PitchDetector detector;std::array<skelly::vocal::Psola,2> voices;
};
}
