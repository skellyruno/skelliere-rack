#!/bin/sh
# Owner-only portable test runner; no download and no dependency substitution.
set -eu
cd "$(dirname "$0")/.."
N=${SKELLIERE_NATIVE_SOURCE_ROOT:?Set owner source root containing delay and glitch}
python3 - "$N" <<'PY'
import hashlib,json,pathlib,sys
root=pathlib.Path(sys.argv[1]); pins=json.load(open('tools/native_imports.sha256.json'));pins.update(json.load(open('tools/native_parity_candidates.sha256.json')))
for path,expected in pins.items():
    if not path.startswith(('delay/','glitch/')): continue
    if hashlib.sha256((root/path).read_bytes()).hexdigest()!=expected: raise SystemExit('Owner-source hash mismatch: '+path)
print('Exact Delay and Glitch source hashes passed')
PY
bin=$(mktemp -d)
trap 'rm -rf "$bin"' EXIT HUP INT TERM
${CXX:-g++} -std=c++17 ${CXXFLAGS:--O2} -I"$N/delay" tests/test_native_delay_exchange.cpp "$N/delay/native/Delay.cpp" "$N/delay/native/SkellayEngine.cpp" "$N/delay/native/FruityDelayBank.cpp" -o "$bin/delay"
${CXX:-g++} -std=c++17 ${CXXFLAGS:--O2} -I"$N/glitch" tests/test_native_glitch_pattern.cpp -o "$bin/glitch"
"$bin/delay" "$@"
"$bin/glitch" "$@"
