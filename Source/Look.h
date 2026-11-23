#pragma once
// Skelliere look: blacked-out analog gear. One chassis, graphite brushed faceplates, knurled knobs,
// printed legends and cream needle meters. All colours live here so the whole plugin can be re-skinned in one place.
#include <JuceHeader.h>
#include "Modules.h"

namespace skl::ui {

using juce::Colour;
using juce::Graphics;

namespace pal {
inline const Colour chassis   {0xff08090a};
inline const Colour chassis2  {0xff131416};
inline const Colour barTop    {0xff2b2d31};
inline const Colour barBot    {0xff17181a};
inline const Colour plateTop  {0xff2a2c30};
inline const Colour plateBot  {0xff131416};
inline const Colour ink       {0xffdedacb};
inline const Colour inkDim    {0xff8f908a};
inline const Colour line      {0xff3a3d42};
inline const Colour amber     {0xfff2b92e};
inline const Colour lcdBg     {0xff060a07};
inline const Colour lcdInk    {0xff8ff08a};
inline const Colour warn      {0xffef6a5b};
inline const Colour vuFaceTop {0xfff6edc9};
inline const Colour vuFaceBot {0xffd9c68a};
inline const Colour vuInk     {0xff2b2418};
inline const Colour vuRed     {0xffa02a1d};
}

inline juce::Font font(float h, bool bold = false) {
    return juce::Font(juce::FontOptions(h).withStyle(bold ? "Bold" : "Regular"));
}

inline Colour categoryColour(int cat) {
    switch (cat) {
        case CatDynamics: return Colour(0xff3fb0a6);
        case CatTone:     return Colour(0xffe0a634);
        case CatSpace:    return Colour(0xff8f84dc);
        case CatUtility:  return Colour(0xff9aa0a8);
        default:          return Colour(0xff55585d);
    }
}
// Knob cap colour per family, so a glance down the rack tells you what is what.
inline Colour capColour(int cat) {
    switch (cat) {
        case CatDynamics: return Colour(0xff25272b);   // black caps
        case CatSpace:    return Colour(0xffe6dec6);   // cream caps
        default:          return Colour(0xffc9cbce);   // aluminium caps
    }
}
inline Colour pointerColour(int cat) { return cat == CatDynamics ? Colour(0xfff1eee2) : Colour(0xff17181a); }

inline juce::String knobText(const Knob& k, float norm) {
    const float v = toReal(k, norm);
    switch (k.unit) {
        case Unit::Db:    return juce::String(v > 0.05f ? "+" : "") + juce::String(v, 1) + " dB";
        case Unit::Hz:
            if (juce::String(k.label) == "HPF" && v <= 22.0f) return "Off";
            return v >= 1000.0f ? juce::String(v / 1000.0f, 1) + " kHz" : juce::String((int) std::lround(v)) + " Hz";
        case Unit::Ms:    return v < 10.0f ? juce::String(v, 1) + " ms" : juce::String((int) std::lround(v)) + " ms";
        case Unit::Pct:   return juce::String((int) std::lround(v * 100.0f)) + "%";
        case Unit::Ratio: return juce::String(v, 1) + ":1";
        case Unit::Sec:   return juce::String(v, 1) + " s";
        case Unit::LfoHz: return juce::String(v, 2) + " Hz";
        case Unit::Pan:   return std::fabs(v) < 0.02f ? juce::String("Center") : juce::String(v < 0 ? "L " : "R ") + juce::String((int) std::lround(std::fabs(v) * 100.0f));
        case Unit::Steps: { auto names = juce::StringArray::fromTokens(k.names ? k.names : "", ",", ""); return names[(int) v]; }
    }
    return {};
}

inline const juce::Image& brushedImage() {
    static const juce::Image img = [] {
        juce::Image im(juce::Image::ARGB, 256, 256, true);
        Graphics g(im);
        juce::Random rng(1234);
        for (int y = 0; y < 256; ++y) {
            g.setColour((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha(rng.nextFloat() * 0.07f));
            g.drawHorizontalLine(y, 0.0f, 256.0f);
        }
        return im;
    }();
    return img;
}

inline void paintBrushed(Graphics& g, juce::Rectangle<float> r, float opacity = 1.0f) {
    g.setTiledImageFill(brushedImage(), 0, 0, opacity);
    g.fillRect(r);
}

inline void paintScrew(Graphics& g, float cx, float cy, float r, float turn) {
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillEllipse(cx - r, cy - r + 1.0f, 2 * r, 2 * r);
    g.setGradientFill(juce::ColourGradient(Colour(0xfff0f0f0), cx - r * 0.4f, cy - r * 0.5f, Colour(0xff6c6c6c), cx + r * 0.7f, cy + r * 0.8f, true));
    g.fillEllipse(cx - r, cy - r, 2 * r, 2 * r);
    g.setColour(Colour(0xff2a2a2a));
    g.drawEllipse(cx - r, cy - r, 2 * r, 2 * r, 0.8f);
    juce::Path cross;
    cross.addRectangle(-r * 0.75f, -0.9f, r * 1.5f, 1.8f);
    cross.addRectangle(-0.9f, -r * 0.75f, 1.8f, r * 1.5f);
    g.fillPath(cross, juce::AffineTransform::rotation(turn).translated(cx, cy));
}

// The needle meter used by the compressor, de-esser and limiter. value is gain reduction in dB (0 to 20).
inline void paintVU(Graphics& g, juce::Rectangle<float> r, float value) {
    g.setColour(Colour(0xff101010));
    g.fillRoundedRectangle(r, 4.0f);
    auto face = r.reduced(3.0f);
    g.setGradientFill(juce::ColourGradient::vertical(pal::vuFaceTop, face.getY(), pal::vuFaceBot, face.getBottom()));
    g.fillRoundedRectangle(face, 3.0f);

    juce::Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(face.toNearestInt());
    const float cx = face.getCentreX(), cy = face.getBottom() + 2.0f, sc = face.getWidth() / 130.0f;
    auto pt = [&](float rad, float v) {
        const float a = juce::degreesToRadians(-50.0f + v * 5.0f);
        return juce::Point<float>(cx + rad * sc * std::sin(a), cy - rad * sc * std::cos(a));
    };
    for (float v = 0.0f; v <= 20.0f; v += 2.0f) {
        const bool major = std::fmod(v, 4.0f) < 0.01f;
        g.setColour(v >= 12.0f ? pal::vuRed : pal::vuInk);
        g.drawLine(juce::Line<float>(pt(57.0f, v), pt(major ? 49.0f : 53.0f, v)), major ? 1.4f : 0.9f);
        if (major) { g.setFont(font(8.0f * sc, true)); g.drawText(juce::String((int) v), juce::Rectangle<float>(0, 0, 18, 10).withCentre(pt(41.0f, v)), juce::Justification::centred, false); }
    }
    juce::Path arc;
    arc.startNewSubPath(pt(57.0f, 0.0f));
    for (float v = 0.0f; v <= 20.0f; v += 1.0f) arc.lineTo(pt(57.0f, v));
    g.setColour(pal::vuInk); g.strokePath(arc, juce::PathStrokeType(1.1f));
    juce::Path red;
    red.startNewSubPath(pt(54.0f, 12.0f));
    for (float v = 12.0f; v <= 20.0f; v += 1.0f) red.lineTo(pt(54.0f, v));
    g.setColour(pal::vuRed); g.strokePath(red, juce::PathStrokeType(3.0f * sc));
    g.setColour(pal::vuInk); g.setFont(font(8.0f * sc, true));
    g.drawText("GR dB", face.withTrimmedLeft(7.0f).removeFromBottom(14.0f), juce::Justification::centredLeft, false);

    const float v = juce::jlimit(0.0f, 20.0f, value);
    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.drawLine(juce::Line<float>(juce::Point<float>(cx + 1.5f, cy + 1.5f), pt(52.0f, v) + juce::Point<float>(1.5f, 1.5f)), 1.6f);
    g.setColour(pal::vuRed);
    g.drawLine(juce::Line<float>(juce::Point<float>(cx, cy), pt(52.0f, v)), 1.5f);
    g.setColour(Colour(0xff1a1a1a)); g.fillEllipse(cx - 6.0f * sc, cy - 6.0f * sc, 12.0f * sc, 12.0f * sc);
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.28f), face.getX(), face.getY(), juce::Colours::white.withAlpha(0.0f), face.getX(), face.getY() + face.getHeight() * 0.5f, false));
    g.fillRect(face.withHeight(face.getHeight() * 0.45f));
}

