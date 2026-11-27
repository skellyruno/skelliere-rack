# Owner-source native rack pilot — 2026-11-27

Base: 32b50cecbb5e645ec4653038d3fef9218b59bb41. The public default catalog remains 31 modules; owner-source builds expose 34. IDs 50–52 are appended; missing source modules in restored states transparently bypass and indicate unavailable source. Do not redistribute the provisional-license engine sources or private research/audio. The adapters are independent rack code; the DSP kernels remain external.

## Dependencies and build

Use the verified offline JUCE 8.0.12 kit. Set `SKELLIERE_NATIVE_SOURCE_ROOT` to a directory containing `flang`, `delay`, `glitch`. The exact required relative files and SHA-256 hashes are in `tools/native_imports.sha256.json`. Configuration fails on missing or changed files. Copy only the owner-authorized source files; no network fetch or silent fallback.

- Flang: native Classic Chorus and Flangus sources at the tested upstream source revision `fad039cdcfc6118288168418b6ea16ab59d6cbb3` (skell-flang-vst); the Flanger/Dimension bank is the genuine archived `Reproduction/research/VisibleDSP_after.h` from the 618f15c checkpoint, installed as `flang/FlangDSP.h` (Git blob 5e53c35a5d56ea87fbe27020e60ca1d997752148). This is not a claim of parity with the latest standalone Flanger/Dimension revision. Keep the original archive and source license.
- Delay: `skellay` native `Delay.h` and `Delay.cpp`, commit `72522005da84c9e6b53d8f3b7e218e4f2b903652`. Classic only; Modern and Bank require further lifecycle and memory work. Tempo/time changes preserve Classic's existing line-clear behavior. Native clock ticks are 1/192 quarter note.
- Glitch: original seven headers from `skell-gate-vst` commit `9618e6f31689973809b03b897b6b50d595294b34`. Auto, full-band, zero-reported-latency, fixed 16-step pattern; no standalone MIDI/sidechain triggers, full step editor, band filters, chaos or aligned-PDC mode yet. Extreme bar lengths beyond the eight-second capture budget are not validated.

Configure with `-DJUCE_PATH=/path/to/pinned/JUCE -DFETCHCONTENT_FULLY_DISCONNECTED=ON -DSKELLIERE_NATIVE_IMPORTS=ON -DSKELLIERE_NATIVE_SOURCE_ROOT=/path/to/owner/root -DSKELLIERE_BUILD_UI_RENDERER=ON`. Run `SKELLIERE_NATIVE_SOURCE_ROOT=/path/to/owner/root sh tests/run_native_imports.sh`. For sanitizer coverage use `CXXFLAGS='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer'`.

## Local evidence

Three rates (44.1/48/96 kHz), mono/stereo, fixed and automated controls, block partitions 63/257, bypass, reset and C++ allocation probes passed. Direct wet-path comparison: all Flang modes <=2.98e-8, Classic delay <=1.49e-8, Glitch gate exact. This is kernel parity with tested settings, not full standalone preset or host equivalence. 27 existing portable tests, native JUCE build and 23 captures passed. ASan/UBSan focused tests passed. One ten-second real dry vocal: at full wet Glitch reverse/gate peaked +0.39/+0.41 dBFS; use headroom, no hidden limiter added. Listening, Windows VST3, FL Studio, larger rate/instance memory and latest Flang bank parity remain unverified.
