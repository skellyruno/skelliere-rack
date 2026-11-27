#include "../Source/RackGeometry.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace skl::ui::rackgeo;
 int checked=0;
 for(bool library:{false,true}){
  const int w=width(library),n=count(library);
  assert(end(library)==1242);
  assert(1252-(end(library)-pad)==14);
  for(int f=-2;f<=slotCount+2;++f){
   const int start=clampFirst(f,library);
   assert(firstFromOffset(offset(f,library),library)==start);
   assert(offset(f,library)+viewWidth(library)<=extent(slotCount,w));
   for(int slot=0;slot<slotCount;++slot){
    const int first=firstForSlot(start,slot,library);
    assert(first<=slot && slot<first+n);
    const int x=pad+slot*stride(library)-offset(first,library);
    assert(x>=pad && x+w<=viewWidth(library)-pad);
    ++checked;
   }
  }
 }
 // Exhaust all mixed arrangements, at every possible integer scroll offset
 // sampled by anchors, and verify target visibility in both library modes.
 for(bool library:{false,true})for(int layout=0;layout<6561;++layout){ // 3^8: regular, SSL, fixed Demon
  int state=layout;std::array<bool,slotCount> narrow{},fixed{};
  for(int i=0;i<slotCount;++i){narrow[i]=state%3==1;fixed[i]=state%3==2;state/=3;}
  const auto g=pack(library,narrow,fixed);
  for(int i=0;i<slotCount;++i){if(narrow[i])assert(g.w[i]==width(library)-16);else if(fixed[i])assert(g.w[i]==width(library));else assert(g.w[i]>=width(library)&&g.w[i]<=width(library)+16);}
  assert(g.x[0]==pad&&g.total==g.x.back()+g.w.back()+pad);
  for(int i=1;i<slotCount;++i)assert(g.x[i]==g.x[i-1]+g.w[i-1]+gap);
  for(int start=0;start<slotCount;++start){const int off=g.anchor(start);
   for(int target=0;target<slotCount;++target){const int next=g.reveal(target,off);assert(next>=0&&next<=g.maxOffset()&&g.fullyVisible(target,next));++checked;}
   assert(g.page(1,off,library)>=off);assert(g.page(-1,off,library)<=off);
  }
 }
 // Approved Demon width must not change. Reclaimed SSL space is assigned to
 // the other panels, eliminating the sliver of the next slot at page start.
 for(bool library:{false,true}){
  std::array<bool,slotCount> narrow{},fixed{};narrow[1]=true;fixed[0]=true;
  const auto g=pack(library,narrow,fixed);
  assert(g.w[0]==width(library)&&g.w[1]==width(library)-16);
  assert(g.x[count(library)-1]+g.w[count(library)-1]+pad==g.view);
  assert(g.x[count(library)]==g.view);++checked;
 }
 for(int n=0;n<=slotCount;++n){
  auto g=signalPath(1210,n);
  assert(g.x>=207 && g.cell>=35 && g.cell<=150);
  assert(g.x+2*g.port+(n+1)*g.arrow+n*g.cell<=1210-18);
  ++checked;
 }
 std::cout<<checked<<" rack viewport, paging, and signal-path geometry cases passed\n";
}
