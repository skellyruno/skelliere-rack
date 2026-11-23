# Skelliere DSP study — TVB Rack V2, Virtual Mix Rack and independent Skelliere work

**2026-11-23 · static native-binary evidence + live source inspection + original prototype testing**

## Scope and crucial distinction

This report is based on the user's supplied `TVB Rack V2.vst3`, `Virtual Mix Rack.vst3`, and reassembled `Virtual Mix Rack.zip` (original SHA-256 `353f23373ac05d7b2121c313073f08c708e879d839115d4ce5d30657732f481e`, 899/899 ZIP CRC checks). We also inspected the Skelliere Rack draft PR #3's actual C++ implementation. **Neither proprietary Windows plugin was executed in a DAW. Their exact private DSP algorithms, phase response, nonlinear transfer functions, latency and CPU are not established by strings, exports or factory presets.** No DRM, activation or licensing system was bypassed.

## Confirmed architecture: TVB Rack V2

- A single x64 JUCE VST3 with a prefixed embedded UI ZIP; the archive is primarily frontend resources, not a disclosure of DSP source.
- The main native DLL contains **24 numbered `vq_bXX_*` parameter groups**, each with **17 parameter IDs**: `on`, `freq`, `gain`, `q`, `slope`, `shape`, `stereo`, `dyn_mode`, `dyn_range`, `dyn_thr`, `dyn_attack`, `dyn_release`, `dyn_src`, `dyn_lo`, `dyn_hi`, `dyn_density`, `dyn_tilt` (408 band parameter IDs). These are evidence of multiband **controls**, not proof that 24 dynamic filters always process audio.
- Additional native IDs include `fx_comp_algorithm`, `fx_comp2_algorithm`, `fx_comp2_la2a_limit`, `fx_deess_lookahead`, `fx_gate_suppression`, `fx_limiter_mix`, and `fx_width_mix`. The code/actual threshold and times mapping remain unknown.
- Whisper/ggml text/exports indicate local transcription dependencies; they do **not** demonstrate neural vocal DSP.

**Independent design lesson for Skelliere:** preserve a fixed parameter identity per dynamic EQ band and separate each band's frequency/shape/static gain from its detector, threshold, dynamics, stereo and sidechain. Start with one validated dynamic bell filter rather than enabling 24 cost-intensive processors immediately.

## Confirmed architecture: VMR

- The top-level rack DLL loads separately packaged native module binaries; all 31 module `.epl` files were found in the verified ZIP. Module manifests report 2.6.4.0.
- The `.epl` modules have exported `createEplModule`/`destroyEplModule` entry points and separate resources. Separate module packaging does not itself imply one processor architecture or CPU strategy.
- Factory presets are XML, with **391 module presets and 140 rack-chain presets** parsed. They supply parameter IDs and normalized values; those values are not attack milliseconds, dB or Hertz without independently verified transfer functions.
- Relevant control evidence:

| Native effect | Evidence from factory preset / manifest | Not demonstrated |
|---|---|---|
| FG-116/FET family | Attack, Release, Ratio, Input, Output, Mix; `OversamplingFactor` text in three FG-116 DLLs | True oversampling rate, alias attenuation, FET circuit curve |
| FG-2A | `PeakReduction`, `PreEmphasis`, `Gain`, `Mode`, `Mix`, `StereoLink`; manufacturer describes smooth opto and warmth | Exact optical cell response, program-dependent release function, total harmonics |
| FG-Stress | `SidechainHp`, `SidechainPeak`, `AudioHp`, `Dist`, `Link`, `Ratio`, `Attack`, `Release`, `Mix` | Filter transfer curves, distortion harmonics, compressor topology |
| FG-401 | `Circuit`, `Transformer`, `Threshold`, `Ratio`, `Attack`, `Release`, `Mix` | Two physical or numerical circuit models and transformer wave-shaping equation |
| CS-EQ | Four band on/off + frequency/gain IDs, HPF, LPF, OutGain; manufacturer describes tuned curves/harmonic profiles | Filter topology/Q law, harmonic level or anti-aliasing strategy |

**Independent design lesson for Skelliere:** do not combine every characteristic behind a single 'analog' knob. Separate detector sidechain, gain law, timing/recovery, output color, stereo linking and wet/dry behavior. Prove each stage by a corresponding controlled WAV render.

## Current Skelliere DSP confirmed from source

- Existing legacy Opto C uses a stereo-linked instantaneous peak follower with 8 ms decay; gain reduction is attack/release smoothed against a 6 dB soft-knee dB curve. It has no separate sidechain HP or program-dependent reduction memory.
- Existing Fast C uses a linked peak detector + 3 dB soft knee; Bus C uses an energy/RMS-style detector + 9 dB soft knee. Both are independent Skelliere code and not emulations of named third-party circuits.
- EQ/Air effects have RBJ-style biquad filters and, in the newer factory classes, 20 ms control smoothing plus 5 ms bypass fades. Filters are updated on a 16-sample cadence.
- `Warm Drive` applies a tanh softclip at host sample rate with no oversampling in its current implementation; this is measurable aliasing risk.
- The plugin's published APVTS IDs, first 12 module identities, factory presets and the single Tune latency contract must not change. The 0–127 `s{slot}_extra` extension parameters permit compatible new effect types.

