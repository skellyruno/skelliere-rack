#include "../../research/FormantDoublerGradient.h"
#include <cstdio>
#include <fstream>
using namespace skl;
int main(int argc,char**argv){constexpr int sr=48000;for(int j=1;j<argc;++j){std::ifstream in(argv[j],std::ios::binary|std::ios::ate);if(!in)return 2;std::vector<float>x(size_t(in.tellg())/4);in.seekg(0);in.read(reinterpret_cast<char*>(x.data()),x.size()*4);research::FormantDoublerGradient a;a.prepare(sr);PitchDoublerCandidate b;b.prepare(sr);b.setVoices({PitchDoublerCandidate::Voice{18,35,-1,1},PitchDoublerCandidate::Voice{18,-35,1,1}},1);std::ofstream out(std::string(argv[j])+"-gradient.raw",std::ios::binary);double peak=0;for(float v:x){auto y=a.process(v,v),z=b.process(v,v);for(float k:{y[0],y[1],z[0],z[1]}){if(!std::isfinite(k))return 3;out.write(reinterpret_cast<char*>(&k),4);}peak=std::max(peak,double(std::max(std::abs(y[0]),std::abs(y[1]))));}std::printf("Fixture %d gradient candidate peak %.2f dBFS\n",j,20*std::log10(std::max(1e-15,peak)));}}
