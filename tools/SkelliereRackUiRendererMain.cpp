// Screenshots and checks use the real plugin editor, not a reconstruction.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <SkellCore/HeadlessUIRenderer.h>
#include <cstdio>
#include <chrono>
#include <fstream>
#if JUCE_WINDOWS
#include <windows.h>
#include <psapi.h>
#endif
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
    using Clock=std::chrono::steady_clock;
    auto ms=[](Clock::time_point t){return std::chrono::duration<double,std::milli>(Clock::now()-t).count();};
    const auto startEditor=Clock::now();
    SkellierEditor editor(processor);
    const auto editorMs=ms(startEditor);
    const bool fullPack=skl::ui::rackSkinAvailable(skl::Comp,1)&&skl::ui::rackSkinAvailable(skl::ProgramOpto,1)
        &&skl::ui::rackSkinAvailable(skl::FastComp,1)&&skl::ui::rackSkinAvailable(skl::FastComp,2)
        &&skl::ui::rackSkinAvailable(skl::FastComp,3)&&skl::ui::rackSkinAvailable(skl::BusComp,1)
        &&skl::ui::rackSkinAvailable(skl::Trim,2);
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
        {"rack_native_150", 1890, 1230, 0, 1},
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
    if(fullPack) {
    options.scenes.push_back({"rack_full_opto_bus",1260,820,6,1});
    options.scenes.push_back({"rack_full_fet_classic",1260,820,7,1});
    options.scenes.push_back({"rack_full_fet_modern",1260,820,7,2});
    options.scenes.push_back({"rack_full_fet_vintage",1260,820,7,3});
    options.scenes.push_back({"rack_full_instrument_fader",1260,820,5,2});
    }
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
        if(scene.theme==4 || scene.theme==6 || scene.theme==7) {
            for(int i=0;i<skl::kNumSlots;++i)if(skl::info(processor.slotType(i)).hasVU) {
                processor.apvts.getParameter(SkellierProcessor::onId(i))->setValueNotifyingHost(1);
                processor.apvts.getParameter(SkellierProcessor::knobId(i,0))->setValueNotifyingHost(.25f);
            }
            juce::AudioBuffer<float> signal(2,512);juce::MidiBuffer midi;
            for(int block=0;block<64;++block) {
                for(int c=0;c<2;++c)for(int n=0;n<512;++n)
                    signal.setSample(c,n,.75f*std::sin(float((block*512+n)*.057595865)));
                processor.processBlock(signal,midi);
            }
            int slotIndex=0;
            visit(editor,[&](juce::Component& c){
                if(auto* slot=dynamic_cast<skl::ui::SlotComponent*>(&c))slot->setMeter(processor.gainReduction(slotIndex++));
            });
        }
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
            if((id<=skl::ui::BlackbirdFader || fullPack) && (!sprite.valid() || !sprite.endpointsDiffer()))
                errors.add("Packed sprite missing or invalid endpoint frames");
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
                const int width=panel.getWidth(),height=panel.getHeight();
                if (!panel.isValid() || !((width==215 && height==(choice==3?567:570)) ||
                    (width==430 && height==(choice==3?1138:1140)))
                    || !skl::ui::rackMaterial(type, choice).isValid())
                    errors.add("Embedded artwork or material failed asset-specific decoding");
            }
        // Catch a future rotate/transpose regression with scanline sample positions.
        const auto raw = juce::ImageFileFormat::loadFrom(BinaryData::air_lift_faceplate_png,
            static_cast<size_t>(BinaryData::air_lift_faceplate_pngSize));
        const auto& portrait = skl::ui::rackPanel(skl::Air, 1);
        if (raw.isValid() && portrait.isValid()) {
            if(portrait.getWidth()==215) for (int offset : {0, 214, 215, 569, 570, 119853})
                if (portrait.getPixelAt(offset % 215, offset / 215) != raw.getPixelAt(offset % 570, offset / 570))
                    errors.add("Packed artwork scanlines were rotated or transposed");
        } else errors.add("Artwork source could not be decoded");
        const auto birdRaw = juce::ImageFileFormat::loadFrom(BinaryData::blackbird_faceplate_png,
            static_cast<size_t>(BinaryData::blackbird_faceplate_pngSize));
        const auto& bird = skl::ui::rackPanel(skl::Eq, 3);
        if (birdRaw.isValid() && bird.isValid()) {
            if(bird.getWidth()==215) for(int y:{0,1,2,200,566})for(int x:{0,100,214}) {
                const int offset=y*216+x;
                if(bird.getPixelAt(x,y)!=birdRaw.getPixelAt(offset%570,offset/570))
                    errors.add("Blackbird padded RGB scanline regression");
            }
        } else errors.add("Blackbird source unavailable");
        for (int type : {skl::Eq, skl::Air}) for(int choice : {1,2,3}) {
            const auto& raw=skl::ui::rackPanel(type,choice);
            const auto& restored=skl::ui::restoredRackPanel(type,choice);
            if(!raw.isValid() || !restored.isValid() || raw.getBounds()!=restored.getBounds())
                errors.add("Restored full panel is invalid");
            else {
                // Blackbird preserves its upper blank chamber by design.
                const int y=choice==3 ? raw.getHeight()*4/5 : raw.getHeight()/2;
                if(raw.getPixelAt(107,y)==restored.getPixelAt(107,y))
                    errors.add("Faceplate restoration did not remove baked background controls");
            }
            /*
                errors.add("This block is replaced by the checked source-region test");
            */
        }
        if(fullPack) for(const auto& pair:std::array<std::array<int,2>,7>{{{{skl::Comp,1}},{{skl::ProgramOpto,1}},{{skl::FastComp,1}},{{skl::FastComp,2}},{{skl::FastComp,3}},{{skl::BusComp,1}},{{skl::Trim,2}}}}) {
            if(!skl::ui::rackSkinAvailable(pair[0],pair[1])) errors.add("Full-pack native appearance missing");
        }
        return errors;
    };
    const auto output = argc > 1 ? juce::File(juce::String::fromUTF8(argv[1]))
        : juce::File::getCurrentWorkingDirectory().getChildFile("ui-screenshots");
    const auto result = skellcore::ui::headless::render(editor, output, options);
    for (const auto& error : result.errors) std::fprintf(stderr, "%s\n", error.toRawUTF8());
    std::printf("Captured %d real Skelliere Rack editor screenshots\n", result.captured);
    // Same exact production editor and processor, a warmed eight-module rack.
    processor.clearRack();
    const int types[]={skl::Eq,skl::Air,skl::Comp,skl::FastComp,skl::BusComp,skl::ProgramOpto,skl::Trim,skl::Reverb};
    for(int i=0;i<8;++i)processor.addModule(types[i],i);
    const auto setupStart=Clock::now();
    visit(editor,[&](juce::Component& c){
        if(auto* ui=dynamic_cast<skl::ui::RackUI*>(&c))if(ui->onScale)ui->onScale(1.0f);
        if(auto* slot=dynamic_cast<skl::ui::SlotComponent*>(&c)){slot->rebuild();slot->setArtworkChoice(1);}
    });
    const auto setupMs=ms(setupStart);
    std::vector<double> paint,audio;
    juce::AudioBuffer<float> signal(2,512);juce::MidiBuffer midi;
    for(int i=0;i<32;++i) {
        const auto t=Clock::now();
        const auto image=editor.createComponentSnapshot(editor.getLocalBounds(),true,1.0f);
        if(!image.isValid())return 1;
        if(i>1)paint.push_back(ms(t));
    }
    for(int i=0;i<256;++i) {
        for(int c=0;c<2;++c)for(int n=0;n<512;++n)signal.setSample(c,n,.2f*std::sin(float((i*512+n)*.057595865)));
        const auto t=Clock::now();processor.processBlock(signal,midi);
        if(i>=16)audio.push_back(ms(t));
    }
    auto percentile=[](std::vector<double> v,double q){std::sort(v.begin(),v.end());return v[static_cast<size_t>(q*(v.size()-1))];};
    std::ofstream perf(output.getChildFile("performance.csv").getFullPathName().toStdString());
    perf<<"metric,value,unit\n";
    perf<<"editor_initial_construct,"<<editorMs<<",ms\n";
    perf<<"warm_eight_module_setup,"<<setupMs<<",ms\n";
    perf<<"snapshot_p50,"<<percentile(paint,.5)<<",ms\n";
    perf<<"snapshot_p95,"<<percentile(paint,.95)<<",ms\n";
    perf<<"processblock_p50,"<<percentile(audio,.5)<<",ms\n";
    perf<<"processblock_p95,"<<percentile(audio,.95)<<",ms\n";
    perf<<"processblock_budget_percent_p95,"<<percentile(audio,.95)*48000/512/10<<",percent\n";
    perf<<"full_pack,"<<fullPack<<",bool\n";
    perf<<"sample_rate,48000,Hz\nblock_size,512,samples\nmodule_count,8,count\n";
    perf<<"cpu_count,"<<juce::SystemStats::getNumCpus()<<",count\n";
    perf<<"cpu_model,\""<<juce::SystemStats::getCpuModel().toStdString()<<"\",text\n";
#if JUCE_WINDOWS
    PROCESS_MEMORY_COUNTERS_EX mem{};
    if(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&mem),sizeof(mem))) {
        perf<<"process_working_set,"<<mem.WorkingSetSize<<",bytes\n";
        perf<<"process_private_bytes,"<<mem.PrivateUsage<<",bytes\n";
    }
#endif
    perf.close();
    if(!perf)return 1;
    processor.releaseResources();
    return result.passed() && result.captured == static_cast<int>(options.scenes.size()) ? 0 : 1;
}

