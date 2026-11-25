#include "OriginalSpace.h"
#include <SkellCore/ResearchWetDuck.h>
#include <iostream>
#include <iomanip>

namespace {
using skell_lab::Frame;
using skell_lab::OriginalSpace;
using skellcore::research::WetDuck;
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct Energy {
    double raw=0., old=0., candidate=0.;
    void add(Frame x, Frame y, Frame z) {
        for (int c=0;c<2;++c) { raw+=x[c]*x[c]; old+=y[c]*y[c]; candidate+=z[c]*z[c]; }
    }
    double oldDb() const { return 10.*std::log10(old/raw); }
    double newDb() const { return 10.*std::log10(candidate/raw); }
};
Frame phrase(double t) {
    constexpr double pi=3.14159265358979323846;
    const double local=t<1.0?t:(t>=1.35 && t<2.35?t-1.35:-1.);
    if (local<0. || local>=1.) return {};
    const double ramp=std::min({1.,local/.025,(1.-local)/.025});
    const double envelope=ramp*(.50+.35*std::sin(2*pi*3.1*local));
    const double phase=2*pi*177.*t+.21*std::sin(2*pi*5.2*t);
    const double x=envelope*(.65*std::sin(phase)+.23*std::sin(2*phase)+.12*std::sin(5*phase));
    return {x,.72*x+.05*envelope*std::sin(3*phase+.7)};
}
void checkRate(double rate) {
    OriginalSpace raw(rate,1.8,4200.,0.), old(rate);
    WetDuck candidate(rate), bypass(rate,0.);
    Energy active, tail, gap;
    double peak=0., maxGainStep=0., lastGain=1.;
    const int stop=int(2.35*rate), total=int(4.*rate);
    for (int i=0;i<total;++i) {
        const double t=i/rate;
        const auto dry=phrase(t);
        const auto wet=raw.wet(dry), legacy=old.wet(dry);
        const double g=candidate.process(dry[0],dry[1]);
        require(bypass.process(dry[0],dry[1])==1.,"zero-depth changes wet gain");
        const Frame newer{wet[0]*g,wet[1]*g};
        require(g>=skell_lab::db(-9.) && g<=1.,"wet gain bounds");
        maxGainStep=std::max(maxGainStep,std::abs(g-lastGain));lastGain=g;
        for (int c=0;c<2;++c) {
            require(std::isfinite(newer[c]),"nonfinite FDN output");
            require(std::abs(legacy[c]-wet[c]*old.wetGain())<1e-14,"FDN wet streams not equivalent");
            peak=std::max(peak,std::abs(newer[c]));
        }
        if ((t>=.2 && t<.95)||(t>=1.55 && t<2.30)) active.add(wet,legacy,newer);
        if (t>=1.10 && t<1.35) gap.add(wet,legacy,newer);
        if (i>=stop+int(.10*rate) && i<stop+int(.50*rate)) tail.add(wet,legacy,newer);
    }
    require(active.raw>0. && gap.raw>0. && tail.raw>0.,"empty measurement window");
    require(active.newDb()<-2.,"candidate fails to attenuate active wet signal");
    require(tail.newDb()>tail.oldDb()+1.,"candidate does not restore the wet tail");
    require(peak<1. && maxGainStep<.002,"output peak or gain discontinuity");
    raw.reset();old.reset();candidate.reset();
    require(raw.wet({})==Frame{} && old.wet({})==Frame{} && candidate.gain()==1.,"reset leaves history");
    std::cout<<std::fixed<<std::setprecision(3)<<rate<<" Hz active old/new "<<active.oldDb()<<" / "<<active.newDb()
        <<" dB, gap "<<gap.oldDb()<<" / "<<gap.newDb()<<" dB, tail "<<tail.oldDb()<<" / "<<tail.newDb()
        <<" dB, candidate peak "<<peak<<", max gain step "<<maxGainStep<<'\n';
}
}
int main() { for (double rate : {44100.,48000.,96000.,192000.}) checkRate(rate); }
