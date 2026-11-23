cmake_minimum_required(VERSION 3.22)
project(SkellierRack VERSION 0.3.0 LANGUAGES C CXX)
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

# Installed plugins use embedded data, never build-directory paths or loose files.
set(SKELLIERE_ART_DIR "${CMAKE_CURRENT_BINARY_DIR}/rack-artwork")
file(MAKE_DIRECTORY "${SKELLIERE_ART_DIR}")
foreach(archive IN ITEMS air-eq-air-artwork.zip additional-effect-artwork.zip)
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/${archive}")
    message(FATAL_ERROR "Missing assets/${archive}")
  endif()
  file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_SOURCE_DIR}/assets/${archive}" DESTINATION "${SKELLIERE_ART_DIR}")
endforeach()

set(SKELLIERE_PANEL_FILES)
function(skelliere_panel relative_source simple_name)
  set(source "${SKELLIERE_ART_DIR}/${relative_source}")
  set(output "${SKELLIERE_ART_DIR}/${simple_name}.png")
  if(NOT EXISTS "${source}")
    message(FATAL_ERROR "Missing rack artwork: ${relative_source}")
  endif()
  file(COPY_FILE "${source}" "${output}" ONLY_IF_DIFFERENT)
  list(APPEND SKELLIERE_PANEL_FILES "${output}")
  set(SKELLIERE_PANEL_FILES "${SKELLIERE_PANEL_FILES}" PARENT_SCOPE)
endfunction()

skelliere_panel("AirEQAir/RESOURCES/AirEqAir Resources__16209186/CONVERTED_PNG/0e10d454_1x_570x215.png" air_lift_faceplate)
skelliere_panel("AirEQEarth/RESOURCES/AirEqEarth Resources__26b93620/CONVERTED_PNG/59981b3f_1x_570x215.png" earth_faceplate)
skelliere_panel("CS-EQ/RESOURCES/CS-EQ Resources__ad3070b8/CONVERTED_PNG/10c608ac_1x_570x215.png" classic_eq_faceplate)
skelliere_panel("CS-Lift/RESOURCES/CS-Lift Resources__ca15dd88/CONVERTED_PNG/195ddaf3_1x_570x215.png" classic_lift_faceplate)
skelliere_panel("Blackbird Studio/RESOURCES/VMC Classic Tubes Resources__211e6581/CONVERTED_PNG/34294264_1x_570x215.png" blackbird_faceplate)
juce_add_binary_data(SkelliereArtwork SOURCES ${SKELLIERE_PANEL_FILES})

target_sources(SkellierRack PRIVATE
  Source/Modules.h Source/Dsp.h Source/RackModuleFactory.h
  Source/PluginProcessor.cpp Source/PluginProcessor.h
  Source/Look.h Source/RackArtwork.h
  Source/PluginEditor.cpp Source/PluginEditor.h)

target_compile_features(SkellierRack PRIVATE cxx_std_17)
target_compile_definitions(SkellierRack PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0)
target_compile_definitions(SkellierRack PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(SkellierRack PRIVATE
  juce::juce_audio_utils juce::juce_audio_processors juce::juce_gui_extra
  SkellCore::DSP SkelliereArtwork)

option(SKELLIERE_BUILD_UI_RENDERER "Build real Skelliere Rack JUCE editor screenshots" OFF)
if(SKELLIERE_BUILD_UI_RENDERER)
  add_executable(SkelliereRackUIRenderer tools/SkelliereRackUiRendererMain.cpp)
  target_include_directories(SkelliereRackUIRenderer PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/SkellierRack_artefacts/JuceLibraryCode")
  # The renderer includes the plugin's generated JuceHeader.h, which itself
  # includes BinaryData.h. Propagate the binary-data include directory explicitly.
  target_link_libraries(SkelliereRackUIRenderer PRIVATE
    SkellierRack SkelliereArtwork SkellCore::UIRenderer SkellCore::DSP
    juce::juce_audio_utils juce::juce_gui_extra)
  target_compile_definitions(SkelliereRackUIRenderer PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  target_compile_features(SkelliereRackUIRenderer PRIVATE cxx_std_17)
endif()

