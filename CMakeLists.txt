cmake_minimum_required(VERSION 3.22)
project(SkellierRack VERSION 0.1.0 LANGUAGES C CXX)
include(FetchContent)

# JUCE: pass -DJUCE_PATH=/path/to/JUCE to use a local copy, otherwise JUCE 8.0.12 is fetched.
if(DEFINED JUCE_PATH AND EXISTS "${JUCE_PATH}/CMakeLists.txt")
  add_subdirectory("${JUCE_PATH}" JUCE)
else()
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.12 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
endif()

add_subdirectory(SkellCore)

set(SKELLIERE_FORMATS VST3 Standalone)
if(APPLE)
  list(APPEND SKELLIERE_FORMATS AU)
endif()

juce_add_plugin(SkellierRack
  COMPANY_NAME "Skell Audio"
  BUNDLE_ID "audio.skell.skelliere-rack"
  PLUGIN_MANUFACTURER_CODE SkAu
  PLUGIN_CODE SkRk
  FORMATS ${SKELLIERE_FORMATS}
  PRODUCT_NAME "Skelliere Rack"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(SkellierRack)

target_sources(SkellierRack PRIVATE
  Source/Modules.h Source/Dsp.h
  Source/PluginProcessor.cpp Source/PluginProcessor.h
  Source/Look.h
  Source/PluginEditor.cpp Source/PluginEditor.h)

target_compile_features(SkellierRack PRIVATE cxx_std_17)
target_compile_definitions(SkellierRack PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0)
target_compile_definitions(SkellierRack PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(SkellierRack PRIVATE
  juce::juce_audio_utils juce::juce_audio_processors juce::juce_gui_extra
  SkellCore::DSP)


# Capture the actual Skelliere Rack editor in CI; never substitute the SkellCore demo.
option(SKELLIERE_BUILD_UI_RENDERER "Build real Skelliere Rack JUCE editor screenshots" OFF)
if(SKELLIERE_BUILD_UI_RENDERER)
  add_executable(SkelliereRackUIRenderer tools/SkelliereRackUiRendererMain.cpp)
  target_include_directories(SkelliereRackUIRenderer PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/SkellierRack_artefacts/JuceLibraryCode")
  target_link_libraries(SkelliereRackUIRenderer PRIVATE
    SkellierRack SkellCore::UIRenderer SkellCore::DSP
    juce::juce_audio_utils juce::juce_gui_extra)
  target_compile_definitions(SkelliereRackUIRenderer PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  target_compile_features(SkelliereRackUIRenderer PRIVATE cxx_std_17)
endif()
