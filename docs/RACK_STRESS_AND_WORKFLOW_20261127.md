# Rack stress and workflow pass — 2026-11-27

Baseline: 9139958289bb77f91ab8c32b46e330711f6c585d. Pinned owner-only DSP and offline JUCE 8.0.12 reused; no owner source or private audio redistributed. No preset IDs or existing factory values changed.

- Compact Flang preparation (default on for owner builds): two rack-used bank modes, three voices, logical reset of preallocated history. Original vs compact sample-bit hashes match at 32/44.1/48/96/192 kHz with automation and resets; callback new count zero. Build-local generated header only, never overwrite pinned owner files. The full sustained stress run used the first compact version; the final logical reset version also passed the focused parity probe and built locally.
- Release and reprepare free the previous catalog allocations; publish reduction telemetry through processor-owned atomics. Snapshot types for consistent rack routing and latency-aligned bypass; preserve first-Tune behavior.
- Context menus: effect preset submenu, type-safe copy/paste, reset, undoable power toggle, duplicate disabled when full, consistent popup theme. Factory selector tracks edited/custom racks. Processor regression checks copy/paste, reset, power undo and factory matching. Actual JUCE renderer passes; popup peer capture blocked by headless X11 BadAtom, so menu appearance is not independently visually verified.

## Stress evidence
Native-heavy chain: two Flang, two Glitch, four Delay, alternating silence and synthetic tones/impulses, 17/32/64/127/257/511/1024 frames. Timing excludes input generation but includes scheduler noise. Linux shared runner; no DAW. First compact version (without experimental epoch delay):

| Rate / instances / audio duration | Loaded RSS KiB | Mean / p95 / p99 buffer budget | Misses / callbacks | Wrong delay |
|---|---:|---|---:|---:|
| 48k / 1 / 30s | 161260 | 7.47 / 9.90 / 17.29% | 1/4795 | 0 |
| 96k / 1 / 30s | 209856 | 12.55 / 18.09 / 30.85% | 7/9590 | 0 |
| 192k / 1 / 60s | 390008 | 24.45 / 34.77 / 53.58% | 130/39024 | 1 |
| 192k / 2 / 30s | 773024 | 53.48 / 77.49 / 158.46% | 487/19180 | 0 |

All 52 identities including hidden legacy exercised at 48/96/192k, 2 seconds per eight-slot group, zero nonfinite samples and instrumented C++ new/delete in callbacks. Catalog deadline misses 0/1169, 1/2310, 8/4606 respectively. Worst catalog peak 11.934 linear at 192k (exceeds unity, not a clipping-free claim). Reprepare/release passed. Compare previous fixed-chain loaded memory 175096/237868/446188 KiB at 48/96/192k: compact stress loaded RSS is lower but workloads differ; not a controlled identical workload A/B. Final epoch prototype fixed-chain loaded 160440/208388/387916 KiB. Epoch prototype not production default.

## Research gate
Paged Classic Delay logical clear: 600k randomized operations, ASan/UBSan, five-rate bit parity, zero callback allocations. Isolated 192k timing reset worst 2166.27 -> 0.77 microseconds. Sustained prototype results inconsistent: 238/39024 misses at 192k one instance, one wrong model; 286/19180 misses at two instances. Keep SKELLIERE_NATIVE_DELAY_EPOCH OFF. No production formant or Exciter alias changes: previous candidates still fail envelope/headroom or desirable harmonic/latency gates. High-rate delay budget/rejected model, headroom, sustained deadlines, full latest Windows and FL Studio checks remain open. Prior public-catalog commit 9139958 Windows VST3 built and pluginval reported success; separate VST3 validator not present. Do not label this new commit release-ready.
