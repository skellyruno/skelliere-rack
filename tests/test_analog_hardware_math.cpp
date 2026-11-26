#include "../Source/AnalogHardwareMath.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 using namespace skl::ui::metal;
 int checked=0;
 for(int ir=0;ir<=100;++ir)for(int ia=0;ia<360;++ia)for(float original:{0.f,.25f,.5f,.75f,1.f}){
  const float r=ir*.01f,a=ia*3.14159265359f/180.f;
  const auto v=cap(r,a,original);
  assert(std::isfinite(v.brightness)&&v.brightness>=0&&v.brightness<=1);
  assert(v.edge>=0&&v.edge<=1);
  const float scale=engravingScale(r);assert(scale>=.72f&&scale<=1.f);
  assert(engravingWeight(r,0.f)>=engravingWeight(r,.5f));
  assert(engravingWeight(r,.5f)>=engravingWeight(r,1.f));
  if(r>.52f){assert(scale==1.f);assert(engravingWeight(r,0.f)==0.f);}
  ++checked;
 }
 assert(cap(.98f,-2.2f,.5f).brightness<cap(.65f,-2.2f,.5f).brightness);
 assert(cap(.65f,-1.85f,.5f).brightness>cap(.65f,-1.85f+1.5707963f,.5f).brightness+.2f);
 std::cout<<checked<<" bounded analog metal/engraving samples passed\n";
}
