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
    if(!skl::ui::necromancerArtworkReady()){std::fprintf(stderr,"Missing original Necromancer hero-dial artwork\n");return 1;}
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
    skellcore::ui::headless::Options options;
    options.project="Skelliere Rack / "+juce::String(skl::kNumRackTypes-1)+" Skell effects";
    options.scenes={{"catalog_1",1260,790,0,1},{"catalog_2",1260,790,1,1},{"catalog_3",1260,790,2,1},{"catalog_4",1260,790,3,1},{"catalog_5",1260,790,4,1},{"catalog_6",1260,790,5,1},{"catalog_7",1260,790,6,1},
      {"catalog_80",1008,632,0,1},{"catalog_125",1575,988,3,1},{"catalog_150",1890,1185,4,1},{"catalog_min",1260,790,6,1},{"catalog_max",1260,790,6,1},{"catalog_silver",1260,790,0,2},{"catalog_empty",1260,790,7,1},{"approved_studio",1260,790,8,1},{"approved_studio_150",1890,1185,8,1},{"approved_bypass",1260,790,9,1},{"studio_graphite",1260,790,8,0},{"studio_midnight",1260,790,8,3},{"studio_80",1008,632,8,1},{"full_rack",1260,790,10,1},{"studio_edit",1260,790,8,1},{"library_hidden",1260,790,10,1},{"library_hidden_edit",1260,790,10,1},{"library_hidden_80",1008,632,10,1},{"library_hidden_150",1890,1185,10,1},{"library_restored",1260,790,8,1},{"rack_last_slots",1260,790,10,1},{"rack_last_hidden",1260,790,10,1}};
    options.mustDiffer={{"catalog_min","catalog_max"},{"catalog_1","catalog_silver"},{"approved_studio","approved_bypass"},{"studio_graphite","studio_midnight"},{"approved_studio","studio_edit"},{"full_rack","library_hidden"},{"library_hidden","library_hidden_edit"},{"full_rack","rack_last_slots"},{"library_hidden","rack_last_hidden"}};
    int expectedWidth=1260,expectedHeight=790,expectedCount=4;bool bindings=true,libraryOK=true,expectedLibrary=true,navOK=true,expectedLast=false;
    options.configure=[&](const skellcore::ui::headless::Scene& scene){
        expectedWidth=scene.width;expectedHeight=scene.height;expectedCount=0;bindings=true;libraryOK=true;navOK=true;processor.clearRack();
        expectedLast=scene.id=="rack_last_slots"||scene.id=="rack_last_hidden";
        expectedLibrary=!(scene.id=="library_hidden"||scene.id=="library_hidden_edit"||scene.id=="library_hidden_80"||scene.id=="library_hidden_150"||scene.id=="rack_last_hidden");
        const bool edit=scene.id=="studio_edit"||scene.id=="library_hidden_edit";
        if(scene.theme<7)for(int i=0;i<4;++i){const int type=1+scene.theme*4+i;if(type<skl::kNumRackTypes){processor.addModule(type,i);++expectedCount;}}
        if(scene.theme==8||scene.theme==9){
            const int models[]={skl::Comp,skl::Bell,skl::Delay,skl::Flanger};
            for(int k=0;k<4;++k)processor.addModule(models[k],k);
            expectedCount=4;
            if(scene.theme==9)processor.apvts.getParameter(SkellierProcessor::onId(0))->setValueNotifyingHost(0);
        }
        if(scene.theme==10){const int models[]={skl::Comp,skl::Bell,skl::Delay,skl::Flanger,skl::SkellAirFX,skl::SkellDeEsserFX,skl::Gate,skl::Trim};
            for(int k=0;k<8;++k)processor.addModule(models[k],k);expectedCount=8;}
        visit(editor,[&](juce::Component& c){
            if(auto* ui=dynamic_cast<skl::ui::RackUI*>(&c)){
                ui->setEditMode(edit);
                ui->setLibraryVisible(true);
                auto* toggle=dynamic_cast<juce::TextButton*>(ui->findChildWithID("rack.library.toggle"));
                if(toggle&&toggle->onClick){
                    toggle->onClick();libraryOK=!ui->isLibraryVisible()&&ui->libraryLayoutIsValid()&&libraryOK;
                    toggle->onClick();libraryOK=ui->isLibraryVisible()&&ui->libraryLayoutIsValid()&&libraryOK;
                    if(!expectedLibrary)toggle->onClick();
                }else libraryOK=false;
                if(ui->onScale)ui->onScale(float(scene.width)/1260.f);
            }
            if(auto* slot=dynamic_cast<skl::ui::SlotComponent*>(&c)){slot->rebuild();slot->setArtworkChoice(scene.mode);bindings=slot->exerciseSpriteBindings()&&bindings;}
            if(auto* vp=dynamic_cast<juce::Viewport*>(&c))if(dynamic_cast<skl::ui::RackStrip*>(vp->getViewedComponent()))vp->setViewPosition(0,0);
        });
        visit(editor,[&](juce::Component& c){if(auto* ui=dynamic_cast<skl::ui::RackUI*>(&c)){
            ui->revealSlot(7);navOK=ui->slotIsFullyVisible(7)&&navOK;
            ui->revealSlot(0);navOK=ui->slotIsFullyVisible(0)&&navOK;
            if(expectedLast)ui->revealSlot(7);
        }});
        // Check all 27 models in both layouts, then restore requested screenshot mode.
        visit(editor,[&](juce::Component& c){if(auto* slot=dynamic_cast<skl::ui::SlotComponent*>(&c)){slot->setEditMode(true);bindings=slot->artworkLayoutIsValid()&&bindings;slot->setEditMode(false);bindings=slot->artworkLayoutIsValid()&&bindings;slot->setEditMode(edit);}});
        if(scene.id=="catalog_min"||scene.id=="catalog_max")for(int s=0;s<expectedCount;++s)
            for(int k=0;k<skl::info(processor.slotType(s)).numKnobs;++k)
                processor.apvts.getParameter(SkellierProcessor::knobId(s,k))->setValueNotifyingHost(scene.id=="catalog_max"?1.f:0.f);
        juce::AudioBuffer<float> signal(2,512);juce::MidiBuffer midi;
        for(int b=0;b<64;++b){for(int c=0;c<2;++c)for(int n=0;n<512;++n)signal.setSample(c,n,.65f*std::sin(float((b*512+n)*.057595865)));processor.processBlock(signal,midi);}
        // Use the production timer update so I/O and GR are measured rather than painted as fake data.
        visit(editor,[&](juce::Component& c){if(auto* ui=dynamic_cast<skl::ui::RackUI*>(&c))ui->refreshLiveState();});
    };
    options.inspect=[&]{
        juce::StringArray errors;if(!bindings)errors.add("Live parameter binding failed");
        if(!libraryOK)errors.add("Library toggle round-trip failed");
        if(!navOK)errors.add("Rack navigation round-trip failed");
        visit(editor,[&](juce::Component& c){if(auto* ui=dynamic_cast<skl::ui::RackUI*>(&c))if(!ui->slotIsFullyVisible(expectedLast?7:0))errors.add("Rack navigation scene wrong");});
        visit(editor,[&](juce::Component& c){if(auto* ui=dynamic_cast<skl::ui::RackUI*>(&c))if(ui->isLibraryVisible()!=expectedLibrary||!ui->libraryLayoutIsValid())errors.add("Library layout mismatch");});
        if(editor.getWidth()!=expectedWidth||editor.getHeight()!=expectedHeight)errors.add("Editor scaling mismatch");
        int count=0;visit(editor,[&](juce::Component& c){if(auto* slot=dynamic_cast<skl::ui::SlotComponent*>(&c)){if(slot->currentType()!=skl::Empty)++count;if(!slot->artworkLayoutIsValid())errors.add("Original faceplate layout invalid");}});
        if(count!=expectedCount)errors.add("Unexpected slot count");return errors;
    };
    const auto output = argc > 1 ? juce::File(juce::String::fromUTF8(argv[1]))
        : juce::File::getCurrentWorkingDirectory().getChildFile("ui-screenshots");
    const auto result = skellcore::ui::headless::render(editor, output, options);
    for (const auto& error : result.errors) std::fprintf(stderr, "%s\n", error.toRawUTF8());
    std::printf("Captured %d real Skelliere Rack editor screenshots\n", result.captured);
    // Same exact production editor and processor, a warmed eight-module rack.
    processor.clearRack();
    const int types[]={skl::Eq,skl::Gate,skl::Transient,skl::Bell,skl::SoftClip,skl::Phaser,skl::Flanger,skl::Slap};
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
    perf<<"original_catalog,"<<skl::kNumRackTypes-1<<",count\n";
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
#if JUCE_LINUX
    std::ifstream procStatus("/proc/self/status");
    std::string line;
    while(std::getline(procStatus,line)) {
        if(line.rfind("VmRSS:",0)==0 || line.rfind("VmHWM:",0)==0) {
            const auto kb=std::stoull(line.substr(6));
            perf<<(line.rfind("VmRSS:",0)==0?"process_working_set,":"process_peak_working_set,")<<kb*1024<<",bytes\n";
        }
    }
#endif
    perf.close();
    if(!perf)return 1;
    processor.releaseResources();
    return result.passed() && result.captured == static_cast<int>(options.scenes.size()) ? 0 : 1;
}

