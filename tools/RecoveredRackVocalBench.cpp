#include "RackModuleFactory.h"
#include <fstream>
#include <chrono>
#include <iostream>
// Input and output are interleaved stereo float32 little-endian raw PCM.
// Keep private recordings outside the repository.
using namespace skl;
int main(int argc,char**argv){if(argc!=5)return 1;int sr=atoi(argv[3]);int type=atoi(argv[4]);std::ifstream f(argv[1],std::ios::binary|std::ios::ate);if(!f||f.tellg()<=0||size_t(f.tellg())%8||sr<8000||sr>192000||type<1||type>=kNumRackTypes)return 2;std::vector<float>a(size_t(f.tellg())/sizeof(float));f.seekg(0);f.read((char*)a.data(),a.size()*sizeof(float));auto m=makeRackModule(type);m->prepare(sr,257,2);float k[6]{};for(int i=0;i<info(type).numKnobs;++i)k[i]=defaultNorm(type,i);if(type==SkellMixHead)k[1]=.65f;else if(type==SkellDemon){k[0]=.25f;k[1]=.35f;}else if(type==SkellNoise){k[0]=.85f;}std::vector<float> l(a.size()/2+m->latency()),r(l.size());for(size_t i=0;i<a.size()/2;++i){l[i]=a[i*2];r[i]=a[i*2+1];}float* ptr[]={l.data(),r.data()};auto start=std::chrono::steady_clock::now();for(int i=0;i<(int)l.size();i+=257){juce::AudioBuffer<float>b(ptr,2,i,std::min(257,(int)l.size()-i));m->process(b,k,true);}double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();std::cout<<"type "<<type<<" latency "<<m->latency()<<" frames "<<l.size()<<" elapsed "<<sec<<" realtime factor "<<l.size()/sr/sec<<" ns/stereo frame "<<sec*1e9/l.size()<<"\n";std::ofstream o(argv[2],std::ios::binary);for(size_t i=m->latency();i<l.size();++i){o.write((char*)&l[i],4);o.write((char*)&r[i],4);}return !o;}
