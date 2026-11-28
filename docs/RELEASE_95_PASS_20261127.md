# Release-target pass: memory, live metering and research (2026-11-27)

Base: 19930eab9bdeff46d5e35525eaf12dffa71d2ddf. Reused verified Quick4 JUCE 8.0.12 cache and the pinned owner sources. No private recordings or owner kernel sources included.

## Production
Each rack slot prepares one shared rack-owned delay arena, with disjoint ranges within an effect and reused ranges between mutually exclusive effects. Effect activation already resets history; logical history invalidation replaces linear-time zeroing. The existing single Tune, native Delay drain path and global PDC bypass do not share slot arenas. All allocations remain in prepare. Preset and parameter identities unchanged. Standalone lines retain separate storage. New tests compare exact linear interpolation and repeated reset/wrap to the prior zero-filled reference, and verify live Exciter residual peak, read/reset and legacy isolation. Both focused tests passed O2 and ASan/UBSan. The full portable suite with arena passed before the final meter-only change; meter test passed separately.

Native complete processor profiling, 257 frames, two Flang, two Glitch and four Delay (prior vs updated RSS KiB):
- 48 kHz one Modern: 180324 -> 175096 (5.1 MiB saving).
- 48 kHz two Modern: 354456 -> 343880 (10.3 MiB).
- 96 kHz one Bank: 248176 -> 237868 (10.1 MiB).
- 192 kHz one Bank: 467216 -> 446188 (20.5 MiB).
All models active, finite output, zero reported chain latency. Worst measured buffer budget 19.483%, 28.776%, 77.542%, 59.681%, respectively. Shared VM wall-time tests are not a real-time guarantee; average CPU did not uniformly improve. Native capture buffers and all-catalog preparation still dominate idle memory. No change to owner-native engines or immediate automation semantics.

Exciter advanced panel now displays the measured output difference peak in dBFS with display decay. Transfer curve remains explicitly schematic. Actual JUCE build and capture, artwork motion/pause checks, advanced APVTS bindings and 23 rack scenes plus four advanced panels. Latest Windows VST3 and FL Studio pending.

## Research only
Phase-matched oversampled Exciter (one-sample residual average to match first-order ADAA's fractional delay): at 48 kHz 18 dB drive, 100% residual, 1000 Hz HP and 18 kHz LP, 0.5 amplitude tones, fundamental delta <=0.18 dB for 1/3/5/7/9/11 kHz. At 9 kHz folded third -47.02 -> -77.77 dBFS, at 11 kHz -40.27 -> -110.85 dBFS. Desired third at 1/3/5/7 kHz differs by -0.10/-1.04/-3.75/-15.70 dB. 12-sample latency remains; do not enable until higher harmonics and PDC validated.

PSOLA formant-lock candidate using existing recovered VocalPitch detector and two wet voices, +/-35 cents, 40 ms wet delay, source formant factor 1. On three private 48 kHz mono vocal excerpts, peak -1.43/+2.77/-0.69 dBFS. Broad envelope RMS dB error (Welch, 300–7000 Hz, 128 log-spaced points, Gaussian smoothing sigma 2, mean removed), previous L/R -> PSOLA L/R: 1.001/0.815 -> 0.652/0.942; 0.774/1.003 -> 0.867/0.977; 1.300/1.176 -> 1.467/1.912. Mixed results and headroom failure, so do not activate. Neither new research engine changes factory sound.

The 95% target is not yet certified. Next: owner ring allocation lifecycle with full automation compatibility, stronger PSOLA confidence/level matching, alias candidate harmonic matching, repeated high-rate stress and Windows DAW tests.
