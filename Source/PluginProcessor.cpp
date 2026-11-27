#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace skl;

// ------------------------------------------------------------------ factory racks
namespace {
struct FSlot { int type; std::vector<std::pair<int, float>> set; };   // knob index -> real value
struct FPreset { const char* name; std::vector<FSlot> slots; };

const std::vector<FPreset>& factory() {
    static const std::vector<FPreset> list = {
        { "Silk Lead", {
            { Tune, {{0, 25}} },
            { Eq, {{0, 90}, {2, -2}, {3, 1.5f}, {4, 1}} },
            { Comp, {{0, -26}, {1, 3.5f}, {2, 12}, {3, 140}, {4, 5}} },
            { DeEss, {} },
            { Sat, {{0, 5}, {2, 0.35f}} },
            { Air, {{0, 3.5f}} },
            { Limiter, {} } } },
        { "Warm Vocal", {
            { Comp, {{0, -22}, {1, 2.5f}, {2, 20}, {3, 200}, {4, 3}} },
            { Sat, {{0, 9}, {1, 8000}, {2, 0.5f}} },
            { DeEss, {} },
            { Chorus, {{2, 0.18f}} },
            { Delay, {{3, 0.18f}} },
            { skl::Reverb, {{3, 0.22f}} } } },
        { "Glue Bus", {
            { Trim, {{0, -2}} },
            { Eq, {{0, 30}, {1, 1}, {4, 1.5f}} },
            { Sat, {{0, 4}, {2, 0.4f}} },
            { Comp, {{0, -18}, {1, 2}, {2, 30}, {3, 250}, {4, 2}} },
            { Limiter, {{0, 2}} } } },
        { "Empty rack", {} }
    };
    return list;
}
} // namespace

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
    latencyUpdater.cancelPendingUpdate();
    for (int s = 0; s < kNumSlots; ++s) { apvts.removeParameterListener(typeId(s), this); apvts.removeParameterListener(extraId(s), this); }
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

void SkellierProcessor::prepareToPlay(double sr, int samplesPerBlock) {
    sampleRate_ = std::isfinite(sr) && sr > 1000.0 ? sr : 48000.0;
    maxBlock = juce::jmax(1, samplesPerBlock);
    const int ch = juce::jlimit(1, 2, getMainBusNumInputChannels());
    for (int s = 0; s < kNumSlots; ++s)
        for (int t = Eq; t < kNumRackTypes; ++t) {
            if (t == Tune) continue;
            mods[s][t] = makeModule(t);
            mods[s][t]->prepare(sampleRate_, maxBlock, ch);
        }
    tuneModule = makeModule(Tune);
    tuneModule->prepare(sampleRate_, maxBlock, ch);
    bypassDelay.prepare(tuneModule->latency()+kNumSlots*std::max(mods[0][SkellDemon]->latency(),mods[0][SkellMixHead]->latency()));
    for (int s = 0; s < kNumSlots; ++s) {lastType[s] = -1;jetPeak[static_cast<size_t>(s)].store(0,std::memory_order_relaxed);}
    inputGain.reset(sampleRate_, .020); outputGain.reset(sampleRate_, .020);
    inputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(inRaw->load()));
    outputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outRaw->load()));
    prepared = true;
    updateLatency();
}

void SkellierProcessor::updateLatency() {
    setLatencySamples(rackLatency());
}

int SkellierProcessor::latencyOfTune() const {
    if (!tuneModule) return 0;
    for (int s = 0; s < kNumSlots; ++s)
        if (slotType(s) == Tune) return tuneModule->latency();
    return 0;
}

int SkellierProcessor::rackLatency() const {
    int latency=latencyOfTune();
    for(int s=0;s<kNumSlots;++s){const auto t=slotType(s);if(t!=Tune&&t>0&&t<kNumRackTypes&&mods[s][t])latency+=mods[s][t]->latency();}
    return latency;
}

void SkellierProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples(), ch = buffer.getNumChannels();
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c) buffer.clear(c, 0, total);
    if (!prepared || total <= 0 || ch < 1) return;

    const float inG = juce::Decibels::decibelsToGain(juce::jlimit(-24.0f, 12.0f, inRaw->load()));
    const float outG = juce::Decibels::decibelsToGain(juce::jlimit(-24.0f, 12.0f, outRaw->load()));
    const bool bypass = bypassRaw->load() > 0.5f;
    // Advance the untouched input's shadow delay for *both* modes. JUCE's reported
    // latency remains the same when the user toggles Bypass All, avoiding a dry
    // timing jump relative to other tracks when a Tune module is present.
    const float rawPeak = buffer.getMagnitude(0, total);
    bypassDelay.process(buffer, rackLatency(), bypass);
    if (bypass) {
        for(auto& peak:jetPeak)peak.store(0,std::memory_order_relaxed);
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

    int tuneSlot = -1;
    int types[kNumSlots];
    for (int s = 0; s < kNumSlots; ++s) {
        types[s] = slotType(s);
        if (types[s] == Tune && tuneSlot < 0) tuneSlot = s;
    }
    const int solo = soloSlot.load();
    float measuredJet[kNumSlots]{};

    for (int start = 0; start < total; start += maxBlock) {
        const int len = juce::jmin(maxBlock, total - start);
        juce::AudioBuffer<float> chunk(buffer.getArrayOfWritePointers(), ch, start, len);
        for (int s = 0; s < kNumSlots; ++s) {
            const int type = types[s];
            if (type != lastType[s]) {
                lastType[s] = type;
                if (auto* m = moduleFor(s, type, tuneSlot)) m->reset();
            }
            auto* m = moduleFor(s, type, tuneSlot);
            if (m == nullptr) continue;
            float k[kMaxKnobs];
            for (int i = 0; i < kMaxKnobs; ++i) k[i] = kRaw[s][i]->load();
            const bool active=onRaw[s]->load()>0.5f && (solo<0||solo==s);
            if(type==SkellSSL){std::array<float,kSslExtraCount> ext{};for(int j=0;j<kSslExtraCount;++j)ext[j]=sslRaw[s][j]->load(std::memory_order_relaxed);static_cast<RecoveredSSL*>(m)->setExtended(ext);}
            m->process(chunk,k,active);
            // Real post-effect peak telemetry for Jet, MixHead and SSL Strip. No audio changes,
            // allocations or locking; the existing buffer is read once.
            if(hasOutputPeak(type)&&active)measuredJet[s]=juce::jmax(measuredJet[s],chunk.getMagnitude(0,len));
        }
    }

    for(int s=0;s<kNumSlots;++s) {
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
    if (t == Tune) return 0.0f;
    if (t <= 0 || !mods[s][t]) return 0.0f;
    return mods[s][t]->meter();
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
    return v;
}

void SkellierProcessor::writeSlot(int s, const SlotValues& v) {
    // Switch the slot off while it is rewritten so the audio thread never runs a module with half-updated knobs.
    setParam(onId(s), 0.0f);
    for (int k = 0; k < kMaxKnobs; ++k) setParam(knobId(s, k), v.k[k]);
    for(int i=0;i<kSslExtraCount;++i)setParam(sslId(s,i),v.ssl[i]);
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
    if (type <= 0 || type >= kNumRackTypes) return false;
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
        for (int k = 0; k < kMaxKnobs; ++k) v.k[k] = k < info(type).numKnobs ? defaultNorm(type, k) : 0.5f;
        writeSlot(target, v);
        return true;
    }
    pushUndo();
    SlotValues v; v.type = type;
    for (int k = 0; k < kMaxKnobs; ++k) v.k[k] = k < info(type).numKnobs ? defaultNorm(type, k) : 0.5f;
    writeSlot(target, v);
    return true;
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
    for (int s = 0; s < kNumSlots; ++s) {
        const auto v = readSlot(s);
        auto* e = root.createNewChildElement("SLOT");
        e->setAttribute("i", s); e->setAttribute("type", v.type); e->setAttribute("on", v.on ? 1 : 0);
        e->setAttribute("art",v.artwork);
        for (int k = 0; k < kMaxKnobs; ++k) e->setAttribute("k" + juce::String(k), (double) v.k[k]);
        if(v.type==SkellSSL)for(int i=0;i<kSslExtraCount;++i)e->setAttribute("ssl_"+juce::String(sslControl(i).id),double(v.ssl[i]));
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
        for(int k=0;k<kSslExtraCount;++k){const float v=float(e->getDoubleAttribute("ssl_"+juce::String(sslControl(k).id),sslDefault(k)));vals[i].ssl[k]=std::isfinite(v)?std::clamp(v,0.f,1.f):sslDefault(k);}
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
                artworkChoices[(size_t) s] = juce::jlimit(0, 3,
                    (int) state.getProperty("art" + juce::String(s), 1));
            }
            apvts.replaceState(state);
        }
    latencyUpdater.triggerAsyncUpdate();
}

juce::AudioProcessorEditor* SkellierProcessor::createEditor() { return new SkellierEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SkellierProcessor(); }

