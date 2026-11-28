#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "RackPresets.h"

using namespace skl;

// ------------------------------------------------------------------ factory racks
// Preset definitions are shared with the portable audio regression.


juce::StringArray SkellierProcessor::modulePresetNames(int type){juce::StringArray names;for(const auto& p:rackModulePresets(type))names.add(p.name);return names;}
bool SkellierProcessor::applyModulePreset(int slot,int expectedType,int preset){
    if(slot<0||slot>=kNumSlots||slotType(slot)!=expectedType)return false;
    const auto& list=rackModulePresets(expectedType);if(preset<0||preset>=int(list.size()))return false;
    const auto& values=list[size_t(preset)].values;const auto& m=info(expectedType);if(int(values.size())!=m.numKnobs)return false;
    pushUndo();for(int k=0;k<m.numKnobs;++k)setParam(knobId(slot,k),toNorm(m.knobs[k],values[size_t(k)]));return true;
}

int SkellierProcessor::matchingFactoryPreset() const {
    const auto& list=factory();
    auto close=[](float a,float b){return std::isfinite(a)&&std::abs(a-b)<1e-5f;};
    for(int index=0;index<int(list.size());++index){const auto& preset=list[size_t(index)];bool match=true;
        // Compare inexpensive type/power first. Only a matching layout needs
        // detailed parameter checks, at a low UI polling rate.
        for(int slot=0;slot<kNumSlots;++slot){const int t=slot<int(preset.slots.size())?preset.slots[size_t(slot)].type:Empty;if(slotType(slot)!=t||(t!=Empty&&!slotOn(slot))){match=false;break;}}
        if(!match)continue;
        for(int slot=0;slot<int(preset.slots.size())&&match;++slot){const auto& fs=preset.slots[size_t(slot)];const int t=fs.type;if(t==Empty)continue;
            float want[kMaxKnobs]{};for(int j=0;j<kMaxKnobs;++j)want[j]=j<info(t).numKnobs?defaultNorm(t,j):.5f;
            for(const auto& kv:fs.set)want[kv.first]=toNorm(info(t).knobs[kv.first],kv.second);
            for(int j=0;j<info(t).numKnobs;++j)match&=close(kRaw[slot][j]->load(std::memory_order_relaxed),want[j]);
            if(t==Tune)match&=close(humanizeRaw[slot]->load(std::memory_order_relaxed),0);
            if(t==SkellSSL)for(int j=0;j<kSslExtraCount;++j)match&=close(sslRaw[slot][j]->load(std::memory_order_relaxed),sslDefault(j));
            if(hasAdvancedEngine(t))for(int j=0;j<kAdvancedCount;++j)match&=close(advancedRaw[slot][j]->load(std::memory_order_relaxed),advancedDefault(j));
            if(importedType(t))for(int j=0;j<kNativeControlCount;++j)match&=close(nativeRaw[slot][j]->load(std::memory_order_relaxed),nativeDefault(j));
        }
        if(match)return index;
    }
    return -1;
}

int SkellierProcessor::numFactory() { return (int) factory().size(); }
juce::String SkellierProcessor::factoryName(int i) { return factory()[(size_t) juce::jlimit(0, numFactory() - 1, i)].name; }

// ------------------------------------------------------------------ parameters
juce::AudioProcessorValueTreeState::ParameterLayout SkellierProcessor::createLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{"in", 1}, "Input", NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{"out", 1}, "Output", NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AudioParameterBool>(ParameterID{"bypass", 1}, "Bypass All", false));
    StringArray names;
    for (int t = 0; t < kNumTypes; ++t) names.add(info(t).name);
    for (int s = 0; s < kNumSlots; ++s) {
        const String slot = String::charToString((juce_wchar) ('A' + s));
        layout.add(std::make_unique<AudioParameterChoice>(ParameterID{typeId(s), 1}, "Slot " + slot + " Module", names, 0));
        layout.add(std::make_unique<AudioParameterBool>(ParameterID{onId(s), 1}, "Slot " + slot + " On", true));
        for (int k = 0; k < kMaxKnobs; ++k)
            layout.add(std::make_unique<AudioParameterFloat>(ParameterID{knobId(s, k), 1}, "Slot " + slot + " Knob " + String(k + 1),
                                                             NormalisableRange<float>(0.0f, 1.0f, 0.0f), 0.5f));
    }
    // Append extension parameters after every legacy parameter. An integer range
    // keeps extension automation stable when more effects are assigned later.
    for (int s = 0; s < kNumSlots; ++s)
        layout.add(std::make_unique<AudioParameterInt>(ParameterID{extraId(s), 1},
            "Slot " + String(s + 1) + " Extra Effect", 0, 127, 0));
    for(int slot=0;slot<kNumSlots;++slot)for(int i=0;i<kSslExtraCount;++i){
        const auto& c=sslControl(i);
        layout.add(std::make_unique<AudioParameterFloat>(ParameterID{sslId(slot,i),1},
            "Slot "+String(slot+1)+" SSL "+c.id,NormalisableRange<float>(0.f,1.f,0.f),sslDefault(i)));
    }
    // Append, never reorder the original 243 host parameters. Engine defaults to legacy.
    for(int slot=0;slot<kNumSlots;++slot)for(int i=0;i<kAdvancedCount;++i){
        const auto k=advancedSpec(i);juce::String name="Slot "+juce::String(slot+1)+" Advanced ";
        if(i>=advCross&&i<advBand)name+="Crossover "+juce::String(i-advCross+1);
        else if(i>=advBand&&i<advFloating)name+="Band "+juce::String((i-advBand)/7+1)+" "+k.label;
        else if(i>=advFloating&&i<advVoice)name+="Floating "+juce::String((i-advFloating)/2+1)+" "+k.label;
        else if(i>=advVoice)name+="Voice "+juce::String((i-advVoice)/4+1)+" "+k.label;
        else name+=k.label;
        layout.add(std::make_unique<AudioParameterFloat>(ParameterID{advancedId(slot,i),1},name,NormalisableRange<float>(0.f,1.f,k.steps>1?1.f:0.f),advancedDefault(i)));
    }
    // Append after existing advanced parameters; preserve host IDs and order.
    for(int s=0;s<kNumSlots;++s)layout.add(std::make_unique<AudioParameterFloat>(ParameterID{humanizeId(s),1},"Slot "+String(s+1)+" Tune Humanize",NormalisableRange<float>(0.f,1.f,0.f),0.f));
    // Dedicated, append-only native controls; never change old knob IDs.
    for(int slot=0;slot<kNumSlots;++slot)for(int i=0;i<kNativeControlCount;++i){
        const auto spec=nativeSpec(i);auto name="Slot "+String(slot+1)+" Native "+spec.label;
        if(i>=nGlitchState)name+=" "+String((i>=nGlitchProbability?i-nGlitchProbability:i-nGlitchState)+1);
        layout.add(std::make_unique<AudioParameterFloat>(ParameterID{nativeId(slot,i),1},name,NormalisableRange<float>(0.f,1.f,spec.steps>1?1.f/float(spec.steps-1):0.f),nativeDefault(i)));
    }
    return layout;
}

