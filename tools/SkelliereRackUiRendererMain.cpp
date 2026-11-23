// Capture Skelliere Rack's real JUCE editor, not a UI mockup or the SkellCore demo.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <SkellCore/HeadlessUIRenderer.h>
#include <JuceHeader.h>
#include <cstdio>

int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    SkellierProcessor processor;
    processor.prepareToPlay(48000.0, 512);
    SkellierEditor editor(processor);
    skellcore::ui::headless::Options options;
    options.project = "Skelliere Rack real editor";
    options.scenes = { { "rack_default_100", 1260, 820, 0 } };
    options.inspect = [&editor] {
        juce::StringArray errors;
        if (editor.getNumChildComponents() == 0)
            errors.add("Real rack editor has no child components");
        if (editor.getLocalBounds().getWidth() != 1260 || editor.getLocalBounds().getHeight() != 820)
            errors.add("Real rack editor does not match default dimensions");
        return errors;
    };
    const auto output = argc > 1
        ? juce::File(juce::String::fromUTF8(argv[1]))
        : juce::File::getCurrentWorkingDirectory().getChildFile("ui-screenshots");
    const auto result = skellcore::ui::headless::render(editor, output, options);
    for (const auto& error : result.errors)
        std::fprintf(stderr, "%s\n", error.toRawUTF8());
    std::printf("Captured %d real Skelliere Rack editor screenshots\n", result.captured);
    processor.releaseResources();
    return result.passed() && result.captured == 1 ? 0 : 1;
}