## Independent Skelliere saturation risk measurement (not VMR/TVB)

We reproduced the source-visible Skelliere Warm Drive **math offline**, compared with a separately oversampled 4× mathematical reference. Conditions: 48 kHz; 17 kHz input sine, amplitude 0.45; +12 dB pre-drive; tone LP 12 kHz; 100% wet; 1 s steady-state projection after warm-up. This is **not a rendered VST3 benchmark** and not a drop-in production oversampler; oversampling changes phase, output level and latency unless properly aligned.

| Artifact | Legacy Skelliere waveshaper | 4× oversampled math-only reference | Difference |
|---|---:|---:|---:|
| Folded third harmonic, 3 kHz | -22.02 dBFS | -71.06 dBFS | 49.04 dB less in reference |
| Folded second harmonic, 14 kHz | -38.38 dBFS | -97.25 dBFS | 58.87 dB less in reference |

**Next:** design half-band polyphase filters with an explicit latency contract, align dry/wet, and validate 48/96 kHz alias rejection and CPU. Keep legacy Warm Drive's sound as the default and add a separate user-selectable cleaner mode after listening approval; do not silently alter old projects.

## Original implemented Skelliere prototype: Program Opto

Added as an **optional compatible extension**, not a replacement for Opto C. Its pure C++ independent core uses stereo-linked sidechain power, a first-order sidechain HP, 6 dB soft knee, 2/12 ms detector smoothing, attack-controlled gain reduction, and a slow memory of recent reduction that increases the release coefficient after deep sustained compression. No analog circuit, oversampling or automatic harmonic color is claimed.

- Controls: Thresh, Ratio, Attack, Release, Sidechain HP, Makeup.
- Dedicated `ProgramOptoCore` is heap-free, lock-free and allocation-free in its sample function. The JUCE wrapper uses 20 ms parameter smoothing, 5 ms bypass fades and fixed 16-sample coefficient updates.
- An optional extended module slot index is used, leaving the old host choices and parameter IDs intact; nominal internal module latency is zero.
- Local standalone tests: 70 Hz signal 0.4 amplitude, -30 dB threshold/4:1 ratio — with detector HP20, gain was -15.66 dB; with HP350, gain was -5.14 dB (10.52 dB less low-bass gain reduction); silence and NaNs handled, polarity retained at 44.1/48/96/192 kHz. A 100 ms post-input recovery test found the reduction after deep and shallow input differed; normalized time constants still need listening.
- Local AMD EPYC 9V74 core microbenchmark: original prototype 113.4 ns per stereo frame and optimized version 101.5 ns per frame (12 passes / 2.88 million frames, `g++ -O3`). This is a **host-free source-core** benchmark, not DAW %CPU; warm-up/compiler/cache/environment can affect timings.

## Tests needed before any quality comparison

1. Render identical dry vocals and test signals through each plugin, one effect at a time, at 48/96 kHz and various block sizes, in a licensed Windows host. Preserve dry copies and normalize level before subjective A/B.
2. Compressor: five amplitude steps, 1 kHz steady-state amplitude sweep, impulses and acoustic plosives. Extract threshold/knee/ratio and first 10/63/90% attack and release times; test mono/stereo linking and detector HPF separately.
3. EQ/dynamic EQ: 20 Hz-20 kHz log sweeps, phase/deconvolution, per-band dynamic threshold excitation, left-only/right-only/mid-side input and automation.
4. Harmonic stages: 1 kHz/17 kHz, various drive levels, 48 and 96 kHz; calculate harmonic bins, alias bins and optional 4× true peak; test dry/wet null and gain matching.
5. Performance/compatibility: measured reported latency and CPU on stable machine, zero allocation on audio thread, true-peak clipping, denormals, preset recall and automation, real FL Studio UI/audio validation.

## Evidence and repository state

- Original VST3 input SHA-256: TVB `cefaa0c08642093832b0cf9cd93fea791ebd8146dae395d310a2624610a8fff9`; VMR `8da77cc3d3e9935db56f56dabc586f1ff38dc1e667fa9ff6a9fbc2302f66c838`.
- VMR complete ZIP SHA-256: `353f23373ac05d7b2121c313073f08c708e879d839115d4ce5d30657732f481e`.
- Exact Skelliere code inspected from `feature/additional-rack-artwork` in `skellyruno/skelliere-rack` on November 23, 2026.
- No vendor DSP implementation or copyrighted binaries are embedded in Skelliere or this report.

**Unverified:** true TVB/VMR math and signal routing; behavior of any actual licensed plugin; audio preference versus real vocals; Skelliere's full VST3 host results until exact-commit CI and Windows audio A/B pass.