SkellierProcessor::SkellierProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "SKELLIERE", createLayout()) {
    for (int s = 0; s < kNumSlots; ++s) {
        typeRaw[s] = apvts.getRawParameterValue(typeId(s));
        extraRaw[s] = apvts.getRawParameterValue(extraId(s));
        onRaw[s] = apvts.getRawParameterValue(onId(s));
        for (int k = 0; k < kMaxKnobs; ++k) kRaw[s][k] = apvts.getRawParameterValue(knobId(s, k));
        for(int i=0;i<kSslExtraCount;++i)sslRaw[s][i]=apvts.getRawParameterValue(sslId(s,i));
        for(int i=0;i<kAdvancedCount;++i)advancedRaw[s][i]=apvts.getRawParameterValue(advancedId(s,i));
        humanizeRaw[s]=apvts.getRawParameterValue(humanizeId(s));
        for(int i=0;i<kNativeControlCount;++i)nativeRaw[s][i]=apvts.getRawParameterValue(nativeId(s,i));
        lastType[s] = -1;
        apvts.addParameterListener(typeId(s), this);
        apvts.addParameterListener(extraId(s), this);
    }
    inRaw = apvts.getRawParameterValue("in");
    outRaw = apvts.getRawParameterValue("out");
    bypassRaw = apvts.getRawParameterValue("bypass");
    applyFactory(0);
    undoStack.clear(); redoStack.clear();
}

SkellierProcessor::~SkellierProcessor() {
    stopTimer();
    std::lock_guard<std::mutex> lock(nativeMutex);
    latencyUpdater.cancelPendingUpdate();
    for (int s = 0; s < kNumSlots; ++s) { apvts.removeParameterListener(typeId(s), this); apvts.removeParameterListener(extraId(s), this); }
}

float SkellierProcessor::advancedBandReduction(int slot,int band) const noexcept {
    if(slot<0||slot>=kNumSlots||slotType(slot)!=Multiband||!slotOn(slot)||bypassRaw->load()>.5f||(soloSlot.load()>=0&&soloSlot.load()!=slot))return 0;
    if(band<0||band>=8||meterType[size_t(slot)].load(std::memory_order_acquire)!=Multiband)return 0;
    return bandReduction[size_t(slot)][size_t(band)].load(std::memory_order_relaxed);
}

float SkellierProcessor::takeExciterResidualPeak(int slot) noexcept {
    if(slot<0||slot>=kNumSlots)return 0;
    const float peak=exciterPeak[size_t(slot)].exchange(0.f,std::memory_order_relaxed);
    if(slotType(slot)!=Exciter||!slotOn(slot)||bypassRaw->load()>.5f||advancedRaw[slot][advMode]->load()<.5f||(soloSlot.load()>=0&&soloSlot.load()!=slot))return 0;
    return peak;
}

void SkellierProcessor::parameterChanged(const juce::String&, float) { latencyUpdater.triggerAsyncUpdate(); }

bool SkellierProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    const auto in = l.getMainInputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo()) && in == l.getMainOutputChannelSet();
}

// ------------------------------------------------------------------ audio
skl::ModuleBase* SkellierProcessor::moduleFor(int slot, int type, int tuneSlot) const {
    if (type <= 0 || type >= kNumRackTypes) return nullptr;
    if (type == Tune) return slot == tuneSlot ? tuneModule.get() : nullptr;   // a second Tune slot stays silent
    return mods[slot][type].get();
}

