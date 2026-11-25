#include "../Source/SpriteFrames.h"
#include <limits>
#include <stdexcept>
#include <iostream>
using namespace skl::ui;
static void check(bool ok){if(!ok)throw std::runtime_error("sprite frame regression");}
int main(){
    check(spriteFrame(0,65)==0 && spriteFrame(.5,65)==32 && spriteFrame(1,65)==64);
    check(spriteFrame(-1,65)==0 && spriteFrame(2,65)==64 && spriteFrame(std::numeric_limits<double>::quiet_NaN(),65)==0);
    check(spriteFrame(0,2)==0 && spriteFrame(1,2)==1 && spriteFrame(.2,0)==0);
    int last=-1;
    for(int i=0;i<=1000;++i){int frame=spriteFrame(i/1000.,65);check(frame>=last && frame<65);last=frame;}
    for(int scale:{1,2}) {
        auto a=packedFrameLayout(7670*scale,111*scale,65);check(a.valid() && a.frameWidth==111*scale && a.frameHeight==118*scale);
        auto b=packedFrameLayout(104*scale,77*scale,2);check(b.valid() && b.frameWidth==77*scale && b.frameHeight==52*scale);
    }
    check(!packedFrameLayout(91,92,2).valid() && !packedFrameLayout(0,0,65).valid());
    std::cout<<"Sprite index and packed dimensions passed\n";
}
