#!/bin/sh
# An explicit local invocation only; no auto install, provider setup or private reference inputs.
set -eu
if [ "$#" -ne 1 ]; then
  echo "Usage: sh tools/run_craft_dsp_lab.sh 'configured backend name'" >&2
  exit 2
fi
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cli=${CRAFT_CLI:-craft-anything-cli}
if ! command -v "$cli" >/dev/null 2>&1; then
  echo "Craft Anything CLI is not installed; install official v0.6.1 and configure an authorized local coding agent." >&2
  exit 3
fi
"$cli" --version
work=$(mktemp -d "${TMPDIR:-/tmp}/skelliere-craft-lab.XXXXXXXX")
# Only explicit independently authored research files and the owned opto core.
# No repository credentials, original reference binaries, extracted material, artwork or audio.
for file in OriginalDynamics.h OriginalAdaptiveCompressor.h test_original.cpp test_adaptive.cpp compare_dynamics.cpp CMakeLists.txt BUILDER_BRIEF.md; do
  cp "$root/experiments/independent_dsp/$file" "$work/$file"
done
cp "$root/Source/ProgramOptoCore.h" "$work/ProgramOptoCore.h"
echo "Isolated worktree: $work"
cmake -S "$work" -B "$work-baseline" -DCMAKE_BUILD_TYPE=Release
cmake --build "$work-baseline" --parallel
ctest --test-dir "$work-baseline" --output-on-failure
# Target '-' means no original target. Keep approval prompts enabled and use the
# existing C++ lab rather than generating a replacement rack or website.
"$cli" --clean-room rebuild - "$(cat "$work/BUILDER_BRIEF.md")" --project-folder "$work" --existing edit --backend "$1" --rounds 2
cmake -S "$work" -B "$work-candidate" -DCMAKE_BUILD_TYPE=Release
cmake --build "$work-candidate" --parallel
ctest --test-dir "$work-candidate" --output-on-failure
echo "Candidate remains isolated at $work; review the diff and compare measurements before importing anything."
