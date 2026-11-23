#pragma once
#include <JuceHeader.h>
#include "RackModuleFactory.h"

// Skelliere Rack: eight slots, signal runs A to H. Every slot is described by plain host parameters
// (module type, on/off, six knobs), so presets, automation and DAW project recall all work with no extra state.
class SkellierProcessor final : public juce::AudioProcessor,
                                private juce::AudioProcessorValueTreeState::Listener {
public:
    SkellierProcessor();
    ~SkellierProcessor() override;

    // ---- juce::AudioProcessor
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Skelliere Rack"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // ---- parameters
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::String typeId(int slot) { return "s" + juce::String(slot) + "_type"; }
    static juce::String onId(int slot) { return "s" + juce::String(slot) + "_on"; }
    static juce::String knobId(int slot, int k) { return "s" + juce::String(slot) + "_k" + juce::String(k); }

    // ---- reading the rack (any thread)
    int slotType(int slot) const;
    bool slotOn(int slot) const;
    float gainReduction(int slot) const;
    float takeInputPeak() { return inPeak.exchange(0.0f); }
    float takeOutputPeak() { return outPeak.exchange(0.0f); }
    std::atomic<int> soloSlot{-1};

    // Appearance is message-thread-only, per instance and per slot. It survives
    // closing the editor but is not a host parameter or a saved sound setting.
    std::array<int, skl::kNumSlots> artworkChoices{{1, 1, 1, 1, 1, 1, 1, 1}};

    // ---- editing the rack (message thread only). Each of these is one undo step.
    bool addModule(int type, int slot = -1);
    void removeSlot(int slot);
    void moveSlot(int from, int to, bool copy);
    void clearRack();
    void applyFactory(int index);
    static int numFactory();
    static juce::String factoryName(int index);

    bool canUndo() const { return !undoStack.empty(); }
    bool canRedo() const { return !redoStack.empty(); }
    void undo();
    void redo();

    // A/B snapshots (kept for the session)
    int currentSnapshot() const { return curSnap; }
    void switchSnapshot(int to);
    void copySnapshotToOther();

    // Dream Strips: whole racks saved as small files in the user's Skell Audio folder
    static juce::File stripsFolder();
    juce::StringArray listStrips() const;
    bool saveStrip(const juce::String& name);
    bool loadStrip(const juce::String& name);
    void deleteStrip(const juce::String& name);

    int latencyOfTune() const;

private:
    // Existing preparation sites call this class factory, also exercised directly
    // by the host-free tests through skl::makeRackModule.
    static std::unique_ptr<skl::ModuleBase> makeModule(int type) { return skl::makeRackModule(type); }
    struct SlotValues { int type = 0; bool on = true; float k[skl::kMaxKnobs] = {0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f}; };

    void parameterChanged(const juce::String&, float) override;
    void setParam(const juce::String& id, float realValue);
    SlotValues readSlot(int slot) const;
    void writeSlot(int slot, const SlotValues&);
    void writeEmpty(int slot);
    juce::String captureRack() const;
    void restoreRack(const juce::String&);
    void pushUndo();
    void updateLatency();
    skl::ModuleBase* moduleFor(int slot, int type, int tuneSlot) const;

    struct LatencyUpdater final : juce::AsyncUpdater {
        explicit LatencyUpdater(SkellierProcessor& o) : owner(o) {}
        void handleAsyncUpdate() override { owner.updateLatency(); }
        SkellierProcessor& owner;
    } latencyUpdater{*this};

    std::atomic<float>* typeRaw[skl::kNumSlots] = {};
    std::atomic<float>* onRaw[skl::kNumSlots] = {};
    std::atomic<float>* kRaw[skl::kNumSlots][skl::kMaxKnobs] = {};
    std::atomic<float>* inRaw = nullptr;
    std::atomic<float>* outRaw = nullptr;
    std::atomic<float>* bypassRaw = nullptr;

    // Every slot owns all light modules; Tune is shared by the first Tune slot.
    std::unique_ptr<skl::ModuleBase> mods[skl::kNumSlots][skl::kNumTypes];
    std::unique_ptr<skl::ModuleBase> tuneModule;
    skl::LatencyAlignedBypass bypassDelay;
    int lastType[skl::kNumSlots];
    int maxBlock = 512;
    double sampleRate_ = 48000.0;
    bool prepared = false;
    float prevIn = 1.0f, prevOut = 1.0f;
    std::atomic<float> inPeak{0.0f}, outPeak{0.0f};

    std::vector<juce::String> undoStack, redoStack;
    juce::String snaps[2];
    int curSnap = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkellierProcessor)
};
