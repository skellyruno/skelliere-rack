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
# Keep the complete user-supplied packs local. Public CI uses the previously
# embedded originals only; a private build can select the full set and 2x art.
set(SKELLIERE_FULL_ASSET_DIR "" CACHE PATH "Local directory containing both EFFECTS_FULL_PART ZIPs")
set(SKELLIERE_FULL_ARTWORK 0)
if(SKELLIERE_FULL_ASSET_DIR)
  foreach(archive IN ITEMS EFFECTS_FULL_PART_01.zip EFFECTS_FULL_PART_02.zip)
    if(NOT EXISTS "${SKELLIERE_FULL_ASSET_DIR}/${archive}")
      message(FATAL_ERROR "Missing local artwork pack: ${archive}")
    endif()
    file(ARCHIVE_EXTRACT INPUT "${SKELLIERE_FULL_ASSET_DIR}/${archive}"
      DESTINATION "${SKELLIERE_ART_DIR}/full")
  endforeach()
  set(SKELLIERE_FULL_ARTWORK 1)
endif()

function(skelliere_sprite module resource simple_name)
  if(SKELLIERE_FULL_ARTWORK)
    file(GLOB_RECURSE choices "${SKELLIERE_ART_DIR}/full/MODULES/${module}/${resource}_2x_*.png")
    if(NOT choices)
      file(GLOB_RECURSE choices "${SKELLIERE_ART_DIR}/full/MODULES/${module}/${resource}_1x_*.png")
    endif()
  else()
    file(GLOB_RECURSE choices "${SKELLIERE_ART_DIR}/${module}/${resource}_1x_*.png")
  endif()
  list(LENGTH choices count)
  if(NOT count EQUAL 1)
    message(FATAL_ERROR "Expected one exact sprite ${module}/${resource}, found ${count}")
  endif()
  list(GET choices 0 source)
  set(output "${SKELLIERE_ART_DIR}/${simple_name}.png")
  file(COPY_FILE "${source}" "${output}" ONLY_IF_DIFFERENT)
  list(APPEND SKELLIERE_PANEL_FILES "${output}")
  set(SKELLIERE_PANEL_FILES "${SKELLIERE_PANEL_FILES}" PARENT_SCOPE)
endfunction()
skelliere_sprite("AirEQAir" 1d481ea2 air_knob_large)
skelliere_sprite("AirEQAir" f3b941ee air_knob_small)
skelliere_sprite("AirEQAir" ebd6eca7 air_knob_small_alt)
skelliere_sprite("AirEQAir" ac130149 air_switch)
skelliere_sprite("CS-EQ" 1a0cf220 classic_eq_knob_a)
skelliere_sprite("CS-EQ" 1ac7e4f1 classic_eq_knob_b)
skelliere_sprite("CS-EQ" 1df7c43c classic_eq_knob_c)
skelliere_sprite("CS-EQ" 3d0d5671 classic_eq_knob_d)
skelliere_sprite("CS-EQ" 4c78f351 classic_eq_knob_e)
skelliere_sprite("CS-Lift" 3f28e535 classic_lift_knob_big)
skelliere_sprite("CS-Lift" 8b0d03b3 classic_lift_knob_small)
skelliere_sprite("CS-Lift" 78cb4858 classic_in_switch)
skelliere_sprite("Blackbird Studio" 64ca5d37 blackbird_fader)
if(SKELLIERE_FULL_ARTWORK)
  skelliere_sprite("Custom Opto" 78a70dc8 custom_opto_large)
  skelliere_sprite("Custom Opto" e6f98d44 custom_opto_small)
  skelliere_sprite("Custom Opto" 21eaf559 custom_opto_switch)
  skelliere_sprite("FG-116" 35e0e51b fet_classic)
  skelliere_sprite("FG-116 Modern" c4bb198f fet_modern)
  skelliere_sprite("FG-116 Vintage" 5afc6ace fet_vintage)
  skelliere_sprite("FG-2A" b8ed4c29 optical_knob)
  skelliere_sprite("FG-2A" de491a87 optical_switch)
  skelliere_sprite("FG-401" 1adcfe4b bus_knob)
  skelliere_sprite("FG-401" 99de1620 bus_switch)
  skelliere_sprite("Classic Instruments" 91b50241 instrument_fader)