void SkellierProcessor::clearMeterTelemetry() noexcept {
    for(int s=0;s<kNumSlots;++s){meterType[size_t(s)].store(Empty,std::memory_order_release);rackReduction[size_t(s)].store(0,std::memory_order_relaxed);for(auto& b:bandReduction[size_t(s)])b.store(0,std::memory_order_relaxed);jetPeak[size_t(s)].store(0,std::memory_order_relaxed);exciterPeak[size_t(s)].store(0,std::memory_order_relaxed);}
}
void SkellierProcessor::releasePreparedStorage(){
    // Host lifecycle: processing is stopped before prepare/release. UI reads
    // only processor-owned atomic telemetry, never destroyed module objects.
    prepared=false;clearMeterTelemetry();
    for(auto& slot:mods)for(auto& module:slot)module.reset();
    tuneModule.reset();bypassDelay=LatencyAlignedBypass{};
    for(auto& v:nativeModel)v.store(-1,std::memory_order_relaxed);
    for(auto& v:nativeBlocked)v.store(false,std::memory_order_relaxed);
}
void SkellierProcessor::releaseResources(){stopTimer();std::lock_guard<std::mutex> lock(nativeMutex);releasePreparedStorage();}
void SkellierProcessor::editNativeControl(int slot,int effect,int control,float value){
    if(slot<0||slot>=kNumSlots||slotType(slot)!=effect||!hasNativePanel(effect)||control<0||control>=kNativeControlCount||!std::isfinite(value))return;
    pushUndo();if(auto* param=apvts.getParameter(nativeId(slot,control))){param->beginChangeGesture();setParam(nativeId(slot,control),std::clamp(value,0.f,1.f));param->endChangeGesture();}
}
void SkellierProcessor::triggerNativeGlitch(int slot) noexcept {if(slot>=0&&slot<kNumSlots&&slotType(slot)==GlitchNative)glitchTrigger[size_t(slot)].store(true,std::memory_order_release);}
bool SkellierProcessor::acceptsMidi() const {return SKELLIERE_NATIVE_IMPORTS!=0;}
NativeValues SkellierProcessor::nativeDisplayValues(int slot) const noexcept {
    auto v=nativeDefaults();if(slot>=0&&slot<kNumSlots)for(int i=0;i<kNativeControlCount;++i)v[i]=nativeRaw[slot][i]->load(std::memory_order_relaxed);return v;
}
int SkellierProcessor::nativeDelayModel(int slot) const noexcept {return slot>=0&&slot<kNumSlots?nativeModel[size_t(slot)].load(std::memory_order_relaxed):-1;}
bool SkellierProcessor::nativeDelayBlocked(int slot) const noexcept {return slot>=0&&slot<kNumSlots&&nativeBlocked[size_t(slot)].load(std::memory_order_relaxed);}
void SkellierProcessor::serviceNative(){std::lock_guard<std::mutex> lock(nativeMutex);if(prepared)serviceNativeLocked();}
void SkellierProcessor::serviceNativeLocked(){
#if SKELLIERE_NATIVE_IMPORTS
    for(int s=0;s<kNumSlots;++s){auto* m=static_cast<NativeDelayRack*>(mods[s][DelayNative].get());if(!m)continue;
        NativeValues ext{};for(int j=0;j<kNativeControlCount;++j)ext[j]=nativeRaw[s][j]->load(std::memory_order_relaxed);
        float k[kMaxKnobs]{};for(int j=0;j<kMaxKnobs;++j)k[j]=kRaw[s][j]->load(std::memory_order_relaxed);
        const bool selected=slotType(s)==DelayNative;
        nativeBlocked[size_t(s)].store(selected&&!m->service(ext,k,selected),std::memory_order_relaxed);
        if(!selected)m->service(ext,k,false);
    }
#endif
}
void SkellierProcessor::prepareToPlay(double sr, int samplesPerBlock) {
    stopTimer();std::lock_guard<std::mutex> lock(nativeMutex);
    releasePreparedStorage(); // avoid overlapping old/new high-rate allocations
#if SKELLIERE_NATIVE_IMPORTS
    auto delayBudget=std::make_shared<native_delay::RackBudget>();
#endif
    sampleRate_ = std::isfinite(sr) && sr > 1000.0 ? sr : 48000.0;
    graphTop.store(float(std::min(16000.,std::clamp(sampleRate_,8000.,384000.)*.4)),std::memory_order_relaxed);
    maxBlock = juce::jmax(1, samplesPerBlock);
    const int ch = juce::jlimit(1, 2, getMainBusNumInputChannels());
    for (int s = 0; s < kNumSlots; ++s) {
        SlotDelayArena arena;
        for (int t = Eq; t < kNumRackTypes; ++t) {
            if (t == Tune) continue;
            mods[s][t] = makeModule(t);
#if SKELLIERE_NATIVE_IMPORTS
            if(t==DelayNative)static_cast<NativeDelayRack*>(mods[s][t].get())->setBudget(delayBudget);
#endif
            SlotDelayPreparation scope(arena);
            mods[s][t]->prepare(sampleRate_, maxBlock, ch);
        }
        arena.storage->shrink_to_fit(); // control thread, before audio can access any slice
    }
    tuneModule = makeModule(Tune);
    tuneModule->prepare(sampleRate_, maxBlock, ch);
    for(int t=1;t<kNumRackTypes;++t)typeLatency[size_t(t)].store(t==Tune?tuneModule->latency():(mods[0][t]?mods[0][t]->latency():0),std::memory_order_relaxed);
    bypassDelay.prepare(tuneModule->latency()+kNumSlots*std::max(mods[0][SkellDemon]->latency(),mods[0][SkellMixHead]->latency()));
    for (int s = 0; s < kNumSlots; ++s) {lastType[s] = -1;jetPeak[static_cast<size_t>(s)].store(0,std::memory_order_relaxed);exciterPeak[size_t(s)].store(0,std::memory_order_relaxed);}
    inputGain.reset(sampleRate_, .020); outputGain.reset(sampleRate_, .020);
    inputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(inRaw->load()));
    outputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outRaw->load()));
    for(auto& v:nativeModel)v.store(-1,std::memory_order_relaxed);
    prepared = true;
#if SKELLIERE_NATIVE_IMPORTS
    serviceNativeLocked();startTimerHz(10);
#endif
    updateLatency();
}

void SkellierProcessor::updateLatency() {
    setLatencySamples(rackLatency());
}

int SkellierProcessor::latencyOfTune() const {
    for (int s = 0; s < kNumSlots; ++s)
        if (slotType(s) == Tune) return typeLatency[Tune].load(std::memory_order_relaxed);
    return 0;
}

int SkellierProcessor::rackLatency() const {
    int latency=latencyOfTune();
    for(int s=0;s<kNumSlots;++s){const auto t=slotType(s);if(t!=Tune&&t>0&&t<kNumRackTypes)latency+=typeLatency[size_t(t)].load(std::memory_order_relaxed);}
    return latency;
}

void SkellierProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
#if !SKELLIERE_NATIVE_IMPORTS
    juce::ignoreUnused(midi);
