# Empty rack startup, main preset menu and stability — 2026-11-27

Base 69aa237. New processor instances use empty APVTS defaults, unity I/O, guard off and no undo entries. Factory preset order/IDs and all automation IDs are preserved. Main menu visually puts Empty rack (existing ID 4) first and uses the actual JUCE popup inside the editor, below the selector. Previous/next follow visual order; Default strip now selects Empty rack. Existing session restore unchanged. Actual owner-source editor captures and processor regression passed, including new instance zero latency, exact unit-gain dry samples, actual visible popup component, preset select and undo. Reused verified offline JUCE 8.0.12 and pinned owner inputs.

CI screenshot workflow on base failed solely on stale Quick4 count (renderer emitted five scenes, workflow expected four); update exact expected counts to 5 Quick4 scenes and 46 full scenes, plus named startup/menu captures. Previous public-catalog Windows VST3 job and pluginval succeeded; independent VST3 validator skipped. New exact commit still requires Windows build, renderer CI and FL Studio host tests.

## Delay and stress

Skip redundant Classic restart when first process already seeds and clears it. Cache successfully applied raw control tuples in PreparedSlot::Owned to avoid repeatedly expanding/validating the unchanged Bank or Modern controls. Preserve tempo updates, generation handling, and repeat failed control updates. Native controls parity passed at 44.1/48/96/192 kHz with zero callback new/delete. Portable regression suite passed.

Actual owner-source rack, same variable-block synthetic native-heavy workload as prior; 17-1024 frames. Thread CPU clock separates scheduler time from callback compute, with measurement overhead. All finite, zero instrumented C++ callback new/delete, zero wrong Delay models, release/reprepare passed.

| workload | wall mean | previous/new misses | thread CPU mean/p99 | thread CPU misses |
|---|---:|---:|---:|---:|
| 48k 1 x 30s | 7.96% | 6/6 of 4795 | 7.98/15.36% | 6 |
| 192k 1 x 60s | 24.20% | 154/114 of 39024 | 24.29/56.78% | 116 |
| 192k 2 x 30s | 49.64% | 315/262 of 19180 | 49.81/110.24% | 263 |

Worst thread CPU budget 660.32/825.58/6124.35% respectively. Real computational spikes remain; not simply shared VM scheduling. Do not certify real-time stability.

## Research only
Gradient-based formant correction predicts source-envelope displacement from requested pitch offset and dry band energy slopes (12 bands, bounded EQ). Three prior private 48k mono excerpts, +/-35 cents, normalized Welch envelope RMS error 300–7000Hz: old L/R 1.0012/.8153, .7738/1.0033, 1.2999/1.1759 dB; candidate .9473/.7822, .7401/.9953, 1.3035/1.1688 dB. Five of six improved; peaks -2.66/-0.02/-1.54 dBFS. Not actual independent formant tracking, only previously used recordings and one detune, no listening approval. Keep inactive.

Shaped 4x residual decimator: 257 taps, 64 sample alignment, passband gain correction designed from original tone fixture. At 48k training six tones, max fundamental 0.216 dB, desired third max 0.32 dB vs 11.54 dB previous long-FIR, folded third 9k -47.02 -> -99.08 dBFS, 11k -40.27 -> -108.24. 48 held-out combinations across four rates, frequencies and two drives: 37 desired cases above -100 dBFS, five >1 dB delta (max 28.085 dB near Nyquist); max fundamental .505 dB, 10 alias cases with at least 54.516 dB reduction. Three private vocal fixtures aligned relative null -90.92/-90.48/-77.82 dB at 9dB drive, 35% mix, 3500Hz HP, 12k tone. Processing time approximately 14–15x current mono engine in these fixtures. 129-tap variant has 32 samples but worse desired harmonic match. Four-tap fractional phase compensation alone also traded fundamental accuracy for harmonic accuracy. Keep research-only, no production change; latency/CPU/held-out harmonics and listening gates remain. No private audio or owner kernels published.
