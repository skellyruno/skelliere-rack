#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>
#include "Modules.h"

namespace skl::ui {
// Asset-specific row strides, confirmed against the exact embedded PNGs.
// Four metallic faces use 215 pixels. Blackbird uses 216; the landscape
// container truncates its last partial row, so retain only complete rows.
inline juce::Image decodeRackPanel(const void* bytes, int size, int stride = 215) {
    const auto raw = juce::ImageFileFormat::loadFrom(bytes, static_cast<size_t>(size));
    if (!raw.isValid() || raw.getWidth() != 570 || raw.getHeight() != 215
        || (stride != 215 && stride != 216)) return {};
    const int rows = raw.getWidth() * raw.getHeight() / stride;
    juce::Image panel(juce::Image::ARGB, stride, rows, true);
    juce::Image::BitmapData source(raw, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData destination(panel, juce::Image::BitmapData::writeOnly);
    for (int i = 0; i < stride * rows; ++i)
        destination.setPixelColour(i % stride, i / stride, source.getPixelColour(i % 570, i / 570));
    return panel;
}
inline bool hasRackSkin(int type) { return type == skl::Eq || type == skl::Air; }
inline const juce::Image& rackPanel(int type, int choice) {
    static const juce::Image air = decodeRackPanel(BinaryData::air_lift_faceplate_png, BinaryData::air_lift_faceplate_pngSize);
    static const juce::Image earth = decodeRackPanel(BinaryData::earth_faceplate_png, BinaryData::earth_faceplate_pngSize);
    static const juce::Image eq = decodeRackPanel(BinaryData::classic_eq_faceplate_png, BinaryData::classic_eq_faceplate_pngSize);
    static const juce::Image lift = decodeRackPanel(BinaryData::classic_lift_faceplate_png, BinaryData::classic_lift_faceplate_pngSize);
    static const juce::Image bird = decodeRackPanel(BinaryData::blackbird_faceplate_png, BinaryData::blackbird_faceplate_pngSize, 216);
    static const juce::Image empty;
    if (!hasRackSkin(type) || choice < 1 || choice > 3) return empty;
    if (choice == 3) return bird;
    if (type == skl::Eq) return choice == 1 ? earth : eq;
    return choice == 1 ? air : lift;
}
// The supplied artwork's printed controls are not our DSP controls. Sample
// inspected, label-free material regions, not the unrelated printed legends.
// Mirroring the tile prevents a hard seam at the texture boundaries.
inline juce::Image materialTile(const juce::Image& panel, juce::Rectangle<int> sample) {
    if (!panel.isValid() || !panel.getBounds().contains(sample) || sample.isEmpty()) return {};
    const int w = sample.getWidth(), h = sample.getHeight();
    juce::Image result(juce::Image::ARGB, w * 2, h * 2, true);
    juce::Image::BitmapData source(panel, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dest(result, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < h * 2; ++y)
        for (int x = 0; x < w * 2; ++x)
            dest.setPixelColour(x, y, source.getPixelColour(
                sample.getX() + (x < w ? x : 2*w - 1 - x),
                sample.getY() + (y < h ? y : 2*h - 1 - y)));
    return result;
}
inline const juce::Image& rackMaterial(int type, int choice) {
    static const auto earth = materialTile(rackPanel(skl::Eq, 1), {28, 39, 159, 26});
    static const auto air = materialTile(rackPanel(skl::Air, 1), {28, 39, 159, 26});
    static const auto eq = materialTile(rackPanel(skl::Eq, 2), {26, 38, 163, 9});
    static const auto lift = materialTile(rackPanel(skl::Air, 2), {26, 38, 163, 9});
    static const auto bird = materialTile(rackPanel(skl::Eq, 3), {28, 103, 160, 237});
    static const juce::Image empty;
    if (!hasRackSkin(type) || choice < 1 || choice > 3) return empty;
    if (choice == 3) return bird;
    if (type == skl::Eq) return choice == 1 ? earth : eq;
    return choice == 1 ? air : lift;
}
inline bool lightRackSkin(int type, int choice) {
    return (choice == 1 || choice == 2) && rackMaterial(type, choice).isValid();
}
inline juce::String rackPanelName(int type, int choice) {
    if (choice == 0) return "Original";
    if (choice == 3) return "Blackbird graphite";
    if (type == skl::Eq) return choice == 1 ? "AirEQ Earth" : "CS-EQ";
    return choice == 1 ? "AirEQ Air" : "CS-Lift";
}
class ArtworkButton final : public juce::TextButton {
public:
    ArtworkButton(std::function<int()> getType, std::function<int()> getChoice,
                  std::function<void(int)> apply)
        : juce::TextButton("SKIN"), getType_(std::move(getType)),
          getChoice_(std::move(getChoice)), apply_(std::move(apply)) {
        setTooltip("Change the material of this module. All controls retain their real parameter bindings.");
        onClick = [this] {
            const int type = getType_();
            if (!hasRackSkin(type)) return;
            juce::PopupMenu menu;
            for (int choice = 0; choice < 4; ++choice)
                menu.addItem(choice + 1, rackPanelName(type, choice),
                             choice == 0 || rackMaterial(type, choice).isValid(), getChoice_() == choice);
            juce::Component::SafePointer<ArtworkButton> safeThis(this);
            menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [safeThis, type](int result) {
                if (safeThis == nullptr || safeThis->getType_() != type || result < 1 || result > 4) return;
                safeThis->apply_(result - 1);
            });
        };
    }
private:
    std::function<int()> getType_, getChoice_;
    std::function<void(int)> apply_;
};
}