#endif
    juce::ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples(), ch = buffer.getNumChannels();
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c) buffer.clear(c, 0, total);
    if (!prepared || total <= 0 || ch < 1) return;

    const float inG = juce::Decibels::decibelsToGain(juce::jlimit(-24.0f, 12.0f, inRaw->load()));
    const float outG = juce::Decibels::decibelsToGain(juce::jlimit(-24.0f, 12.0f, outRaw->load()));
    const bool bypass = bypassRaw->load() > 0.5f;
    // One coherent type snapshot for audio routing and its bypass delay.
    int tuneSlot=-1,latency=0;int types[kNumSlots];
    for(int s=0;s<kNumSlots;++s){const int t=types[s]=slotType(s);if(t==Tune){if(tuneSlot<0){tuneSlot=s;latency+=typeLatency[Tune].load(std::memory_order_relaxed);}}else if(t>0&&t<kNumRackTypes)latency+=typeLatency[size_t(t)].load(std::memory_order_relaxed);}
    // Advance the untouched input's shadow delay for *both* modes. JUCE's reported
    // latency remains the same when the user toggles Bypass All, avoiding a dry
    // timing jump relative to other tracks when a Tune module is present.
    const float rawPeak = buffer.getMagnitude(0, total);
    bypassDelay.process(buffer, latency, bypass);
    if (bypass) {
        clearMeterTelemetry();
        inPeak.store(juce::jmax(inPeak.load(), rawPeak));
        outPeak.store(juce::jmax(outPeak.load(), buffer.getMagnitude(0, total)));
        return;
    }

    inputGain.setTargetValue(inG); outputGain.setTargetValue(outG);
    for(int i=0;i<total;++i) {
        const float gain=inputGain.getNextValue();
        for(int c=0;c<ch;++c) buffer.getWritePointer(c)[i]*=gain;
    }
    inPeak.store(juce::jmax(inPeak.load(), buffer.getMagnitude(0, total)));

    double bpm=120.,ppq=0.;int sigNum=4,sigDen=4;bool validPpq=false;
    if(auto* ph=getPlayHead())if(auto pos=ph->getPosition()){
        if(auto value=pos->getBpm())bpm=*value;
        if(auto value=pos->getPpqPosition()){ppq=*value;validPpq=pos->getIsPlaying()&&std::isfinite(ppq);}
        if(auto value=pos->getTimeSignature()){sigNum=value->numerator;sigDen=value->denominator;}
    }
    bpm=std::clamp(std::isfinite(bpm)?bpm:120.,30.,300.);
    const int solo = soloSlot.load();
    float measuredJet[kNumSlots]{},measuredExciter[kNumSlots]{};

    for (int start = 0; start < total; start += maxBlock) {
        const int len = juce::jmin(maxBlock, total - start);
        juce::AudioBuffer<float> chunk(buffer.getArrayOfWritePointers(), ch, start, len);
        for (int s = 0; s < kNumSlots; ++s) {
            const int type = types[s];
            if (type != lastType[s]) {
#if SKELLIERE_NATIVE_IMPORTS
                if(lastType[s]==DelayNative){static_cast<NativeDelayRack*>(mods[s][DelayNative].get())->leaveSlot();nativeModel[size_t(s)].store(-1,std::memory_order_relaxed);}
                if(type!=GlitchNative)glitchTrigger[size_t(s)].store(false,std::memory_order_relaxed);
                const bool alreadyPrepared=type==DelayNative&&lastType[s]==-1;
#else
                const bool alreadyPrepared=false;
#endif
                lastType[s] = type;
                if (!alreadyPrepared)if(auto* m = moduleFor(s, type, tuneSlot)) m->reset();
            }
#if SKELLIERE_NATIVE_IMPORTS
            if(type!=DelayNative)static_cast<NativeDelayRack*>(mods[s][DelayNative].get())->drain(len);
#endif
            auto* m = moduleFor(s, type, tuneSlot);
            if (m == nullptr) continue;
            float k[kMaxKnobs];
            for (int i = 0; i < kMaxKnobs; ++i) k[i] = kRaw[s][i]->load();
            const bool active=onRaw[s]->load()>0.5f && (solo<0||solo==s);
            if(type==SkellSSL){std::array<float,kSslExtraCount> ext{};for(int j=0;j<kSslExtraCount;++j)ext[j]=sslRaw[s][j]->load(std::memory_order_relaxed);static_cast<RecoveredSSL*>(m)->setExtended(ext);}
            if(hasAdvancedEngine(type)){AdvancedValues ext{};for(int j=0;j<kAdvancedCount;++j)ext[j]=advancedRaw[s][j]->load(std::memory_order_relaxed);static_cast<AdvancedRackModule*>(m)->setExtended(ext,bandSolo[size_t(s)].load(std::memory_order_relaxed));}
            if(type==Tune){auto* tune=static_cast<skl::SkellyTune*>(m);tune->setHumanize(humanizeRaw[s]->load(std::memory_order_relaxed));tune->setReferenceA(tuneReferenceHz(k[5]));}
            if(type==EchoUnified||type==DelayNative)m->setTempo(bpm);
            if(type==GlitchNative)m->setTransport(bpm,ppq+double(start)*bpm/(60.*sampleRate_),sigNum,sigDen,validPpq);
#if SKELLIERE_NATIVE_IMPORTS
            if(type==GlitchNative||type==DelayNative){
                NativeValues ext{};for(int j=0;j<kNativeControlCount;++j)ext[j]=nativeRaw[s][j]->load(std::memory_order_relaxed);
                if(type==DelayNative)static_cast<NativeDelayRack*>(m)->setExtended(ext);
                else {
                    auto* g=static_cast<NativeGlitchRack*>(m);g->setExtended(ext);
                    const int source=int(nativeReal(nGlitchSource,ext[nGlitchSource]));
                    if(glitchTrigger[size_t(s)].exchange(false,std::memory_order_acq_rel)&&source==2)g->trigger();
                    if(source==1){
                        // Audio buffer slices split exactly at MIDI note-on sample positions.
                        int cursor=0;for(const auto e:midi){const int at=e.samplePosition-start;if(at<0)continue;if(at>=len)break;
                            if(e.numBytes<3||!e.data||(e.data[0]&0xf0U)!=0x90U||e.data[2]==0)continue;
                            if(at>cursor){juce::AudioBuffer<float> part(chunk.getArrayOfWritePointers(),ch,cursor,at-cursor);g->process(part,k,active);}
                            g->trigger(e.data[2]/127.f,e.data[1]&0x7f);cursor=at;
                        }
                        if(cursor<len){juce::AudioBuffer<float> part(chunk.getArrayOfWritePointers(),ch,cursor,len-cursor);g->process(part,k,active);}continue;
                    }
                }
            }
#endif
            m->process(chunk,k,active);
            if(type==Exciter){const float peak=static_cast<AdvancedRackModule*>(m)->takeResidualPeak();if(active)measuredExciter[s]=std::max(measuredExciter[s],peak);}
#if SKELLIERE_NATIVE_IMPORTS
            if(type==DelayNative)nativeModel[size_t(s)].store(static_cast<NativeDelayRack*>(m)->renderedModel(),std::memory_order_relaxed);
#endif
            // Real post-effect peak telemetry for Jet, MixHead and SSL Strip. No audio changes,
            // allocations or locking; the existing buffer is read once.
            if(hasOutputPeak(type)&&active)measuredJet[s]=juce::jmax(measuredJet[s],chunk.getMagnitude(0,len));
        }
    }

    for(int s=0;s<kNumSlots;++s) {
        const int t=types[s];const bool live=onRaw[s]->load()>.5f&&(solo<0||solo==s);
        auto* measured=live&&t!=Tune?moduleFor(s,t,tuneSlot):nullptr;
        if(meterType[size_t(s)].load(std::memory_order_relaxed)!=t)meterType[size_t(s)].store(Empty,std::memory_order_release);
        rackReduction[size_t(s)].store(measured?sane(measured->meter()):0.f,std::memory_order_relaxed);
        if(t==Multiband)for(int j=0;j<8;++j)bandReduction[size_t(s)][size_t(j)].store(measured?static_cast<AdvancedRackModule*>(measured)->bandReduction(j):0.f,std::memory_order_relaxed);
        meterType[size_t(s)].store(t,std::memory_order_release);
        auto& residual=exciterPeak[size_t(s)];
        if(types[s]!=Exciter||onRaw[s]->load()<=.5f||advancedRaw[s][advMode]->load()<.5f||(solo>=0&&solo!=s))residual.store(0,std::memory_order_relaxed);
        else {auto old=residual.load(std::memory_order_relaxed);for(int attempt=0;attempt<2&&measuredExciter[s]>old;++attempt)if(residual.compare_exchange_strong(old,measuredExciter[s],std::memory_order_relaxed))break;}
        if(!hasOutputPeak(types[s]) || onRaw[s]->load()<=.5f || (solo>=0&&solo!=s))
            jetPeak[static_cast<size_t>(s)].store(0,std::memory_order_relaxed);
        else {
            auto& peak=jetPeak[static_cast<size_t>(s)];
            auto old=peak.load(std::memory_order_relaxed);
            // Single writer, single exchange reader: do not lose a peak
            // while the UI consumes the preceding one.
            // At most two attempts; the UI may consume the peak between
            // load and CAS. Audio never waits for the reader.
            for(int attempt=0;attempt<2&&measuredJet[s]>old;++attempt)
                if(peak.compare_exchange_strong(old,measuredJet[s],std::memory_order_relaxed))break;
        }
    }
    for(int i=0;i<total;++i) {
        const float gain=outputGain.getNextValue();
        for(int c=0;c<ch;++c) buffer.getWritePointer(c)[i]*=gain;
    }
    for (int c = 0; c < ch; ++c) {
        float* d = buffer.getWritePointer(c);
        for (int i = 0; i < total; ++i) d[i] = juce::jlimit(-16.0f, 16.0f, sane(d[i]));
    }
    outPeak.store(juce::jmax(outPeak.load(), buffer.getMagnitude(0, total)));
}

