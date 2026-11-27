#pragma once
#include "../Source/PluginProcessor.h"
#include <stdexcept>
inline void processorRegression() {
    auto require=[](bool ok,const char* message) { if(!ok) throw std::runtime_error(message); };
    SkellierProcessor p;
    require(p.getParameters().size()==75+skl::kNumSlots*(skl::kSslExtraCount+skl::kAdvancedCount+1),"Parameter count changed unexpectedly");
    require(skl::kNumRackTypes==49 && skl::ProgramOpto==16 && skl::Gate==17 && skl::Slap==25 && skl::SkellAirFX==26 && skl::SkellDeEsserFX==27 && skl::SkellDope==28 && skl::SkellMixHead==29 && skl::SkellRubyQ==30 && skl::SkellDemon==31 && skl::SkellSSL==32 && skl::SkellNoise==33,"Catalog identities changed");
    p.prepareToPlay(48000,7);
    int parameterIndex=0;
    auto ordered=[&](const juce::String& id) {
        auto* parameter=dynamic_cast<juce::AudioProcessorParameterWithID*>(p.getParameters()[parameterIndex++]);
        require(parameter && parameter->paramID==id,"Existing host parameter order changed");
    };
    ordered("in"); ordered("out"); ordered("bypass");
    for(int s=0;s<skl::kNumSlots;++s) { ordered(p.typeId(s)); ordered(p.onId(s)); for(int k=0;k<skl::kMaxKnobs;++k) ordered(p.knobId(s,k)); }
    for(int s=0;s<skl::kNumSlots;++s) ordered(p.extraId(s));
    for(int s=0;s<skl::kNumSlots;++s)for(int i=0;i<skl::kSslExtraCount;++i)ordered(p.sslId(s,i));
    for(int s=0;s<skl::kNumSlots;++s)for(int i=0;i<skl::kAdvancedCount;++i)ordered(p.advancedId(s,i));
    for(int s=0;s<skl::kNumSlots;++s)ordered(p.humanizeId(s));
    auto set=[&](const juce::String& id,float real) { auto* parameter=p.apvts.getParameter(id); require(parameter!=nullptr,"Parameter missing"); parameter->setValueNotifyingHost(parameter->convertTo0to1(real)); };
    p.clearRack();require(p.addModule(skl::Tune,0),"Tune missing");
    require(std::abs(p.apvts.getRawParameterValue(p.knobId(0,0))->load()-.5f)<1e-6f&&skl::tuneReferenceHz(p.apvts.getRawParameterValue(p.knobId(0,5))->load())==440.f,"Tune defaults");
    // A saved normalized value originally represented 0..200 ms or +/-50 cents.
    const auto oldReference=skl::migrateTuneReference(.82f);
    auto legacyTuneState=p.apvts.copyState();legacyTuneState.removeProperty("tuneControlVersion",nullptr);
    legacyTuneState.getChildWithProperty("id",p.knobId(0,0)).setProperty("value",.125f,nullptr);
    legacyTuneState.getChildWithProperty("id",p.knobId(0,5)).setProperty("value",.82f,nullptr);
    juce::MemoryBlock legacyBinary;juce::AudioProcessor::copyXmlToBinary(*legacyTuneState.createXml(),legacyBinary);
    p.setStateInformation(legacyBinary.getData(),int(legacyBinary.getSize()));
    require(std::abs(p.apvts.getRawParameterValue(p.knobId(0,0))->load()-.75f)<1e-6f&&std::abs(p.apvts.getRawParameterValue(p.knobId(0,5))->load()-oldReference)<1e-6f,"Legacy Tune migration");
    juce::MemoryBlock migrated;p.getStateInformation(migrated);p.setStateInformation(migrated.getData(),int(migrated.getSize()));
    require(std::abs(p.apvts.getRawParameterValue(p.knobId(0,0))->load()-.75f)<1e-6f&&std::abs(p.apvts.getRawParameterValue(p.knobId(0,5))->load()-oldReference)<1e-6f,"Tune migration repeated on recall");
    const auto fixtureName="RetuneMigrationTest-"+juce::Uuid().toString();
    const auto strips=SkellierProcessor::stripsFolder();
    require(strips.createDirectory().wasOk(),"Cannot create strip test directory");
    const auto fixture=strips.getChildFile(fixtureName+".xml");
    require(fixture.replaceWithText("<RACK><SLOT i=\"0\" type=\"11\" k0=\"0.125\" k5=\"0.82\"/></RACK>"),"Cannot write legacy strip fixture");
    const bool loaded=p.loadStrip(fixtureName);p.deleteStrip(fixtureName);
    require(loaded,"Cannot load legacy strip fixture");
    require(std::abs(p.apvts.getRawParameterValue(p.knobId(0,0))->load()-.75f)<1e-6f&&std::abs(p.apvts.getRawParameterValue(p.knobId(0,5))->load()-oldReference)<1e-6f,"Legacy strip migration");
    set(p.humanizeId(0),.64f);set(p.knobId(0,5),.82f);set(p.knobId(0,1),.71f);set(p.knobId(0,2),.42f);
    p.moveSlot(0,1,false);require(std::abs(p.apvts.getRawParameterValue(p.humanizeId(1))->load()-.64f)<1e-6f,"Humanize lost on move");require(std::abs(p.apvts.getRawParameterValue(p.knobId(1,5))->load()-.82f)<1e-6f,"Detune lost on move");
    p.undo();require(std::abs(p.apvts.getRawParameterValue(p.humanizeId(0))->load()-.64f)<1e-6f,"Humanize lost on undo");
    juce::MemoryBlock tuneState;p.getStateInformation(tuneState);p.clearRack();p.setStateInformation(tuneState.getData(),int(tuneState.getSize()));
    require(std::abs(p.apvts.getRawParameterValue(p.humanizeId(0))->load()-.64f)<1e-6f,"Humanize lost on state recall");require(std::abs(p.apvts.getRawParameterValue(p.knobId(0,5))->load()-.82f)<1e-6f,"Detune lost on state recall");
    auto tuneOld=p.apvts.copyState();for(int i=tuneOld.getNumChildren();--i>=0;)if(tuneOld.getChild(i).getProperty("id").toString().endsWith("_tune_humanize"))tuneOld.removeChild(i,nullptr);
    auto tuneOldXml=tuneOld.createXml();juce::MemoryBlock tuneOldBinary;juce::AudioProcessor::copyXmlToBinary(*tuneOldXml,tuneOldBinary);p.setStateInformation(tuneOldBinary.getData(),int(tuneOldBinary.getSize()));
    require(p.apvts.getRawParameterValue(p.humanizeId(0))->load()==0,"Legacy Tune did not default to original processing");
    require(std::abs(p.apvts.getRawParameterValue(p.knobId(0,1))->load()-.71f)<1e-6f&&std::abs(p.apvts.getRawParameterValue(p.knobId(0,2))->load()-.42f)<1e-6f,"Legacy wet/amount changed");
    // Display-only enharmonic preference must not change the key parameter.
    set(p.knobId(0,3),1.f/11.f);p.tuneFlats.store(true);
    juce::MemoryBlock spelling;p.getStateInformation(spelling);p.tuneFlats.store(false);
    p.setStateInformation(spelling.getData(),int(spelling.getSize()));
    require(p.tuneFlats.load()&&std::abs(p.apvts.getRawParameterValue(p.knobId(0,3))->load()-1.f/11.f)<1e-6f,"Enharmonic preference or pitch changed on recall");
    auto noSpelling=p.apvts.copyState();noSpelling.removeProperty("tuneFlats",nullptr);auto noSpellingXml=noSpelling.createXml();juce::MemoryBlock noSpellingBinary;juce::AudioProcessor::copyXmlToBinary(*noSpellingXml,noSpellingBinary);
    p.setStateInformation(noSpellingBinary.getData(),int(noSpellingBinary.getSize()));require(!p.tuneFlats.load(),"Old session retained stale note spelling");
    // Module presets are explicit user actions and never replace the whole rack.
    for(int type:{skl::EchoUnified,skl::RoomUnified,skl::DriveUnified,skl::FastComp,skl::BusComp}){
        p.clearRack();require(p.addModule(type,0)&&p.addModule(skl::Trim,1),"Preset modules missing");
        set(p.onId(0),0);set(p.knobId(1,0),.81f);p.artworkChoices[0].store(2);
        require(p.modulePresetNames(type).size()==3,"Preset names missing");
        for(int j=0;j<3;++j){const float old=p.apvts.getRawParameterValue(p.knobId(0,0))->load();require(p.applyModulePreset(0,type,j),"Module preset not applied");
            require(p.slotType(0)==type&&!p.slotOn(0)&&p.artworkChoices[0].load()==2&&p.slotType(1)==skl::Trim&&std::abs(p.apvts.getRawParameterValue(p.knobId(1,0))->load()-.81f)<1e-6f,"Module preset changed unrelated state");
            for(int k=0;k<skl::info(type).numKnobs;++k){const float v=p.apvts.getRawParameterValue(p.knobId(0,k))->load();require(std::isfinite(v)&&v>=0&&v<=1,"Module preset value out of range");}
            p.undo();require(std::abs(p.apvts.getRawParameterValue(p.knobId(0,0))->load()-old)<1e-6f,"Module preset undo failed");}
        require(!p.applyModulePreset(0,skl::Trim,0)&&!p.applyModulePreset(0,type,3)&&!p.applyModulePreset(-1,type,0),"Stale or invalid preset accepted");
    }
    p.clearRack(); // isolate the legacy choice test from extension-preset fixtures
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
    // Extra SSL controls travel with the slot, survive undo and state recall,
    // and default when loading an older 75-parameter session.
    p.clearRack();require(p.addModule(skl::SkellSSL,0),"Expanded SSL missing");
    for(int j=0;j<skl::kSslExtraCount;++j)set(p.sslId(0,j),.73f);
    p.moveSlot(0,1,false);for(int j=0;j<skl::kSslExtraCount;++j)require(std::abs(p.apvts.getRawParameterValue(p.sslId(1,j))->load()-.73f)<1e-6f,"SSL controls lost on move");
    p.undo();require(p.slotType(0)==skl::SkellSSL,"SSL undo lost type");for(int j=0;j<skl::kSslExtraCount;++j)require(std::abs(p.apvts.getRawParameterValue(p.sslId(0,j))->load()-.73f)<1e-6f,"SSL controls lost on undo");
    p.moveSlot(0,1,true);juce::MemoryBlock sslState;p.getStateInformation(sslState);p.clearRack();p.setStateInformation(sslState.getData(),int(sslState.getSize()));
    require(p.slotType(1)==skl::SkellSSL,"SSL copied state lost module");for(int j=0;j<skl::kSslExtraCount;++j)require(std::abs(p.apvts.getRawParameterValue(p.sslId(1,j))->load()-.73f)<1e-6f,"SSL controls lost on state recall");
    auto compactState=p.apvts.copyState();for(int i=compactState.getNumChildren();--i>=0;)if(compactState.getChild(i).getProperty("id").toString().contains("_ssl_"))compactState.removeChild(i,nullptr);
    auto compactXml=compactState.createXml();juce::MemoryBlock compactBinary;juce::AudioProcessor::copyXmlToBinary(*compactXml,compactBinary);p.setStateInformation(compactBinary.getData(),int(compactBinary.getSize()));
    require(p.slotType(1)==skl::SkellSSL,"Legacy SSL state lost type");for(int s=0;s<skl::kNumSlots;++s)for(int j=0;j<skl::kSslExtraCount;++j)require(std::abs(p.apvts.getRawParameterValue(p.sslId(s,j))->load()-skl::sslDefault(j))<1e-6f,"Missing SSL control retained stale value");
    p.clearRack();require(p.addModule(skl::Multiband,0),"Advanced Multiband missing");
    set(p.advancedId(0,skl::advMode),1);set(p.advancedId(0,skl::bandControl(0,0)),.27f);
    p.moveSlot(0,1,false);require(p.apvts.getRawParameterValue(p.advancedId(1,skl::advMode))->load()==1,"Advanced mode lost on move");
    p.undo();require(std::abs(p.apvts.getRawParameterValue(p.advancedId(0,skl::bandControl(0,0)))->load()-.27f)<1e-6f,"Advanced band lost on undo");
    p.moveSlot(0,1,true);juce::MemoryBlock advancedState;p.getStateInformation(advancedState);p.clearRack();p.setStateInformation(advancedState.getData(),int(advancedState.getSize()));
    require(p.apvts.getRawParameterValue(p.advancedId(1,skl::advMode))->load()==1,"Advanced state lost");
    auto previous=p.apvts.copyState();for(int i=previous.getNumChildren();--i>=0;)if(previous.getChild(i).getProperty("id").toString().contains("_adv_"))previous.removeChild(i,nullptr);
    auto previousXml=previous.createXml();juce::MemoryBlock previousBinary;juce::AudioProcessor::copyXmlToBinary(*previousXml,previousBinary);p.setStateInformation(previousBinary.getData(),int(previousBinary.getSize()));
    for(int i=0;i<skl::kAdvancedCount;++i)require(std::abs(p.apvts.getRawParameterValue(p.advancedId(1,i))->load()-skl::advancedDefault(i))<1e-6f,"Legacy state retained advanced setting");
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
    for(int meterType:{skl::Flanger,skl::SkellMixHead,skl::SkellSSL,skl::SkellDope}){
    p.clearRack();require(p.addModule(meterType,0),"Peak-meter module not selectable");
    p.soloSlot.store(-1);set("in",0);set("out",0);set("bypass",0);set(p.onId(0),1);
    juce::AudioBuffer<float> jet(2,63);juce::MidiBuffer jetMidi;
    auto fillJet=[&]{for(int c=0;c<2;++c)for(int n=0;n<63;++n)
        jet.setSample(c,n,.15f*std::sin(float(n)*.17f));};
    for(int block=0;block<64;++block){fillJet();p.processBlock(jet,jetMidi);}
    (void)p.takeJetPeak(0);
    fillJet();p.processBlock(jet,jetMidi);
    const float observed=p.takeJetPeak(0);
    require(observed>0.f&&std::abs(observed-jet.getMagnitude(0,63))<.0001f,"Module peak differs from real post-effect output");
    require(p.takeJetPeak(0)==0.f,"Module meter did not consume peak");
    fillJet();p.processBlock(jet,jetMidi);set(p.onId(0),0);
    fillJet();p.processBlock(jet,jetMidi);
    require(p.takeJetPeak(0)==0.f,"Bypassed module retained stale peak");
    p.removeSlot(0);fillJet();p.processBlock(jet,jetMidi);
    require(p.takeJetPeak(0)==0.f,"Removed module retained stale peak");
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
