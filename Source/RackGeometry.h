#pragma once
#include <algorithm>
#include <array>
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
// Physical slot widths, not a nominal grid with inset panels.
// Every offset is derived from actual module bounds.
struct PackedRack {
 std::array<int,slotCount> x{},w{};
 int total=0,view=0;
 constexpr int maxOffset() const {return std::max(0,total-view);}
 constexpr int anchor(int slot) const {return std::clamp(x[std::clamp(slot,0,slotCount-1)]-pad,0,maxOffset());}
 constexpr bool fullyVisible(int slot,int scroll) const {return x[slot]>=scroll&&x[slot]+w[slot]<=scroll+view;}
 constexpr int first(int scroll) const {
  for(int i=0;i<slotCount;++i)if(fullyVisible(i,scroll))return i;
  return 0;
 }
 constexpr int last(int scroll) const {
  for(int i=slotCount-1;i>=0;--i)if(fullyVisible(i,scroll))return i;
  return slotCount-1;
 }
 constexpr int reveal(int slot,int scroll) const {
  if(fullyVisible(slot,scroll))return scroll;
  if(x[slot]<scroll)return anchor(slot);
  // Align the earliest preceding module that still fits the requested slot.
  for(int i=0;i<=slot;++i){const auto a=anchor(i);if(fullyVisible(slot,a)&&a>=scroll)return a;}
  return anchor(slot);
 }
 constexpr int page(int direction,int scroll,bool library) const {
  if(direction>0)return last(scroll)==slotCount-1?maxOffset():anchor(last(scroll)+1);
  return anchor(first(scroll)-count(library));
 }
};
constexpr PackedRack pack(bool library,const std::array<bool,slotCount>& narrow,
                          const std::array<bool,slotCount>& fixed={}){
 PackedRack g;g.view=viewWidth(library);
 for(int i=0;i<slotCount;++i)g.w[i]=width(library)-(narrow[i]?16:0);
 // Reuse the recovered width within each nominal page, when possible.
 // Width-sensitive artwork (Demon) is excluded. No hidden padding beside SSL.
 // In an all-narrow/fixed page the rack really is shorter, and can scroll freely.
 for(int start=0;start<slotCount;start+=count(library)){
  const int end=std::min(slotCount,start+count(library));int spare=0,flex=0;
  for(int i=start;i<end;++i){if(narrow[i])spare+=16;else if(!fixed[i])++flex;}
  for(int i=start;i<end&&flex>0;++i)if(!narrow[i]&&!fixed[i]){
   const int add=std::min(16,spare/flex);g.w[i]+=add;spare-=add;--flex;
  }
 }
 int x=pad;for(int i=0;i<slotCount;++i){g.x[i]=x;x+=g.w[i]+gap;}
 g.total=x-gap+pad;return g;
}
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
