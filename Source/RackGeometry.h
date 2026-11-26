#pragma once
#include <algorithm>
namespace skl::ui::rackgeo {
constexpr int pad=4,gap=4,slotCount=8;
constexpr int width(bool library){return library?238:240;}
constexpr int count(bool library){return library?4:5;}
constexpr int left(bool library){return library?270:18;}
constexpr int extent(int n,int w){return 2*pad+n*w+(n-1)*gap;}
constexpr int viewWidth(bool library){return extent(count(library),width(library));}
constexpr int end(bool library){return left(library)+viewWidth(library);}
constexpr int stride(bool library){return width(library)+gap;}
constexpr int clampFirst(int first,bool library){return std::clamp(first,0,slotCount-count(library));}
constexpr int firstForSlot(int first,int slot,bool library){
 return clampFirst(slot<first?slot:slot>=first+count(library)?slot-count(library)+1:first,library);
}
constexpr int offset(int first,bool library){return clampFirst(first,library)*stride(library);}
constexpr int firstFromOffset(int x,bool library){return clampFirst((x+stride(library)/2)/stride(library),library);}
struct PathGeometry{int x,cell,port,arrow;};
constexpr PathGeometry signalPath(int w,int n){
 const int left=207,right=18,port=57,arrow=27,available=w-left-right;
 const int cell=std::clamp((available-2*port-arrow*(n+1))/std::max(1,n),35,150);
 const int total=2*port+arrow*(n+1)+cell*n;
 return {left+std::max(0,(available-total)/2),cell,port,arrow};
}
static_assert(end(true)==1242&&end(false)==1242);
static_assert(extent(slotCount,width(true))-viewWidth(true)==offset(4,true));
static_assert(extent(slotCount,width(false))-viewWidth(false)==offset(3,false));
}
