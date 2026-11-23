#!/bin/sh
# Host-free DSP test. Needs only a C++17 compiler; no JUCE.
set -eu
cd "$(dirname "$0")/.."
# Unique temporary directory avoids clobbering other users and parallel test runs.
test_dir="$(mktemp -d "${TMPDIR:-/tmp}/skelliere-dsp-test.XXXXXXXX")"
trap 'rm -rf "$test_dir"' EXIT HUP INT TERM
g++ -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -Itests/stub -ISkellCore/include -ISource tests/test_dsp.cpp -o "$test_dir/skelliere_dsp_test"
"$test_dir/skelliere_dsp_test"
