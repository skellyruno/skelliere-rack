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
  # The archived bank supports eight modes/voices. The rack exclusively calls
  # mode 1 or 7 with three voices; prepare only these histories in a generated
  # build-local copy of the hash-verified owner header. Never change the owner
  # files or publish the generated engine. The switch provides an A/B baseline.
  option(SKELLIERE_NATIVE_FLANG_COMPACT "Prepare only rack-used Flang bank histories" ON)
  set(_flang_includes "${_native}/flang")
  if(SKELLIERE_NATIVE_FLANG_COMPACT)
    file(READ "${_native}/flang/FlangDSP.h" _flang_original)
    set(_prepare_all "for(auto& mode:voices)for(auto& v:mode)v.prepare(sr);")
    string(FIND "${_flang_original}" "${_prepare_all}" _prepare_pos)
    if(_prepare_pos EQUAL -1)
      message(FATAL_ERROR "Compact Flang preparation anchor missing; turn off specialization or revalidate the pinned bank")
    endif()
    string(REPLACE "${_prepare_all}" "for(int mode=1;mode<=7;mode+=6)for(int voice=0;voice<3;++voice)voices[size_t(mode)][size_t(voice)].prepare(sr);" _flang_compact "${_flang_original}")
    set(_process_anchor "const int mode=std::clamp(which,0,7);")
    string(FIND "${_flang_compact}" "${_process_anchor}" _process_pos)
    if(_process_pos EQUAL -1)
      message(FATAL_ERROR "Compact Flang render guard anchor missing")
    endif()
    string(REPLACE "${_process_anchor}" "${_process_anchor}\nif((mode!=1&&mode!=7)||p.order<1||p.order>3)return {l,r};" _flang_compact "${_flang_compact}")
    # A same-rate prepare previously zeroed every allocated history on the
    # callback. Keep pre-touched capacity and invalidate old samples logically.
    string(REPLACE "data.assign(static_cast<size_t>(std::ceil(rate*seconds)) + 12u, 0.f); head=0;" "const auto count=static_cast<size_t>(std::ceil(rate*seconds))+12u;if(data.size()!=count)data.assign(count,0.f);head=0;valid=0;" _flang_compact "${_flang_compact}")
    string(REPLACE "if(++head==data.size())head=0;" "if(++head==data.size())head=0;if(valid<data.size())++valid;" _flang_compact "${_flang_compact}")
    string(REPLACE "std::fill(data.begin(),data.end(),0.f);head=0;" "head=0;valid=0;" _flang_compact "${_flang_compact}")
    string(REPLACE "int index=int(head)-lag;" "if(lag<0||size_t(lag)>valid)return 0.f;int index=int(head)-lag;" _flang_compact "${_flang_compact}")
    string(REPLACE "size_t head=0;" "size_t head=0,valid=0;" _flang_compact "${_flang_compact}")
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/owner-compact-flang/FlangDSP.h" CONTENT "${_flang_compact}")
    list(PREPEND _flang_includes "${CMAKE_CURRENT_BINARY_DIR}/owner-compact-flang")
  endif()
  # Classic Delay's time changes clear the whole line. Generate a build-local
  # storage-only specialization of the pinned header; the original arithmetic
  # and all owner source files remain unchanged. Keep a baseline switch.
  option(SKELLIERE_NATIVE_DELAY_EPOCH "Experimental paged logical clears for native Classic Delay" OFF)
  set(_delay_includes "${_native}/delay")
  set(_delay_native "${_native}/delay/native")
  option(SKELLIERE_NATIVE_SHARED_STORAGE "Share native Classic and Glitch slot histories" ON)
  if(SKELLIERE_NATIVE_DELAY_EPOCH OR SKELLIERE_NATIVE_SHARED_STORAGE)
    set(_delay_native "${CMAKE_CURRENT_BINARY_DIR}/owner-slot-delay/native")
    foreach(_file Delay.cpp SkellayEngine.cpp FruityDelayBank.cpp SkellayEngine.h FruityDelayBank.h EngineExchange.h codec.h)
      configure_file("${_native}/delay/native/${_file}" "${_delay_native}/${_file}" COPYONLY)
    endforeach()
    file(READ "${_native}/delay/native/Delay.h" _delay_header)
    set(_line_anchor "std::vector<float> line; // interleaved L/R")
    string(FIND "${_delay_header}" "${_line_anchor}" _line_pos)
    if(_line_pos EQUAL -1)
      message(FATAL_ERROR "Native Delay storage specialization anchor missing")
    endif()
    if(SKELLIERE_NATIVE_DELAY_EPOCH)
      set(_storage_header "NativeEpochLine.h")
      set(_storage_type "skl::native_delay::EpochLine")
    else()
      set(_storage_header "SlotDelayStorage.h")
      set(_storage_type "skl::SlotHistoryVector")
    endif()
    string(REPLACE "#include <vector>" "#include <vector>\n#include \"${CMAKE_CURRENT_SOURCE_DIR}/Source/${_storage_header}\"" _delay_header "${_delay_header}")
    string(REPLACE "${_line_anchor}" "${_storage_type} line; // interleaved L/R" _delay_header "${_delay_header}")
    file(GENERATE OUTPUT "${_delay_native}/Delay.h" CONTENT "${_delay_header}")
    list(PREPEND _delay_includes "${CMAKE_CURRENT_BINARY_DIR}/owner-slot-delay")
  endif()
  set(_glitch_includes "${_native}/glitch")
  if(SKELLIERE_NATIVE_SHARED_STORAGE)
    set(_capture_dir "${CMAKE_CURRENT_BINARY_DIR}/owner-slot-glitch/dsp")
    file(READ "${_native}/glitch/dsp/SliceCapture.h" _capture)
    set(_capture_anchor "std::array<std::vector<float>, 2> data;")
    string(FIND "${_capture}" "${_capture_anchor}" _capture_pos)
    if(_capture_pos EQUAL -1)
      message(FATAL_ERROR "Native Glitch storage specialization anchor missing")
    endif()
    string(REPLACE "#include <vector>" "#include <vector>\n#include \"${CMAKE_CURRENT_SOURCE_DIR}/Source/SlotDelayStorage.h\"" _capture "${_capture}")
    string(REPLACE "${_capture_anchor}" "std::array<skl::SlotHistoryVector, 2> data;" _capture "${_capture}")
    file(GENERATE OUTPUT "${_capture_dir}/SliceCapture.h" CONTENT "${_capture}")
    configure_file("${_native}/glitch/dsp/Reverser.h" "${_capture_dir}/Reverser.h" COPYONLY)
    configure_file("${_native}/glitch/dsp/Crossfader.h" "${_capture_dir}/Crossfader.h" COPYONLY)
    list(PREPEND _glitch_includes "${CMAKE_CURRENT_BINARY_DIR}/owner-slot-glitch")
  endif()
  target_sources(SkellierRack PRIVATE "${_native}/flang/NativeChorus/ChorusFx.cpp" "${_native}/flang/NativeFlangus/Flangus.cpp" "${_delay_native}/Delay.cpp" "${_delay_native}/SkellayEngine.cpp" "${_delay_native}/FruityDelayBank.cpp")
  # Limit the feature define to rack translation units so the pinned JUCE
  # objects are reused and the renderer sees exactly the same catalog.
  foreach(_tu Source/PluginProcessor.cpp Source/PluginEditor.cpp tools/SkelliereRackUiRendererMain.cpp)
    set_property(SOURCE ${_tu} APPEND PROPERTY INCLUDE_DIRECTORIES "${_flang_includes};${_delay_includes};${_glitch_includes}")
    set_property(SOURCE ${_tu} APPEND PROPERTY COMPILE_DEFINITIONS SKELLIERE_NATIVE_IMPORTS=1)
  endforeach()
endif()
