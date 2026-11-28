// Compare this exact harness against the untouched and build-local compact
// owner headers. Identical hashes require identical sample bits in both builds.
#include <FlangDSP.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <new>
static bool callback=false;static unsigned allocations=0;
void* operator new(std::size_t n){if(callback)++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
int main(){
    for(int sr:{32000,44100,48000,96000,192000}){
        skellflang::EngineBank e;e.prepare(sr);uint64_t hash=14695981039346656037ull;skellflang::Controls p;p.order=3;
        for(int epoch=0;epoch<16;++epoch){const int mode=epoch%2?1:7;callback=true;e.prepare(sr);callback=false;
            for(int i=0;i<sr/3;++i){const int step=i/257;p.depth=float((step*11+epoch)%101)/100;p.speed=float((step*7+epoch)%101)/100;p.spread=float((step*3+epoch)%101)/100;p.delay=float((step*13+epoch)%101)/100;p.motionQuality=epoch%2?1.f:0.f;const float l=.4f*std::sin(2*skellflang::pi*331*i/sr),r=.35f*std::cos(2*skellflang::pi*1709*i/sr);callback=true;auto y=e.process(mode,l,r,p);callback=false;for(float v:y){if(!std::isfinite(v))return 2;uint32_t bits;std::memcpy(&bits,&v,4);hash^=bits;hash*=1099511628211ull;}}
        }
        std::printf("%d Hz hash %016llx callback allocations %u\n",sr,(unsigned long long)hash,allocations);
    }
    return allocations?1:0;
}
