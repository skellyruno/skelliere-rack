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
        {"rack_sprites_min", 1260, 820, 0, 1},
        {"rack_sprites_max", 1260, 820, 0, 1},
        {"rack_fader_100", 1260, 820, 5, 1},
        {"rack_blackbird_100", 1260, 820, 0, 3},
        {"rack_aireq_80", 1008, 656, 0, 1},
        {"rack_aireq_125", 1575, 1025, 0, 1},
        {"rack_classic_80", 1008, 656, 0, 2},
        {"rack_blackbird_125", 1575, 1025, 0, 3},
        {"rack_extensions_100", 1260, 820, 1, 0},
        {"rack_extensions_80", 1008, 656, 1, 0},
        {"rack_extensions_125", 1575, 1025, 1, 0},
        {"rack_program_opto_100", 1260, 820, 4, 0},
        {"rack_program_opto_80", 1008, 656, 4, 0},
        {"rack_empty_100", 1260, 820, 2, 0},
        {"rack_scroll_100", 1260, 820, 3, 0}
    };
    options.mustDiffer = {
        {"rack_original_100", "rack_aireq_100"},
        {"rack_aireq_100", "rack_classic_100"},
        {"rack_classic_100", "rack_blackbird_100"},
        {"rack_sprites_min", "rack_sprites_max"}
    };
#if SKELLIERE_FULL_ARTWORK
    options.scenes.push_back({"rack_full_opto_bus",1260,820,6,1});
    options.scenes.push_back({"rack_full_fet_modern",1260,820,7,2});
    options.scenes.push_back({"rack_full_fet_vintage",1260,820,7,3});
    options.scenes.push_back({"rack_full_instrument_fader",1260,820,5,2});
#endif
    int expectedWidth = 1260, expectedHeight = 820;
    int expectedArtworkSlots = 2;
    bool expectProgramOpto = false;
    bool bindingChecks = true;
    options.configure = [&](const skellcore::ui::headless::Scene& scene) {
        expectedWidth = scene.width; expectedHeight = scene.height;
        processor.clearRack();
        expectedArtworkSlots = scene.theme == 0 ? 2 : 0;
        expectProgramOpto = scene.theme == 4 || scene.theme == 6;
        bindingChecks = true;
        if(scene.theme == 0) { processor.addModule(skl::Eq,0); processor.addModule(skl::Air,1); }
        if(scene.theme == 1) {
            processor.addModule(skl::Tilt,0); processor.addModule(skl::Stereo,1);
            processor.addModule(skl::FastComp,2); processor.addModule(skl::BusComp,3);
        }
        if(scene.theme == 3) { processor.addModule(skl::Stereo,6); processor.addModule(skl::Tilt,7); }
        if(scene.theme == 4) {
            processor.addModule(skl::ProgramOpto,0); processor.addModule(skl::FastComp,1);
            processor.addModule(skl::BusComp,2);
        }
        if(scene.theme == 5) processor.addModule(skl::Trim,0);
        if(scene.theme == 6) { processor.addModule(skl::Comp,0);processor.addModule(skl::ProgramOpto,1);processor.addModule(skl::BusComp,2); }
        if(scene.theme == 7) processor.addModule(skl::FastComp,0);
        visit(editor, [&](juce::Component& c) {
            if (auto* ui = dynamic_cast<skl::ui::RackUI*>(&c))
                if (ui->onScale) ui->onScale(static_cast<float>(scene.width) / 1260.0f);
            if (auto* slot = dynamic_cast<skl::ui::SlotComponent*>(&c)) { slot->rebuild(); slot->setArtworkChoice(scene.mode); bindingChecks=slot->exerciseSpriteBindings() && bindingChecks; }
            if(auto* viewport = dynamic_cast<juce::Viewport*>(&c))
                if(dynamic_cast<skl::ui::RackStrip*>(viewport->getViewedComponent())) viewport->setViewPosition(scene.theme == 3 ? 1200 : 0,0);
        });
        if(scene.id=="rack_sprites_min" || scene.id=="rack_sprites_max") {
            const float value=scene.id=="rack_sprites_max"?1.0f:0.0f;
            for(int slot=0;slot<2;++slot) {
                processor.apvts.getParameter(SkellierProcessor::onId(slot))->setValueNotifyingHost(value);
                for(int k=0;k<skl::info(processor.slotType(slot)).numKnobs;++k)
                    processor.apvts.getParameter(SkellierProcessor::knobId(slot,k))->setValueNotifyingHost(value);
            }
        }
    };
    options.inspect = [&] {
        juce::StringArray errors;
        if(!bindingChecks) errors.add("Live sprite slider/button parameter binding failed");
        for(int id=1;id<skl::ui::RackSpriteCount;++id) {
            const auto& sprite=skl::ui::rackSprite(id);
            if(!sprite.valid() || !sprite.endpointsDiffer()) errors.add("Packed sprite missing or invalid endpoint frames");
        }
        if (editor.getWidth() != expectedWidth || editor.getHeight() != expectedHeight)
            errors.add("Editor scaling dimensions do not match the scene");
        int visibleArtworkSlots = 0;
        int programOptoSlots = 0;
        visit(editor, [&](juce::Component& c) {
            if (auto* slot = dynamic_cast<skl::ui::SlotComponent*>(&c)) {
                if (slot->currentType() == skl::ProgramOpto) ++programOptoSlots;
                if (!slot->artworkLayoutIsValid()) errors.add("Live controls clipped, overlapping or skin unavailable");
                if (slot->currentType() == skl::Eq || slot->currentType() == skl::Air) {
                    ++visibleArtworkSlots;
                }
            }
        });
        if (visibleArtworkSlots != expectedArtworkSlots) errors.add("Unexpected artwork slot count");
        if (programOptoSlots != (expectProgramOpto ? 1 : 0)) errors.add("Unexpected Program Opto slot count");
        for (int type : {skl::Eq, skl::Air})
            for (int choice : {1, 2, 3}) {
                const auto& panel = skl::ui::rackPanel(type, choice);
                const int width = choice == 3 ? 216 : 215, height = choice == 3 ? 567 : 570;
                if (!panel.isValid() || panel.getWidth() != width || panel.getHeight() != height
                    || !skl::ui::rackMaterial(type, choice).isValid())
                    errors.add("Embedded artwork or material failed asset-specific decoding");
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
        const auto birdRaw = juce::ImageFileFormat::loadFrom(BinaryData::blackbird_faceplate_png,
            static_cast<size_t>(BinaryData::blackbird_faceplate_pngSize));
        const auto& bird = skl::ui::rackPanel(skl::Eq, 3);
        if (birdRaw.isValid() && bird.isValid()) {
            for (int offset : {0, 215, 216, 432, 119853, 122471})
                if (bird.getPixelAt(offset % 216, offset / 216) != birdRaw.getPixelAt(offset % 570, offset / 570))
                    errors.add("Blackbird packed row stride regression");
        } else errors.add("Blackbird source unavailable");
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