// ------------------------------------------------------------------ reading
int SkellierProcessor::slotType(int s) const {
    s = juce::jlimit(0, kNumSlots - 1, s);
    return resolveRackType(typeRaw[s]->load(), extraRaw[s]->load());
}
bool SkellierProcessor::slotOn(int s) const { return onRaw[juce::jlimit(0, kNumSlots - 1, s)]->load() > 0.5f; }
float SkellierProcessor::gainReduction(int s) const {
    // Bypass All returns from processBlock before any module runs, so each module
    // keeps its last stored reduction. Never present that stale value as live.
    const int solo=soloSlot.load();
    if(bypassRaw->load()>.5f||!slotOn(s)||(solo>=0&&solo!=s))return 0.f;
    const int t = slotType(s);
    if(s<0||s>=kNumSlots||t==Tune||t<=0||meterType[size_t(s)].load(std::memory_order_acquire)!=t)return 0.f;
    return rackReduction[size_t(s)].load(std::memory_order_relaxed);
}

// ------------------------------------------------------------------ editing (message thread)
void SkellierProcessor::setParam(const juce::String& id, float realValue) {
    if (auto* p = apvts.getParameter(id)) p->setValueNotifyingHost(p->convertTo0to1(realValue));
}

SkellierProcessor::SlotValues SkellierProcessor::readSlot(int s) const {
    SlotValues v; v.type = slotType(s); v.on = slotOn(s);
    v.artwork=artworkChoices[static_cast<size_t>(s)].load();
    for (int k = 0; k < kMaxKnobs; ++k) v.k[k] = kRaw[s][k]->load();
    for(int i=0;i<kSslExtraCount;++i)v.ssl[i]=sslRaw[s][i]->load();
    for(int i=0;i<kAdvancedCount;++i)v.advanced[i]=advancedRaw[s][i]->load(std::memory_order_relaxed);
    v.humanize=humanizeRaw[s]->load(std::memory_order_relaxed);
    for(int i=0;i<kNativeControlCount;++i)v.native[i]=nativeRaw[s][i]->load(std::memory_order_relaxed);
    return v;
}

