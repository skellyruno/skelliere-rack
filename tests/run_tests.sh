#!/bin/sh
# Host-free DSP tests. Only a C++17 compiler is needed; no JUCE.
set -eu
cd "$(dirname "$0")/.."
test_dir="$(mktemp -d "${TMPDIR:-/tmp}/skelliere-dsp-test.XXXXXXXX")"
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
for f in ConsoleEG VocalPitch VoiceShiftCore; do
  "${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -ISource/Recovered -c "Source/Recovered/$f.cpp" -o "$test_dir/$f.o"
done
for source in tests/test_dsp.cpp tests/test_smooth_tone.cpp tests/test_rack_extensions.cpp tests/test_program_opto_core.cpp experiments/independent_dsp/test_original.cpp experiments/independent_dsp/test_adaptive.cpp experiments/independent_dsp/compare_dynamics.cpp experiments/independent_dsp/test_wet_duck.cpp experiments/independent_dsp/test_space_duck.cpp tests/test_sprite_frames.cpp tests/test_native_face.cpp tests/test_panel_meter.cpp tests/test_analog_catalog.cpp tests/test_recovered_modules.cpp tests/test_rack_geometry.cpp tests/test_analog_hardware_math.cpp tests/test_artwork_motion.cpp; do
  name="$(basename "$source" .cpp)"
  "${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter \
    -Itests/stub -ISkellCore/include -ISource -ISource/Recovered "$source" "$test_dir/ConsoleEG.o" "$test_dir/VocalPitch.o" "$test_dir/VoiceShiftCore.o" -o "$test_dir/$name"
  "$test_dir/$name"
done

