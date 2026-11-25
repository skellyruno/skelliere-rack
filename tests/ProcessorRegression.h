#pragma once
#include "../Source/PluginProcessor.h"
#include <stdexcept>
inline void processorRegression() {
    auto require=[](bool ok,const char* message) { if(!ok) throw std::runtime_error(message); };
    SkellierProcessor p;
    require(p.getParameters().size()==75,"Legacy parameter ordering/count changed");
    require(skl::kNumRackTypes==26 && skl::ProgramOpto==16 && skl::Gate==17 && skl::Slap==25,"Catalog identities changed");
    p.prepareToPlay(48000,7);
    int parameterIndex=0;
    auto ordered=[&](const juce::String& id) {
        auto* parameter=dynamic_cast<juce::AudioProcessorParameterWithID*>(p.getParameters()[parameterIndex++]);
        require(parameter && parameter->paramID==id,"Existing host parameter order changed");
    };
    ordered("in"); ordered("out"); ordered("bypass");
    for(int s=0;s<skl::kNumSlots;++s) { ordered(p.typeId(s)); ordered(p.onId(s)); for(int k=0;k<skl::kMaxKnobs;++k) ordered(p.knobId(s,k)); }
    for(int s=0;s<skl::kNumSlots;++s) ordered(p.extraId(s));
    auto set=[&](const juce::String& id,float real) { auto* parameter=p.apvts.getParameter(id); require(parameter!=nullptr,"Parameter missing"); parameter->setValueNotifyingHost(parameter->convertTo0to1(real)); };
    // Original normalized choice values must retain every legacy identity.
    auto* legacy=p.apvts.getParameter(p.typeId(0));
    auto* choices=dynamic_cast<juce::AudioParameterChoice*>(legacy);
    require(choices && choices->choices.size()==12 && choices->choices[1]=="Quad EQ" && choices->choices[11]=="Skelly Tune","Host legacy choice labels changed");
    for(int t=0;t<skl::kNumTypes;++t) { legacy->setValueNotifyingHost((float)t/11); require(p.slotType(0)==t,"Legacy choice automation remapped"); }
    p.clearRack();
    for(int type=skl::Tilt;type<skl::kNumRackTypes;++type) {
        p.clearRack(); require(p.addModule(type,0),"New module not selectable");
        require(p.slotType(0)==type,"New module selection lost");
        p.removeSlot(0); p.undo(); require(p.slotType(0)==type,"New module undo lost");
        p.redo(); require(p.slotType(0)==skl::Empty,"New module redo lost"); p.undo();
        p.copySnapshotToOther(); p.switchSnapshot(1); require(p.slotType(0)==type,"Snapshot lost extension");
        p.moveSlot(0,1,false); require(p.slotType(1)==type,"Move lost extension");
        juce::MemoryBlock state; p.getStateInformation(state); p.clearRack();
        p.setStateInformation(state.getData(),(int)state.getSize()); require(p.slotType(1)==type,"State lost extension");
        juce::AudioBuffer<float> b(2,63); juce::MidiBuffer midi;
        for(int c=0;c<2;++c) for(int i=0;i<63;++i) b.getWritePointer(c)[i]=.1f;
        p.processBlock(b,midi);
        for(int c=0;c<2;++c) for(int i=0;i<63;++i) require(std::isfinite(b.getSample(c,i)),"Production process invalid");
    }
    auto old=p.apvts.copyState();
    for(int i=old.getNumChildren();--i>=0;)
        if(old.getChild(i).getProperty("id").toString().endsWith("_extra")) old.removeChild(i,nullptr);
    auto xml=old.createXml(); juce::MemoryBlock oldState;
    juce::AudioProcessor::copyXmlToBinary(*xml,oldState);
    p.setStateInformation(oldState.getData(),(int)oldState.getSize());
    require(p.slotType(1)==skl::Empty,"Loading old state retained stale extension");
    for(int s=0;s<skl::kNumSlots;++s) require(p.apvts.getRawParameterValue(p.extraId(s))->load()==0,"Missing extension default was not zero");
    set(p.typeId(0),(float)skl::Eq); set(p.extraId(0),1);
    require(p.slotType(0)==skl::Eq,"Extension overrode legacy selection");
    // Per-instance artwork must survive editor recreation and session save only.
    SkellierProcessor other; p.artworkChoices[0]=3;
    require(other.artworkChoices[0]==1,"Artwork leaked between instances");
    juce::MemoryBlock state; p.getStateInformation(state); p.artworkChoices[0]=0;
    p.setStateInformation(state.getData(),(int)state.getSize()); require(p.artworkChoices[0]==3,"Artwork state lost");
}
