#pragma once
#include "Dsp.h"
#include <limits>
namespace skl {
// Opt-in engine. Legacy processing and saved parameter mappings are unchanged.
// Six complementary crossover bands and two floating dynamic EQ bands.
class AdvancedMultiband {
public:
    struct Band { float threshold=-24,ratio=2,range=6,attack=15,release=150,makeup=0; bool enabled=true; };
    struct Floating { Band dynamics; float frequency=2500,q=4; bool enabled=false; };
    struct Settings {
        std::array<float,5> crossover{120,350,1000,3000,7500};
        std::array<Band,6> bands{};
        std::array<Floating,2> floating{};
        float mix=1;
        int solo=-1; // detector audition: -1 means full processed mix
    };
    void prepare(double sr) noexcept {
        rate=std::clamp(std::isfinite(sr)?sr:48000.,8000.,384000.);
        reset(); set(Settings{});
    }
    void reset() noexcept {
        for(auto& f:split)f.reset();
        for(auto& f:shape)f.reset();
        for(auto& f:detector)f.reset();
        for(auto& f:eq)f.reset();
        energy.fill(0); reduction.fill(0); applied.fill(0); shapeGain.fill(std::numeric_limits<float>::infinity()); eqGain.fill(std::numeric_limits<float>::infinity()); hz.fill(0); floatHz.fill(0); floatQ.fill(0); tick=0; seeded=false; soloWeights.fill(0); soloWeights[0]=1;
    }
    void set(const Settings& s) noexcept {
        target=s; target.mix=bounded(s.mix,0,1,1); target.solo=std::clamp(s.solo,-1,7);
        // Keep every crossover ordered even with hostile automation or low sample rates.
        const float top=float(rate*.40), spacing=1.2f;
        float previous=30.f/spacing;
        for(int i=0;i<5;++i){const float upper=top/std::pow(spacing,float(4-i));target.crossover[i]=bounded(s.crossover[i],std::min(upper,previous*spacing),upper,upper);previous=target.crossover[i];}
        for(int i=0;i<8;++i){auto& b=i<6?target.bands[i]:target.floating[i-6].dynamics;
            b.threshold=bounded(b.threshold,-80,0,-24);b.ratio=bounded(b.ratio,1,20,2);b.range=bounded(b.range,0,24,6);b.attack=bounded(b.attack,.1f,200,15);b.release=bounded(b.release,5,2000,150);b.makeup=bounded(b.makeup,-12,12,0);
            attack[i]=onePole(rate,b.attack);release[i]=onePole(rate,b.release);thresholdPower[i]=db2g(2*b.threshold);ratioFactor[i]=1-1/b.ratio;
        }
        for(auto& f:target.floating){f.frequency=bounded(f.frequency,40,top,2500);f.q=bounded(f.q,.5f,12,4);}
        slew=onePole(rate,20); coefficientSlew=std::pow(slew,32.f); soloSlew=onePole(rate,5);
    }
    std::array<float,8> gainReduction() const noexcept {return reduction;}
    std::array<float,5> crossovers() const noexcept {return target.crossover;}
    std::array<float,2> process(float left,float right,int channels=2) noexcept {
        const int ch=channels==1?1:2; const float input[2]={sane(left),sane(right)};
        if(!seeded){hz=target.crossover;for(int j=0;j<2;++j){floatHz[j]=target.floating[j].frequency;floatQ[j]=target.floating[j].q;} mix=target.mix;soloWeights.fill(0);soloWeights[target.solo+1]=1;seeded=true;}
        if((tick++&31u)==0){
            for(int j=0;j<5;++j){const float next=smooth(hz[j],target.crossover[j],coefficientSlew);const float gain=applied[j]-applied[j+1];if(tick==1||next!=hz[j])split[j].lowPass(rate,next);if(next!=hz[j]||gain!=shapeGain[j]){shape[j].lowShelf(rate,next,gain);shapeGain[j]=gain;}hz[j]=next;}
            for(int j=0;j<2;++j){const float f=smooth(floatHz[j],target.floating[j].frequency,coefficientSlew),q=smooth(floatQ[j],target.floating[j].q,coefficientSlew);const bool changed=tick==1||f!=floatHz[j]||q!=floatQ[j];if(changed){const double w=2*kPi*f/rate,alpha=std::sin(w)/(2*q);detector[j].set(alpha,0,-alpha,1+alpha,-2*std::cos(w),1-alpha);}if(changed||eqGain[j]!=applied[6+j]){eq[j].peaking(rate,f,q,applied[6+j]);eqGain[j]=applied[6+j];}floatHz[j]=f;floatQ[j]=q;}
        }
        mix=smooth(mix,target.mix,slew);float bands[2][6]{};
        for(int c=0;c<ch;++c){float prev=0;for(int j=0;j<5;++j){const float next=split[j].process(c,input[c]);bands[c][j]=next-prev;prev=next;}bands[c][5]=input[c]-prev;}
        for(int j=0;j<6;++j){const float peak=std::max(std::abs(bands[0][j]),std::abs(bands[1][j]));advance(j,peak,target.bands[j],target.bands[j].enabled);}
        float output[2]={input[0],input[1]};
        const float highGain=db2g(applied[5]);for(int c=0;c<ch;++c){output[c]*=highGain;for(auto& f:shape)output[c]=f.process(c,output[c]);}
        float floatingSignal[2][2]{};
        for(int j=0;j<2;++j){const auto& f=target.floating[j];for(int c=0;c<ch;++c)floatingSignal[c][j]=detector[j].process(c,input[c]);advance(6+j,std::max(std::abs(floatingSignal[0][j]),std::abs(floatingSignal[1][j])),f.dynamics,f.enabled&&f.dynamics.enabled);for(int c=0;c<ch;++c)output[c]=eq[j].process(c,output[c]);}
        // Crossfade audition taps; each tap is the actual detector signal, not a
        // second unrelated EQ. Preserve the normal mix when no band is soloed.
        if(target.solo>=0||soloWeights[0]<.99999f){
            for(int j=0;j<9;++j)soloWeights[j]=smooth(soloWeights[j],j==target.solo+1?1.f:0.f,soloSlew);
            for(int c=0;c<ch;++c){float y=soloWeights[0]*(input[c]+mix*(output[c]-input[c]));for(int j=0;j<6;++j)y+=soloWeights[j+1]*bands[c][j];for(int j=0;j<2;++j)y+=soloWeights[j+7]*floatingSignal[c][j];output[c]=y;}
            return {sane(output[0]),ch==2?sane(output[1]):0.f};
        }
        soloWeights.fill(0);soloWeights[0]=1;
        return {sane(input[0]+mix*(output[0]-input[0])),ch==2?sane(input[1]+mix*(output[1]-input[1])):0.f};
    }
private:
    static float bounded(float v,float lo,float hi,float fallback) noexcept {return std::clamp(std::isfinite(v)?v:fallback,lo,hi);}
    static float smooth(float old,float want,float p) noexcept {return want+p*(old-want);}
    void advance(int j,float peak,const Band& b,bool enabled) noexcept {
        const float power=peak*peak;energy[j]=smooth(energy[j],power,power>energy[j]?attack[j]:release[j]);
        const float desired=enabled&&b.range>0&&ratioFactor[j]>0&&energy[j]>thresholdPower[j]?std::min(b.range,std::max(0.f,10.f*std::log10(energy[j])-b.threshold)*ratioFactor[j]):0;
        reduction[j]=smooth(reduction[j],desired,desired>reduction[j]?attack[j]:release[j]);
        applied[j]=smooth(applied[j],enabled?b.makeup-reduction[j]:0,slew);
    }
    double rate=48000;Settings target;Biquad split[5],shape[5],detector[2],eq[2];
    std::array<float,8> energy{},reduction{},applied{},attack{},release{},thresholdPower{},ratioFactor{};
    std::array<float,5> hz{},shapeGain{};std::array<float,2> floatHz{},floatQ{},eqGain{};
    std::array<float,9> soloWeights{};
    float slew=0,coefficientSlew=0,soloSlew=0,mix=1;unsigned tick=0;bool seeded=false;
};
}
