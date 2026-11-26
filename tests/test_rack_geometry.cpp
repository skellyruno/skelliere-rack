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
 for(int n=0;n<=slotCount;++n){
  auto g=signalPath(1210,n);
  assert(g.x>=207 && g.cell>=35 && g.cell<=174);
  assert(g.x+2*g.port+(n+1)*g.arrow+n*g.cell<=1210-18);
  ++checked;
 }
 std::cout<<checked<<" rack viewport, paging, and signal-path geometry cases passed\n";
}
