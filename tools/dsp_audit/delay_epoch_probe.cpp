// Build twice, using the pinned native Delay and generated storage-only variant.
#include <native/Delay.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <new>
#include <chrono>
#include <cmath>
static bool rt=false;static size_t allocations=0;
void* operator new(size_t n){if(rt)++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}void* operator new[](size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}void operator delete(void* p,size_t)noexcept{std::free(p);}void operator delete[](void* p,size_t)noexcept{std::free(p);}
int main(){using namespace SKELLdelay;using Clock=std::chrono::steady_clock;for(int sr:{32000,44100,48000,96000,192000}){uint64_t hash=14695981039346656037ull;double total=0,worst=0;Delay2State s{delay2Default};Delay2Engine e(s,sr,120,1,true);e.reserve(30);for(int epoch=0;epoch<24;++epoch){s.v[4]=1+(epoch*127)%768;s.v[5]=(epoch%2?1:-1)*(epoch*33%513);s.v[6]=epoch%3;s.v[3]=50+epoch;double tempo=epoch%3==0?30:epoch%3==1?120:300;auto start=Clock::now();rt=true;if(epoch%4==0)e.restart(s,tempo,true);else e.set(s,tempo);rt=false;double us=std::chrono::duration<double,std::micro>(Clock::now()-start).count();total+=us;worst=std::max(worst,us);for(int i=0;i<sr/8;++i){float a=.4f*std::sin(.031*i),b=.3f*std::cos(.021*i);rt=true;e.process(&a,&b,1);rt=false;for(float v:{a,b}){if(!std::isfinite(v))return 2;uint32_t bits;std::memcpy(&bits,&v,4);hash^=bits;hash*=1099511628211ull;}}}
 // Separate live-timing path (control-thread preparation).
 Delay2Engine live(s,sr,120,1,true);live.enableLiveTiming(30);for(int i=0;i<sr/4;++i){if(i%1024==0){s.v[4]=1+(i/1024*73)%768;rt=true;live.set(s,120);rt=false;}float a=.2f*std::sin(.04*i),b=-a;rt=true;live.process(&a,&b,1);rt=false;for(float v:{a,b}){uint32_t bits;std::memcpy(&bits,&v,4);hash^=bits;hash*=1099511628211ull;}}
 std::printf("%d Hz hash %016llx allocations %zu\n",sr,(unsigned long long)hash,allocations);std::fprintf(stderr,"%d Hz 24 timing resets: total %.2f us, worst %.2f us\n",sr,total,worst);}return allocations?1:0;}