void SkellierProcessor::writeSlot(int s, const SlotValues& v) {
    // Switch the slot off while it is rewritten so the audio thread never runs a module with half-updated knobs.
    setParam(onId(s), 0.0f);
    for (int k = 0; k < kMaxKnobs; ++k) setParam(knobId(s, k), v.k[k]);
    for(int i=0;i<kSslExtraCount;++i)setParam(sslId(s,i),v.ssl[i]);
    for(int i=0;i<kAdvancedCount;++i)if(advancedRaw[s][i]->load()!=v.advanced[i])setParam(advancedId(s,i),v.advanced[i]);
    setParam(humanizeId(s),v.humanize);
    for(int i=0;i<kNativeControlCount;++i)if(nativeRaw[s][i]->load()!=v.native[i])setParam(nativeId(s,i),v.native[i]);
    glitchTrigger[size_t(s)].store(false,std::memory_order_relaxed);
    bandSolo[size_t(s)].store(-1,std::memory_order_relaxed);
    setParam(extraId(s), 0.0f);
    setParam(typeId(s), v.type < kNumTypes ? (float) v.type : 0.0f);
    if (v.type >= kNumTypes) setParam(extraId(s), (float) (v.type - kNumTypes + 1));
    if(v.artwork>=0)artworkChoices[static_cast<size_t>(s)].store(juce::jlimit(0,3,v.artwork));
    setParam(onId(s), v.on ? 1.0f : 0.0f);
}

void SkellierProcessor::writeEmpty(int s) {
    SlotValues v; v.type = Empty; v.on = true;
    writeSlot(s, v);
}

void SkellierProcessor::pushUndo() {
    undoStack.push_back(captureRack());
    if (undoStack.size() > 50) undoStack.erase(undoStack.begin());
    redoStack.clear();
}

bool SkellierProcessor::addModule(int type, int slot) {
    if (type <= 0 || type >= kNumRackTypes || !importedAvailable(type)) return false;
    int target = slot;
    if (target < 0 || target >= kNumSlots) {
        target = -1;
        for (int s = 0; s < kNumSlots; ++s) if (slotType(s) == Empty) { target = s; break; }
        if (target < 0) return false;
    } else if (slotType(target) != Empty) {
        // Make room: find an empty slot to the right and shift everything between one place right.
        int empty = -1;
        for (int s = target + 1; s < kNumSlots; ++s) if (slotType(s) == Empty) { empty = s; break; }
        if (empty < 0) return false;
        pushUndo();
        for (int s = empty; s > target; --s) writeSlot(s, readSlot(s - 1));
        const int solo=soloSlot.load();
        if(solo>=target&&solo<empty)soloSlot.store(solo+1);
        SlotValues v; v.type = type;
        if(type==GlitchNative)v.native[nGlitchTrim]=(24.f-1.3f)/24.f;
        for (int k = 0; k < kMaxKnobs; ++k) v.k[k] = k < info(type).numKnobs ? defaultNorm(type, k) : 0.5f;
        writeSlot(target, v);
        return true;
    }
    pushUndo();
    SlotValues v; v.type = type;
        if(type==GlitchNative)v.native[nGlitchTrim]=(24.f-1.3f)/24.f;
    for (int k = 0; k < kMaxKnobs; ++k) v.k[k] = k < info(type).numKnobs ? defaultNorm(type, k) : 0.5f;
    writeSlot(target, v);
    return true;
}

bool SkellierProcessor::resetSlotControls(int slot,int expectedType){
    if(slot<0||slot>=kNumSlots||expectedType<=Empty||slotType(slot)!=expectedType||!importedAvailable(expectedType))return false;
    const auto current=readSlot(slot);SlotValues defaults;defaults.type=expectedType;defaults.on=current.on;defaults.artwork=current.artwork;
    for(int k=0;k<kMaxKnobs;++k)defaults.k[k]=k<info(expectedType).numKnobs?defaultNorm(expectedType,k):.5f;
    if(expectedType==GlitchNative)defaults.native[nGlitchTrim]=(24.f-1.3f)/24.f;
    pushUndo();writeSlot(slot,defaults);return true;
}
bool SkellierProcessor::toggleSlotPower(int slot,int expectedType){
    if(slot<0||slot>=kNumSlots||expectedType<=Empty||slotType(slot)!=expectedType)return false;
    pushUndo();auto* p=apvts.getParameter(onId(slot));p->beginChangeGesture();p->setValueNotifyingHost(slotOn(slot)?0.f:1.f);p->endChangeGesture();return true;
}
bool SkellierProcessor::copySlotControls(int slot,int expectedType){
    if(slot<0||slot>=kNumSlots||expectedType<=Empty||slotType(slot)!=expectedType)return false;
    clipboard=readSlot(slot);return true;
}
bool SkellierProcessor::canPasteSlotControls(int slot,int expectedType) const{
    return slot>=0&&slot<kNumSlots&&expectedType>Empty&&clipboard.type==expectedType&&slotType(slot)==expectedType;
}
bool SkellierProcessor::pasteSlotControls(int slot,int expectedType){
    if(!canPasteSlotControls(slot,expectedType))return false;
    auto value=clipboard;value.on=slotOn(slot);value.artwork=artworkChoices[size_t(slot)].load();pushUndo();writeSlot(slot,value);return true;
}

void SkellierProcessor::removeSlot(int slot) {
    if (slot < 0 || slot >= kNumSlots || slotType(slot) == Empty) return;
    pushUndo();
    if (soloSlot.load() == slot) soloSlot.store(-1);
    writeEmpty(slot);
}

