// Screenshots and checks use the real plugin editor, not a reconstruction.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

#include <SkellCore/HeadlessUIRenderer.h>
#include <cstdio>
#include <chrono>
#include <fstream>
#include "RackCaptureArgs.h"
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
    const auto args=rackcapture::parse(std::vector<std::string>(argv+1,argv+argc));
    if(!args.error.empty()){std::fprintf(stderr,"%s\n",args.error.c_str());return 2;}
    juce::ScopedJuceInitialiser_GUI gui;
    {
        juce::Image canvas(juce::Image::RGB,190,100,true,juce::SoftwareImageType());
        juce::Graphics g(canvas);g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::black.withAlpha(.12f));
        skl::ui::paintLampGlass(g,canvas.getBounds().toFloat());
        const auto lit=canvas.getPixelAt(95,56);
        if(lit.getRed()<190||lit.getGreen()<140||lit.getBlue()<80){std::fprintf(stderr,"Lamp lens raster is unexpectedly dim\n");return 1;}
    }
    {
        const auto& art=skl::ui::referenceTapeArtwork();
        if(!art.isValid()||art.getWidth()!=250||art.getHeight()!=142){std::fprintf(stderr,"Reference tape art decode failed\n");return 1;}
        juce::Image canvas(juce::Image::RGB,206,120,true,juce::SoftwareImageType());
        juce::Graphics g(canvas);g.fillAll(juce::Colours::black);g.setColour(juce::Colours::black.withAlpha(.1f));
        skl::ui::paintTapeDetail(g,canvas.getBounds().toFloat(),true);
        if(canvas.getPixelAt(30,40).getRed()<85){std::fprintf(stderr,"Reference tape art unexpectedly dim\n");return 1;}
    }
    if(!skl::ui::necromancerArtworkReady()||!skl::ui::litNecroCap().isValid()||skl::ui::litNecroCap().getWidth()!=256){std::fprintf(stderr,"Missing original Necromancer hero-dial artwork\n");return 1;}
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
      {"catalog_80",1008,632,0,1},{"catalog_125",1575,988,3,1},{"catalog_150",1890,1185,4,1},{"catalog_min",1260,790,6,1},{"catalog_max",1260,790,6,1},{"catalog_silver",1260,790,0,2},{"catalog_empty",1260,790,7,1},{"approved_studio",1260,790,8,1},{"approved_studio_150",1890,1185,8,1},{"approved_bypass",1260,790,9,1},{"studio_graphite",1260,790,8,0},{"studio_midnight",1260,790,8,3},{"studio_80",1008,632,8,1},{"full_rack",1260,790,10,1},{"studio_edit",1260,790,8,1},{"library_hidden",1260,790,10,1},{"library_hidden_edit",1260,790,10,1},{"library_hidden_80",1008,632,10,1},{"library_hidden_150",1890,1185,10,1},{"library_restored",1260,790,8,1},{"rack_last_slots",1260,790,10,1},{"rack_last_hidden",1260,790,10,1},{"color_modules",1260,790,11,1},{"color_modules_edit",1260,790,11,1},{"color_modules_80",1008,632,11,1},{"color_modules_150",1890,1185,11,1}};
    options.mustDiffer={{"catalog_min","catalog_max"},{"catalog_1","catalog_silver"},{"approved_studio","approved_bypass"},{"studio_graphite","studio_midnight"},{"approved_studio","studio_edit"},{"full_rack","library_hidden"},{"library_hidden","library_hidden_edit"},{"full_rack","rack_last_slots"},{"library_hidden","rack_last_hidden"}};
    if(!args.full){
        options.scenes={{"approved_studio",1260,790,8,1},{"studio_edit",1260,790,8,1},{"library_hidden",1260,790,10,1},{"studio_midnight",1260,790,8,3}};
        options.mustDiffer={{"approved_studio","studio_edit"},{"approved_studio","studio_midnight"}};
    }
    if(args.colors){options.scenes={{"color_modules",1260,790,11,1},{"color_modules_edit",1260,790,11,1},{"color_modules_80",1008,632,11,1},{"color_modules_150",1890,1185,11,1}};options.mustDiffer={{"color_modules","color_modules_edit"}};}
    int expectedWidth=1260,expectedHeight=790,expectedCount=4;bool bindings=true,libraryOK=true,expectedLibrary=true,navOK=true,expectedLast=false;
    options.configure=[&](const skellcore::ui::headless::Scene& scene){
        expectedWidth=scene.width;expectedHeight=scene.height;expectedCount=0;bindings=true;libraryOK=true;navOK=true;processor.clearRack();
        expectedLast=scene.id=="rack_last_slots"||scene.id=="rack_last_hidden";
        expectedLibrary=!(scene.id=="library_hidden"||scene.id=="library_hidden_edit"||scene.id=="library_hidden_80"||scene.id=="library_hidden_150"||scene.id=="rack_last_hidden");
        const bool edit=scene.id=="studio_edit"||scene.id=="library_hidden_edit"||scene.id=="color_modules_edit";
        if(scene.theme<7)for(int i=0;i<4;++i){const int type=1+scene.theme*4+i;if(type<skl::kNumRackTypes){processor.addModule(type,i);++expectedCount;}}
        if(scene.theme==11){const int models[]={skl::SkellDope,skl::SkellMixHead,skl::SkellRubyQ};
            for(int k=0;k<3;++k)processor.addModule(models[k],k);expectedCount=3;}
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
            // Visit every slot, not only the ends. Each navigation step must preserve
            // the exact whole-module fit and keep its target fully visible.
            for(int i=0;i<skl::kNumSlots;++i){ui->revealSlot(i);navOK=ui->slotIsFullyVisible(i)&&ui->fittedRackIsValid()&&navOK;}
            for(int i=skl::kNumSlots-1;i>=0;--i){ui->revealSlot(i);navOK=ui->slotIsFullyVisible(i)&&ui->fittedRackIsValid()&&navOK;}
            if(expectedLast)ui->revealSlot(7);
        }});
        // Check the current models in both layouts, then restore requested screenshot mode.
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
    const auto output=juce::File::getCurrentWorkingDirectory().getChildFile(juce::String(args.output));
    options.configure({"approved_studio",1260,790,8,1});
    skl::ui::SlotComponent* tape=nullptr;
    visit(editor,[&](juce::Component& c){if(auto* slot=dynamic_cast<skl::ui::SlotComponent*>(&c))if(slot->currentType()==skl::Delay)tape=slot;});
    if(!tape)return 1;
    const auto detail=skl::ui::originalLayout(skl::Delay,tape->getWidth(),false).detail;
    const auto motionOutput=output.getChildFile("motion");motionOutput.createDirectory();
    auto snap=[&](int frame){auto im=tape->createComponentSnapshot(detail,true,2.f);auto file=motionOutput.getChildFile("tape-motion-"+juce::String(frame)+".png");file.deleteFile();juce::FileOutputStream stream(file);juce::PNGImageFormat png;if(!stream.openedOk()||!png.writeImageToStream(im,stream))return juce::Image{};return im;};
    auto first=snap(0);if(!first.isValid())return 1;
    for(int i=1;i<=24;++i){tape->advanceArtworkMotion(.05f,true,true);if(!snap(i).isValid())return 1;}
    auto before=snap(25);tape->advanceArtworkMotion(.1f,true,false);auto paused=snap(26);
    tape->advanceArtworkMotion(.1f,false,true);auto hidden=snap(27);
    int changed=0,unstable=0;
    for(int y=0;y<before.getHeight();++y)for(int x=0;x<before.getWidth();++x){
        const auto b=before.getPixelAt(x,y);if(b!=first.getPixelAt(x,y))++changed;
        if(b!=paused.getPixelAt(x,y)||b!=hidden.getPixelAt(x,y))++unstable;
    }
    if(changed<1000||unstable!=0||first.getPixelAt(3,3)!=before.getPixelAt(3,3)){std::fprintf(stderr,"Tape motion regression: %d changed, %d unstable\n",changed,unstable);return 1;}
    std::printf("Tape motion: %d changed pixels, pause and offscreen freeze passed\n",changed);
    // Real native frames from every animated module; exact pause equality.
    const int animated[]={skl::Air,skl::SkellAirFX,skl::Chorus,skl::Reverb,skl::Sat,skl::SoftClip,skl::Eq,skl::Tremolo,skl::Phaser,skl::Stereo,skl::Transient,skl::Slap,skl::DeEss,skl::SkellDeEsserFX};
    for(int type:animated){
        processor.clearRack();processor.addModule(type,0);
        skl::ui::SlotComponent* target=nullptr;
        visit(editor,[&](juce::Component& c){if(auto* slot=dynamic_cast<skl::ui::SlotComponent*>(&c)){slot->rebuild();slot->setArtworkChoice(1);if(slot->currentType()==type)target=slot;}});
        if(!target)return 1;
        const auto area=target->artworkMotionBounds();if(area.isEmpty())return 1;
        auto capture=[&](int i){auto im=target->createComponentSnapshot(area,true,2.f);auto f=motionOutput.getChildFile("module-"+juce::String(type)+"-"+juce::String(i)+".png");f.deleteFile();juce::FileOutputStream stream(f);juce::PNGImageFormat png;if(!im.isValid()||!stream.openedOk()||!png.writeImageToStream(im,stream))return juce::Image{};return im;};
        auto a=capture(0);if(!a.isValid())return 1;
        const int frames=args.full?47:12;
        for(int i=1;i<=frames;++i){target->advanceArtworkMotion(.1f,true,true,.75f);if((args.full||i==frames)&&!capture(i).isValid())return 1;}
        auto b=target->createComponentSnapshot(area,true,2.f);
        target->advanceArtworkMotion(.1f,false,true,0.f);target->advanceArtworkMotion(.1f,true,false,0.f);
        auto c=target->createComponentSnapshot(area,true,2.f);
        int delta=0,unstable=0;
        for(int y=0;y<a.getHeight();++y)for(int x=0;x<a.getWidth();++x){delta+=a.getPixelAt(x,y)!=b.getPixelAt(x,y);unstable+=b.getPixelAt(x,y)!=c.getPixelAt(x,y);}
        if(delta<20||unstable){std::fprintf(stderr,"Artwork motion %d failed: %d changed, %d unstable\n",type,delta,unstable);return 1;}
        std::printf("Artwork motion %d: %d changed, pause passed\n",type,delta);
    }
    const auto result = skellcore::ui::headless::render(editor, output, options);
    for (const auto& error : result.errors) std::fprintf(stderr, "%s\n", error.toRawUTF8());
    std::printf("Captured %d real Skelliere Rack editor screenshots\n", result.captured);
    if(!args.full){
        if(!result.passed()||result.captured!=4)return 1;
        juce::Image sheet(juce::Image::RGB,1260,790,true);juce::Graphics g(sheet);
        int i=0;for(const auto& scene:options.scenes){
            const auto im=juce::ImageFileFormat::loadFrom(output.getChildFile(juce::String(scene.id)+".png"));
            if(!im.isValid())return 1;
            g.drawImage(im,juce::Rectangle<float>(float((i%2)*630),float((i/2)*395),630.f,395.f));++i;
        }
        auto file=output.getChildFile("contact-sheet.png");file.deleteFile();juce::FileOutputStream stream(file);juce::PNGImageFormat png;
        if(!stream.openedOk()||!png.writeImageToStream(sheet,stream))return 1;
        processor.releaseResources();return 0;
    }
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

