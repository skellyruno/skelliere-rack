#include "OriginalAdaptiveCompressor.h"
#include "ProgramOptoCore.h"
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok) { if(!ok) throw std::runtime_error("independent rack lab regression"); }
constexpr double pi=3.14159265358979323846;
struct Metrics { double onset=0, steady=0, steadyDry=0, at100=0, at500=0; };
}
int main() {
    std::cout<<std::setprecision(9);
    for(double fs:{44100.,48000.,96000.,192000.}) {
        skell_lab::AdaptiveCompressor adaptive(fs,-18,4,6,1),fixed(fs,-18,4,6,0);
        skl::ProgramOptoCore opto;opto.prepare(fs);opto.configure(-18,4,12,240,80,0);
        std::array<Metrics,3> m{};
        const int endHigh=int(fs*2),end=int(fs*3),onset=int(fs*.012);
        for(int i=0;i<end;++i) {
            const double x=(i<endHigh?.75:.025)*std::sin(2*pi*1000*i/fs);
            const skell_lab::Frame f{x,.5*x};
            const auto a=adaptive.process(f), b=fixed.process(f);
            float ol=0,orr=0;opto.processPair(float(f[0]),float(f[1]),true,ol,orr);
            const std::array<skell_lab::Frame,3> out{a,b,skell_lab::Frame{ol,orr}};
            const std::array<double,3> gr{-adaptive.reductionDb(),-fixed.reductionDb(),opto.gainReductionDb()};
            for(std::size_t k=0;k<out.size();++k) {
                const auto y=out[k];check(std::isfinite(y[0])&&std::isfinite(y[1]));
                check(std::abs(y[0]-2*y[1])<1e-6);
                check(std::abs(y[0])<=std::abs(x)+1e-6);
                if(i<onset)m[k].onset+=y[0]*y[0];
                if(i>=int(fs*1.5)&&i<endHigh){m[k].steady+=y[0]*y[0];m[k].steadyDry+=x*x;}
                if(i==endHigh+int(fs*.1))m[k].at100=gr[k];
                if(i==endHigh+int(fs*.5))m[k].at500=gr[k];
            }
        }
        check(m[0].onset>m[1].onset);
        const std::array<const char*,3> names{"adaptive","fixed","program_opto"};
        for(std::size_t i=0;i<m.size();++i) {
            const auto v=m[i];check(v.steady>0&&v.steady<v.steadyDry);
            check(v.at100>=0&&v.at500>=0&&v.at500<v.at100);
            std::cout<<"SKELLIERE_LAB rate="<<fs<<" design="<<names[i]
              <<" onset_energy="<<v.onset<<" steady_gain_db="<<10*std::log10(v.steady/v.steadyDry)
              <<" reduction_100ms_db="<<v.at100<<" reduction_500ms_db="<<v.at500<<"\n";
        }
    }
}
