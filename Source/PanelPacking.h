#pragma once
#include <cstddef>
namespace skl::ui {
struct PanelPacking {
    int width=0,rows=0;
    size_t rowBytes=0;
    int channels=0;
    bool valid() const {return width>0 && rows>0 && rowBytes>0;}
    size_t offset(int x,int y) const {return static_cast<size_t>(y)*rowBytes+static_cast<size_t>(x)*channels;}
};
// The converted PNG width/height are swapped. RGB source records retained
// 4-byte scanline padding inside the byte stream. RGBA already aligns.
inline PanelPacking panelPacking(int rawW,int rawH,bool rgbPadded) {
    if(!((rawW==570&&rawH==215)||(rawW==1140&&rawH==430))) return {};
    const int channels=rgbPadded?3:4;
    const auto width=rawH;
    const auto payload=static_cast<size_t>(rawW)*rawH*channels;
    const auto stride=(static_cast<size_t>(width)*channels+3u)&~size_t(3u);
    return {width,static_cast<int>(payload/stride),stride,channels};
}
}
