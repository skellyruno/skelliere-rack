#include "../Source/DelayRenderTransition.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
using namespace skl::native_delay;
static void check(bool b,const char* what){if(!b){std::fprintf(stderr,"%s\n",what);std::exit(1);}}
int main(){
    for(int wanted=1;wanted<=2;++wanted){
        check(chooseRenderModel(wanted,false,-1,0)==0,"pending loses Classic");
        check(chooseRenderModel(wanted,true,0,0)==0,"Classic sentinel loses Classic");
        check(chooseRenderModel(wanted,true,3-wanted,0)==0,"stale publication replaces Classic");
        check(chooseRenderModel(wanted,true,wanted,0)==wanted,"ready target not selected");
        check(chooseRenderModel(wanted,true,3-wanted,3-wanted)==3-wanted,"playing advanced model lost while pending");
        check(chooseRenderModel(wanted,false,-1,-1)==-1,"cold start pretends ready");
        check(chooseRenderModel(wanted,true,3-wanted,-1)==-1,"cold start uses stale model");
    }
    for(double sr:{32000.,44100.,48000.,96000.,192000.}){
        RenderTransition t;
        const float step=float(1/(.005*sr));
        check(t.begin(0,false,-1)==0,"Classic unavailable");
        for(int i=0;i<257;++i){const float x=std::sin(float(i)*.3f);auto y=t.process(x,-x,x,-x,step);check(y[0]==x&&y[1]==-x,"Classic changed");}
        for(int i=0;i<1024;++i){check(t.begin(1,true,0)==0,"dry gap during model preparation");const float x=std::sin(float(i)*.2f);auto y=t.process(x,-x,x,-x,step);check(y[0]==x&&y[1]==-x,"pending changed sound");}
        check(t.begin(1,true,1)==1,"published Modern not rendered");
        float weight=0;for(int i=0;i<int(sr*.005)+4;++i){float x=std::sin(float(i)*.17f),z=std::cos(float(i)*.21f);weight=std::min(1.f,weight+step);auto y=t.process(x,-x,z,-z,step);check(std::abs(y[0]-(x+weight*(z-x)))<2e-7f&&y[1]==-y[0],"live stereo crossfade mismatch");}
        auto y=t.process(.2f,-.2f,.6f,-.6f,step);check(y[0]==.6f&&y[1]==-.6f,"settled wet path not exact");
        check(t.begin(2,true,1)==1,"Bank request loses Modern");
        check(t.begin(0,true,1)==0,"Classic request not honored");
        t.reset();check(t.begin(2,false,-1)==-1,"reset retains stale model");
    }
    std::puts("Delay rendering policy: pending/stale/ready models, live 5 ms stereo crossfade, five rates and reset passed");
}
