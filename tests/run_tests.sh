#!/bin/sh
# Host-free DSP tests. Only a C++17 compiler is needed; no JUCE.
set -eu
cd "$(dirname "$0")/.."
test_dir="$(mktemp -d "${TMPDIR:-/tmp}/skelliere-dsp-test.XXXXXXXX")"
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
for source in tests/test_dsp.cpp tests/test_smooth_tone.cpp tests/test_rack_extensions.cpp; do
  name="$(basename "$source" .cpp)"
  "${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter \
    -Itests/stub -ISkellCore/include -ISource "$source" -o "$test_dir/$name"
  "$test_dir/$name"
done

