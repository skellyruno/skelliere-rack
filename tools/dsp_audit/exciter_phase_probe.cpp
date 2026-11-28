#include "../../research/ExciterPhaseMatched.h"
#include <cstdio>
using namespace skl;
static double db(double x){return 20*std::log10(std::max(1e-15,x));}
static double bin(const std::vector<float>& x,double f,int sr){double a=0,b=0;for(int i=sr;i<2*sr;++i){a+=x[i]*std::cos(2*kPi*f*i/sr);b+=x[i]*std::sin(2*kPi*f*i/sr);}return 2*std::hypot(a,b)/sr;}
int main(){const int sr=48000;for(int f:{1000,3000,5000,7000,9000,11000}){ExciterCandidate a;a.prepare(sr);a.set(1000,18,1,18000);research::ExciterPhaseMatched b;b.prepare(sr);b.set(1000,18,1,18000);std::vector<float>x(2*sr),y(2*sr);for(int i=0;i<2*sr;++i){const float v=.5f*std::sin(2*kPi*f*i/sr);x[i]=a.process(0,v);y[i]=b.process(0,v);}const int fold=std::abs((3*f+sr/2)%sr-sr/2);std::printf("%d Hz fundamental %.2f/%.2f dBFS, %s third %.2f/%.2f dBFS\n",f,db(bin(x,f,sr)),db(bin(y,f,sr)),3*f>sr/2?"folded":"desired",db(bin(x,fold,sr)),db(bin(y,fold,sr)));}}
