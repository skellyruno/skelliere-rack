#pragma once
#include "RackSprites.h"
#include "MeterCalibration.h"
#include <map>
#include <string>
namespace skl::ui {
inline const PackedSprite& meterLayer(const char* name,int w,int h) {
    static std::map<std::string,PackedSprite> cache;
    const std::string key(name);
    if(auto it=cache.find(key);it!=cache.end())return it->second;
    return cache.emplace(key,PackedSprite(loadRackResource(name),1,w,h)).first->second;
}
inline bool drawSourceMeter(juce::Graphics& g,juce::Rectangle<int> bounds,int type,int choice,float reduction,bool on) {
    const bool opt=type==skl::Comp||type==skl::ProgramOpto;
    const bool bus=type==skl::BusComp;
    if(!opt && !bus && type!=skl::FastComp)return false;
    const char* face=type==skl::Comp?(on?"custom_meter_face":"custom_meter_dim"):
        type==skl::ProgramOpto?(on?"optical_meter_face":"optical_meter_dim"):bus?"bus_meter_face":"fet_meter_face";
    const char* bezel=type==skl::Comp?"custom_meter_bezel":type==skl::ProgramOpto?"optical_meter_bezel":
        bus?"bus_meter_bezel":choice==1?"fet_meter_bezel":choice==2?"fet_modern_meter_bezel":"fet_vintage_meter_bezel";
    const int w=opt?150:bus?137:147,h=opt?93:bus?141:122;
    const int fw=opt?122:w,fh=opt?64:h;
    const auto& background=meterLayer(face,fw,fh);
    const auto& frame=meterLayer(bezel,w,h);
    if(!background.valid()||!frame.valid())return false;
    const auto outer=frame.destination(bounds);
    const auto scale=std::min(float(outer.getWidth())/w,float(outer.getHeight())/h);
    const auto inner=opt?juce::Rectangle<int>(juce::roundToInt(fw*scale),juce::roundToInt(fh*scale)).withCentre(outer.getCentre()):outer;
    frame.draw(g,outer,0);
    background.draw(g,inner,0);
    const auto mode=type==skl::Comp?MeterScale::Custom:type==skl::ProgramOpto?MeterScale::Optical:bus?MeterScale::Bus:MeterScale::Fet;
    const auto end=meterTick(mode,reduction);
    const MeterPoint pivot{fw*.5f,opt?85.0f:bus?92.0f:107.0f};
    const auto actual=background.destination(inner);
    const float sx=actual.getWidth()/float(fw),sy=actual.getHeight()/float(fh);
    const auto point=[&](MeterPoint p){return juce::Point<float>(actual.getX()+p.x*sx,actual.getY()+p.y*sy);};
    const MeterPoint base{end.x+(pivot.x-end.x)*.46f,end.y+(pivot.y-end.y)*.46f};
    g.setColour(juce::Colour(0xff161c20));
    g.drawLine({point(base),point(end)},juce::jmax(1.0f,scale*1.4f));
    if(opt) {
        const auto& glass=meterLayer(type==skl::Comp?"custom_meter_glass":"optical_meter_glass",122,64);
        glass.draw(g,inner,0);
        // The frame is layered last; the central aperture in these original
        // optical bezel images is transparent.
        frame.draw(g,outer,0);
    }
    return true;
}
}
