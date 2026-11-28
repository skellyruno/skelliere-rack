#pragma once
#include <JuceHeader.h>
#include "RackModuleFactory.h"
#include <mutex>
class SkellierProcessor final : public juce::AudioProcessor,
                                private juce::AudioProcessorValueTreeState::Listener, private juce::Timer {
public:
    SkellierProcessor();
    ~SkellierProcessor() override;
    void prepareToPlay(double,int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Skelliere Rack"; }
    bool acceptsMidi() const override;
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
    static juce::String sslId(int s,int k) { return "s"+juce::String(s)+"_ssl_"+skl::sslControl(k).id; }
    static juce::String advancedId(int s,int k) { return "s"+juce::String(s)+"_adv_"+juce::String(k); }
    static juce::String humanizeId(int s) { return "s"+juce::String(s)+"_tune_humanize"; }
    static juce::String nativeId(int s,int k) { return "s"+juce::String(s)+"_native_"+juce::String(k); }
    void editNativeControl(int slot,int effect,int control,float value);
    void triggerNativeGlitch(int slot) noexcept;
    skl::NativeValues nativeDisplayValues(int slot) const noexcept;
    int nativeDelayModel(int slot) const noexcept;
    bool nativeDelayBlocked(int slot) const noexcept;
    void serviceNative(); // non-audio thread; also used by real editor regression harness
    float advancedBandReduction(int slot,int band) const noexcept;
    float takeExciterResidualPeak(int slot) noexcept;
    float advancedGraphTop() const noexcept {return graphTop.load(std::memory_order_relaxed); }
    std::array<std::atomic<int>,skl::kNumSlots> bandSolo{{-1,-1,-1,-1,-1,-1,-1,-1}};
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
    static juce::StringArray modulePresetNames(int type);
    bool applyModulePreset(int slot,int expectedType,int preset);
    std::atomic<bool> tuneFlats{false}; // display preference, never a DSP parameter
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
    int rackLatency() const;
private:
    static std::unique_ptr<skl::ModuleBase> makeModule(int t) { return skl::makeRackModule(t); }
    struct SlotValues { int type=0; int artwork=-1; bool on=true; float k[skl::kMaxKnobs]={.5f,.5f,.5f,.5f,.5f,.5f}; std::array<float,skl::kSslExtraCount> ssl=skl::sslDefaults(); skl::AdvancedValues advanced=skl::advancedDefaults(); float humanize=0.f; skl::NativeValues native=skl::nativeDefaults(); };
    void timerCallback() override {serviceNative();}
    void serviceNativeLocked();
    std::mutex nativeMutex; // prepare/release and non-RT producer only
    std::array<std::atomic<bool>,skl::kNumSlots> glitchTrigger{};
    std::array<std::atomic<int>,skl::kNumSlots> nativeModel{{-1,-1,-1,-1,-1,-1,-1,-1}};
    std::array<std::atomic<bool>,skl::kNumSlots> nativeBlocked{};
    std::atomic<float>* nativeRaw[skl::kNumSlots][skl::kNativeControlCount]={};
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
    std::atomic<float>* sslRaw[skl::kNumSlots][skl::kSslExtraCount]={};
    std::atomic<float>* advancedRaw[skl::kNumSlots][skl::kAdvancedCount]={};
    std::atomic<float>* humanizeRaw[skl::kNumSlots]={};
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
    std::atomic<float> inPeak{0},outPeak{0},graphTop{16000};
    std::array<std::atomic<float>,skl::kNumSlots> jetPeak{},exciterPeak{};
    std::vector<juce::String> undoStack,redoStack;
    juce::String snaps[2];
    int curSnap=0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkellierProcessor)
};

