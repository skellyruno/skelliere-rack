# Skelliere Rack 0.3.0 RACK-01

Continues draft PR #3 and feature/additional-rack-artwork; no identity, legacy
choice range, parameter ordering, factory preset or latency changes.

## Implemented

- Completed the unfinished extension path through production allocation,
  selection, library, host automation, state, undo/redo, A/B and Dream Strips.
- Added appended integer parameters s0_extra through s7_extra with fixed range
  0..127. Legacy type is authoritative unless Empty. Values 1/2/3/4 select Tilt
  Tone / Stereo Stage / Fast C / Bus C. Unassigned values resolve to Empty.
- Old states explicitly reset missing extra parameters to zero, including when
  loaded over an instance that previously used extensions.
- Tilt Tone: complementary broad shelves, pivot, output. Stereo Stage: M/S
  width, side high-pass mono-bass, output; mono input remains mono.
- Fast C: original linked peak compressor, 3 dB knee. Bus C: original linked RMS
  compressor, 9 dB knee. Both have threshold, ratio, attack, release and makeup.
  Neither claims proprietary circuit emulation or adds saturation/latency.
- Extensions use 20 ms control slew and 5 ms bypass fades; calculations are
  cached with a persistent 16-sample update cadence. Rack gain uses 20 ms
  smoothing independent of callback size. Legacy Opto C is preserved.
- Module library now scrolls independently to prevent new tiles covering help.
- Existing per-slot EQ/Air artwork preferences now save with the instance state.

## Artwork status

Production build still embeds the five existing supplied faceplates from
assets/air-eq-air-artwork.zip and assets/additional-effect-artwork.zip. These are
AirEQ Air, AirEQ Earth, CS-EQ, CS-Lift and Blackbird appearance-only choices.
Blackbird has no microphone processing. Printed artwork is separate from the
live labelled knobs; CHOOSE ARTWORK is at the bottom of each EQ/Air slot.

The four individually named archives were not found by current Library or Drive
filename searches. Drive root instead contains these newer full-resource packs:

- EFFECTS_FULL_PART_01.zip, Drive file 1do1NKdeYHLAv8A2s1vUqFOV9V2xO4FRP,
  87,433,930 bytes, modified 2026-11-23 11:37:29 UTC.
- EFFECTS_FULL_PART_02.zip, Drive file 1brKWXbYLayCIVpWUsj512n-joFZ01uH8,
  84,594,536 bytes, modified 2026-11-23 11:38:22 UTC.

Authenticated download references were obtained, but this host cannot resolve
the transfer host. Their bytes have NOT been inventoried, deduplicated, decoded
or embedded. Compressor artwork is therefore unfinished. Do not claim those
packs are thumbnail-only, full panels, or identical to older uploads without
opening their bytes. No fonts or DSP algorithms are assumed supplied/licensed.
To unblock, materialize both exact files into the working directory
work/artwork/ (keep them outside git); inspect every member, preserve hashes,
resolution/animation variants and provenance, then commit selected production
resources under assets/ with explicit per-image decoding metadata. Do not
blindly repack thumbnails or filmstrips.

## Verification

The previous head 7d84025 passed build, DSP and six-scene renderer CI; that did
not cover extension selection. This change adds production-factory regression
tests plus real JUCE processor state/automation/undo/snapshot checks in the real
editor renderer, with eleven editor scenes including new effects, scaling,
empty rack and horizontal scrolling. BinaryData renderer wiring is preserved.

Local MSVC legacy DSP, smoothing and production extension tests passed before
commit. Full JUCE builds/screenshots require the repository artwork archives,
which are not locally downloadable on this host; use the exact-commit CI.
ASan/UBSan is added to CI. CI results must be checked after pushing. No CPU
improvement, visual approval, FL Studio or pluginval pass is claimed here.

## Test build

Use PR #3 Actions artifacts for the exact commit: build -> SkellierRack-windows-latest.
Close the DAW, back up the current Skelliere Rack.vst3 bundle, replace it with
the complete artifact bundle in the existing VST3 directory, then rescan.
Do not change its filename, identity, internal bundle layout or overwrite a
loaded binary. The installed plugin has not been changed by this work.
Find new effects in the module library; double-click or drag to an empty slot.
Version label: v0.3.0 RACK-01. Treat this as an unfinished test build until
artwork integration, actual screenshot inspection and host tests pass.
