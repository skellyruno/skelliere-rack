#pragma once
#include <JuceHeader.h>
#include "RackModuleFactory.h"
class SkellierProcessor final : public juce::AudioProcessor,
                                private juce::AudioProcessorValueTreeState::Listener {
public:
    SkellierProcessor();
    ~SkellierProcessor() override;
    void prepareToPlay(double,int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Skelliere Rack"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::String typeId(int s) { return "s"+juce::String(s)+"_type"; }
    static juce::String onId(int s) { return "s"+juce::String(s)+"_on"; }
    static juce::String knobId(int s,int k) { return "s"+juce::String(s)+"_k"+juce::String(k); }
    // Appended host parameters select new effects only while legacy type is Empty.
    // This leaves all original choice normalisation and parameter ordering intact.
    static juce::String extraId(int s) { return "s"+juce::String(s)+"_extra"; }
    int slotType(int) const;
    bool slotOn(int) const;
    float gainReduction(int) const;
    float takeJetPeak(int slot) noexcept { return jetPeak[static_cast<size_t>(slot)].exchange(0.0f,std::memory_order_relaxed); }
    float takeInputPeak() { return inPeak.exchange(0); }
    float takeOutputPeak() { return outPeak.exchange(0); }
    std::atomic<int> soloSlot{-1};
    std::array<std::atomic<int>,skl::kNumSlots> artworkChoices{{1,1,1,1,1,1,1,1}};
    bool addModule(int,int slot=-1);
    void removeSlot(int);
    void moveSlot(int,int,bool);
    void clearRack();
    void applyFactory(int);
    static int numFactory();
    static juce::String factoryName(int);
    bool canUndo() const { return !undoStack.empty(); }
    bool canRedo() const { return !redoStack.empty(); }
    void undo();
    void redo();
    int currentSnapshot() const { return curSnap; }
    void switchSnapshot(int);
    void copySnapshotToOther();
    static juce::File stripsFolder();
    juce::StringArray listStrips() const;
    bool saveStrip(const juce::String&);
    bool loadStrip(const juce::String&);
    void deleteStrip(const juce::String&);
    int latencyOfTune() const;
private:
    static std::unique_ptr<skl::ModuleBase> makeModule(int t) { return skl::makeRackModule(t); }
    struct SlotValues { int type=0; int artwork=-1; bool on=true; float k[skl::kMaxKnobs]={.5f,.5f,.5f,.5f,.5f,.5f}; };
    void parameterChanged(const juce::String&,float) override;
    void setParam(const juce::String&,float);
    SlotValues readSlot(int) const;
    void writeSlot(int,const SlotValues&);
    void writeEmpty(int);
    juce::String captureRack() const;
    void restoreRack(const juce::String&);
    void pushUndo();
    void updateLatency();
    skl::ModuleBase* moduleFor(int,int,int) const;
    struct LatencyUpdater final : juce::AsyncUpdater {
        explicit LatencyUpdater(SkellierProcessor& o):owner(o){}
        void handleAsyncUpdate() override { owner.updateLatency(); }
        SkellierProcessor& owner;
    } latencyUpdater{*this};
    std::atomic<float>* typeRaw[skl::kNumSlots]={};
    std::atomic<float>* extraRaw[skl::kNumSlots]={};
    std::atomic<float>* onRaw[skl::kNumSlots]={};
    std::atomic<float>* kRaw[skl::kNumSlots][skl::kMaxKnobs]={};
    std::atomic<float>* inRaw=nullptr;
    std::atomic<float>* outRaw=nullptr;
    std::atomic<float>* bypassRaw=nullptr;
    std::unique_ptr<skl::ModuleBase> mods[skl::kNumSlots][skl::kNumRackTypes];
    std::unique_ptr<skl::ModuleBase> tuneModule;
    skl::LatencyAlignedBypass bypassDelay;
    int lastType[skl::kNumSlots];
    int maxBlock=512;
    double sampleRate_=48000;
    bool prepared=false;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGain, outputGain;
    std::atomic<float> inPeak{0},outPeak{0};
    std::array<std::atomic<float>,skl::kNumSlots> jetPeak{};
    std::vector<juce::String> undoStack,redoStack;
    juce::String snaps[2];
    int curSnap=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkellierProcessor)
};

