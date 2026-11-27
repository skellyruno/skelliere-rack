#include "../Source/RackModuleFactory.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace skl;
static void check(bool ok,const char* what){if(!ok){std::fprintf(stderr,"%s\n",what);std::exit(1);}}
static std::vector<float> run(int t,const std::vector<float>& x,std::initializer_list<std::pair<int,float>> settings={},float bpm=120,bool active=true){
 auto m=makeRackModule(t);m->prepare(48000,257,1);m->setTempo(bpm);float k[6]{};
 for(int j=0;j<info(t).numKnobs;++j)k[j]=defaultNorm(t,j);
 for(auto p:settings)k[p.first]=toNorm(info(t).knobs[p.first],p.second);
 auto y=x;const int delay=m->latency();y.resize(x.size()+delay);float* ptr[]={y.data()};
 for(int i=0;i<int(y.size());i+=257){juce::AudioBuffer<float> b(ptr,1,i,std::min(257,int(y.size())-i));m->process(b,k,active);}
 for(float a:y)check(std::isfinite(a),"Nonfinite output");
 if(delay)y.erase(y.begin(),y.begin()+delay);
 return y;
}
static std::vector<float> tone(float hz,float amp){std::vector<float>x(96000);for(int i=0;i<int(x.size());++i)x[i]=amp*std::sin(2*kPi*hz*i/48000);return x;}
static double rms(const std::vector<float>& x,int a=48000,int b=96000){double sum=0;for(int i=a;i<b;++i)sum+=x[i]*x[i];return std::sqrt(sum/(b-a));}
static double diff(const std::vector<float>& a,const std::vector<float>& b){double sum=0;for(size_t i=0;i<a.size();++i)sum+=(a[i]-b[i])*(a[i]-b[i]);return std::sqrt(sum/a.size());}
int main(){
 static_assert(activeCatalogCount()==31&&VoxRider==35&&StereoUnified==48&&SmartGate==34,"Catalog migration");
 const auto low=tone(200,.03f), high=tone(6500,.2f), loud=tone(3000,.3f), quiet=tone(3000,.0002f);
 for(int t=VoxRider;t<kNumRackTypes;++t){check(run(t,loud,{},120,false)==loud,"Bypass mismatch");}
 check(rms(run(VoxRider,low,{{0,-20}}))>rms(low)*1.7,"Rider not lifting quiet voice");
 check(rms(run(VoxRider,loud,{{0,-30}}))<rms(loud)*.6,"Rider not reducing loud voice");
 check(diff(run(VoxRider,quiet),quiet)<1e-8,"Rider raises background");
 check(rms(run(DynamicEQ,loud,{{0,3000},{2,-40},{3,-9}}))<rms(loud)*.5,"Dynamic EQ cut missing");
 check(rms(run(DynamicEQ,low,{{0,3000},{2,-40},{3,-9}}))>rms(low)*.97,"Dynamic EQ affects unrelated tone");
 check(diff(run(Multiband,loud,{{1,1}}),loud)<1e-8,"Multiband neutral reconstruction");
 check(rms(run(Multiband,loud,{{0,-40},{2,12}}))<rms(loud)*.8,"Multiband compression missing");
 check(rms(run(Resonance,loud,{{0,1},{1,10},{2,3000},{3,6}}))<rms(loud)*.95,"Resonance no suppression");
 check(diff(run(Doubler,loud),loud)>.001,"Doubler inactive");
 check(diff(run(Exciter,high,{{1,18},{2,1}}),high)>.01,"Exciter inactive");
 for(int mode:{0,1})check(rms(run(AirUnified,high,{{0,.6f},{1,.6f},{5,float(mode)}}))>rms(high)*1.1,"Air mode inactive");
 for(int mode:{0,1})check(rms(run(DeEsserUnified,high,{{0,4000},{1,-45},{2,12},{5,float(mode)}}))<rms(high)*.95,"De-Esser mode inactive");
 for(int mode:{0,1}){check(rms(run(NoiseGateUnified,quiet,{{5,float(mode)}}))<rms(quiet)*.02,"Gate leaks quiet noise");check(rms(run(NoiseGateUnified,loud,{{5,float(mode)}}))>rms(loud)*.99,"Gate cuts loud voice");}
 std::vector<float> impulse(96000);impulse[0]=.5f;
 const auto slap=run(EchoUnified,impulse,{{0,50},{3,1},{5,2}});check(rms(slap,2390,2450)>.001,"Slapback missing");check(rms(slap,4800,4900)<1e-8,"Slapback has feedback");
 const auto synced=run(EchoUnified,impulse,{{0,1000},{3,1},{5,3}},120);check(rms(synced,47990,48050)>.001,"Tempo delay timing wrong");
 const auto synced2=run(EchoUnified,impulse,{{0,1000},{3,1},{5,3}},240);check(rms(synced2,23990,24050)>.001,"Tempo change timing wrong");
 const auto plate=run(RoomUnified,impulse,{{1,0},{3,1},{5,0}}),hall=run(RoomUnified,impulse,{{1,0},{3,1},{5,1}}),room=run(RoomUnified,impulse,{{1,0},{3,1},{5,2}});check(diff(plate,hall)>1e-5&&diff(hall,room)>1e-5,"Room modes not distinct");
 const auto tube=run(DriveUnified,loud,{{3,0}}),transistor=run(DriveUnified,loud,{{3,1}}),excite=run(DriveUnified,loud,{{3,2}});check(diff(tube,transistor)>.001&&diff(tube,excite)>.001,"Drive modes not distinct");
 for(int mode=0;mode<4;++mode){const auto y=run(ClipUnified,loud,{{0,24},{1,-6},{3,float(mode)}});for(float x:y)check(std::abs(x)<=db2g(-6)+1e-6,"Clip exceeds sample ceiling");check(diff(y,loud)>.01,"Clip mode inactive");}
 check(diff(run(ClipUnified,loud,{{2,0}}),loud)<1e-8,"Clip mix alignment");
 auto stereo=makeRackModule(StereoUnified);stereo->prepare(48000,257,2);juce::AudioBuffer<float> b(2,48000);float k[6]{};for(int j=0;j<4;++j)k[j]=defaultNorm(StereoUnified,j);for(int i=0;i<48000;++i)b.getWritePointer(0)[i]=b.getWritePointer(1)[i]=loud[i];stereo->process(b,k,true);check(stereo->meter()>.99f,"Correlation not live");
 auto ping=makeRackModule(EchoUnified);ping->prepare(48000,257,2);float pk[6]{};for(int j=0;j<6;++j)pk[j]=defaultNorm(EchoUnified,j);pk[0]=toNorm(info(EchoUnified).knobs[0],50);pk[3]=1;pk[5]=toNorm(info(EchoUnified).knobs[5],1);juce::AudioBuffer<float> pb(2,10000);pb.getWritePointer(1)[0]=.5f;ping->process(pb,pk,true);check(std::abs(pb.getWritePointer(0)[2400])>.01f,"Ping-pong loses right input");
 // The old IDs still restore their original DSP; only catalog visibility changes.
 static_assert(!catalogVisible(Trim)&&!catalogVisible(VoxRider)&&catalogVisible(LevelUnified)&&LevelUnified==49);
 check(diff(run(LevelUnified,loud),loud)<1e-6,"Level default not neutral");
 check(rms(run(LevelUnified,low,{{0,6}}))>rms(low)*1.9,"Level manual gain missing");
 check(rms(run(LevelUnified,low,{{2,-20},{5,1}}))>rms(low)*1.7,"Level rider not lifting quiet voice");
 check(diff(run(LevelUnified,quiet,{{5,1}}),quiet)<1e-8,"Level rider raises background");
 auto level=makeRackModule(LevelUnified);level->prepare(48000,257,2);float lk[6]{};for(int j=0;j<6;++j)lk[j]=defaultNorm(LevelUnified,j);
 lk[1]=toNorm(info(LevelUnified).knobs[1],-1);juce::AudioBuffer<float> lp(2,257);for(int c=0;c<2;++c)for(int i=0;i<257;++i)lp.getWritePointer(c)[i]=.1f;level->process(lp,lk,true);
 check(std::abs(lp.getWritePointer(0)[256]-.14142136f)<1e-6&&std::abs(lp.getWritePointer(1)[256])<1e-7,"Level hard left pan incorrect");
 std::puts("Consolidated modes, 31 active catalog, latency/bypass and functional DSP checks passed");
}
