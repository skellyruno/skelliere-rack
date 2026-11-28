#include "../../research/FormantDoublerLpc.h"
#include <cstdio>
#include <fstream>
#include <vector>
using namespace skl;
static double db(double x){return 20*std::log10(std::max(1e-15,x));}
static double bin(const std::vector<float>& x,double f,int sr){double a=0,b=0;for(int i=sr;i<2*sr;++i){a+=x[i]*std::cos(2*kPi*f*i/sr);b+=x[i]*std::sin(2*kPi*f*i/sr);}return 2*std::hypot(a,b)/sr;}
int main(int argc,char**argv){
 for(int sr:{44100,48000,96000,192000}){research::LpcEnvelope e;e.prepare(sr);double err=0;for(int i=0;i<sr;++i){float x=.2f*std::sin(2*kPi*217*i/sr)+.08f*std::sin(2*kPi*1970*i/sr);err=std::max(err,std::abs(double(e.restore(e.whiten(x)))-x));}std::printf("LPC %d Hz analysis/synthesis max error %.9g, faults %u\n",sr,err,e.faultCount());if(err>1e-5||e.faultCount())return 1;}
 const int sr=48000;for(int f:{1000,3000,5000,7000,9000,11000}){ExciterCandidate a;a.prepare(sr);a.set(1000,18,1,18000);ExciterOversampledCandidate b;b.prepare(sr);b.set(1000,18,1,18000);std::vector<float>x(2*sr),y(2*sr);for(int i=0;i<2*sr;++i){const float v=.5f*std::sin(2*kPi*f*i/sr);x[i]=a.process(0,v);y[i]=b.process(0,v);}const int fold=std::abs((3*f+sr/2)%sr-sr/2);std::printf("Exciter %d Hz fundamental %.2f/%.2f dBFS, %s third %d Hz %.2f/%.2f dBFS\n",f,db(bin(x,f,sr)),db(bin(y,f,sr)),3*f>sr/2?"folded":"desired",fold,db(bin(x,fold,sr)),db(bin(y,fold,sr)));}
 for(int j=1;j<argc;++j){std::ifstream in(argv[j],std::ios::binary|std::ios::ate);if(!in)return 2;std::vector<float>x(size_t(in.tellg())/4);in.seekg(0);in.read(reinterpret_cast<char*>(x.data()),x.size()*4);research::FormantDoublerLpc a;a.prepare(sr);a.set(18,35,1);PitchDoublerCandidate b;b.prepare(sr);b.setVoices({PitchDoublerCandidate::Voice{18,35,-1,1},PitchDoublerCandidate::Voice{18,-35,1,1}},1);std::ofstream out(std::string(argv[j])+"-lpc-comparison.raw",std::ios::binary);double peak=0;for(float v:x){auto y=a.process(v,v),z=b.process(v,v);for(float k:{y[0],y[1],z[0],z[1]})out.write(reinterpret_cast<char*>(&k),4);peak=std::max(peak,double(std::max(std::abs(y[0]),std::abs(y[1]))));}std::printf("Private fixture %d: LPC peak %.2f dBFS faults %u\n",j,db(peak),a.faults());if(a.faults())return 3;}
}
