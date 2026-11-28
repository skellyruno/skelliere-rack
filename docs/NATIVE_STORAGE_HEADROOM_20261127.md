# Native storage, Delay admission and headroom — 2026-11-27

Base d271200. Verified offline JUCE 8.0.12 and pinned owner inputs. Only build-local generated specializations of native history storage; original owner kernels unchanged and not distributed.

Production: shared preallocated Classic Delay/Glitch history per slot, separate disjoint stereo slices, original capture length, reset on entry; standalone storage independent. Preserve new Delay publication on entry and service once per slot. Separate peak construction reservation from persistent Bank storage (128 MiB persistent limit, 384 MiB shared budget, temporary per-engine cap 256 MiB). Single eight-tap Bank at 192 kHz passed, 99.63 MiB persistent. Peak guard append-only parameter output_guard, off by default and in restored old sessions, stereo-linked -1 dBFS sample ceiling, 100 ms release, zero lookahead, bypass all remains exact. Not true peak; enabling can change dynamics. Click meter for option, stereo peak readout and color feedback. Existing preset sound and IDs preserved.

## Verification

33 portable test executables passed. Native controls at 44.1/48/96/192k passed. Shared storage repeated switch + 17s Glitch reverse capture at 30 BPM, two cycles at each of four rates: zero sample error, zero instrumented callback C++ new/delete; ASan/UBSan passed. Peak guard at five rates: exact default-off, linked overload cap, monotonic release, unity recovery; sanitizer passed. Actual owner-source JUCE processor/editor compiled, renderer linked with cached matching JUCE objects. Processor regressions and real UI/motion captures passed. Popup window appearance not verified in headless session.

Stress probe: same variable block sizes and native workload as prior report, Linux shared runner, timing includes scheduling. Loaded RSS KiB old/new; misses old/new; mean updated buffer budget; wrong delay old/new.

| workload | RSS KiB | misses | mean | wrong Delay |
|---|---:|---:|---:|---:|
| 48k 30s | 161260 / 148704 | 1 / 6 of 4795 | 8.19% | 0 / 0 |
| 96k 30s | 209856 / 192864 | 7 / 9 of 9590 | 13.06% | 0 / 0 |
| 192k 60s | 390008 / 358072 | 130 / 154 of 39024 | 25.72% | 1 / 0 |
| 192k two racks 30s | 773024 / 670536 | 487 / 315 of 19180 | 51.70% | 0 / 0 |

All five stress runs exit zero, finite audio and zero instrumented callback C++ new/delete, release/reprepare passed. Catalog guard 192k all 52 identities, 2s per group: peak 0.891 linear (bounded at -1 dBFS), 3/4606 misses, no ceiling violations. Single-rack timing regressed; deadline issue remains. No claim of DAW stability.

## Research only

FormantDoublerEnvelope: 8-band per-channel slowly adaptive bounded wet spectral envelope EQ. Three 10s private vocal excerpts, 48k, +/-35 cents. Normalized Welch broad envelope RMS error (dB; 300-7000 Hz, 128 log points, Gaussian sigma 2, mean removed) old -> new L/R: 1.001/0.815 -> 0.979/0.819; 0.774/1.003 -> 0.738/1.037; 1.300/1.176 -> 1.294/1.165. Four of six improved, two regressed. Peaks -2.75, approximately 0 but below unity, -1.21 dBFS. Not direct formant tracking, not listening-approved. Keep inactive.

ExciterLongFir: 129-tap 4x phase-matched interpolation/decimation, 32 samples alignment latency. 48k six tones, drive18, wet100%, tone18k, HP1k, amplitude .5. Fundamental max delta 0.17dB. Folded third at 9k -47.02 -> -110.88 dBFS; 11k -40.27 -> -119.83. Desired third at 1/3/5/7k -29.45/-28.10/-29.74/-43.94 -> -29.55/-29.14/-33.19/-55.48. Desired harmonics and latency still fail acceptance. Keep inactive. Private audio not distributed.

Latest exact Windows VST3 and FL Studio testing remain pending; no production formant or Exciter change.
