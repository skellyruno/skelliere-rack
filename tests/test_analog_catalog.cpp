#include "../Source/RackModuleFactory.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace skl;
static void require(bool v,const char* m){if(!v){std::fprintf(stderr,"%s\n",m);std::exit(1);}}
static std::vector<float> run(int type,const std::vector<float>& input,const std::vector<std::pair<int,float>>& changes={},bool active=true,int block=257){
    auto m=makeRackModule(type);require(m!=nullptr,"Missing module");
    m->prepare(48000,block,1);
    float k[6]{};for(int j=0;j<info(type).numKnobs;++j)k[j]=defaultNorm(type,j);
    for(auto p:changes)k[p.first]=toNorm(info(type).knobs[p.first],p.second);
    auto output=input;float* ch[]={output.data()};
    for(int i=0;i<(int)output.size();i+=block){juce::AudioBuffer<float> b(ch,1,i,std::min(block,(int)output.size()-i));m->process(b,k,active);}
    for(auto x:output)require(std::isfinite(x),"Nonfinite analog output");
    return output;
}
static double rms(const std::vector<float>& v,int start=0,int end=-1){
    if(end<0)end=(int)v.size();double s=0;for(int i=start;i<end;++i)s+=v[i]*v[i];return std::sqrt(s/(end-start));
}
static std::vector<float> tone(float freq,float amp=0.15f){std::vector<float> a(48000);for(size_t i=0;i<a.size();++i)a[i]=amp*std::sin(2*kPi*freq*i/48000);return a;}
static double delta(const std::vector<float>& a,const std::vector<float>& b){double e=0;for(size_t i=0;i<a.size();++i){const auto d=a[i]-b[i];e+=d*d;}return std::sqrt(e/a.size());}
int main(){
    static_assert(kNumRackTypes==26&&kNumTypes==12&&ProgramOpto==16&&Gate==17&&Slap==25,"Catalog IDs changed");
    const auto quiet=tone(1000,.001f), loud=tone(1000,.2f), low=tone(80),high=tone(8000);
    require(rms(run(Gate,quiet,{{0,-35},{1,48}}),24000)<rms(quiet,24000)*.02,"Gate fails quiet attenuation");
    require(rms(run(Gate,loud,{{0,-35}}),24000)>rms(loud,24000)*.98,"Gate fails loud opening");
    std::vector<float> burst(48000);for(int i=1000;i<8000;++i)burst[i]=.1f*std::sin(2*kPi*1000*i/48000);
    require(rms(run(Transient,burst,{{0,12},{1,0}}),1000,1200)>rms(burst,1000,1200)*1.35,"Transient attack does not shape");
    require(rms(run(Bell,loud,{{0,1000},{1,12},{2,1}}),24000)>rms(loud,24000)*3.5,"Bell boost missing");
    require(rms(run(DualFilter,low,{{0,1000}}),24000)<rms(low,24000)*.02,"Highpass missing");
    require(rms(run(DualFilter,high,{{1,1000}}),24000)<rms(high,24000)*.04,"Lowpass missing");
    const auto clipped=run(SoftClip,loud,{{0,24},{1,-6},{2,1}});
    for(float x:clipped)require(std::abs(x)<=db2g(-6)+.00001f,"Clip ceiling exceeded");
    require(delta(run(SoftClip,loud,{{2,0}}),loud)<1e-7,"Clip mix zero not dry");
    require(rms(run(Tremolo,loud,{{1,1}}),24000)<rms(loud,24000)*.8,"Tremolo amplitude not modulated");
    require(delta(run(Phaser,loud),loud)>.005,"Phaser has no effect");
    require(delta(run(Flanger,loud),loud)>.005,"Flanger has no effect");
    std::vector<float> impulse(10000);impulse[0]=.5f;const auto slap=run(Slap,impulse,{{0,50},{2,1}});
    require(std::abs(slap[0]-.5f)<1e-6,"Slap dry impulse lost");
    require(rms(slap,1,2399)==0,"Slap repeats before configured time");
    require(rms(slap,2400,2500)>0.005,"Slap repeat missing");
    for(int t=Gate;t<=Slap;++t){
        require(run(t,loud,{},false)==loud,"Disabled new module not bit-exact dry");
        require(run(t,loud,{},true,1)==run(t,loud,{},true,511),"Block size changes new DSP");
        require(resolveRackType(0,t-kNumTypes+1)==t,"New reserved selector mismatch");
    }
    std::puts("25-effect catalog: nine new DSP behavior, bypass and block-size tests passed");
}
