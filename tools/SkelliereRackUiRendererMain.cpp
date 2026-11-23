// Screenshots and checks use the real plugin editor, not a reconstruction.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <SkellCore/HeadlessUIRenderer.h>
#include <cstdio>
#include "../tests/ProcessorRegression.h"

static void visit(juce::Component& component, const std::function<void(juce::Component&)>& action) {
    action(component);
    for (int i = 0; i < component.getNumChildComponents(); ++i)
        visit(*component.getChildComponent(i), action);
}

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    try { processorRegression(); }
    catch(const std::exception& e) { std::fprintf(stderr,"Processor regression: %s\n",e.what()); return 1; }
    SkellierProcessor processor;
    processor.prepareToPlay(48000.0, 512);
    // Put both artwork-enabled effects in view without changing factory presets.
    processor.clearRack();
    processor.addModule(skl::Eq, 0);
    processor.addModule(skl::Air, 1);
    SkellierEditor editor(processor);
    skellcore::ui::headless::Options options;
    options.project = "Skelliere Rack v0.3.0 RACK-01 production/editor regression";
    options.scenes = {
        {"rack_original_100", 1260, 820, 0, 0},
        {"rack_aireq_100", 1260, 820, 0, 1},
        {"rack_classic_100", 1260, 820, 0, 2},
        {"rack_blackbird_100", 1260, 820, 0, 3},
        {"rack_aireq_80", 1008, 656, 0, 1},
        {"rack_aireq_125", 1575, 1025, 0, 1},
        {"rack_extensions_100", 1260, 820, 1, 0},
        {"rack_extensions_80", 1008, 656, 1, 0},
        {"rack_extensions_125", 1575, 1025, 1, 0},
        {"rack_empty_100", 1260, 820, 2, 0},
        {"rack_scroll_100", 1260, 820, 3, 0}
    };
    options.mustDiffer = {
        {"rack_original_100", "rack_aireq_100"},
        {"rack_aireq_100", "rack_classic_100"},
        {"rack_classic_100", "rack_blackbird_100"}
    };
    int expectedWidth = 1260, expectedHeight = 820;
    int expectedArtworkSlots = 2;
    options.configure = [&](const skellcore::ui::headless::Scene& scene) {
        expectedWidth = scene.width; expectedHeight = scene.height;
        processor.clearRack();
        expectedArtworkSlots = scene.theme == 0 ? 2 : 0;
        if(scene.theme == 0) { processor.addModule(skl::Eq,0); processor.addModule(skl::Air,1); }
        if(scene.theme == 1) {
            processor.addModule(skl::Tilt,0); processor.addModule(skl::Stereo,1);
            processor.addModule(skl::FastComp,2); processor.addModule(skl::BusComp,3);
        }
        if(scene.theme == 3) { processor.addModule(skl::Stereo,6); processor.addModule(skl::Tilt,7); }
        visit(editor, [&](juce::Component& c) {
            if (auto* ui = dynamic_cast<skl::ui::RackUI*>(&c))
                if (ui->onScale) ui->onScale(static_cast<float>(scene.width) / 1260.0f);
            if (auto* slot = dynamic_cast<skl::ui::SlotComponent*>(&c)) { slot->rebuild(); slot->setArtworkChoice(scene.mode); }
            if(auto* viewport = dynamic_cast<juce::Viewport*>(&c))
                if(dynamic_cast<skl::ui::RackStrip*>(viewport->getViewedComponent())) viewport->setViewPosition(scene.theme == 3 ? 1200 : 0,0);
        });
    };
    options.inspect = [&] {
        juce::StringArray errors;
        if (editor.getWidth() != expectedWidth || editor.getHeight() != expectedHeight)
            errors.add("Editor scaling dimensions do not match the scene");
        int visibleArtworkSlots = 0;
        visit(editor, [&](juce::Component& c) {
            if (auto* slot = dynamic_cast<skl::ui::SlotComponent*>(&c)) {
                if (slot->currentType() == skl::Eq || slot->currentType() == skl::Air) {
                    ++visibleArtworkSlots;
                    if (!slot->artworkLayoutIsValid()) errors.add("Artwork overlaps live knobs or selector is hidden");
                }
            }
        });
        if (visibleArtworkSlots != expectedArtworkSlots) errors.add("Unexpected artwork slot count");
        for (int type : {skl::Eq, skl::Air})
            for (int choice : {1, 2, 3}) {
                const auto& panel = skl::ui::rackPanel(type, choice);
                if (!panel.isValid() || panel.getWidth() != 215 || panel.getHeight() != 570)
                    errors.add("Embedded artwork failed portrait decoding");
            }
        // Catch a future rotate/transpose regression with scanline sample positions.
        const auto raw = juce::ImageFileFormat::loadFrom(BinaryData::air_lift_faceplate_png,
            static_cast<size_t>(BinaryData::air_lift_faceplate_pngSize));
        const auto& portrait = skl::ui::rackPanel(skl::Air, 1);
        if (raw.isValid() && portrait.isValid()) {
            for (int offset : {0, 214, 215, 569, 570, 119853})
                if (portrait.getPixelAt(offset % 215, offset / 215) != raw.getPixelAt(offset % 570, offset / 570))
                    errors.add("Packed artwork scanlines were rotated or transposed");
        } else errors.add("Artwork source could not be decoded");
        return errors;
    };
    const auto output = argc > 1 ? juce::File(juce::String::fromUTF8(argv[1]))
        : juce::File::getCurrentWorkingDirectory().getChildFile("ui-screenshots");
    const auto result = skellcore::ui::headless::render(editor, output, options);
    for (const auto& error : result.errors) std::fprintf(stderr, "%s\n", error.toRawUTF8());
    std::printf("Captured %d real Skelliere Rack editor screenshots\n", result.captured);
    processor.releaseResources();
    return result.passed() && result.captured == static_cast<int>(options.scenes.size()) ? 0 : 1;
}

