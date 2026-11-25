#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>
namespace skl::ui {
// Optional local artwork is never fetched, scanned from project state, or
// loaded on the audio thread. All resource names are compile-time identifiers.
inline const juce::File& localArtworkDirectory() {
    // Resolve once. Restart the process after changing the installed pack.
    static const auto dir=[] {
        const auto overridePath=juce::SystemStats::getEnvironmentVariable("SKELLIERE_ARTWORK_DIR",{});
        if(overridePath.isNotEmpty()) return juce::File(overridePath);
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Skell Audio").getChildFile("Skelliere Rack").getChildFile("artwork-v1");
    }();
    return dir;
}
inline juce::Image loadRackResource(const char* name) {
    const auto f=localArtworkDirectory().getChildFile(juce::String(name)+".png");
    if(f.existsAsFile() && f.getSize()>0 && f.getSize()<=64*1024*1024) {
        auto im=juce::ImageFileFormat::loadFrom(f);
        if(im.isValid()) return im;
    }
    int size=0;
    const auto symbol=juce::String(name)+"_png";
    const auto* bytes=BinaryData::getNamedResource(symbol.toRawUTF8(),size);
    return bytes && size>0?juce::ImageFileFormat::loadFrom(bytes,static_cast<size_t>(size)):juce::Image{};
}
inline bool hasRackResource(const char* name) {
    if(localArtworkDirectory().getChildFile(juce::String(name)+".png").existsAsFile()) return true;
    int size=0;
    const auto symbol=juce::String(name)+"_png";
    return BinaryData::getNamedResource(symbol.toRawUTF8(),size)!=nullptr && size>0;
}
}
