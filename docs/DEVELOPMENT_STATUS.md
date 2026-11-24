# Skelliere Rack — initial source audit (2026-11-23)

## Implemented locally
- Fixed Bypass All so the unprocessed input passes through a shadow delay matching the first Skelly Tune slot's host-reported latency. History is always advanced even when the rack is active, avoiding a timing gap when toggling bypass.
- Added deterministic mono/stereo, odd-block, zero-latency and active-to-bypass impulse regression tests.
- Made the Dream Strip context-menu callback lifetime-safe if its button is destroyed before the menu closes.
- Added PR-only CI concurrency and corrected `find -maxdepth` ordering in artifact collection.

## Tested
- ZIP integrity test: passed on original supplied upload.
- `sh tests/run_tests.sh` on Linux g++ C++17: passed, including bypass regressions.
- Baseline and updated ASan/UBSan instrumented DSP tests: passed.

## Blocked / unverified
- Full JUCE configure/build: blocked because JUCE 8.0.12 is not installed and the runtime cannot resolve github.com for FetchContent. No VST3 binary was produced or host-tested.
- Pluginval, Windows/FL Studio, audio A/B, latency measurements in a host and real GUI screenshot inspection remain unverified.
- Public publishing of the bundled SkellCore copies needs a provenance/licensing review; the source archive explicitly warns that upstream sources have different terms.

## Next release gates
1. Acquire pinned JUCE and run Windows/macOS/Linux CI, pluginval and FL Studio with the exact artifact SHA.
2. Measure timing and phase continuity across Tune enable/disable, Bypass All and automation; record samples at 44.1/48/96k and variable block sizes.
3. Record and inspect actual editor screenshots at supported scaling modes and fix visible defects.
4. Compare real dry vocal recordings for de-esser, Tune, compressor, saturator, reverb and limiter, loudness matched, before changing sound tuning.


## Independent Craft DSP lab — 2026-11-24
See [CRAFT_DSP_LAB.md](CRAFT_DSP_LAB.md). Imported four independently authored research files from exact decompiler commit ae2c64e307658a249b90167619c9671500bb9111 into an isolated experiment and added three-way common-signal comparison plus CMake/CTest and sanitizer CI. Craft AI builder blocked locally by missing executable/agent and DNS; no AI candidate or new plugin build claimed. Production unchanged; new CI pending.