class SkellierLook final : public juce::LookAndFeel_V4 {
public:
    SkellierLook() {
        setColour(juce::ResizableWindow::backgroundColourId, pal::chassis);
        setColour(juce::TextButton::textColourOffId, pal::ink);
        setColour(juce::TextButton::textColourOnId, pal::amber);
        setColour(juce::Label::textColourId, pal::ink);
        setColour(juce::ComboBox::backgroundColourId, Colour(0xff0d0e10));
        setColour(juce::ComboBox::outlineColourId, Colour(0xff000000));
        setColour(juce::ComboBox::textColourId, pal::ink);
        setColour(juce::ComboBox::arrowColourId, pal::amber);
        setColour(juce::PopupMenu::backgroundColourId, Colour(0xff17181a));
        setColour(juce::PopupMenu::textColourId, pal::ink);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, pal::amber);
        setColour(juce::PopupMenu::highlightedTextColourId, Colour(0xff1d1704));
        setColour(juce::TextEditor::backgroundColourId, Colour(0xff0d0e10));
        setColour(juce::TextEditor::textColourId, pal::ink);
        setColour(juce::TextEditor::outlineColourId, Colour(0xff000000));
        setColour(juce::TextEditor::focusedOutlineColourId, pal::amber);
        setColour(juce::CaretComponent::caretColourId, pal::amber);
        setColour(juce::ScrollBar::thumbColourId, Colour(0xff5b5e64));
        setColour(juce::ScrollBar::backgroundColourId, Colour(0xff0c0d0e));
    }

    juce::Font getTextButtonFont(juce::TextButton&, int) override { return font(13.0f, true); }
    juce::Font getComboBoxFont(juce::ComboBox&) override { return font(13.0f); }
    juce::Font getPopupMenuFont() override { return font(13.0f); }

    void drawButtonBackground(Graphics& g, juce::Button& b, const Colour&, bool hover, bool down) override {
        auto r = b.getLocalBounds().toFloat().reduced(0.5f);
        const bool on = b.getToggleState();
        Colour top = on ? Colour(0xff0b0c0d) : Colour(0xff3d4045), bot = on ? Colour(0xff1b1c1e) : Colour(0xff1e2023);
        if (down) std::swap(top, bot);
        if (hover && !down) { top = top.brighter(0.12f); bot = bot.brighter(0.08f); }
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(r.translated(0.0f, 1.2f), 3.0f);
        g.setGradientFill(juce::ColourGradient::vertical(top, r.getY(), bot, r.getBottom()));
        g.fillRoundedRectangle(r, 3.0f);
        g.setColour(Colour(0xff000000)); g.drawRoundedRectangle(r, 3.0f, 1.0f);
        if (!on) { g.setColour(juce::Colours::white.withAlpha(0.16f)); g.drawHorizontalLine((int) r.getY() + 1, r.getX() + 3.0f, r.getRight() - 3.0f); }
        if (on && b.getClickingTogglesState() && !b.getProperties().contains("plain")) {   // lit LED on latching buttons
            const float cx = r.getX() + 9.0f, cy = r.getCentreY();
            g.setColour(pal::amber.withAlpha(0.35f)); g.fillEllipse(cx - 5.0f, cy - 5.0f, 10.0f, 10.0f);
            g.setColour(pal::amber); g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
        }
    }
    void drawButtonText(Graphics& g, juce::TextButton& b, bool, bool) override {
        g.setFont(font(13.0f, true));
        g.setColour(b.getToggleState() ? pal::amber : pal::ink);
        auto r = b.getLocalBounds();
        if (b.getToggleState() && b.getClickingTogglesState() && !b.getProperties().contains("plain")) r = r.withTrimmedLeft(12);
        g.drawFittedText(b.getButtonText(), r, juce::Justification::centred, 1);
    }

    void drawComboBox(Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override {
        auto r = juce::Rectangle<float>(0, 0, (float) w, (float) h);
        g.setColour(Colour(0xff0d0e10)); g.fillRoundedRectangle(r, 3.0f);
        g.setColour(Colour(0xff000000)); g.drawRoundedRectangle(r.reduced(0.5f), 3.0f, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.08f)); g.drawHorizontalLine(h - 1, 3.0f, (float) w - 3.0f);
        juce::Path p;
        p.addTriangle((float) w - 18.0f, h * 0.42f, (float) w - 8.0f, h * 0.42f, (float) w - 13.0f, h * 0.62f);
        g.setColour(pal::amber); g.fillPath(p);
    }

    void drawRotarySlider(Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s) override {
        const float size = (float) juce::jmin(w, h);
        const float cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f, R = size * 0.34f;
        const float ang = a0 + pos * (a1 - a0);
        const Colour cap((juce::uint32) (int) s.getProperties().getWithDefault("cap", (int) 0xffc9cbce));
        const Colour ptr((juce::uint32) (int) s.getProperties().getWithDefault("ptr", (int) 0xff17181a));

        for (int i = 0; i <= 10; ++i) {   // printed scale
            const float a = a0 + (a1 - a0) * (float) i / 10.0f, sn = std::sin(a), cs = -std::cos(a);
            const float r0 = R * 1.18f, r1 = R * (i % 5 == 0 ? 1.38f : 1.29f);
            g.setColour(pal::ink.withAlpha(0.7f));
            g.drawLine(cx + sn * r0, cy + cs * r0, cx + sn * r1, cy + cs * r1, i % 5 == 0 ? 1.4f : 1.0f);
        }
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillEllipse(cx - R, cy - R + 2.6f, 2 * R, 2 * R);
        g.setGradientFill(juce::ColourGradient(Colour(0xff3a3c40), cx - R * 0.5f, cy - R * 0.7f, Colour(0xff0a0a0b), cx + R * 0.6f, cy + R, true));
        g.fillEllipse(cx - R, cy - R, 2 * R, 2 * R);
        for (int i = 0; i < 30; ++i) {   // knurled skirt
            const float a = (float) i * juce::MathConstants<float>::twoPi / 30.0f, sn = std::sin(a), cs = std::cos(a);
            g.setColour((i % 2 ? juce::Colours::white : juce::Colours::black).withAlpha(i % 2 ? 0.20f : 0.55f));
            g.drawLine(cx + sn * R * 0.80f, cy + cs * R * 0.80f, cx + sn * R * 0.98f, cy + cs * R * 0.98f, 1.0f);
        }
        const float r = R * 0.72f;
        g.setGradientFill(juce::ColourGradient(cap.brighter(0.35f), cx - r * 0.6f, cy - r * 0.7f, cap.darker(0.45f), cx + r * 0.8f, cy + r * 0.9f, true));
        g.fillEllipse(cx - r, cy - r, 2 * r, 2 * r);
        g.setColour(juce::Colours::black.withAlpha(0.45f)); g.drawEllipse(cx - r, cy - r, 2 * r, 2 * r, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.12f)); g.drawEllipse(cx - r * 0.62f, cy - r * 0.62f, r * 1.24f, r * 1.24f, 0.8f);
        const float sn = std::sin(ang), cs = -std::cos(ang);
        g.setColour(ptr);
        g.drawLine(cx + sn * r * 0.30f, cy + cs * r * 0.30f, cx + sn * r * 0.93f, cy + cs * r * 0.93f, 2.6f);
        if (s.hasKeyboardFocus(false)) { g.setColour(pal::amber.withAlpha(0.8f)); g.drawEllipse(cx - R * 1.1f, cy - R * 1.1f, R * 2.2f, R * 2.2f, 1.2f); }
    }

    void drawScrollbar(Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart, int thumbSize, bool over, bool down) override {
        g.setColour(Colour(0xff0c0d0e)); g.fillRect(x, y, w, h);
        auto t = vertical ? juce::Rectangle<int>(x + 2, thumbStart, w - 4, thumbSize) : juce::Rectangle<int>(thumbStart, y + 2, thumbSize, h - 4);
        g.setColour(Colour(down ? 0xff7a7d84 : over ? 0xff6a6d74 : 0xff55585e)); g.fillRoundedRectangle(t.toFloat(), 3.0f);
    }
};

} // namespace skl::ui
