#include "../Source/NativeRackEngines.h"
#include <cstdio>
#include <cstdlib>
#include <new>
using namespace skl;
static bool rt=false;static unsigned allocations=0,deletions=0;
void* operator new(std::size_t n){if(rt)++allocations;if(auto*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{if(rt&&p)++deletions;std::free(p);}void operator delete[](void* p)noexcept{::operator delete(p);}
void operator delete(void* p,std::size_t)noexcept{::operator delete(p);}void operator delete[](void* p,std::size_t)noexcept{::operator delete(p);}
static void check(bool b,const char* why){if(!b){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
static std::vector<float> tone(int sr){std::vector<float>x(sr*2);for(size_t i=0;i<x.size();++i)x[i]=.15f*std::sin(2*kPi*330*i/sr);return x;}
static double diff(const std::vector<float>& a,const std::vector<float>& b){double e=0;for(size_t i=0;i<a.size();++i){check(std::isfinite(a[i])&&std::isfinite(b[i]),"finite");e=std::max(e,std::abs(double(a[i])-b[i]));}return e;}
static void run(ModuleBase& m,float*k,std::vector<float>& a,std::vector<float>& b,int block=257){float* ptr[]={a.data(),b.data()};for(int i=0;i<int(a.size());i+=block){juce::AudioBuffer<float> buf(ptr,2,i,std::min(block,int(a.size())-i));rt=true;m.process(buf,k,true);rt=false;}}
static void delay(int sr){float k[6];for(int j=0;j<6;++j)k[j]=defaultNorm(DelayNative,j);k[5]=1;auto x=tone(sr);auto budget=std::make_shared<native_delay::RackBudget>();NativeDelayRack rack;rack.setBudget(budget);rack.prepare(sr,257,2);auto ext=nativeDefaults();auto state=skellayRackState(k);for(int model=1;model<=2;++model){rack.reset();ext[nDelayModel]=model*.5f;rack.setExtended(ext);check(rack.service(ext,k,true),"prepare model");auto a=x,b=x;run(rack,k,a,b);check(rack.renderedModel()==model,"selected model not audible");auto config=native_delay::configuration(native_delay::selection(model,2,0),state.v);auto direct=skellay::prepare(config,sr,120,257,30);check(bool(direct),"reference prepare");auto c=x,d=x;auto ctl=native_delay::macroControls(native_delay::selection(model,2,0),state.v);for(int i=0;i<int(c.size());i+=257){int n=std::min(257,int(c.size())-i);direct->updateRealtimeControls(ctl);direct->updateTempo(120);check(direct->process(c.data()+i,d.data()+i,n),"reference render");}double error=diff(a,c);check(error<2e-6,"native delay reference parity");check(diff(a,x)>1e-4,"effect audible");std::printf("Delay %d %d Hz max reference error %.9g; reserved %zu bytes\n",model,sr,error,budget->used());rack.service(ext,k,true);}
 ext[nDelayModel]=0;rack.setExtended(ext);check(rack.service(ext,k,true),"return Classic");auto a=x,b=x;run(rack,k,a,b);rack.service(ext,k,true);check(rack.renderedModel()==0,"Classic restored");check(budget->used()==0,"advanced budget released");}
static void pendingDelay(int sr){
 float k[6];for(int j=0;j<6;++j)k[j]=defaultNorm(DelayNative,j);k[5]=1;
 NativeDelayRack reference,rack;reference.prepare(sr,257,2);rack.prepare(sr,257,2);
 auto ext=nativeDefaults();auto x=tone(sr);auto a=x,b=x,c=x,d=x;
 run(reference,k,a,b);run(rack,k,c,d);check(diff(a,c)<1e-6,"Classic baseline");
 ext[nDelayModel]=.5f;rack.setExtended(ext);
 a=x;b=x;c=x;d=x;run(reference,k,a,b);run(rack,k,c,d);
 check(rack.renderedModel()==0,"pending Modern must retain Classic");
 check(diff(a,c)<1e-6&&diff(b,d)<1e-6,"pending Modern changed Classic audio");
 check(rack.service(ext,k,true),"prepare Modern after pending");
 c=x;d=x;run(rack,k,c,d);check(rack.renderedModel()==1,"Modern did not take over");
}
static void glitch(int sr){float k[6];for(int j=0;j<6;++j)k[j]=defaultNorm(GlitchNative,j);k[0]=.5;k[5]=1;auto x=tone(sr);NativeGlitchRack g;g.prepare(sr,257,2);auto ext=nativeDefaults();for(int i=0;i<32;++i)ext[nGlitchState+i]=1.f/3.f;g.setExtended(ext);auto a=x,b=x;run(g,k,a,b);check(diff(a,x)<1e-6,"editable all-on steps");for(int i=0;i<32;++i)ext[nGlitchProbability+i]=0;g.reset();g.setExtended(ext);a=x;b=x;run(g,k,a,b);check(diff(a,x)>.05,"probabilities change output");ext[nGlitchSource]=1.f;g.reset();g.setExtended(ext);a=x;b=x;run(g,k,a,b);check(diff(a,x)<1e-6,"manual waiting dry");g.trigger();a=x;b=x;run(g,k,a,b);check(diff(a,x)>.05,"manual trigger works");std::printf("Glitch %d Hz pattern/probability/manual trigger passed\n",sr);}
int main(){for(int sr:{44100,48000,96000,192000}){delay(sr);pendingDelay(sr);glitch(sr);}check(allocations==0&&deletions==0,"callback memory activity");std::puts("Native rack controls passed; zero callback allocations and deletions");}
