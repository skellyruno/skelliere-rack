#include "../research/NativeDelayExchange.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <new>
#include <vector>
static bool track=false;static unsigned allocations=0,deallocations=0;
void* operator new(std::size_t n){if(track)++allocations;if(auto*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void*p)noexcept{if(track&&p)++deallocations;std::free(p);}void operator delete[](void*p)noexcept{::operator delete(p);}
void operator delete(void*p,std::size_t)noexcept{::operator delete(p);}void operator delete[](void*p,std::size_t)noexcept{::operator delete(p);}
static void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
using namespace skellay;
using namespace skl::native_delay;
static Configuration bank(int count,int os=0){auto s=skelly::fldbank::defaultState();s.p[0]=0;s.p[132]=os;for(int i=0;i<8;++i){s.bank[i].enabled=i<count;s.p[4+16*i+14]=32;s.p[4+16*i+8]=1440+360*i;}return {Model::bank,skelly::fldbank::serialise(s)};}
static void parity(const Configuration& c,int rate){
 constexpr int block=257;Exchange exchange;check(exchange.initialise(c,rate,120,block),"prepare model");auto ref=prepare(c,rate,120,block,30);check(bool(ref),"reference prepare");
 auto controls=liveControls(c);float l[block],r[block],a[block],b[block];double err=0,energy=0;
 for(int pos=0;pos<rate;pos+=block){int n=std::min(block,rate-pos);for(int i=0;i<n;++i){l[i]=a[i]=.15f*std::sin(.031*(pos+i));r[i]=b[i]=.11f*std::cos(.042*(pos+i));}double bpm=pos<rate/2?120.:75.;
 if(c.model==Model::classic)controls.values[3]=pos<rate/2?30:55;
 if(c.model==Model::modern)controls.values[14]=pos<rate/2?1500:2400;
 if(c.model==Model::bank)controls.values[10]=pos<rate/2?20:50;
 ref->updateRealtimeControls(controls);ref->updateTempo(bpm);check(ref->process(a,b,n),"reference process");track=true;const auto ok=exchange.process(l,r,n,bpm,&controls);track=false;check(ok,"exchange process");
 for(int i=0;i<n;++i){err=std::max({err,std::abs(double(a[i])-l[i]),std::abs(double(b[i])-r[i])});energy+=l[i]*l[i]+r[i]*r[i];}}
 check(err==0&&energy>0,"native parity");std::printf("%d %dHz exact parity max %.9g estimated delay storage %.2f MiB, bank measured %.2f MiB\n",int(c.model),rate,err,double(delayStorageEstimate(c,rate,30,block))/MiB,double(ref->delayStorageBytes())/MiB);
}
int main(int argc,char**argv){
 for(int sr:{44100,48000,96000,192000})for(auto m:{Model::classic,Model::modern,Model::bank})parity(m==Model::bank?bank(2):defaults(m),sr);
 auto excessive=bank(8,2);check(delayStorageEstimate(excessive,192000,30,257)>128u*MiB,"bank budget estimate");
 Exchange e;check(e.initialise(defaults(Model::classic),48000,120,257),"initialise exchange");check(!e.publish(excessive,120),"oversized bank accepted");check(!e.publish(defaults(Model::bank),20),"tempo floor ignored");
 float l[257],r[257];for(int k=0;k<12;++k){auto c=k%3==0?defaults(Model::classic):k%3==1?defaults(Model::modern):bank(2);check(e.publish(c,120),"publish");for(int j=0;j<16;++j){std::fill_n(l,257,.1f);std::fill_n(r,257,-.1f);track=true;bool ok=e.process(l,r,257,120);track=false;check(ok,"switch process");for(int i=0;i<257;++i)check(std::isfinite(l[i])&&std::isfinite(r[i]),"nonfinite switch");}e.collect();}
 check(allocations==0&&deallocations==0,"audio allocation or destruction");std::printf("12 model publications, zero new/delete calls in measured processing. Rejected 8-bank 4x overbudget estimate %.2f MiB at 192 kHz.\n",double(delayStorageEstimate(excessive,192000,30,257))/MiB);
 if(argc>1){std::ifstream f(argv[1],std::ios::binary);std::vector<float>x(480000);f.read(reinterpret_cast<char*>(x.data()),x.size()*sizeof(float));x.resize(size_t(f.gcount())/sizeof(float));check(x.size()>48000,"missing vocal");for(auto m:{Model::classic,Model::modern,Model::bank}){auto c=m==Model::bank?bank(2):defaults(m);Exchange y;check(y.initialise(c,48000,120,257),"vocal prepare");auto a=x,b=x;double peak=0,energy=0;for(size_t i=0;i<x.size();i+=257){int n=int(std::min(size_t(257),x.size()-i));check(y.process(a.data()+i,b.data()+i,n,120),"vocal process");for(int j=0;j<n;++j){peak=std::max({peak,double(std::abs(a[i+j])),double(std::abs(b[i+j]))});energy+=double(a[i+j])*a[i+j];}}std::printf("Vocal model %d: peak %.2f dBFS RMS %.2f dBFS\n",int(m),20*std::log10(std::max(peak,1e-12)),10*std::log10(energy/x.size()));}}
}
