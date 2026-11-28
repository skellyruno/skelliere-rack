#include "../Source/NativeEpochLine.h"
#include <cstdio>
#include <cstdlib>
using skl::native_delay::EpochLine;
static void check(bool b,const char* why){if(!b){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
int main(){EpochLine x;std::vector<float> ref;unsigned r=1234;auto next=[&](){r=r*1664525u+1013904223u;return r;};
 x.reserve(8193);for(int epoch=0;epoch<200;++epoch){size_t n=1+next()%8193;x.assign(n,0.f);ref.assign(n,0.f);for(int j=0;j<3000;++j){const auto i=next()%n;const auto k=next()%n;float v=float(int(next()%1000)-500)*.001f;x[i]+=v;ref[i]+=v;check(x[k]==ref[k],"lazy read/write parity");if(j==100){x.reserve(12001+epoch);ref.reserve(12001+epoch);} }const auto& read=x;for(size_t i=0;i<n;++i)check(read[i]==ref[i],"const read and invalid history parity");std::vector<float> other(219,0.25f),expected=other;x.swap(other);ref.swap(expected);check(other==expected,"swap previous history");for(size_t i=0;i<ref.size();++i)check(x[i]==ref[i],"swap incoming history");x.clear();check(x.empty(),"logical clear");}
 std::puts("Epoch delay storage: zeroed history, wrap-sized pages, reserve, repeated reset and swap parity passed");}
