# Rack tape-kernel transfer from independent DSP research

Parent 57c97d6bdf9c9f71f3257eb5393f357e4aa195d4. Source handoff commit and nested archive SHA-256 verified. Reused the cached Quick4 kit, all 13 manifest checks passed. Research baseline: skell-vst-decompiler a647e3c, which showed contiguous rings can preserve our original larger oversampling engines sample-for-sample. Tested whether the same optimization benefits Rack's original Skell MixHead 49-tap interpolation/decimation kernel.

## Actual implementation

Extract original AlignedTape4x from NewColorModules.h into Source/AlignedTape4x.h. Keep the exact existing coefficient math and active-amount path. When !(amount>0), write four zero residual entries as one contiguous group and advance the residual ring by four and dry ring by one. The residual index starts at zero after reset and is advanced by four per process call, so each zero group fits within the 64-entry ring. This also preserves the original condition for NaN amount, although valid controls are finite. No state is skipped: history is updated and the same 12-sample dry value returned. The factory, control IDs, gain controls, effect types and UI remain unchanged. The frozen owned reference in tests is from the exact parent class, with a local pi constant identical to Dsp.h.

## Real-vocal evidence

36 private real-vocal excerpts, 159.25 seconds at 44.1 kHz; each file fingerprint checked. Three gain/amount/bias-control conditions: (1,0,.5), (1,.5,.5), (2,1,0). Compared original aligned dry and nonlinear residual separately, not full module renders. Three paired repeats with alternating order; includes preparation and native adapter. 108 configurations and 648 timed runs per candidate. All candidate outputs are sample-equal to the original, with 108 exact dry-alignment checks for the accepted candidate.

| Amount | Original ms/audio-second | Mirrored ms/audio-second | Mirrored median paired saving | Zero-path candidate ms/audio-second | Zero-path median paired saving |
|---:|---:|---:|---:|---:|---:|
| 0 | .424 | .710 | -68.93% | .333 | +22.01% |
| .5 | 9.804 | 12.353 | -26.69% | 9.595 | +2.17% |
| 1 | 9.914 | 12.359 | -24.77% | 9.653 | +3.70% |

Original medians shown from mirrored run; separate baseline runs for zero-path candidate (.4243/9.7953/9.8973). Mirrored rings were slower in every excerpt at each setting: rejected. Zero-path candidate faster in 36/29/32 of 36 excerpts. Active-path differences are small wall-time observations, not proof of a general CPU improvement. Full private report hashes in aggregate JSON; no identities, audio or individual data committed.

## Gates

Full host-free tests/run_tests.sh passed on exact parent source plus patch, including 33-effect catalog and existing compatibility checks. Dedicated kernel comparison passed with GCC optimized and ASan/UBSan, 300000 per-channel calls with deterministic changing controls and resets, mono/stereo and impulse dry delay. Full tests/test_dsp.cpp also passed under ASan/UBSan. Larger sanitizer batch exceeded execution limit after that test; remaining sanitizer results not claimed. Dedicated test integrated in normal and sanitizer CI. Native Windows VST3, Quick4 capture and FL Studio checks pending. No UI change. No vendor execution or reconstruction.

Reproduce private benchmark: python tools/bench_tape_vocals.py --manifest /private/manifest.json --output /private/metrics.json (NumPy, SoundFile, GCC). Never commit the manifest, rows or recordings. Run sh tests/run_tests.sh for host-free checks.
