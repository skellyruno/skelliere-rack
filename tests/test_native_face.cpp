#define SKELLIERE_FULL_ARTWORK 1
#include "../Source/NativeFaceLayout.h"
#include <iostream>
#include <stdexcept>
int main() {
    using namespace skl;
    const int pairs[][2]={{Air,1},{Air,2},{Air,3},{Eq,1},{Eq,2},{Eq,3},{Trim,1},{Trim,2},{Comp,1},{ProgramOpto,1},{FastComp,1},{FastComp,2},{FastComp,3},{BusComp,1}};
    for(const auto& pair:pairs) {
        if(!ui::validNativeFace(pair[0],pair[1]))
            throw std::runtime_error(std::string("Native face geometry: ")+info(pair[0]).name+" choice "+std::to_string(pair[1]));
    }
    std::cout<<"14 native layouts passed source-canvas bounds and overlap checks\n";
}
