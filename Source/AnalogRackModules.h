#pragma once
#include "Dsp.h"
namespace skl {
// Independent Skell DSP. Bounded preallocation, fixed sample cadence for
// coefficient updates and control ramps, no allocation in process.
class AnalogRackModule final : public ModuleBase {
public:
    explicit AnalogRackModule(int type):type_(type){}
    void prepare(double sr,int,int) override {
        sr_=std::isfinite(sr)&&sr>=8000?sr:48000;
        if(type_==Flanger||type_==Slap)
            for(auto& d:delay_)d.prepare(static_cast<int>(std::ceil(sr_*(type_==Slap?.125:.016)))+8);
        reset();
    }
    void reset() override {
        for(auto& f:filters_)f.reset();
        for(auto& d:delay_)if(!d.buf.empty())d.clear();
        for(auto& c:allpass_)for(auto& z:c)z=0;
        feedback_[0]=feedback_[1]=tone_[0]=tone_[1]=0;
        fast_=slow_=gainDb_=phase_=0;wet_=1;seeded_=false;dirty_=true;countdown_=0;
        meter_.store(0,std::memory_order_relaxed);
    }
    float meter()const override{return meter_.load(std::memory_order_relaxed);}
    void process(juce::AudioBuffer<float>& b,const float* k,bool active) override {
        const int ch=std::min(2,b.getNumChannels()),n=b.getNumSamples();
        if(ch==0||n==0)return;
        const int count=info(type_).numKnobs;
        float targets[kMaxKnobs]{};
        for(int j=0;j<count;++j)targets[j]=std::isfinite(k[j])?std::clamp(k[j],0.f,1.f):defaultNorm(type_,j);
        if(!seeded_){for(int j=0;j<count;++j)current_[j]=targets[j];wet_=active?1.f:0.f;seeded_=true;countdown_=0;dirty_=true;}
        const float step=static_cast<float>(1/(.020*sr_)),fade=static_cast<float>(1/(.005*sr_));
        float* data[2]={b.getWritePointer(0),ch==2?b.getWritePointer(1):nullptr};
        for(int i=0;i<n;++i) {
            for(int j=0;j<count;++j){const float v=current_[j]+std::clamp(targets[j]-current_[j],-step,step);dirty_=dirty_||v!=current_[j];current_[j]=v;}
            if(countdown_==0){if(dirty_){update();dirty_=false;}countdown_=16;}--countdown_;
            wet_+=std::clamp((active?1.f:0.f)-wet_,-fade,fade);
            const float a=sane(data[0][i]),r=ch==2?sane(data[1][i]):a;
            const float peak=std::min(16.f,std::max(std::abs(a),std::abs(r)));
            float linked=1,mod=0,ap=0;
            if(type_==Gate) {
                fast_=std::max(peak,fast_*follow_);
                const float target=-std::clamp((v_[0]-g2db(fast_))*2.f,0.f,v_[1]);
                const float coeff=target>gainDb_?attack_:release_;
                gainDb_=target+(gainDb_-target)*coeff;linked=db2g(gainDb_);
            } else if(type_==Transient) {
                fast_=peak+(fast_-peak)*(peak>fast_?attack_:release_);
                slow_=peak+(slow_-peak)*slowCoeff_;
                const float shape=std::clamp(4.f*(fast_-slow_)/(fast_+slow_+.0001f),-1.f,1.f);
                linked=db2g(v_[0]*std::max(0.f,shape)+v_[1]*std::max(0.f,-shape)+v_[3]);
            } else if(type_==Tremolo||type_==Phaser||type_==Flanger) {
                mod=.5f+.5f*std::sin(phase_);
                phase_+=phaseStep_;if(phase_>=float(2*kPi))phase_-=float(2*kPi);
                if(type_==Tremolo)linked=1-v_[1]*(mod+(mod*mod-mod)*v_[2]);
                if(type_==Phaser){const float freq=std::min(float(sr_*.42),180.f+v_[1]*3300.f*mod);
                    const float t=std::tan(float(kPi)*freq/float(sr_));ap=(1-t)/(1+t);}
            }
            for(int c=0;c<ch;++c){
                const float x=c==0?a:r;float y=x;
                switch(type_){
                case Gate:case Transient:case Tremolo:y=x*linked;break;
                case Bell:y=filters_[0].process(c,x)*out_;break;
                case DualFilter:{const float hp=filters_[0].process(c,x),cut=x+hpMix_*(hp-x);
                    const float lp=filters_[1].process(c,cut),filtered=cut+lpMix_*(lp-cut);y=x+v_[3]*(filtered-x);break;}
                case SoftClip:{const float z=std::clamp(x*drive_/ceiling_,-32.f,32.f);
                    const float clipped=ceiling_*std::tanh(z);y=x+v_[2]*(clipped-x);break;}
                case Phaser:{float z=x+v_[2]*feedback_[c];
                    for(auto& state:allpass_[c]){const float t=-ap*z+state;state=z+ap*t;z=t;}
                    feedback_[c]=std::clamp(sane(z),-16.f,16.f);y=x+v_[3]*(z-x);break;}
                case Flanger:{const float tap=delay_[c].read(static_cast<float>(sr_*.001)*(1.f+v_[1]*8.f*mod));
                    delay_[c].write(std::clamp(x+v_[2]*tap,-16.f,16.f));y=x+v_[3]*(tap-x);break;}
                case Slap:{const float tap=delay_[c].read(static_cast<float>(sr_*.001)*v_[0]);
                    delay_[c].write(std::clamp(x,-16.f,16.f));tone_[c]=tap+(tone_[c]-tap)*toneCoeff_;y=x+v_[2]*tone_[c];break;}
                default:break;
                }
                data[c][i]=wet_==0?x:sane(x+wet_*(y-x));
            }
        }
        meter_.store(type_==Gate?-gainDb_*wet_:0,std::memory_order_relaxed);
    }
private:
    void update(){
        for(int j=0;j<info(type_).numKnobs;++j)v_[j]=realOf(type_,j,current_);
        switch(type_){
        case Gate:attack_=onePole(sr_,v_[2]);release_=onePole(sr_,v_[3]);follow_=onePole(sr_,8);break;
        case Transient:attack_=onePole(sr_,2);release_=onePole(sr_,12);slowCoeff_=onePole(sr_,v_[2]);break;
        case Bell:filters_[0].peaking(sr_,v_[0],v_[2],v_[1]);out_=db2g(v_[3]);break;
        case DualFilter:filters_[0].highPass(sr_,v_[0],v_[2]);filters_[1].lowPass(sr_,v_[1],v_[2]);
            hpMix_=std::clamp((v_[0]-20.f)/10.f,0.f,1.f);lpMix_=std::clamp((20000.f-v_[1])/1500.f,0.f,1.f);break;
        case SoftClip:drive_=db2g(v_[0]);ceiling_=db2g(v_[1]);break;
        case Tremolo:case Phaser:case Flanger:phaseStep_=float(2*kPi)*v_[0]/float(sr_);break;
        case Slap:toneCoeff_=std::exp(float(-2*kPi)*std::min(v_[1],float(sr_*.45))/float(sr_));break;
        default:break;
        }
    }
    int type_,countdown_=0;double sr_=48000;
    bool seeded_=false,dirty_=true;float current_[kMaxKnobs]{},v_[kMaxKnobs]{},wet_=1;
    float fast_=0,slow_=0,gainDb_=0,phase_=0,phaseStep_=0,attack_=0,release_=0,follow_=0,slowCoeff_=0;
    float hpMix_=0,lpMix_=0,out_=1,drive_=1,ceiling_=1,toneCoeff_=0;
    float feedback_[2]{},tone_[2]{},allpass_[2][4]{};
    Biquad filters_[2];DelayLine delay_[2];std::atomic<float> meter_{0};
};
}
