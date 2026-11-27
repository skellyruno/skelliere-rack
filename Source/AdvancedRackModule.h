#pragma once
#include "ConsolidatedModules.h"
#include "AdvancedMultiband.h"
#include "AdvancedColorSpace.h"
#include "ResonanceCandidate.h"
#include "AdvancedControlSpec.h"
namespace skl {
// Legacy processing is sample-identical unless the user explicitly enables the
// advanced engine. During a mode switch both paths crossfade over 20 ms.
class AdvancedRackModule final:public ModuleBase {
public:
    explicit AdvancedRackModule(int t):type(t),legacy(t){}
    void prepare(double sr,int maxBlock,int channels) override {
        rate=std::clamp(std::isfinite(sr)?sr:48000.,8000.,384000.);maxLen=std::max(1,maxBlock);for(auto& s:scratch)s.resize(size_t(maxLen));
        legacy.prepare(rate,maxLen,channels);if(type==Multiband)multiband.prepare(rate);if(type==Resonance)resonance.prepare(rate);if(type==Doubler)doubler.prepare(rate);if(type==Exciter)exciter.prepare(rate);reset();
    }
    void reset() override {legacy.reset();resetCandidate();seeded=false;advancedSeeded=false;legacyWarm=true;controlDirty=false;blend=0;meterDb=0;for(auto& a:bandMeters)a.store(0,std::memory_order_relaxed);}
    void setExtended(const AdvancedValues& x,int solo=-1) noexcept {extended=x;audition=std::clamp(solo,-1,7);}
    float meter() const override {return meterDb.load(std::memory_order_relaxed);}
    float bandReduction(int band) const noexcept {return band>=0&&band<8?bandMeters[size_t(band)].load(std::memory_order_relaxed):0;}
    void process(juce::AudioBuffer<float>& b,const float* k,bool active) override {
        const int ch=std::min(2,b.getNumChannels()),n=b.getNumSamples();if(ch==0||n==0)return;
        const bool enabled=extended[advMode]>.5f;
        if(!seeded){blend=enabled?1.f:0.f;seeded=true;}
        if(enabled&&blend==0&&!advancedSeeded)resetCandidate();
        if(!enabled&&blend==1.f&&!legacyWarm){legacy.reset();legacyWarm=true;}
        if(!enabled&&blend==0){legacy.process(b,k,active);meterDb=legacy.meter();advancedSeeded=false;clearBands();return;}
        if(enabled&&blend==1){render(b,k,active);legacyWarm=false;return;}
        const float step=float(1/(.02*rate));
        for(int offset=0;offset<n;offset+=maxLen){const int len=std::min(maxLen,n-offset);float* pp[2]={scratch[0].data(),scratch[1].data()};for(int c=0;c<ch;++c)std::copy_n(b.getWritePointer(c)+offset,len,pp[c]);
            juce::AudioBuffer<float> old(pp,ch,0,len),now(b.getArrayOfWritePointers(),ch,offset,len);legacy.process(old,k,active);render(now,k,active);
            for(int i=0;i<len;++i){blend+=std::clamp((enabled?1.f:0.f)-blend,-step,step);for(int c=0;c<ch;++c){auto* out=now.getWritePointer(c);out[i]=pp[c][i]+blend*(out[i]-pp[c][i]);}}
        }
        if(blend==0){advancedSeeded=false;clearBands();meterDb=legacy.meter();}if(blend==1)legacyWarm=false;
    }
private:
    void clearBands() noexcept {for(auto& a:bandMeters)a.store(0,std::memory_order_relaxed);}
    void resetCandidate() noexcept {if(type==Multiband)multiband.reset();if(type==Resonance)resonance.reset();if(type==Doubler)doubler.reset();if(type==Exciter)exciter.reset();advancedSeeded=false;tick=0;wet=1;}
    void configure(const float* k) noexcept {
        auto val=[&](int i){return advancedReal(i,extended[size_t(i)]);};
        if(type==Multiband){AdvancedMultiband::Settings s;for(int j=0;j<5;++j)s.crossover[j]=val(advCross+j);
            for(int j=0;j<8;++j){auto& v=j<6?s.bands[j]:s.floating[j-6].dynamics;v.threshold=val(bandControl(j,0))+realOf(type,0,k)+24;v.ratio=val(bandControl(j,1))*realOf(type,1,k)/2;v.range=val(bandControl(j,2))*realOf(type,2,k)/6;v.attack=val(bandControl(j,3))*realOf(type,3,k)/15;v.release=val(bandControl(j,4))*realOf(type,4,k)/150;v.makeup=val(bandControl(j,5));v.enabled=val(bandControl(j,6))>.5f;if(j>=6){auto& f=s.floating[j-6];f.enabled=v.enabled;f.frequency=val(floatingControl(j-6,0));f.q=val(floatingControl(j-6,1));}}
            s.mix=realOf(type,5,k);s.solo=audition;multiband.set(s);
        }else if(type==Resonance)resonance.set(realOf(type,0,k),realOf(type,1,k),realOf(type,2,k),realOf(type,3,k),realOf(type,4,k),realOf(type,5,k));
        else if(type==Doubler){std::array<PitchDoublerCandidate::Voice,2> v;for(int j=0;j<2;++j){auto& x=v[j];x.delayMs=val(voiceControl(j,0))+realOf(type,0,k)-18;x.cents=val(voiceControl(j,1))+(j?-1:1)*(realOf(type,1,k)-8);x.pan=val(voiceControl(j,2))*realOf(type,3,k)/.8f;x.gain=val(voiceControl(j,3));}doubler.setVoices(v,realOf(type,4,k),std::clamp(32.f*.45f/realOf(type,2,k),8.f,45.f));}
        else if(type==Exciter)exciter.set(realOf(type,0,k),realOf(type,1,k),realOf(type,2,k),realOf(type,3,k));
    }
    void render(juce::AudioBuffer<float>& b,const float* k,bool active) noexcept {
        const int n=b.getNumSamples(),ch=std::min(2,b.getNumChannels());auto*l=b.getWritePointer(0);auto*r=ch==2?b.getWritePointer(1):nullptr;
        float targets[kMaxKnobs]{};for(int j=0;j<info(type).numKnobs;++j)targets[j]=std::isfinite(k[j])?std::clamp(k[j],0.f,1.f):defaultNorm(type,j);
        if(!advancedSeeded){std::copy_n(targets,kMaxKnobs,current);wet=active?1.f:0.f;configure(current);lastExtended=extended;lastAudition=audition;advancedSeeded=true;}
        const float step=float(1/(.02*rate)),fade=float(1/(.005*rate));controlDirty|=lastExtended!=extended||lastAudition!=audition;
        for(int i=0;i<n;++i){for(int j=0;j<info(type).numKnobs;++j){const float next=current[j]+std::clamp(targets[j]-current[j],-step,step);controlDirty|=next!=current[j];current[j]=next;}
            if((tick++&31u)==0&&controlDirty){configure(current);controlDirty=false;lastExtended=extended;lastAudition=audition;}
            wet+=std::clamp((active?1.f:0.f)-wet,-fade,fade);const float x=sane(l[i]),y=r?sane(r[i]):0;std::array<float,2> z{x,y};
            if(type==Multiband)z=multiband.process(x,y,ch);else if(type==Resonance)z=resonance.process(x,y,ch);else if(type==Doubler)z=doubler.process(x,y,ch);else if(type==Exciter){z[0]=exciter.process(0,x);if(r)z[1]=exciter.process(1,y);}
            l[i]=wet==0?x:sane(x+wet*(z[0]-x));if(r)r[i]=wet==0?y:sane(y+wet*(z[1]-y));
        }
        float gr=0;if(type==Multiband){const auto g=multiband.gainReduction();for(int j=0;j<8;++j){bandMeters[j].store(g[j]*wet,std::memory_order_relaxed);gr=std::max(gr,g[j]);}}else if(type==Resonance)gr=resonance.gainReduction();meterDb.store(gr*wet,std::memory_order_relaxed);
    }
    int type,maxLen=1,audition=-1,lastAudition=-1;double rate=48000;ConsolidatedModule legacy;AdvancedMultiband multiband;ResonanceCandidate resonance;PitchDoublerCandidate doubler;ExciterCandidate exciter;
    AdvancedValues extended=advancedDefaults(),lastExtended=advancedDefaults();std::vector<float>scratch[2];float current[kMaxKnobs]{},blend=0,wet=1;bool seeded=false,advancedSeeded=false,legacyWarm=true,controlDirty=false;unsigned tick=0;std::atomic<float>meterDb{0};std::array<std::atomic<float>,8>bandMeters{};
};
}
