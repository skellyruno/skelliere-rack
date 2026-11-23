#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>
#include "PluginProcessor.h"
#include "Look.h"

namespace skl::ui {

// UI-only appearance choices. Audio processing and host parameters never depend
// on these values. They are shared between instances in this process and are
// intentionally not saved into a DAW project or rack preset.
inline int& eqArtworkChoice() { static int choice = 1; return choice; }
inline int& airArtworkChoice() { static int choice = 1; return choice; }

// ------------------------------------------------------------------ one knob: dial, printed label, value readout
class KnobCell final : public juce::Component {
public:
    using Fmt = std::function<juce::String(double)>;
    // compact=true lays the dial left of its label, for the input/output knobs in the top bar.
    KnobCell(const juce::String& label, juce::RangedAudioParameter& param, Fmt fmt, juce::Colour cap, juce::Colour pointer,
             double resetValue, double step = 0.0, bool compact = false);
    void resized() override;
    void paint(juce::Graphics&) override;
    juce::Slider slider;
private:
    juce::String label_, text_;
    Fmt fmt_;
    bool compact_;
    juce::SliderParameterAttachment attachment;
};

// ------------------------------------------------------------------ small displays
class LcdDisplay final : public juce::Component {
public:
    void setText(const juce::String& t) { text = t.toUpperCase(); repaint(); }
    void paint(juce::Graphics&) override;
private:
    juce::String text;
};

class LevelMeter final : public juce::Component {
public:
    void setLevels(float inDb, float outDb) { inDb_ = inDb; outDb_ = outDb; repaint(); }
    void paint(juce::Graphics&) override;
private:
    float inDb_ = -90.0f, outDb_ = -90.0f;
};

class PowerButton final : public juce::Button {
public:
    PowerButton() : juce::Button("Power") { setClickingTogglesState(true); }
    void paintButton(juce::Graphics&, bool hover, bool down) override;
};

// Converted resource images are stored landscape in the archives, so rotate
// once on the message thread before drawing them. No image work is in the DSP.
inline juce::Image uprightArtwork(const void* bytes, int size) {
    auto raw = juce::ImageFileFormat::loadFrom(bytes, static_cast<size_t>(size));
    if (!raw.isValid()) return {};
    juce::Image upright(juce::Image::ARGB, raw.getHeight(), raw.getWidth(), true);
    juce::Graphics imageGraphics(upright);
    imageGraphics.drawImageTransformed(raw,
        juce::AffineTransform::rotation(juce::MathConstants<float>::halfPi)
            .translated(static_cast<float>(raw.getHeight()), 0.0f));
    return upright;
}

class ArtworkPanelBackground final : public juce::Component {
public:
    ArtworkPanelBackground(juce::Component& parent, std::function<int()> getType)
        : getType_(std::move(getType)) {
        setInterceptsMouseClicks(false, false);
        setBounds(0, 0, 168, 540);
        parent.addAndMakeVisible(*this); // first child: power, knobs and buttons stay on top
    }
    void paint(juce::Graphics& g) override {
        const int type = getType_();
        const int choice = type == skl::Eq ? eqArtworkChoice() : type == skl::Air ? airArtworkChoice() : 0;
        if (choice == 0) return;
        static const juce::Image air = uprightArtwork(BinaryData::air_lift_faceplate_png, BinaryData::air_lift_faceplate_pngSize);
        static const juce::Image earth = uprightArtwork(BinaryData::earth_faceplate_png, BinaryData::earth_faceplate_pngSize);
        static const juce::Image classicEq = uprightArtwork(BinaryData::classic_eq_faceplate_png, BinaryData::classic_eq_faceplate_pngSize);
        static const juce::Image classicLift = uprightArtwork(BinaryData::classic_lift_faceplate_png, BinaryData::classic_lift_faceplate_pngSize);
        static const juce::Image blackbird = uprightArtwork(BinaryData::blackbird_faceplate_png, BinaryData::blackbird_faceplate_pngSize);
        const juce::Image* face = nullptr;
        if (choice == 3) face = &blackbird;
        else if (type == skl::Eq) face = choice == 1 ? &earth : &classicEq;
        else if (type == skl::Air) face = choice == 1 ? &air : &classicLift;
        if (face == nullptr || !face->isValid()) return;
        g.drawImageTransformed(*face, juce::AffineTransform::scale(
            static_cast<float>(getWidth()) / static_cast<float>(face->getWidth()),
            static_cast<float>(getHeight()) / static_cast<float>(face->getHeight())));
        if (choice == 3) {
            g.setColour(juce::Colours::black.withAlpha(0.88f));
            g.fillRect(7, 464, getWidth() - 14, 30);
            g.setColour(juce::Colours::white);
            g.setFont(juce::Font(juce::FontOptions(10.0f).withStyle("Bold")));
            g.drawFittedText("ART ONLY / NO MIC MODELING", 10, 468, getWidth() - 20, 22,
                             juce::Justification::centred, 1);
        }
    }
private:
    std::function<int()> getType_;
};

// Appearance is chosen from a visible button rather than from painted knobs.
// This button does not create a module or promise any new audio processing.
class ArtworkButton final : public juce::TextButton {
public:
    ArtworkButton(juce::Component& parent, std::function<int()> getType)
        : juce::TextButton("LOOK"), getType_(std::move(getType)) {
        setBounds(42, 500, 112, 26);
        setTooltip("Choose artwork; Blackbird is appearance only and does not model a microphone");
        parent.addAndMakeVisible(*this);
        onClick = [this] {
            const int type = getType_();
            if (type != skl::Eq && type != skl::Air) return;
            const int selected = type == skl::Eq ? eqArtworkChoice() : airArtworkChoice();
            juce::PopupMenu menu;
            menu.addItem(1, "Original rack", true, selected == 0);
            menu.addItem(2, type == skl::Eq ? "AirEQ Earth art" : "AirEQ Air art", true, selected == 1);
            menu.addItem(3, type == skl::Eq ? "CS-EQ art" : "CS-Lift art", true, selected == 2);
            menu.addSeparator();
            menu.addItem(4, "Blackbird art only (no mic processing)", true, selected == 3);
            juce::Component::SafePointer<ArtworkButton> safeThis(this);
            menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [safeThis, type](int result) {
                if (safeThis == nullptr || result < 1 || result > 4) return;
                (type == skl::Eq ? eqArtworkChoice() : airArtworkChoice()) = result - 1;
                if (auto* parent = safeThis->getParentComponent()) parent->repaint();
                safeThis->repaint();
            });
        };
    }
    bool hitTest(int x, int y) override {
        return (getType_() == skl::Eq || getType_() == skl::Air) && juce::TextButton::hitTest(x, y);
    }
    void paintButton(juce::Graphics& g, bool hover, bool down) override {
        if (getType_() == skl::Eq || getType_() == skl::Air)
            juce::TextButton::paintButton(g, hover, down);
    }
private:
    std::function<int()> getType_;
};

