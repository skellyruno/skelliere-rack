// Host-free eight-slot native chain. Owner sources required, never bundle them.
#include "NativeRackEngines.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#if defined(__linux__)
#include <sys/resource.h>
#endif
using namespace skl;
static long rssKB(){std::ifstream in("/proc/self/status");std::string line;while(std::getline(in,line))if(line.rfind("VmRSS:",0)==0)return std::strtol(line.c_str()+6,nullptr,10);return -1;}
static long peakKB(){
#if defined(__linux__)
    rusage r{};return getrusage(RUSAGE_SELF,&r)==0?r.ru_maxrss:-1;
#else
    return -1;
#endif
}
struct Rack {
    std::shared_ptr<native_delay::RackBudget> budget=std::make_shared<native_delay::RackBudget>();
    std::array<std::unique_ptr<ModuleBase>,8> slots;
    std::array<std::array<float,6>,8> knobs{};
    int rejected=0;
    Rack(int sr,int block,int model,int banks){
        const int types[]={FlangNative,DelayNative,GlitchNative,FlangNative,DelayNative,GlitchNative,DelayNative,DelayNative};
        for(int j=0;j<8;++j){int t=types[j];for(int k=0;k<6;++k)knobs[j][k]=defaultNorm(t,k);knobs[j][5]=.25f;
            if(t==DelayNative){auto d=std::make_unique<NativeDelayRack>();d->setBudget(budget);d->prepare(sr,block,2);auto ext=nativeDefaults();ext[nDelayModel]=model*.5f;ext[nDelayBanks]=(banks-1)/7.f;d->setExtended(ext);if(!d->service(ext,knobs[j].data(),true))++rejected;slots[j]=std::move(d);}
            else if(t==FlangNative){auto f=std::make_unique<NativeFlangRack>();f->prepare(sr,block,2);knobs[j][0]=j==0?0.f:2.f/3.f;slots[j]=std::move(f);}
            else{auto g=std::make_unique<NativeGlitchRack>();g->prepare(sr,block,2);slots[j]=std::move(g);}}
    }
    void process(juce::AudioBuffer<float>& b){for(int j=0;j<8;++j)slots[j]->process(b,knobs[j].data(),true);}
};
int main(int argc,char**argv){const int sr=argc>1?std::atoi(argv[1]):48000,instances=argc>2?std::atoi(argv[2]):1,model=argc>3?std::atoi(argv[3]):1,banks=argc>4?std::atoi(argv[4]):2;
 if(sr<32000||sr>192000||instances<1||instances>2||model<0||model>2||banks<1||banks>8)return 2;
 constexpr int block=257;const long before=rssKB();auto start=std::chrono::steady_clock::now();std::vector<std::unique_ptr<Rack>> racks;for(int j=0;j<instances;++j)racks.push_back(std::make_unique<Rack>(sr,block,model,banks));const double prepare=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();const long prepared=rssKB();
 double total=0,maximum=0;std::vector<double> times;int rejected=0;std::size_t reserved=0;for(auto& r:racks){rejected+=r->rejected;reserved+=r->budget->used();}
 juce::AudioBuffer<float> b(2,block);for(int frame=0;frame<sr*3;frame+=block){for(int c=0;c<2;++c)for(int i=0;i<block;++i)b.getWritePointer(c)[i]=.12f*std::sin(2*kPi*(c?331.:330.)*(frame+i)/sr);start=std::chrono::steady_clock::now();for(auto&r:racks)r->process(b);const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();if(frame>=sr){total+=ms;maximum=std::max(maximum,ms);times.push_back(ms);}for(int c=0;c<2;++c)for(int i=0;i<block;++i)if(!std::isfinite(b.getWritePointer(c)[i]))return 3;}
 std::sort(times.begin(),times.end());const double period=1000.*block/sr;std::printf("%d Hz %d instances model %d taps %d: prepare %.2f ms RSS before/prepared/end/peak %ld/%ld/%ld/%ld KiB reserved %zu bytes rejected %d; mean %.3f%% p95 %.3f%% worst %.3f%% of buffer budget\n",sr,instances,model,banks,prepare,before,prepared,rssKB(),peakKB(),reserved,rejected,100*total/times.size()/period,100*times[std::min(times.size()-1,times.size()*95/100)]/period,100*maximum/period);
}
