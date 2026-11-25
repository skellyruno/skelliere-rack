#include <SkellCore/ResearchWetDuck.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

void check(bool ok) { if (!ok) throw std::runtime_error("wet duck regression"); }
int main() {
    for (double rate : {44100.,48000.,96000.,192000.}) {
        using skellcore::research::WetDuck;
        WetDuck duck(rate), swapped(rate), bypass(rate,0.);
        double env=0.;
        const double minGain=std::pow(10.,-9./20.);
        const double a=std::exp(-1000./(rate*14.)), r=std::exp(-1000./(rate*260.));
        auto oldGain=[&](double peak) { const double c=peak>env?a:r; env=c*env+(1.-c)*peak; return 1.-(1.-minGain)*env/(env+.1); };
        double old=1., now=1.;
        for (int i=0;i<int(rate*2);++i) {
            old=oldGain(.5); now=duck.process(.5,-.2);
            check(now==swapped.process(-.2,.5));
            check(bypass.process(.5,-.2)==1.);
            check(now>=minGain && now<=1.);
        }
        check(std::abs(now-old)<1e-9);
        const double active=now;
        int old90=-1,new90=-1;
        const double recovery=1.-.1*(1.-active);
        for (int i=1;i<=int(rate*3);++i) {
            old=oldGain(0.); const double next=duck.process(0.,0.);
            check(next>=now && next<=1.); now=next;
            if(old90<0 && old>=recovery) old90=i;
            if(new90<0 && now>=recovery) new90=i;
        }
        check(old90>new90 && new90>0);
        check(std::abs(new90/rate-.260*std::log(10.))<2./rate);
        duck.reset(); check(duck.process(0.,0.)==1.);
        check(duck.process(std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity())==1.);
        std::cout<<rate<<" Hz: active gain "<<active<<", old t90 "<<1000.*old90/rate<<" ms, new t90 "<<1000.*new90/rate<<" ms\n";
    }
    bool rejected=false;
    try { skellcore::research::WetDuck invalid(0.); } catch(const std::invalid_argument&) { rejected=true; }
    check(rejected);
}
