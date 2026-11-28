#include "../Source/OutputPeakGuard.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
using skl::OutputPeakGuard;
static void check(bool ok,const char* msg){if(!ok){std::fprintf(stderr,"FAIL %s\n",msg);std::exit(1);}}
int main(){for(int sr:{32000,44100,48000,96000,192000}){OutputPeakGuard g;g.prepare(sr);float l=3,r=-.75f;g.process(l,r,false);check(l==3&&r==-.75f,"default off exact");l=.1f;r=-.25f;g.process(l,r,true);check(l==.1f&&r==-.25f,"below ceiling exact");l=11.934f;r=-3.f;g.process(l,r,true);check(l<=g.ceiling&&std::abs(r/l+3.f/11.934f)<1e-6f,"linked overload protection");float prior=0;for(int i=0;i<sr*2;++i){l=.5f;r=-.25f;g.process(l,r,true);check(l>=prior&&l<=.5f&&r==-.5f*l,"monotonic linked release");prior=l;}check(l==.5f,"unity restored");g.reset();for(int i=0;i<sr;++i){l=8.f*std::sin(.073f*i);r=12.f*std::cos(.044f*i);g.process(l,r,true);check(std::max(std::abs(l),std::abs(r))<=g.ceiling,"automated peak bound");}g.reset();l=2;r=-2;g.process(l,r,false);check(l==2&&r==-2,"reset off exact");}std::puts("Output peak guard: five rates, default bypass exact, stereo link, -1 dBFS sample ceiling and release passed");}
