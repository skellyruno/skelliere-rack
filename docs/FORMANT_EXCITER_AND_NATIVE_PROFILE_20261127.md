# Formant/Exciter, hardware panels and native-heavy validation

Baseline 0c4aefce21ff019b3a92e26732408dcb020cca11; recovered the exact CI source archive and verified its SHA-256. Cached pinned offline JUCE 8.0.12 and owner-native sources reused. No private DSP or recordings redistributed.

## Release gate repair
GitHub build run 46100121668 failed the combined neutral impulse assertion, so its Windows job was skipped. Explicitly skip zero-dB shelves/bells in Advanced Multiband rather than rely on floating-point cancellation of identity coefficients; reset those filter histories when they go neutral. Keep detectors and non-neutral paths running. Split per-effect assertions to identify any future failure. Local four-rate focused regression passed O2, native FMA and ASan/UBSan; the remote compiler failure was not reproduced locally and exact new CI still required.

## Research, not production
16-order adaptive LPC lattice whitens excitation, shifts it through the existing two voices, and restores the spectral envelope. Analysis/synthesis error 1.49e-8 at 44.1/48/96/192 kHz. Real private vocals: broad level-normalized Welch envelope (300–7000 Hz, log frequency grid, smoothed PSD), left/right RMS errors old -> candidate: 0.868/0.946 -> 0.900/1.101 dB; 1.234/1.258 -> 1.281/1.441 dB; 1.223/1.095 -> 1.571/1.188 dB. Candidate peaks -1.74/+1.73/-0.18 dBFS, no lattice safety faults. Reject production activation: envelope metric worse and headroom fails on one clip. This metric is not direct formant tracking or listening approval.

4x aligned Exciter vs current ADAA: at 48 kHz, 18 dB drive, 1000 Hz HP, 18 kHz tone, 100% residual, 0.5 sine amplitude, folded third at 9 kHz input -47.02 -> -63.57 dBFS and 11 kHz input -40.27 -> -105.74 dBFS; fundamental differs by -6.01/-5.71 dB, and desired third differs by up to 1.51 dB on 1–7 kHz tests. Candidate incurs 12 samples of alignment latency. Do not enable; preserve current sound and latency. Probe source retained.

## Hardware
Actual Resonance and Exciter advanced panels: main macro controls bound to the same APVTS IDs, measured resonance reduction, parameter-driven Exciter blend, explicitly schematic curves. Real owner-mode JUCE screenshot capture and inspection, then axis label/spacing correction and recapture. All 23 rack scenes plus four advanced panels and native control checks passed, using current application source and one set of JUCE module objects for the renderer link. Windows host not tested.

## Full processor native profile
AMD EPYC 7763, Linux, stereo, 257 frames, two Flang, two Glitch, four native Delay; complete processors including idle allocations. Fresh process for each test, one second warmup and ~two seconds measured. RSS KiB, buffer-period CPU wall-time percentages, no DAW or UI render overhead.

| rate | instances | delay | empty RSS KiB | loaded RSS KiB | mean | p95 | max |
|---|---:|---|---:|---:|---:|---:|---:|
| 48000 | 1 | Modern | 112508 | 180324 | 5.907% | 6.701% | 9.348% |
| 48000 | 2 | Modern | 218940 | 354456 | 12.597% | 15.403% | 48.088% |
| 96000 | 1 | Bank, 2 taps | 196176 | 248176 | 9.531% | 11.792% | 40.164% |
| 192000 | 1 | Bank, 2 taps | 363360 | 467216 | 19.158% | 25.042% | 59.055% |

All selected models verified active, no rejected models, finite output, zero reported latency for this no-Tune/no-lookahead chain. Separate host-free native-chain runs recorded outliers over deadline at 96/192 kHz (123.423/176.573% max), so real-time stability is not certified. High idle memory is the next issue: each slot preallocates the whole catalog. Do not simply allocate in the callback or invalidate automatic effect switching. Full processor probe and native-only probe retained. Real Windows VST3 and FL Studio remain pending; public CI does not include private owner kernels.
