#include "../Source/PanelPacking.h"
#include "../Source/MeterCalibration.h"
#include <cassert>
#include <limits>
#include <iostream>
int main() {
    using namespace skl::ui;
    const auto a=panelPacking(570,215,true), b=panelPacking(1140,430,true),c=panelPacking(1140,430,false);
    assert(a.width==215 && a.rows==567 && a.rowBytes==648);
    assert(b.width==430 && b.rows==1138 && b.rowBytes==1292);
    assert(c.width==430 && c.rows==1140 && c.rowBytes==1720);
    assert(b.offset(0,1)==1292 && b.offset(1,2)==2587);
    assert(b.offset(429,1137)+3<=1140u*430u*3u);
    assert(!panelPacking(10,20,true).valid());
    for(auto m:{MeterScale::Custom,MeterScale::Optical,MeterScale::Fet,MeterScale::Bus}) {
        const auto ticks=meterTicks(m);
        for(size_t i=0;i<meterDb.size();++i) {
            const auto p=meterTick(m,meterDb[i]);assert(p.x==ticks[i].x && p.y==ticks[i].y);
        }
        float last=1000;
        for(int n=0;n<=200;++n){const auto p=meterTick(m,n*.1f);assert(p.x<=last);last=p.x;}
        assert(meterTick(m,std::numeric_limits<float>::quiet_NaN()).x==ticks[0].x);
    }
    assert(smoothReduction(0,10,.033f)>smoothReduction(10,0,.033f)-10);
    assert(smoothReduction(0,10,.033f)>4 && smoothReduction(10,0,.033f)>8);
    std::cout<<"RGB padded panel dimensions, byte offsets, GR tick interpolation and meter ballistics passed\n";
}
