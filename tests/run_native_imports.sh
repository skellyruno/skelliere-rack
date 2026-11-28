#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
N=${SKELLIERE_NATIVE_SOURCE_ROOT:?Set owner source root containing flang, delay, glitch}
bin=$(mktemp /tmp/skelliere-native-test.XXXXXXXX)
trap 'rm -f "$bin"' EXIT HUP INT TERM
${CXX:-g++} -std=c++17 ${CXXFLAGS:--O2} -DSKELLIERE_NATIVE_IMPORTS=1 -Itests/stub -ISkellCore/include -ISource -I"$N/flang" -I"$N/delay" -I"$N/glitch" tests/test_native_imports.cpp "$N/flang/NativeChorus/ChorusFx.cpp" "$N/flang/NativeFlangus/Flangus.cpp" "$N/delay/native/Delay.cpp" "$N/delay/native/SkellayEngine.cpp" "$N/delay/native/FruityDelayBank.cpp" -o "$bin"
"$bin" "$@"

${CXX:-g++} -std=c++17 ${CXXFLAGS:--O2} -DSKELLIERE_NATIVE_IMPORTS=1 -Itests/stub -ISkellCore/include -ISource -I"$N/flang" -I"$N/delay" -I"$N/glitch" tests/test_native_rack_controls.cpp "$N/flang/NativeChorus/ChorusFx.cpp" "$N/flang/NativeFlangus/Flangus.cpp" "$N/delay/native/Delay.cpp" "$N/delay/native/SkellayEngine.cpp" "$N/delay/native/FruityDelayBank.cpp" -o "$bin"
"$bin"