void SkellierProcessor::moveSlot(int from, int to, bool copy) {
    if (from < 0 || from >= kNumSlots || to < 0 || to >= kNumSlots || from == to || slotType(from) == Empty) return;
    const auto a = readSlot(from);
    if (copy) {
        int target = to;
        if (slotType(target) != Empty) {
            target = -1;
            for (int s = 0; s < kNumSlots; ++s) if (slotType(s) == Empty) { target = s; break; }
            if (target < 0) return; // no mutation: preserve both history stacks
        }
        pushUndo();
        writeSlot(target, a);
    } else {
        pushUndo();
        const auto b = readSlot(to);
        writeSlot(to, a);
        writeSlot(from, b);
        if (soloSlot.load() == from) soloSlot.store(to); else if (soloSlot.load() == to) soloSlot.store(from);
    }
}

void SkellierProcessor::clearRack() {
    pushUndo();
    soloSlot.store(-1);
    for (int s = 0; s < kNumSlots; ++s) writeEmpty(s);
}

void SkellierProcessor::applyFactory(int index) {
    const auto& list = factory();
    const auto& p = list[(size_t) juce::jlimit(0, numFactory() - 1, index)];
    pushUndo();
    soloSlot.store(-1);
    for (int s = 0; s < kNumSlots; ++s) {
        if (s >= (int) p.slots.size()) { writeEmpty(s); continue; }
        const auto& fs = p.slots[(size_t) s];
        SlotValues v; v.type = fs.type;
        for (int k = 0; k < kMaxKnobs; ++k) v.k[k] = k < info(fs.type).numKnobs ? defaultNorm(fs.type, k) : 0.5f;
        for (const auto& kv : fs.set) v.k[kv.first] = toNorm(info(fs.type).knobs[kv.first], kv.second);
        writeSlot(s, v);
    }
}

// ------------------------------------------------------------------ undo, snapshots, strips
juce::String SkellierProcessor::captureRack() const {
    juce::XmlElement root("RACK");
    root.setAttribute("tuneControlVersion",tuneControlVersion);
    for (int s = 0; s < kNumSlots; ++s) {
        const auto v = readSlot(s);
        auto* e = root.createNewChildElement("SLOT");
        e->setAttribute("i", s); e->setAttribute("type", v.type); e->setAttribute("on", v.on ? 1 : 0);
        e->setAttribute("art",v.artwork);
        for (int k = 0; k < kMaxKnobs; ++k) e->setAttribute("k" + juce::String(k), (double) v.k[k]);
        if(v.type==SkellSSL)for(int i=0;i<kSslExtraCount;++i)e->setAttribute("ssl_"+juce::String(sslControl(i).id),double(v.ssl[i]));
        if(v.type==Tune)e->setAttribute("humanize",double(v.humanize));
        if(hasAdvancedEngine(v.type))for(int i=0;i<kAdvancedCount;++i)e->setAttribute("adv_"+juce::String(i),double(v.advanced[i]));
        if(importedType(v.type))for(int i=0;i<kNativeControlCount;++i)e->setAttribute("native_"+juce::String(i),double(v.native[i]));
    }
    return root.toString(juce::XmlElement::TextFormat().singleLine());
}

void SkellierProcessor::restoreRack(const juce::String& text) {
    const auto xml = juce::XmlDocument::parse(text);
    if (xml == nullptr || !xml->hasTagName("RACK")) return;
    SlotValues vals[kNumSlots];
    for (auto* e : xml->getChildWithTagNameIterator("SLOT")) {
        const int i = e->getIntAttribute("i", -1);
        if (i < 0 || i >= kNumSlots) continue;
        vals[i].type = juce::jlimit(0, (int) kNumRackTypes - 1, e->getIntAttribute("type", 0));
        vals[i].on = e->getIntAttribute("on", 1) != 0;
        // Older dream strips have no finish field: preserve the receiving slot finish.
        vals[i].artwork = juce::jlimit(-1,3,e->getIntAttribute("art",-1));
        for (int k = 0; k < kMaxKnobs; ++k) vals[i].k[k] = juce::jlimit(0.0f, 1.0f, (float) e->getDoubleAttribute("k" + juce::String(k), 0.5));
        if(vals[i].type==Tune&&xml->getIntAttribute("tuneControlVersion",0)<tuneControlVersion){
            vals[i].k[0]=migrateTuneRetune(float(e->getDoubleAttribute("k0",.125)));
            vals[i].k[5]=migrateTuneReference(float(e->getDoubleAttribute("k5",.5)));
        }
        for(int k=0;k<kSslExtraCount;++k){const float v=float(e->getDoubleAttribute("ssl_"+juce::String(sslControl(k).id),sslDefault(k)));vals[i].ssl[k]=std::isfinite(v)?std::clamp(v,0.f,1.f):sslDefault(k);}
        for(int k=0;k<kNativeControlCount;++k){const float v=float(e->getDoubleAttribute("native_"+juce::String(k),nativeDefault(k)));vals[i].native[k]=std::isfinite(v)?std::clamp(v,0.f,1.f):nativeDefault(k);}
        const float h=float(e->getDoubleAttribute("humanize",0.));vals[i].humanize=std::isfinite(h)?std::clamp(h,0.f,1.f):0.f;
        for(int k=0;k<kAdvancedCount;++k){const float v=float(e->getDoubleAttribute("adv_"+juce::String(k),advancedDefault(k)));vals[i].advanced[k]=std::isfinite(v)?std::clamp(v,0.f,1.f):advancedDefault(k);}
    }
    soloSlot.store(-1);
    for (int s = 0; s < kNumSlots; ++s) writeSlot(s, vals[s]);
}

void SkellierProcessor::undo() {
    if (undoStack.empty()) return;
    redoStack.push_back(captureRack());
    const auto last = undoStack.back(); undoStack.pop_back();
    restoreRack(last);
}
void SkellierProcessor::redo() {
    if (redoStack.empty()) return;
    undoStack.push_back(captureRack());
    const auto next = redoStack.back(); redoStack.pop_back();
    restoreRack(next);
}

