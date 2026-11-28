#include "../Source/AdvancedRackModule.h"
#include <cstdio>
#include <cstdlib>
using namespace skl;
int main(){AdvancedRackModule m(Exciter);m.prepare(48000,257,2);auto e=advancedDefaults();e[advMode]=1;m.setExtended(e);float k[kMaxKnobs]{};for(int i=0;i<info(Exciter).numKnobs;++i)k[i]=defaultNorm(Exciter,i);float a[257],b[257],input[257];for(int i=0;i<257;++i)a[i]=b[i]=input[i]=.5f*std::sin(2*kPi*4000*i/48000);float* data[]={a,b};juce::AudioBuffer<float> buf(data,2,0,257);m.process(buf,k,true);float expected=0;for(int i=0;i<257;++i)expected=std::max(expected,std::abs(a[i]-input[i]));if(expected<1e-5||m.takeResidualPeak()!=expected||m.takeResidualPeak()!=0)return 1;m.reset();if(m.takeResidualPeak()!=0)return 2;e[advMode]=0;m.setExtended(e);m.process(buf,k,true);if(m.takeResidualPeak()!=0)return 3;std::puts("Exciter live residual peak: exact output difference, read/reset and legacy isolation passed");}
