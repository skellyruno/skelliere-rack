# DSP quality milestone — 2026-11-27

Baseline e7cb9ccd242ec5d424d62707ea2b9aeb8cb87fc5. Owner-source mode remains private. No new effects or parameter IDs.

## Production changes

Cache the FET color-stage primitive only at identical drive; recompute when automated. Cache gain/pan invariants in consolidated processors at the existing coefficient cadence. Catch failed native Delay preparation and release its storage reservation. Three original full-rack factory starting points appended; 24 module starting points added (39 total including original 15, 30 in public catalog). Existing factory indices unchanged. Delay's faceplate displays active model/status/taps, Glitch draws saved step states/probabilities; native panels no longer request unnecessary animation redraws.

## Local evidence

Portable benchmark: 4 seconds stereo, 257-frame blocks, best of 3, same CPU/compiler/build, chain Level/76/Bus C/RubyQ/Warm Drive/Exciter/Air/Emerald Clip. Wall-clock processing only, not DAW CPU. Max sample error exactly 0 in all comparisons.

| Hz | Static before -> after (ms) | Automated before -> after (ms) |
|---|---|---|
| 44100 | 105.046 -> 92.875 | 105.705 -> 94.408 |
| 48000 | 111.880 -> 99.015 | 111.096 -> 99.497 |
| 96000 | 222.918 -> 198.151 | 218.926 -> 197.662 |

A private 10-second vocal fixture: 281.312 -> 236.797 ms, sample identical, peak 0.4375. Do not publish the fixture. To reproduce: extract baseline RackCompressor.h and ConsolidatedModules.h via `git show` into a temporary include folder named BaselineRackCompressor.h / BaselineConsolidatedModules.h, compile tools/dsp_audit/bench_eight.cpp with the existing portable stub flags, that include folder, and the three Recovered translation units. Optional argument: local mono 48 kHz float32 raw file.

The three new presets passed finite/headroom checks on synthetic input and the private vocal (sample peaks -13.06, -14.46, -5.23 dBFS respectively; not true-peak certification or listening approval). Native import/controls tests passed at 44.1/48/96/192 kHz, real JUCE editor link and 23-scene captures, new native faceplate capture, state regression. These are local tests; Windows and DAW not tested. Existing portable runner in local cache is older than main's runner, so this is not a claim that every current CI test was executed. Delay storage budget is unchanged; no total memory saving claim.

## Independent saturation research

Source evidence: skell-extraction 858657244a8b8730671f6021f8a2e0debd55f100, artifacts/dsp/2026-11-23/native_equations/EQUATION_RECONSTRUCTION_REPORT.md. Supports transfer functions, reference-span normalization and DC blocker arithmetic; does not establish exact vendor gain laws or control mappings. NormalizedSaturation.h uses original quarter-scale reference anchoring, optional asymmetry, sample-rate-dependent 5 Hz DC removal, and antiderivative treatment of the nonlinear residual. Test at 35% of Fs, drive 4, 0.65 input amplitude: folded third harmonic drops 8.99/9.09 dB (symmetric/asymmetric), but the fundamental increases ~4.0–4.2 dB at this high frequency. Low-frequency third harmonic changes <=0.081 dB at 1 kHz. Passed anchor/DC/finiteness/alias tests at 44.1/48/96/192 kHz. Research only: do not enable by default or claim whole-plugin improvement without frequency-response and loudness-matched A/B. No vendor audio comparison performed.

## Remaining release gates

- Obtain exact authorized Flang full header from standalone draft fad039cdcfc6118288168418b6ea16ab59d6cbb3, Source/FlangDSP.h blob adc545083f9fea76cf409da812ec1c4fc7d4a574. Current compact owner build is still pinned to prior revision. Update locally, verify all required engine sources/manifest hashes, then repeat four-mode direct parity and allocation tests. Do not substitute reconstructed approximations or copy private source into public GitHub.
- Obtain same-input, aligned reference audio for compressor steady state/attack/release, EQ sweep and nonlinear transfer/aliasing. Record plugin settings, SR and version; loudness-match listening. No vendor binaries executed here.
- Repeat 8-slot profiling for native-heavy chains, multiple instances, tail transitions, host latency and memory RSS, as well as broad listening tests.
- Owner-only Windows build: tools/build-owner-windows.ps1 with verified JUCE 8.0.12 and private dependencies. Confirm clean source commit, dependency hash manifest, pluginval, FL Studio, licensing grants for every engine and artwork, and versioned installer/uninstall. Package binary only after gates; never include private source or audio. No signed installer or commercial redistribution approval yet.
