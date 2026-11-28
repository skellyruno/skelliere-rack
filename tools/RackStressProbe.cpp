// Offline wall-clock stress probe of the actual JUCE processor. It is not a DAW
// benchmark: deadline misses include scheduler noise. No private audio required.
#include "../Source/PluginProcessor.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <new>
#include <numeric>
#if defined(__linux__)
#include <time.h>
#endif
static double threadSeconds(){
#if defined(__linux__)
    timespec ts{};if(clock_gettime(CLOCK_THREAD_CPUTIME_ID,&ts)==0)return ts.tv_sec+1e-9*ts.tv_nsec;
#endif
    return -1.;
}
static thread_local bool inCallback=false;static size_t allocations=0,deletions=0;
void* operator new(size_t n){if(inCallback)++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{if(inCallback&&p)++deletions;std::free(p);}void operator delete[](void* p)noexcept{::operator delete(p);}
void operator delete(void* p,size_t)noexcept{::operator delete(p);}void operator delete[](void* p,size_t)noexcept{::operator delete(p);}
static long rss(){std::ifstream in("/proc/self/status");std::string line;while(std::getline(in,line))if(line.rfind("VmRSS:",0)==0)return std::strtol(line.c_str()+6,nullptr,10);return -1;}
static void set(SkellierProcessor& p,const juce::String& id,float v){p.apvts.getParameter(id)->setValueNotifyingHost(v);}
int main(int argc,char** argv){
    juce::ScopedJuceInitialiser_GUI gui;
    const int sr=argc>1?std::atoi(argv[1]):192000,instances=argc>2?std::atoi(argv[2]):1,seconds=argc>3?std::atoi(argv[3]):30;
    const bool catalog=argc>4&&std::string(argv[4]).find("catalog")==0;
    const bool guard=argc>4&&std::string(argv[4]).find("guard")!=std::string::npos;
    if(sr<32000||sr>192000||instances<1||instances>4||seconds<2||seconds>300)return 2;
    const int blocks[]={32,64,127,257,511,1024,17};using Clock=std::chrono::steady_clock;
    const long baseline=rss();std::vector<std::unique_ptr<SkellierProcessor>> racks;
    for(int j=0;j<instances;++j){auto p=std::make_unique<SkellierProcessor>();p->clearRack();p->setRateAndBufferSizeDetails(sr,257);p->prepareToPlay(sr,257);set(*p,"output_guard",guard?1.f:0.f);racks.push_back(std::move(p));}
    const long empty=rss();const int native[]={skl::FlangNative,skl::DelayNative,skl::GlitchNative,skl::FlangNative,skl::DelayNative,skl::GlitchNative,skl::DelayNative,skl::DelayNative};
    int epochs=catalog?(skl::kNumRackTypes-1+7)/8:1;std::vector<double> usage,cpuUsage;size_t cpuMisses=0,misses=0,nonfinite=0,limitHits=0;double maxPeak=0;int tested=0,wrong=0;long loaded=0;
    float l[1024]{},r[1024]{};float* ptr[]={l,r};juce::MidiBuffer midi;
    for(int epoch=0;epoch<epochs;++epoch){
        for(auto& p:racks){p->clearRack();for(int s=0;s<8;++s){const int t=catalog?1+epoch*8+s:native[s];if(t>=skl::kNumRackTypes||!skl::importedAvailable(t))continue;if(!p->addModule(t,s))return 3;
            if(skl::hasAdvancedEngine(t))set(*p,p->advancedId(s,skl::advMode),1);
            if(t==skl::DelayNative)set(*p,p->nativeId(s,skl::nDelayModel),sr>=96000?1.f:.5f);
            if(t==skl::FlangNative)set(*p,p->knobId(s,0),s==0?1.f/3.f:1.f);
        }p->serviceNative();}
        for(int s=0;s<8;++s){int t=catalog?1+epoch*8+s:native[s];if(t<skl::kNumRackTypes&&skl::importedAvailable(t))++tested;}
        loaded=std::max(loaded,rss());const int frames=sr*seconds;int blockIndex=0;
        for(int i=0;i<frames;){int n=std::min(blocks[blockIndex%7],frames-i);++blockIndex;
            // Host-style controls outside the callback. Mode switches intentionally
            // exercise original reset semantics at extreme rates and buffer sizes.
            if(blockIndex%257==0){for(auto& p:racks){for(int s=0;s<8;++s){int t=p->slotType(s);if(t==skl::Empty)continue;const float v=float((blockIndex/257+s)%11)/10;set(*p,p->knobId(s,(blockIndex/257)%skl::info(t).numKnobs),v);if(t==skl::FlangNative)set(*p,p->knobId(s,0),float((blockIndex/257+s)%4)/3);if(t==skl::DelayNative){set(*p,p->nativeId(s,skl::nDelayModel),sr>=96000?1.f:.5f);} }p->serviceNative();}}
            const auto start=Clock::now();double elapsed=0,cpuElapsed=0;
            for(auto& p:racks){for(int k=0;k<n;++k){const int frame=i+k;const float impulse=frame%9973==0?.8f:0.f;const float env=frame%(sr*2)<sr?1.f:0.f;l[k]=env*(.12f*std::sin(2*skl::kPi*330*frame/sr)+.04f*std::sin(2*skl::kPi*5300*frame/sr))+impulse;r[k]=env*.1f*std::sin(2*skl::kPi*441*frame/sr)-impulse;}
                juce::AudioBuffer<float> b(ptr,2,n);const auto tc=threadSeconds();auto a=Clock::now();inCallback=true;p->processBlock(b,midi);inCallback=false;elapsed+=std::chrono::duration<double>(Clock::now()-a).count();const auto tcEnd=threadSeconds();if(tc>=0&&tcEnd>=tc)cpuElapsed+=tcEnd-tc;
                for(int k=0;k<n;++k)for(float v:{l[k],r[k]}){if(!std::isfinite(v))++nonfinite;else{maxPeak=std::max(maxPeak,double(std::abs(v)));if(std::abs(v)>=16)++limitHits;}}
            }
            (void)start;if(i>=sr){double load=100*elapsed*sr/n;usage.push_back(load);if(load>100)++misses;if(cpuElapsed>0){const double cpuLoad=100*cpuElapsed*sr/n;cpuUsage.push_back(cpuLoad);if(cpuLoad>100)++cpuMisses;}}i+=n;
        }
        for(auto& p:racks)for(int s=0;s<8;++s)if(p->slotType(s)==skl::DelayNative){const bool blocked=p->nativeDelayBlocked(s);const int model=p->nativeDelayModel(s);if(blocked||model!=(sr>=96000?2:1)){++wrong;std::printf("Delay slot %d: blocked=%d actual=%d expected=%d\n",s,blocked,model,sr>=96000?2:1);}}
    }
    const long steady=rss();std::sort(usage.begin(),usage.end());const double mean=std::accumulate(usage.begin(),usage.end(),0.)/usage.size();
    std::printf("%d Hz x%d %s %d sec/epoch, %d slots exercised; RSS baseline/empty/loaded/steady %ld/%ld/%ld/%ld KiB; mean/p95/p99/worst %.2f/%.2f/%.2f/%.2f%%, misses %zu/%zu, callback new/delete %zu/%zu, nonfinite %zu, safety ceiling hits %zu, peak %.3f, wrong delay %d\n",sr,instances,catalog?"catalog":"native",seconds,tested,baseline,empty,loaded,steady,mean,usage[usage.size()*95/100],usage[usage.size()*99/100],usage.back(),misses,usage.size(),allocations,deletions,nonfinite,limitHits,maxPeak,wrong);
    if(!cpuUsage.empty()){std::sort(cpuUsage.begin(),cpuUsage.end());const double meanCPU=std::accumulate(cpuUsage.begin(),cpuUsage.end(),0.)/cpuUsage.size();std::printf("Thread CPU budget mean/p95/p99/worst %.2f/%.2f/%.2f/%.2f%%, misses %zu/%zu (clock overhead included)\n",meanCPU,cpuUsage[cpuUsage.size()*95/100],cpuUsage[cpuUsage.size()*99/100],cpuUsage.back(),cpuMisses,cpuUsage.size());}
    for(auto& p:racks){p->releaseResources();if(p->gainReduction(0)!=0||p->advancedBandReduction(0,0)!=0)return 6;}
    std::printf("RSS after release (processors retained): %ld KiB\n",rss());
    for(auto& p:racks){p->prepareToPlay(sr,257);juce::AudioBuffer<float> b(ptr,2,17);b.clear();inCallback=true;p->processBlock(b,midi);inCallback=false;for(int i=0;i<17;++i)if(!std::isfinite(l[i])||!std::isfinite(r[i]))return 7;p->releaseResources();}
    std::printf("Reprepare and release passed; total callback new/delete %zu/%zu\n",allocations,deletions);
    return nonfinite||wrong||allocations||deletions||(guard&&maxPeak>skl::OutputPeakGuard::ceiling)?1:0;
}
