// Real JUCE processor stress probe. Build with the renderer's include/define/link
// flags and owner sources. Never publish private dependencies or audio.
#include "../Source/PluginProcessor.h"
#include <chrono>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#if defined(__linux__)
#include <sys/resource.h>
#endif
static long rssKB(){std::ifstream in("/proc/self/status");std::string line;while(std::getline(in,line))if(line.rfind("VmRSS:",0)==0)return std::strtol(line.c_str()+6,nullptr,10);return -1;}
static long peakKB(){
#if defined(__linux__)
    rusage r{};return getrusage(RUSAGE_SELF,&r)==0?r.ru_maxrss:-1;
#else
    return -1;
#endif
}
int main(int argc,char** argv){
#if !SKELLIERE_NATIVE_IMPORTS
    std::fprintf(stderr,"Requires owner-source build\n");return 2;
#else
    juce::ScopedJuceInitialiser_GUI gui;const int sr=argc>1?std::atoi(argv[1]):48000,instances=argc>2?std::atoi(argv[2]):1,model=argc>3?std::atoi(argv[3]):1;
    if(sr<32000||sr>192000||instances<1||instances>2||model<0||model>2)return 2;
    constexpr int block=257;using Clock=std::chrono::steady_clock;auto ms=[](auto t){return std::chrono::duration<double,std::milli>(Clock::now()-t).count();};
    const int types[]={skl::FlangNative,skl::DelayNative,skl::GlitchNative,skl::FlangNative,skl::DelayNative,skl::GlitchNative,skl::DelayNative,skl::DelayNative};
    const long baseline=rssKB();auto start=Clock::now();std::vector<std::unique_ptr<SkellierProcessor>> rack;
    for(int j=0;j<instances;++j){auto p=std::make_unique<SkellierProcessor>();p->clearRack();p->setRateAndBufferSizeDetails(sr,block);p->prepareToPlay(sr,block);rack.push_back(std::move(p));}
    const double emptyMs=ms(start);const long emptyRSS=rssKB();start=Clock::now();
    for(auto& p:rack){for(int j=0;j<8;++j){if(!p->addModule(types[j],j))return 3;p->apvts.getParameter(SkellierProcessor::knobId(j,5))->setValueNotifyingHost(.25f);if(types[j]==skl::DelayNative)p->apvts.getParameter(SkellierProcessor::nativeId(j,skl::nDelayModel))->setValueNotifyingHost(model*.5f);if(types[j]==skl::FlangNative)p->apvts.getParameter(SkellierProcessor::knobId(j,0))->setValueNotifyingHost(j==0?0.f:2.f/3.f);}p->serviceNative();}
    const double loadMs=ms(start);const long loadedRSS=rssKB();
    std::vector<juce::AudioBuffer<float>> buffers;for(int j=0;j<instances;++j)buffers.emplace_back(2,block);juce::MidiBuffer midi;std::vector<double> times;double peak=0;int rejects=0,wrong=0;float maxPeak=0;
    for(int frame=0;frame<sr*3;frame+=block){for(auto& b:buffers)for(int c=0;c<2;++c)for(int i=0;i<block;++i)b.setSample(c,i,.12f*std::sin(2*skl::kPi*(c?331.:330.)*(frame+i)/sr));start=Clock::now();for(int j=0;j<instances;++j)rack[j]->processBlock(buffers[j],midi);const double time=ms(start);if(frame>=sr){times.push_back(time);peak=std::max(peak,time);}for(auto& b:buffers)for(int c=0;c<2;++c)for(int i=0;i<block;++i){float x=b.getSample(c,i);if(!std::isfinite(x))return 4;maxPeak=std::max(maxPeak,std::abs(x));}}
    for(auto& p:rack)for(int j=0;j<8;++j)if(types[j]==skl::DelayNative){rejects+=p->nativeDelayBlocked(j);wrong+=p->nativeDelayModel(j)!=model;}
    const long steady=rssKB();std::sort(times.begin(),times.end());const double period=1000.*block/sr,avg=std::accumulate(times.begin(),times.end(),0.)/times.size();
    std::printf("%d Hz %d complete processors model %d: empty prepare %.1f ms loaded %.1f ms; RSS base/empty/loaded/steady/peak %ld/%ld/%ld/%ld/%ld KiB; mean %.3f%% p95 %.3f%% max %.3f%% budget, wrong/rejected %d/%d, latency %d samples, peak %.3f\n",sr,instances,model,emptyMs,loadMs,baseline,emptyRSS,loadedRSS,steady,peakKB(),100*avg/period,100*times[times.size()*95/100]/period,100*peak/period,wrong,rejects,rack[0]->getLatencySamples(),maxPeak);
    for(auto& p:rack)p->releaseResources();rack.clear();std::printf("RSS after destruction %ld KiB\n",rssKB());return wrong||rejects?5:0;
#endif
}
