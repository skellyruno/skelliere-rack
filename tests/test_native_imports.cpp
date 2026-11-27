#include "../Source/NativeRackEngines.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <new>
#include <vector>
#include <fstream>
using namespace skl;
static bool tracking=false;static unsigned allocations=0;
void* operator new(std::size_t n){if(tracking)++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
static void check(bool b,const char* why){if(!b){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
static std::unique_ptr<ModuleBase> make(int t){if(t==FlangNative)return std::make_unique<NativeFlangRack>();if(t==DelayNative)return std::make_unique<NativeDelayRack>();return std::make_unique<NativeGlitchRack>();}
static void defaults(int t,float* k){for(int j=0;j<6;++j)k[j]=defaultNorm(t,j);}
static std::vector<float> tone(int sr){std::vector<float>x(sr*2);for(size_t i=0;i<x.size();++i)x[i]=float(.15*std::sin(2*kPi*330*i/sr)+.05*std::sin(2*kPi*2000*i/sr));return x;}
static std::vector<float> render(int t,int mode,int sr,int block,int ch,const std::vector<float>& in,bool active=true,bool animate=false){
 auto m=make(t);m->prepare(sr,block,ch);float k[6]{};defaults(t,k);const int which=t==DelayNative?4:0;const int max=t==FlangNative?3:2;k[which]=float(mode)/max;k[5]=active?1.f:0.f;
 auto a=in,b=in;float* ptr[]={a.data(),b.data()};const unsigned before=allocations;
 for(int i=0;i<int(a.size());){int n=std::min(block,int(a.size())-i);if(animate){n=std::min(n,4096-i%4096);int e=i/4096;k[which]=float(e%(max+1))/max;k[1]=e%2?.95f:.05f;if(t==DelayNative){k[0]=e%2?1.f:.01f;k[3]=e%2?1.f:0.f;}m->setTempo(e%2?30:300);if(e%3==0&&i%4096==0){tracking=true;m->reset();tracking=false;}}
  juce::AudioBuffer<float> buf(ptr,ch,i,n);tracking=true;m->process(buf,k,active);tracking=false;i+=n;}
 check(allocations==before,"callback/reset allocated");for(float v:a)check(std::isfinite(v),"nonfinite left");for(float v:b)check(std::isfinite(v),"nonfinite right");return a;
}
static double maxerr(const std::vector<float>& a,const std::vector<float>& b){double e=0;for(size_t i=0;i<a.size();++i)e=std::max(e,std::abs(double(a[i])-b[i]));return e;}
static void referenceFlang(int mode,int sr){const auto x=tone(sr);auto y=render(FlangNative,mode,sr,257,2,x);float k[6];defaults(FlangNative,k);auto out=x;std::vector<float> r=x;
 if(mode==0){skelly::fruitychorusfx::Engine e;e.setRate(sr);e.load(flangChorusState(k[1],k[2],k[3],k[4]));e.reset();for(size_t i=0;i<x.size();++i){float a[2]={x[i],x[i]};e.render(a,a,1);out[i]=a[0];}}
 else if(mode==2){skell::private_research::FlangusAdapter e;check(e.prepare(sr,flangusState(k[1],k[2],k[3],k[4]),true),"native prepare");check(e.process(out.data(),r.data(),int(out.size())),"native flangus render");}
 else {skellflang::EngineBank e;e.prepare(sr);skellflang::Controls p;p.depth=k[1];p.speed=k[2];p.delay=k[3];p.spread=k[4];for(size_t i=0;i<x.size();++i)out[i]=e.process(mode==1?1:7,x[i],x[i],p)[0];}
 const double err=maxerr(y,out);std::printf("Flang mode %d %d Hz native reference max error %.9g\n",mode,sr,err);check(err<2e-6,"Flang native reference mismatch");}
static void referenceDelay(int sr){auto x=tone(sr),y=render(DelayNative,0,sr,257,2,x);float k[6];defaults(DelayNative,k);auto s=skellayRackState(k);SKELLdelay::Delay2Engine e(s,sr,120,1,true);auto out=x,r=x;for(size_t i=0;i<x.size();++i)e.process(out.data()+i,r.data()+i,1);auto err=maxerr(out,y);std::printf("Delay %d Hz native max error %.9g\n",sr,err);check(err<2e-6,"Delay native reference mismatch");}
static void referenceGlitch(int sr){auto x=tone(sr),y=render(GlitchNative,1,sr,257,2,x),out=x;skell::dsp::TempoClock clock;clock.prepare(sr);clock.setHost(120,4,4,0,false,true,2,500);skell::dsp::GateSequencer q;for(int j=0;j<16;++j)q.setStep(j,j%4==0?2:j%2?0:1,100);skell::dsp::GateShaper g;g.prepare(sr);g.setTimes(3,15);float k[6];defaults(GlitchNative,k);const float close=std::pow(10.f,-96.f*k[2]/20.f);for(size_t i=0;i<x.size();++i){const auto pos=clock.tick(int(i));double c=pos.beat/4.;int step=q.stepAt(c-std::floor(c),16,0);out[i]=x[i]*g.next(q.evaluate(step,int64_t(std::floor(c)),1,close).gain,1);}auto err=maxerr(out,y);std::printf("Glitch gate %d Hz native max error %.9g\n",sr,err);check(err<2e-6,"Glitch gate reference mismatch");}
int main(int argc,char** argv){static_assert(FlangNative==50&&DelayNative==51&&GlitchNative==52&&activeCatalogCount()==34);for(int sr:{44100,48000,96000}){auto x=tone(sr);for(int t:{FlangNative,DelayNative,GlitchNative}){int modes=t==FlangNative?4:3;for(int m=0;m<modes;++m){auto a=render(t,m,sr,257,2,x),b=render(t,m,sr,63,2,x);double err=maxerr(a,b);check(err<2e-6,"block-size mismatch");check(maxerr(a,x)>.001,"engine inaudible");check(render(t,m,sr,511,1,x,false)==x,"dry bypass mismatch");render(t,m,sr,257,1,x,true,true);} }for(int m=0;m<4;++m)referenceFlang(m,sr);referenceDelay(sr);referenceGlitch(sr);}
 if(argc>1){std::ifstream in(argv[1],std::ios::binary);std::vector<float>x(480000);in.read(reinterpret_cast<char*>(x.data()),x.size()*sizeof(float));x.resize(size_t(in.gcount())/sizeof(float));check(x.size()>48000,"missing vocal");for(int t:{FlangNative,DelayNative,GlitchNative})for(int m=0;m<(t==FlangNative?4:3);++m){auto begin=std::chrono::steady_clock::now();auto y=render(t,m,48000,257,2,x);double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();double peak=0,energy=0;for(float v:y){peak=std::max(peak,double(std::abs(v)));energy+=double(v)*v;}std::printf("VOCAL %s mode %d peak %.2f dBFS RMS %.2f dBFS elapsed %.3f sec incl. prepare\n",info(t).name,m,20*std::log10(std::max(peak,1e-12)),10*std::log10(energy/y.size()),seconds);}}
 std::puts("Native imports: reference parity, block invariance, bypass, mono/stereo, automation/reset allocation probe passed");}
