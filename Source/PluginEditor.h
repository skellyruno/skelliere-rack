#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Look.h"
#include "RackArtwork.h"

namespace skl::ui {

class KnobCell final : public juce::Component {
public:
    using Fmt = std::function<juce::String(double)>;
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

class SlotComponent final : public juce::Component, public juce::DragAndDropTarget {
public:
    static constexpr int kWidth = 248, kHeight = 540;
    SlotComponent(SkellierProcessor&, int index);
    int currentType() const { return type; }
    void rebuild();
    void setMeter(float gainReductionDb);
    void syncSolo();
    // Also used by the real-editor screenshot harness, not a second UI.
    void setArtworkChoice(int choice) {
        artworkChoice = juce::jlimit(0, 3, choice);
        artPanel.repaint(); artButton.repaint();
    }
    bool artworkLayoutIsValid() const {
        if (type != Eq && type != Air) return true;
        if (!artPanel.isVisible() || !artButton.isVisible()) return false;
        for (const auto& cell : cells)
            if (cell->getX() < 112 || cell->getRight() > kWidth - 8 || cell->getBottom() > 450) return false;
        return !cells.empty();
    }
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override { dragging = false; }
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
    int& artworkChoice = proc.artworkChoices[static_cast<size_t>(index)];
    ArtworkPanelBackground artPanel{*this, [this] { return type; }, artworkChoice};
    ArtworkButton artButton{*this, [this] { return type; }, artworkChoice};
    PowerButton powerBtn;
    std::unique_ptr<juce::ButtonParameterAttachment> powerAtt;
    juce::TextButton leftBtn{"<"}, rightBtn{">"}, removeBtn{"x"}, soloBtn{"S"};
};

class RackStrip final : public juce::Component {
public:
    explicit RackStrip(SkellierProcessor&);
    static constexpr int kGap = 4, kPad = 8;
    void resized() override {}
    std::array<std::unique_ptr<SlotComponent>, skl::kNumSlots> slots;
};

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
    int category = -1;
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
    std::function<void(float)> onScale;
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
