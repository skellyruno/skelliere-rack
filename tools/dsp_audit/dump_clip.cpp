// Runs the REAL production Ember Clip (SoftClip) module and dumps raw float output.
#include "../../Source/RackModuleFactory.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace skl;
int main(int argc,char**argv){
    const int N=65536; const double sr=48000;
    const int bin=std::atoi(argv[1]); const float amp=(float)std::atof(argv[2]);
    const float driveDb=(float)std::atof(argv[3]), ceilDb=(float)std::atof(argv[4]);
    const char* out=argv[5];
    auto m=makeRackModule(SoftClip); m->prepare(sr,512,1);
    float k[6]{}; k[0]=toNorm(info(SoftClip).knobs[0],driveDb);
    k[1]=toNorm(info(SoftClip).knobs[1],ceilDb); k[2]=toNorm(info(SoftClip).knobs[2],1.f);
    const double f=bin*sr/N;
    std::vector<float> x(N*2);   // 2N: first N lets the 20 ms control ramp settle
    for(int i=0;i<2*N;++i) x[i]=amp*(float)std::sin(2*kPi*f*i/sr);
    float* ch[]={x.data()};
    for(int i=0;i<2*N;i+=512){juce::AudioBuffer<float> b(ch,1,i,std::min(512,2*N-i)); m->process(b,k,true);}
    FILE* fp=std::fopen(out,"wb"); std::fwrite(x.data()+N,sizeof(float),N,fp); std::fclose(fp);
}