// ------------------------------------------------------------------ one rack slot
class SlotComponent final : public juce::Component, public juce::DragAndDropTarget {
public:
    static constexpr int kWidth = 168, kHeight = 540;
    SlotComponent(SkellierProcessor&, int index);
    int currentType() const { return type; }
    void rebuild();                         // rebuild knobs after the module in this slot changed
    void setMeter(float gainReductionDb);   // called from the UI timer
    void syncSolo();
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override {}
    void mouseDrag(const juce::MouseEvent&) override;
    bool isInterestedInDragSource(const SourceDetails&) override;
    void itemDragEnter(const SourceDetails&) override { dragOver = true; repaint(); }
    void itemDragExit(const SourceDetails&) override { dragOver = false; repaint(); }
    void itemDropped(const SourceDetails&) override;
private:
    void paintEmpty(juce::Graphics&, juce::Rectangle<float>);
    void paintFaceplate(juce::Graphics&, juce::Rectangle<float>);

    SkellierProcessor& proc;
    int index, type = 0;
    bool dragOver = false, dragging = false;
    float needle = 0.0f, shownNeedle = -1.0f;
    std::vector<std::unique_ptr<KnobCell>> cells;
    std::vector<juce::Rectangle<int>> sectionRects;
    juce::Rectangle<int> vuRect, footRect;
    ArtworkPanelBackground artPanel{*this, [this] { return type; }};
    ArtworkButton artButton{*this, [this] { return type; }};
    PowerButton powerBtn;
    std::unique_ptr<juce::ButtonParameterAttachment> powerAtt;
    juce::TextButton leftBtn{"<"}, rightBtn{">"}, removeBtn{"x"}, soloBtn{"S"};
};

