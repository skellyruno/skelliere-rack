#include "../Source/ArtworkMotion.h"
#include <cassert>
#include <limits>
#include <iostream>
int main(){using skl::ui::ArtworkMotion;ArtworkMotion m;
 assert(!m.advance(.1f,false,true,1.f));assert(!m.advance(.1f,true,false,1.f));
 assert(!m.advance(std::numeric_limits<float>::infinity(),true,true,1.f));
 assert(!m.advance(-1.f,true,true,1.f));assert(m.phase==0.f&&m.level==0.f);
 assert(m.advance(10.f,true,true,1.f));assert(m.phase<.14f&&m.left<.17f&&m.level>0.f);
 for(int i=0;i<10000;++i){assert(m.advance(.033f,true,true,float(i%2)));assert(m.phase>=0&&m.phase<ArtworkMotion::cycle&&m.left>=0&&m.left<ArtworkMotion::cycle&&m.right>=0&&m.right<ArtworkMotion::cycle&&m.level>=0&&m.level<=1);}
 const auto old=m;assert(!m.advance(.1f,false,true,0.f));assert(!m.advance(.1f,true,false,0.f));assert(old.phase==m.phase&&old.left==m.left&&old.right==m.right&&old.level==m.level);
 std::cout<<"Artwork motion: 10000 bounded steps and visibility/bypass guards passed\n";}
