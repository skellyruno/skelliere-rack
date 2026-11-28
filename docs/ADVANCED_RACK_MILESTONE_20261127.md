# Advanced rack milestone (2026-11-27)

Opt-in advanced engines only: cache multiband coefficients and skip inactive compression gain calculations; refine resonance maxima between detector centers with smoothed center tracking; separate doubler stereo histories with bounded cubic interpolation; cache the exciter ADAA primitive. Legacy parameter IDs, defaults and factory presets unchanged.

Local evidence: four advanced quality test sample rates (44.1/48/96/192 kHz), ASan/UBSan, 24 legacy exact-parity cases and 6,000 automated odd-size blocks. Synthetic 48 kHz off-grid 2710 Hz suppression -1.80 to -5.55 dB; 800/4000 Hz neighbors -0.01/-0.10 dB. Cached exciter max error 0 compared to previous advanced. Higher-order ADAA not enabled: spectral tradeoffs observed; no further alias-reduction claim. Independent formant lock not implemented.

Eight-slot host-free processing comparison, two of each advanced effect, 4 s stereo synthetic signal, 257-frame blocks, best of three:
- 44.1 kHz fixed 238.962 -> 194.849 ms; automated 249.208 -> 200.390 ms.
- 48 kHz fixed 257.686 -> 203.990 ms; automated 260.814 -> 208.807 ms.
- 96 kHz fixed 563.516 -> 429.028 ms; automated 534.542 -> 418.155 ms.
Three private aligned 10 s vocal excerpts: 672.370 -> 544.039 ms, 627.562 -> 481.366 ms, 645.465 -> 553.144 ms. No recordings in Git. Not a host CPU or perceptual-quality certification. Additional doubler delay storage 26912 bytes per instance at 48 kHz; wrapper +272 bytes; total rack RSS unverified. Neutral advanced dry impulse exact; no additional reported host latency introduced.

UI: numeric crossover low/high split controls, shared ordered frequency constraint, fixed-scale measured gain reduction, selected-band readout. Actual JUCE captures and binding tests on local archive build with the modified advanced files; whole exact latest checkout/Windows VST3/FL Studio remain unverified.

Pinned owner-only native dependency recovery: 25 file hashes matched. Latest draft Delay/Glitch rack control tests passed at 44.1/48/96/192 kHz, sanitizer passed, no callback allocation/deletion. Earlier four-mode Flang, Classic Delay, Glitch parity passed at three rates; latest standalone Flang revision not imported. No private sources added. Keep draft until complete builds, host checks and listening.