// the eight slots side by side, scrolled by a viewport
class RackStrip final : public juce::Component {
public:
    explicit RackStrip(SkellierProcessor&);
    static constexpr int kGap = 4, kPad = 8;
    void resized() override {}
    std::array<std::unique_ptr<SlotComponent>, skl::kNumSlots> slots;
};

// ------------------------------------------------------------------ module library
class ModuleTile final : public juce::Component {
public:
    ModuleTile(int moduleType, std::function<void(int)> onSelect, std::function<void(int)> onAdd);
    int type;
    bool selected = false;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override { if (onSelect) onSelect(type); }
    void mouseDoubleClick(const juce::MouseEvent&) override { if (onAdd) onAdd(type); }
    void mouseDrag(const juce::MouseEvent&) override;
private:
    std::function<void(int)> onSelect, onAdd;
};

class LibraryPanel final : public juce::Component {
public:
    explicit LibraryPanel(SkellierProcessor&);
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    void refresh();
    void select(int type);
    SkellierProcessor& proc;
    juce::TextButton cats[5];
    juce::TextEditor search;
    juce::OwnedArray<ModuleTile> tiles;
    juce::Label infoLabel;
    int category = -1;   // -1 = all
    int selectedType = 0;
};

class PathStrip final : public juce::Component {
public:
    explicit PathStrip(SkellierProcessor& p) : proc(p) {}
    void paint(juce::Graphics&) override;
private:
    SkellierProcessor& proc;
};

class StripButton final : public juce::TextButton {
public:
    std::function<void()> onDelete;
    void mouseDown(const juce::MouseEvent& e) override;
};

// ------------------------------------------------------------------ the whole interface at its base size
class RackUI final : public juce::Component,
                     private juce::AudioProcessorValueTreeState::Listener,
                     private juce::AsyncUpdater,
                     private juce::Timer {
public:
    static constexpr int kW = 1260, kH = 820;
    explicit RackUI(SkellierProcessor&);
    ~RackUI() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    std::function<void(float)> onScale;   // asks the editor window to resize
private:
    void parameterChanged(const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override;
    void timerCallback() override;
    void say(const juce::String&);
    void rebuildStripButtons();

    SkellierProcessor& proc;
    int curPreset = 0;
    float inShown = -90.0f, outShown = -90.0f;

    juce::TextButton prevPreset{"<"}, nextPreset{">"}, snapA{"A"}, snapB{"B"}, snapCopy{"COPY"},
        undoBtn{"UNDO"}, redoBtn{"REDO"}, clearBtn{"CLEAR"}, bypassBtn{"BYPASS"};
    juce::TextButton scaleBtn[4];
    juce::ComboBox presetBox;
    std::unique_ptr<juce::ButtonParameterAttachment> bypassAtt;

    LcdDisplay lcd;
    LevelMeter meters;
    KnobCell inKnob, outKnob;

    juce::Label stripsLabel;
    juce::TextButton defaultStrip{"DEFAULT"}, saveStripBtn{"SAVE STRIP"};
    juce::TextEditor stripName;
    juce::OwnedArray<StripButton> stripButtons;

    LibraryPanel library;
    RackStrip strip;
    juce::Viewport viewport;
    PathStrip pathStrip;
};

} // namespace skl::ui

// ------------------------------------------------------------------ the plugin window
class SkellierEditor final : public juce::AudioProcessorEditor, public juce::DragAndDropContainer {
public:
    explicit SkellierEditor(SkellierProcessor&);
    ~SkellierEditor() override;
    void paint(juce::Graphics& g) override { g.fillAll(skl::ui::pal::chassis); }
    void resized() override {}
private:
    void applyScale(float z);
    skl::ui::SkellierLook look;
    std::unique_ptr<skl::ui::RackUI> ui;
};
