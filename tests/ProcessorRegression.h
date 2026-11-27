#pragma once
#include "../Source/PluginProcessor.h"
#include <stdexcept>
inline void processorRegression() {
    auto require=[](bool ok,const char* message) { if(!ok) throw std::runtime_error(message); };
    SkellierProcessor p;
    require(p.getParameters().size()==75,"Legacy parameter ordering/count changed");
    require(skl::kNumRackTypes==31 && skl::ProgramOpto==16 && skl::Gate==17 && skl::Slap==25 && skl::SkellAirFX==26 && skl::SkellDeEsserFX==27 && skl::SkellDope==28 && skl::SkellMixHead==29 && skl::SkellRubyQ==30,"Catalog identities changed");
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
    // A real Jet meter must match the output of a unity-gain single-slot
    // rack, consume its peak once and clear on bypass/removal.
    for(int meterType:{skl::Flanger,skl::SkellMixHead}){
    p.clearRack();require(p.addModule(meterType,0),"Peak-meter module not selectable");
    p.soloSlot.store(-1);set("in",0);set("out",0);set("bypass",0);set(p.onId(0),1);
    juce::AudioBuffer<float> jet(2,63);juce::MidiBuffer jetMidi;
    auto fillJet=[&]{for(int c=0;c<2;++c)for(int n=0;n<63;++n)
        jet.setSample(c,n,.15f*std::sin(float(n)*.17f));};
    for(int block=0;block<64;++block){fillJet();p.processBlock(jet,jetMidi);}
    (void)p.takeJetPeak(0);
    fillJet();p.processBlock(jet,jetMidi);
    const float observed=p.takeJetPeak(0);
    require(observed>0.f&&std::abs(observed-jet.getMagnitude(0,63))<.0001f,"Jet peak differs from real post-effect output");
    require(p.takeJetPeak(0)==0.f,"Jet meter did not consume peak");
    fillJet();p.processBlock(jet,jetMidi);set(p.onId(0),0);
    fillJet();p.processBlock(jet,jetMidi);
    require(p.takeJetPeak(0)==0.f,"Bypassed Jet retained stale peak");
    p.removeSlot(0);fillJet();p.processBlock(jet,jetMidi);
    require(p.takeJetPeak(0)==0.f,"Removed Jet retained stale peak");
    }
    // Bypass All skips every module, so a stored gain reduction must not be shown.
    p.clearRack();require(p.addModule(skl::Comp,0),"Opto not selectable");
    set(p.onId(0),1);set("bypass",0);set(p.knobId(0,0),0.f);set(p.knobId(0,1),1.f);   // lowest threshold, highest ratio (normalized)
    juce::AudioBuffer<float> loud(2,512);juce::MidiBuffer loudMidi;
    for(int block=0;block<200;++block){
        for(int c=0;c<2;++c)for(int n=0;n<512;++n)loud.setSample(c,n,.8f*std::sin(float(n)*.13f));
        p.processBlock(loud,loudMidi);
    }
    require(p.gainReduction(0)>1.f,"Opto test signal produced no gain reduction");
    set(p.onId(0),0);require(p.gainReduction(0)==0.f,"Disabled slot displayed stale gain reduction");
    set(p.onId(0),1);p.soloSlot.store(1);require(p.gainReduction(0)==0.f,"Solo-muted slot displayed stale gain reduction");
    p.soloSlot.store(-1);
    set("bypass",1);
    require(p.gainReduction(0)==0.f,"Bypass All displayed stale gain reduction");
    set("bypass",0);
    // Structural editing preserves finish and history; older strip XML remains accepted.
    p.clearRack();p.addModule(skl::Comp,0);p.addModule(skl::Eq,1);
    p.artworkChoices[0]=3;p.artworkChoices[1]=2;
    p.moveSlot(0,1,false);
    require(p.slotType(1)==skl::Comp&&p.artworkChoices[1]==3&&p.artworkChoices[0]==2,"Move did not carry finish");
    p.undo();require(p.slotType(0)==skl::Comp&&p.artworkChoices[0]==3,"Undo lost finish");
    p.redo();require(p.slotType(1)==skl::Comp&&p.artworkChoices[1]==3,"Redo lost finish");
    p.moveSlot(1,2,true);require(p.slotType(2)==skl::Comp&&p.artworkChoices[2]==3,"Duplicate lost finish");
    p.soloSlot.store(1);require(p.addModule(skl::Gate,1),"Insert failed");
    require(p.slotType(2)==skl::Comp&&p.artworkChoices[2]==3&&p.soloSlot.load()==2,"Insert did not move finish/solo");
    p.clearRack();for(int i=0;i<skl::kNumSlots;++i)require(p.addModule(skl::Trim,i),"Full rack setup failed");
    p.removeSlot(7);p.undo();require(p.canRedo(),"Missing redo before failed copy");
    p.moveSlot(0,1,true);require(p.canRedo(),"Failed copy discarded redo history");
    p.redo();require(p.slotType(7)==skl::Empty,"Redo after failed copy failed");
    // Per-instance artwork must survive editor recreation and session save.
    SkellierProcessor other; p.artworkChoices[0]=3;
    require(other.artworkChoices[0]==1,"Artwork leaked between instances");
    juce::MemoryBlock state; p.getStateInformation(state); p.artworkChoices[0]=0;
    p.setStateInformation(state.getData(),(int)state.getSize()); require(p.artworkChoices[0]==3,"Artwork state lost");
}
