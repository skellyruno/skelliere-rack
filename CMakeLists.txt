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
option(SKELLIERE_NATIVE_IMPORTS "Enable owner-supplied native Flang, Skellay and Glitch engines" OFF)

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
  NEEDS_MIDI_INPUT ${SKELLIERE_NATIVE_IMPORTS}
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(SkellierRack)

# Reuse the actual Skell Necromancer large skull-cap artwork, not a mockup.
juce_add_binary_data(SkelliereKnobArt HEADER_NAME SkelliereKnobData.h
  NAMESPACE SkelliereKnobData SOURCES assets/knob_cap_big.png assets/demon_relief_atlas.png
  assets/mixhead_toggle_up.png assets/mixhead_toggle_down.png)
target_link_libraries(SkellierRack PUBLIC SkelliereKnobArt)

# Original JUCE panels; legacy asset archives are retained as reference only.
set(SKELLIERE_FULL_ARTWORK 0)
target_sources(SkellierRack PRIVATE
  Source/Recovered/ConsoleEG.cpp Source/Recovered/VocalPitch.cpp Source/Recovered/VoiceShiftCore.cpp
  Source/Modules.h Source/Dsp.h Source/RackModuleFactory.h Source/ProgramOptoCore.h Source/RackProgramOpto.h
  Source/PluginProcessor.cpp Source/PluginProcessor.h
  Source/Look.h Source/OriginalPanel.h Source/AnalogRackModules.h
  Source/PluginEditor.cpp Source/PluginEditor.h)

target_include_directories(SkellierRack PRIVATE Source/Recovered)
target_compile_features(SkellierRack PRIVATE cxx_std_17)
target_compile_definitions(SkellierRack PUBLIC JUCE_VST3_CAN_REPLACE_VST2=0 SKELLIERE_FULL_ARTWORK=${SKELLIERE_FULL_ARTWORK})
target_compile_definitions(SkellierRack PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(SkellierRack PRIVATE
  juce::juce_audio_utils juce::juce_audio_processors juce::juce_gui_extra
  SkellCore::DSP)

option(SKELLIERE_BUILD_UI_RENDERER "Build real Skelliere Rack JUCE editor screenshots" OFF)
if(SKELLIERE_BUILD_UI_RENDERER)
  add_executable(SkelliereRackUIRenderer tools/SkelliereRackUiRendererMain.cpp)
  target_include_directories(SkelliereRackUIRenderer PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/SkellierRack_artefacts/JuceLibraryCode")
  # Use the same generated JUCE header as the plugin.
  target_link_libraries(SkelliereRackUIRenderer PRIVATE
    SkellierRack SkellCore::UIRenderer SkellCore::DSP
    juce::juce_audio_utils juce::juce_gui_extra)
  target_compile_definitions(SkelliereRackUIRenderer PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  target_compile_features(SkelliereRackUIRenderer PRIVATE cxx_std_17)
  if(WIN32)
    target_link_libraries(SkelliereRackUIRenderer PRIVATE psapi)
  endif()
endif()


set(SKELLIERE_NATIVE_SOURCE_ROOT "" CACHE PATH "Owner-source directory containing flang, delay and glitch trees")
if(SKELLIERE_NATIVE_IMPORTS)
  set(_native "${SKELLIERE_NATIVE_SOURCE_ROOT}")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/tools/native_imports.sha256.json" _pins)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/tools/native_parity_candidates.sha256.json" _extra_pins)
  string(JSON _extra_count LENGTH "${_extra_pins}")
  math(EXPR _extra_last "${_extra_count}-1")
  foreach(_i RANGE ${_extra_last})
    string(JSON _file MEMBER "${_extra_pins}" ${_i})
    string(JSON _expected GET "${_extra_pins}" "${_file}")
    string(JSON _pins SET "${_pins}" "${_file}" "\"${_expected}\"")
  endforeach()
  string(JSON _count LENGTH "${_pins}")
  math(EXPR _last "${_count}-1")
  foreach(_i RANGE ${_last})
    string(JSON _file MEMBER "${_pins}" ${_i})
    string(JSON _expected GET "${_pins}" "${_file}")
    if(NOT EXISTS "${_native}/${_file}")
      message(FATAL_ERROR "Missing owner-source dependency: ${_file}")
    endif()
    file(SHA256 "${_native}/${_file}" _actual)
    if(NOT _actual STREQUAL _expected)
      message(FATAL_ERROR "Owner-source hash mismatch: ${_file}; use the tested snapshot")
    endif()
  endforeach()
  target_sources(SkellierRack PRIVATE "${_native}/flang/NativeChorus/ChorusFx.cpp" "${_native}/flang/NativeFlangus/Flangus.cpp" "${_native}/delay/native/Delay.cpp" "${_native}/delay/native/SkellayEngine.cpp" "${_native}/delay/native/FruityDelayBank.cpp")
  # Limit the feature define to rack translation units so the pinned JUCE
  # objects are reused and the renderer sees exactly the same catalog.
  foreach(_tu Source/PluginProcessor.cpp Source/PluginEditor.cpp tools/SkelliereRackUiRendererMain.cpp)
    set_property(SOURCE ${_tu} APPEND PROPERTY INCLUDE_DIRECTORIES "${_native}/flang;${_native}/delay;${_native}/glitch")
    set_property(SOURCE ${_tu} APPEND PROPERTY COMPILE_DEFINITIONS SKELLIERE_NATIVE_IMPORTS=1)
  endforeach()
endif()
