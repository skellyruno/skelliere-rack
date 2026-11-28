#include "RackModuleFactory.h"
#define FetCompressor OldFetCompressor
#define RackCompressor OldRackCompressor
#define fetColorResidual oldFetColorResidual
#include "BaselineRackCompressor.h"
#undef FetCompressor
#undef RackCompressor
#undef fetColorResidual
#define ConsolidatedModule OldConsolidatedModule
#include "BaselineConsolidatedModules.h"
#undef ConsolidatedModule
#include <chrono>
#include <cstdio>
#include <fstream>
using namespace skl;
using Samples=std::vector<float>;
static std::unique_ptr<ModuleBase> module(int t,bool old){if(old&&t==FastComp)return std::make_unique<OldFetCompressor>();if(old&&(t==LevelUnified||t==DriveUnified||t==Exciter||t==AirUnified||t==ClipUnified||t==StereoUnified))return std::make_unique<OldConsolidatedModule>(t);return makeRackModule(t);}
static constexpr int chain[]={LevelUnified,FastComp,BusComp,SkellRubyQ,DriveUnified,Exciter,AirUnified,ClipUnified};
static double run(bool old,int rate,int block,bool automation,const Samples& src,Samples& l,Samples& r){
 std::vector<std::unique_ptr<ModuleBase>> mods;for(int t:chain){mods.push_back(module(t,old));mods.back()->prepare(rate,block,2);}l=src;r=src;for(size_t i=0;i<r.size();i++)r[i]*=.8f;
 auto now=std::chrono::steady_clock::now();for(int i=0;i<int(l.size());i+=block){float* data[]={l.data()+i,r.data()+i};juce::AudioBuffer<float> b(data,2,0,std::min(block,int(l.size())-i));for(int j=0;j<8;j++){float k[6]{};for(int k0=0;k0<info(chain[j]).numKnobs;k0++)k[k0]=defaultNorm(chain[j],k0);if(automation){k[0]=.35f+.25f*std::sin(float(i)*.00017f);if(chain[j]==FastComp)k[5]=float((i/(rate/3))%2);if(chain[j]==LevelUnified)k[1]=.5f+.35f*std::sin(float(i)*.00003f);}mods[j]->process(b,k,true);}}
 return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-now).count();}
int main(int argc,char**argv){for(int sr:{44100,48000,96000}){Samples src(sr*4),a,b,c,d;for(int i=0;i<int(src.size());i++)src[i]=.25f*std::sin(2*kPi*173.*i/sr)+.07f*std::sin(2*kPi*5300.*i/sr);for(bool automate:{false,true}){double best0=1e9,best1=1e9;for(int rep=0;rep<3;rep++){best0=std::min(best0,run(true,sr,257,automate,src,a,b));best1=std::min(best1,run(false,sr,257,automate,src,c,d));}double err=0;for(size_t i=0;i<a.size();i++)err=std::max({err,double(std::abs(a[i]-c[i])),double(std::abs(b[i]-d[i]))});printf("%d Hz automation=%d baseline %.3f ms optimized %.3f ms saving %.2f%% max error %.9g\n",sr,automate,best0,best1,100*(1-best1/best0),err);if(err!=0)return 1;}}
std::ifstream f(argc>1?argv[1]:"",std::ios::binary|std::ios::ate);if(f){Samples src(size_t(f.tellg())/sizeof(float));f.seekg(0);f.read(reinterpret_cast<char*>(src.data()),src.size()*sizeof(float));Samples a,b,c,d;auto t0=run(true,48000,257,false,src,a,b),t1=run(false,48000,257,false,src,c,d);double err=0,peak=0;for(size_t i=0;i<a.size();i++){err=std::max({err,double(std::abs(a[i]-c[i])),double(std::abs(b[i]-d[i]))});peak=std::max(peak,double(std::abs(c[i])));}printf("vocal %zu samples: %.3f -> %.3f ms, peak %.4f, max error %.9g\n",src.size(),t0,t1,peak,err);if(err!=0)return 2;}}
