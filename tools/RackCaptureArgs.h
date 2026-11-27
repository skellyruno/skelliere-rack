#pragma once
#include <string>
#include <vector>
namespace rackcapture {
struct Args { std::string output="ui-screenshots", error; bool full=false, legacy=false, colors=false,engines=false,gates=false,consolidated=false; };
inline Args parse(const std::vector<std::string>& argv) {
    Args a; bool outputSet=false;
    for(std::size_t i=0;i<argv.size();++i){
        const auto& v=argv[i];
        if(v=="--full")a.full=true;
        else if(v=="--color-modules")a.colors=true;
        else if(v=="--engine-modules")a.engines=true;
        else if(v=="--gate-modules")a.gates=true;
        else if(v=="--consolidated")a.consolidated=true;
        else if(v=="--output"){
            if(outputSet||i+1>=argv.size()||argv[i+1].empty()||argv[i+1].rfind("--",0)==0){a.error="Invalid --output";return a;}
            a.output=argv[++i];outputSet=true;
        }else if(i==0&&!v.empty()&&v[0]!='-'&&!outputSet){
            a.output=v;outputSet=true;a.legacy=true;a.full=true;
        }else{a.error="Unknown or duplicate argument: "+v;return a;}
    }
    return a;
}
}
