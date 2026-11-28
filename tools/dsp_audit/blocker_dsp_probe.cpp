#include "../../research/ExciterLongFir.h"
#include "../../research/ExciterPhaseMatched.h"
#include "../../research/FormantDoublerEnvelope.h"
#include <cstdio>
#include <fstream>
using namespace skl;
static double db(double x){return 20*std::log10(std::max(1e-15,x));}
static double bin(const std::vector<float>& x,double f,int sr){double a=0,b=0;for(int i=sr;i<2*sr;++i){a+=x[i]*std::cos(2*kPi*f*i/sr);b+=x[i]*std::sin(2*kPi*f*i/sr);}return 2*std::hypot(a,b)/sr;}
int main(int argc,char**argv){const int sr=48000;for(int f:{1000,3000,5000,7000,9000,11000}){ExciterCandidate a;a.prepare(sr);a.set(1000,18,1,18000);research::ExciterPhaseMatched b;b.prepare(sr);b.set(1000,18,1,18000);research::ExciterLongFir<> c;c.prepare(sr);c.set(1000,18,1,18000);std::vector<float>x(2*sr),y(2*sr),z(2*sr);for(int i=0;i<2*sr;++i){const float v=.5f*std::sin(2*kPi*f*i/sr);x[i]=a.process(0,v);y[i]=b.process(0,v);z[i]=c.process(0,v);}const int fold=std::abs((3*f+sr/2)%sr-sr/2);std::printf("%d Hz fundamental %.2f/%.2f/%.2f dBFS, %s third %.2f/%.2f/%.2f dBFS\n",f,db(bin(x,f,sr)),db(bin(y,f,sr)),db(bin(z,f,sr)),3*f>sr/2?"folded":"desired",db(bin(x,fold,sr)),db(bin(y,fold,sr)),db(bin(z,fold,sr)));}
for(int j=1;j<argc;++j){std::ifstream in(argv[j],std::ios::binary|std::ios::ate);if(!in)return 2;std::vector<float>x(size_t(in.tellg())/4);in.seekg(0);in.read(reinterpret_cast<char*>(x.data()),x.size()*4);research::FormantDoublerEnvelope a;a.prepare(sr);PitchDoublerCandidate b;b.prepare(sr);b.setVoices({PitchDoublerCandidate::Voice{18,35,-1,1},PitchDoublerCandidate::Voice{18,-35,1,1}},1);std::ofstream out(std::string(argv[j])+"-envelope.raw",std::ios::binary);double peak=0;for(float v:x){auto y=a.process(v,v),z=b.process(v,v);for(float k:{y[0],y[1],z[0],z[1]}){if(!std::isfinite(k))return 3;out.write(reinterpret_cast<char*>(&k),4);}peak=std::max(peak,double(std::max(std::abs(y[0]),std::abs(y[1]))));}std::printf("Fixture %d envelope candidate peak %.2f dBFS\n",j,db(peak));}}
