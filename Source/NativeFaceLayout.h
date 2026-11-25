#pragma once
// Source coordinate system: the supplied 215 x 570 staticcanvas XML.
// Positions are adapted only where the original parameter has no counterpart.
#include <array>
#include "Modules.h"
namespace skl::ui {
struct FaceRect { int x=0,y=0,w=0,h=0; };
struct NativeFace {
    int width=216;
    std::array<FaceRect,6> controls{};
    FaceRect power{},meter{};
    // Deliberate opaque inserts replace incorrect printed scales/unsupported controls.
    std::array<FaceRect,16> inserts{};
    bool leftReadout=false;
    unsigned int colour=0xffbcc5cc;
    bool light=true;
};
inline NativeFace nativeFace(int type,int choice) {
    NativeFace f;
    f.power={76,28,63,42};
    if(type==skl::Air && choice==1) {
        f.controls={{{48,120,119,145},{68,324,79,106}}};
        f.power={70,70,75,49};
        f.inserts={{{43,102,129,166},{46,273,123,153},{12,412,191,111}}};
    } else if(type==skl::Air && choice==2) {
        f.controls={{{43,86,129,139},{43,270,129,153}}};
        f.power={79,42,56,39};
        f.inserts={{{20,40,175,199},{20,245,175,187},{35,433,145,92}}};
    } else if(type==skl::Eq && choice==1) {
        f.controls={{{12,402,91,113},{50,120,115,140},{12,276,91,115},{112,276,91,115},{112,402,91,113}}};
        f.power={70,70,75,49};
        f.inserts={{{43,102,129,166},{9,270,197,251}}};
    } else if(type==skl::Eq && choice==2) {
        // Right-hand gain column, in original high-to-low order.
        // Reuse the original HP region. Remove unavailable frequency/LP/OUT controls.
        f.leftReadout=true;
        f.controls={{{8,371,94,107},{9,290,197,77},{9,210,197,77},{9,130,197,77},{9,45,197,81}}};
        f.power={23,479,67,40};
        // Preserve the original four separator lines and right-hand dial layout.
        f.inserts={{{7,39,201,87},{7,130,201,77},{7,210,201,77},{7,290,201,77},{7,369,201,156}}};
    } else if((type==skl::Eq || type==skl::Air) && choice==3) {
        f.colour=0xff222528;f.light=false;
        f.power={73,25,69,39};
        if(type==skl::Eq) f.controls={{{63,72,89,91},{13,178,89,91},{113,178,89,91},{13,407,89,91},{113,407,89,91}}};
        else f.controls={{{42,76,131,134},{42,405,131,118}}};
        f.inserts={{{7,379,201,145}}};
    } else if(type==skl::Trim) {
        f.colour=0xff242629;f.light=false;
        f.controls={{{10,206,195,100},{10,400,195,85}}};
        f.power={72,64,71,42};
        f.inserts={{{7,378,201,151}}};
    }
    else if(type==skl::Comp) {
        f.colour=0xff77701d;f.light=false;
        f.meter={32,39,150,93}; f.power={10,483,52,41};
        f.controls={{{70,143,80,104},{19,247,64,84},{135,247,64,84},{67,337,82,98},{67,437,82,87}}};
        f.inserts={{{9,138,197,389}}};
    } else if(type==skl::ProgramOpto) {
        f.colour=0xffc2c6c7;
        f.meter={32,39,150,93}; f.power={14,487,57,40};
        f.controls={{{68,143,90,116},{12,269,89,100},{114,269,89,100},{12,370,89,100},{114,370,89,100},{76,472,100,58}}};
        f.inserts={{{9,138,197,395}}};
    } else if(type==skl::FastComp) {
        f.colour=choice==1?0xff282b2e:choice==2?0xffad7622:0xffc6c9ca; f.light=choice==3;
        f.meter={34,43,147,122}; f.power={17,467,84,46};
        f.controls={{{19,178,76,90},{116,178,81,90},{19,353,77,91},{119,353,77,91},{19,270,76,81}}};
        f.inserts={{{9,170,197,355}}};
    } else if(type==skl::BusComp) {
        f.colour=0xff242db3;f.light=false;
        f.meter={39,37,137,141};f.power={26,482,84,42};
        f.controls={{{12,180,89,101},{115,180,89,101},{12,282,89,97},{115,282,89,97},{12,380,89,98}}};
        f.inserts={{{7,179,201,349}}};
    }
    return f;
}
inline bool faceIntersects(FaceRect a, FaceRect b) {
    return a.w>0 && a.h>0 && b.w>0 && b.h>0 && a.x<b.x+b.w && b.x<a.x+a.w && a.y<b.y+b.h && b.y<a.y+a.h;
}
inline bool faceContains(FaceRect a) {return a.x>=0 && a.y>=0 && a.w>0 && a.h>0 && a.x+a.w<=215 && a.y+a.h<=570;}
inline bool validNativeFace(int type,int choice) {
    const auto f=nativeFace(type,choice);
    if(!faceContains(f.power))return false;
    const auto& mi=skl::info(type);
    if(mi.hasVU && !faceContains(f.meter))return false;
    for(int i=0;i<mi.numKnobs;++i) {
        const auto c=f.controls[i];
        if(!faceContains(c)||faceIntersects(c,f.power)||faceIntersects(c,f.meter))return false;
        for(int j=0;j<i;++j)if(faceIntersects(c,f.controls[j]))return false;
    }
    return !faceIntersects(f.power,f.meter);
}
}
