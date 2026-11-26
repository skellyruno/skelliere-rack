#!/bin/sh
# Repeatable DSP audit used in the 2026-11-26 polish pass. Needs g++ (C++17), python3, numpy, scipy.
# Host-free: uses tests/stub/JuceHeader.h. Run from tools/dsp_audit/.
set -eu
cd "$(dirname "$0")"
INC="-I../../tests/stub -I../../SkellCore/include -I../../Source"
OUT="${TMPDIR:-/tmp}/skl-dsp-audit.$$"; mkdir -p "$OUT"; trap 'rm -rf "$OUT"' EXIT
g++ -std=c++17 -O2 $INC dump_clip.cpp -o "$OUT/dump_clip"
g++ -std=c++17 -O2 $INC bench_all.cpp -o "$OUT/bench_all"
g++ -std=c++17 -O2 $INC meter_audit.cpp -o "$OUT/meter_audit"
echo "=== 1. Ember Clip aliasing: production module vs 8x oversampled reference ==="
DUMP="$OUT/dump_clip" OUTBIN="$OUT/o.bin" python3 analyze_clip.py
echo; echo "=== 2. ADAA1 candidate (NOT shipped): delay-invariant alias metric, linear-region droop, ceiling ==="
python3 adaa_proto2.py
echo; echo "=== 3. CPU: ns per stereo frame, 48 kHz, 512-sample blocks, 25 modules ==="
"$OUT/bench_all"
echo; echo "=== 4. Gain-reduction meter: inactive / silence behaviour ==="
"$OUT/meter_audit"