void SkellierProcessor::switchSnapshot(int to) {
    to = juce::jlimit(0, 1, to);
    if (to == curSnap) return;
    snaps[curSnap] = captureRack();
    curSnap = to;
    if (snaps[to].isNotEmpty()) { pushUndo(); restoreRack(snaps[to]); }
    else snaps[to] = captureRack();
}
void SkellierProcessor::copySnapshotToOther() { snaps[1 - curSnap] = captureRack(); }

juce::File SkellierProcessor::stripsFolder() {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("SkellAudio").getChildFile("SkellierRack").getChildFile("DreamStrips");
}
juce::StringArray SkellierProcessor::listStrips() const {
    juce::StringArray names;
    for (const auto& f : stripsFolder().findChildFiles(juce::File::findFiles, false, "*.xml")) names.add(f.getFileNameWithoutExtension());
    names.sort(true);
    return names;
}
bool SkellierProcessor::saveStrip(const juce::String& name) {
    const auto clean = juce::File::createLegalFileName(name.trim());
    if (clean.isEmpty()) return false;
    const auto dir = stripsFolder();
    return dir.createDirectory().wasOk() && dir.getChildFile(clean + ".xml").replaceWithText(captureRack());
}
bool SkellierProcessor::loadStrip(const juce::String& name) {
    const auto f = stripsFolder().getChildFile(juce::File::createLegalFileName(name) + ".xml");
    if (!f.existsAsFile()) return false;
    const auto text=f.loadFileAsString();
    const auto xml=juce::XmlDocument::parse(text);
    if(xml==nullptr||!xml->hasTagName("RACK")||xml->getChildByName("SLOT")==nullptr)return false;
    pushUndo();
    restoreRack(text);
    return true;
}
void SkellierProcessor::deleteStrip(const juce::String& name) {
    stripsFolder().getChildFile(juce::File::createLegalFileName(name) + ".xml").deleteFile();
}

// ------------------------------------------------------------------ state
void SkellierProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto state = apvts.copyState();
    for (int s = 0; s < kNumSlots; ++s)
        state.setProperty("art" + juce::String(s), artworkChoices[(size_t) s].load(), nullptr);
    state.setProperty("tuneFlats", tuneFlats.load(std::memory_order_relaxed), nullptr);
    state.setProperty("tuneControlVersion",tuneControlVersion,nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, dest);
}
void SkellierProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType())) {
            auto state = juce::ValueTree::fromXml(*xml);
            // APVTS retains missing parameter values on replacement. Explicitly
            // insert zero for old sessions, even over a previously extended rack.
            for (int s = 0; s < kNumSlots; ++s) {
                if (!state.getChildWithProperty("id", extraId(s)).isValid()) {
                    juce::ValueTree parameter("PARAM");
                    parameter.setProperty("id", extraId(s), nullptr);
                    parameter.setProperty("value", 0, nullptr);
                    state.appendChild(parameter, nullptr);
                }
                for(int i=0;i<kSslExtraCount;++i){const auto id=sslId(s,i);if(!state.getChildWithProperty("id",id).isValid()){
                    juce::ValueTree parameter("PARAM");parameter.setProperty("id",id,nullptr);parameter.setProperty("value",sslDefault(i),nullptr);state.appendChild(parameter,nullptr);
                }}
                for(int i=0;i<kAdvancedCount;++i){const auto id=advancedId(s,i);if(!state.getChildWithProperty("id",id).isValid()){
                    juce::ValueTree parameter("PARAM");parameter.setProperty("id",id,nullptr);parameter.setProperty("value",advancedDefault(i),nullptr);state.appendChild(parameter,nullptr);
                }}
                if(int(state.getProperty("tuneControlVersion",0))<tuneControlVersion){
                    const auto typeParam=state.getChildWithProperty("id",typeId(s));
                    if(typeParam.isValid()&&int(typeParam.getProperty("value",0))==Tune){
                        for(int k:{0,5}){const auto id=knobId(s,k);auto param=state.getChildWithProperty("id",id);
                            const float oldValue=param.isValid()?float(param.getProperty("value",k==0?.125f:.5f)):(k==0?.125f:.5f);
                            if(!param.isValid()){param=juce::ValueTree("PARAM");param.setProperty("id",id,nullptr);state.appendChild(param,nullptr);}
                            param.setProperty("value",k==0?migrateTuneRetune(oldValue):migrateTuneReference(oldValue),nullptr);
                        }
                    }
                }
                for(int i=0;i<kNativeControlCount;++i){const auto id=nativeId(s,i);if(!state.getChildWithProperty("id",id).isValid()){
                    juce::ValueTree parameter("PARAM");parameter.setProperty("id",id,nullptr);parameter.setProperty("value",nativeDefault(i),nullptr);state.appendChild(parameter,nullptr);
                }}
                glitchTrigger[size_t(s)].store(false,std::memory_order_relaxed);
                const auto hid=humanizeId(s);if(!state.getChildWithProperty("id",hid).isValid()){juce::ValueTree parameter("PARAM");parameter.setProperty("id",hid,nullptr);parameter.setProperty("value",0.f,nullptr);state.appendChild(parameter,nullptr);}
                bandSolo[size_t(s)].store(-1,std::memory_order_relaxed);
                artworkChoices[(size_t) s] = juce::jlimit(0, 3,
                    (int) state.getProperty("art" + juce::String(s), 1));
            }
            tuneFlats.store(bool(state.getProperty("tuneFlats",false)),std::memory_order_relaxed);
            state.setProperty("tuneControlVersion",tuneControlVersion,nullptr);
            apvts.replaceState(state);
        }
    latencyUpdater.triggerAsyncUpdate();
}

juce::AudioProcessorEditor* SkellierProcessor::createEditor() { return new SkellierEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SkellierProcessor(); }

