#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>
#include "Modules.h"

namespace skl::ui {
// The supplied converted PNGs contain portrait scanlines under a landscape
// header. Repack pixels in row-major order; rotating or transposing is wrong.
inline juce::Image decodeRackPanel(const void* bytes, int size) {
    const auto raw = juce::ImageFileFormat::loadFrom(bytes, static_cast<size_t>(size));
    if (!raw.isValid() || raw.getWidth() != 570 || raw.getHeight() != 215) return {};
    juce::Image panel(juce::Image::ARGB, 215, 570, true);
    juce::Image::BitmapData source(raw, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData destination(panel, juce::Image::BitmapData::writeOnly);
    for (int i = 0; i < 215 * 570; ++i)
        destination.setPixelColour(i % 215, i / 215, source.getPixelColour(i % 570, i / 570));
    return panel;
}
inline const juce::Image& rackPanel(int type, int choice) {
    static const juce::Image air = decodeRackPanel(BinaryData::air_lift_faceplate_png, BinaryData::air_lift_faceplate_pngSize);
    static const juce::Image earth = decodeRackPanel(BinaryData::earth_faceplate_png, BinaryData::earth_faceplate_pngSize);
    static const juce::Image eq = decodeRackPanel(BinaryData::classic_eq_faceplate_png, BinaryData::classic_eq_faceplate_pngSize);
    static const juce::Image lift = decodeRackPanel(BinaryData::classic_lift_faceplate_png, BinaryData::classic_lift_faceplate_pngSize);
    static const juce::Image bird = decodeRackPanel(BinaryData::blackbird_faceplate_png, BinaryData::blackbird_faceplate_pngSize);
    static const juce::Image empty;
    if (choice == 0) return empty;
    if (choice == 3) return bird;
    if (type == skl::Eq) return choice == 1 ? earth : eq;
    if (type == skl::Air) return choice == 1 ? air : lift;
    return empty;
}
inline juce::String rackPanelName(int type, int choice) {
    if (choice == 0) return "Original rack";
    if (choice == 3) return "Blackbird art only";
    if (type == skl::Eq) return choice == 1 ? "AirEQ Earth art" : "CS-EQ art";
    return choice == 1 ? "AirEQ Air art" : "CS-Lift art";
}
class ArtworkPanelBackground final : public juce::Component {
public:
    ArtworkPanelBackground(juce::Component& parent, std::function<int()> getType, int& choice)
        : getType_(std::move(getType)), choice_(choice) {
        setInterceptsMouseClicks(false, false);
        setBounds(0, 0, 248, 540);
        parent.addAndMakeVisible(*this);
    }
    void paint(juce::Graphics& g) override {
        const int type = getType_();
        if (type != skl::Eq && type != skl::Air) return;
        const auto& image = rackPanel(type, choice_);
        const juce::Rectangle<float> area(10.0f, 108.0f, 96.0f, 340.0f);
        if (image.isValid()) {
            g.drawImageWithin(image, 10, 108, 96, 340, juce::RectanglePlacement::centred);
        } else {
            g.setColour(juce::Colours::black.withAlpha(0.35f));
            g.fillRoundedRectangle(area, 3.0f);
            g.setColour(juce::Colours::white);
            g.drawFittedText(choice_ == 0 ? "ORIGINAL RACK" : "ARTWORK UNAVAILABLE", area.toNearestInt().reduced(5), juce::Justification::centred, 3);
        }
        g.setColour(juce::Colours::black.withAlpha(0.85f));
        g.fillRoundedRectangle(8.0f, 450.0f, 232.0f, 43.0f, 3.0f);
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(11.0f).withStyle("Bold")));
        g.drawFittedText(rackPanelName(type, choice_), 14, 453, 220, 16, juce::Justification::centred, 1);
        g.setFont(juce::Font(juce::FontOptions(10.0f)));
        g.drawFittedText(choice_ == 3 ? "Art only / no microphone modeling" : "Artwork only / use the live knobs on the right", 14, 471, 220, 17, juce::Justification::centred, 1);
    }
private:
    std::function<int()> getType_;
    int& choice_;
};
class ArtworkButton final : public juce::TextButton {
public:
    ArtworkButton(juce::Component& parent, std::function<int()> getType, int& choice)
        : juce::TextButton("CHOOSE ARTWORK"), getType_(std::move(getType)), choice_(choice) {
        setBounds(44, 500, 192, 28);
        setTooltip("Artwork changes appearance only. The labeled knobs control the existing effect.");
        parent.addAndMakeVisible(*this);
        onClick = [this] {
            const int type = getType_();
            if (type != skl::Eq && type != skl::Air) return;
            juce::PopupMenu menu;
            for (int choice = 0; choice < 4; ++choice)
                menu.addItem(choice + 1, rackPanelName(type, choice), true, choice_ == choice);
            juce::Component::SafePointer<ArtworkButton> safeThis(this);
            menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [safeThis](int result) {
                if (safeThis == nullptr || result < 1 || result > 4) return;
                safeThis->choice_ = result - 1;
                if (auto* parent = safeThis->getParentComponent()) parent->repaint();
            });
        };
    }
    bool hitTest(int x, int y) override {
        return (getType_() == skl::Eq || getType_() == skl::Air) && juce::TextButton::hitTest(x, y);
    }
    void paintButton(juce::Graphics& g, bool hover, bool down) override {
        if (getType_() == skl::Eq || getType_() == skl::Air) juce::TextButton::paintButton(g, hover, down);
    }
private:
    std::function<int()> getType_;
    int& choice_;
};
}