endif()
# Full private mode prefers the 2x source. Runtime decoder removes RGB byte padding.
function(skelliere_extra_face module id name hires)
  if(hires)
    skelliere_sprite("${module}" "${id}" "${name}")
    set(SKELLIERE_PANEL_FILES "${SKELLIERE_PANEL_FILES}" PARENT_SCOPE)
  else()
    file(GLOB_RECURSE paths "${SKELLIERE_ART_DIR}/full/MODULES/${module}/${id}_1x_*.png")
    list(LENGTH paths count)
    if(NOT count EQUAL 1)
      message(FATAL_ERROR "Expected one 1x faceplate ${module}/${id}, found ${count}")
    endif()
    list(GET paths 0 source)
    set(output "${SKELLIERE_ART_DIR}/${name}.png")
    file(COPY_FILE "${source}" "${output}" ONLY_IF_DIFFERENT)
    list(APPEND SKELLIERE_PANEL_FILES "${output}")
    set(SKELLIERE_PANEL_FILES "${SKELLIERE_PANEL_FILES}" PARENT_SCOPE)
  endif()
endfunction()
if(SKELLIERE_FULL_ARTWORK)
  skelliere_extra_face("Classic Instruments" 4c549b9c instrument_faceplate TRUE)
  skelliere_extra_face("Custom Opto" a1073c02 custom_opto_faceplate TRUE)
  skelliere_extra_face("FG-2A" a15787a2 optical_faceplate TRUE)
  skelliere_extra_face("FG-116" 9c027fe2 fet_faceplate TRUE)
  skelliere_extra_face("FG-116 Modern" 3567fdf8 fet_modern_faceplate TRUE)
  skelliere_extra_face("FG-116 Vintage" 020dafc4 fet_vintage_faceplate TRUE)
  skelliere_extra_face("FG-401" 038b963a bus_faceplate TRUE)
  skelliere_sprite("Custom Opto" d8911352 custom_meter_face)
  skelliere_sprite("Custom Opto" 54d85b16 custom_meter_dim)
  skelliere_sprite("Custom Opto" 02169d01 custom_meter_bezel)
  skelliere_sprite("Custom Opto" 093a02a9 custom_meter_glass)
  skelliere_sprite("FG-2A" 92369a6f optical_meter_face)
  skelliere_sprite("FG-2A" 0dce49e3 optical_meter_dim)
  skelliere_sprite("FG-2A" 9283c400 optical_meter_bezel)
  skelliere_sprite("FG-2A" 186a372a optical_meter_glass)
  skelliere_sprite("FG-116" dc65bf64 fet_meter_face)
  skelliere_sprite("FG-116" 587005bd fet_meter_bezel)
  skelliere_sprite("FG-116 Modern" 593cfae6 fet_modern_meter_bezel)
  skelliere_sprite("FG-116 Vintage" 5a50208b fet_vintage_meter_bezel)
  skelliere_sprite("FG-401" 45dd1bd3 bus_meter_face)
  skelliere_sprite("FG-401" 2219ac45 bus_meter_bezel)
endif()
juce_add_binary_data(SkelliereArtwork SOURCES ${SKELLIERE_PANEL_FILES})

target_sources(SkellierRack PRIVATE
  Source/Modules.h Source/Dsp.h Source/RackModuleFactory.h Source/ProgramOptoCore.h Source/RackProgramOpto.h
  Source/PluginProcessor.cpp Source/PluginProcessor.h
  Source/Look.h Source/RackArtwork.h
  Source/PluginEditor.cpp Source/PluginEditor.h)

target_compile_features(SkellierRack PRIVATE cxx_std_17)
target_compile_definitions(SkellierRack PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0 SKELLIERE_FULL_ARTWORK=${SKELLIERE_FULL_ARTWORK})
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
  if(WIN32)
    target_link_libraries(SkelliereRackUIRenderer PRIVATE psapi)
  endif()
endif()

