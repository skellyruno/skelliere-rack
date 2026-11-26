# Polish pass 2026-11-26 (on top of v0.3.0 / 83a0575)

Status is split into implemented, verified, **not built**, and unverified. Nothing in this
pass was compiled against JUCE, rendered, or run in a DAW. Read the "Not verified" section
before building.

## Environment limits of this pass
No JUCE, CMake, network, or Windows/MSVC toolchain was available. Consequently:
- **No Windows VST3 was built.** The only VST3 is the original verified baseline.
- **No real editor screenshots were captured.** `Evidence/real-ui/*` are still the *old* (pre-pass) captures.
- `pluginval`, FL Studio and a real-vocal listening test were not run.

## Implemented (source edits, uncompiled against JUCE)
Only drawing code and one UI-thread accessor changed. No parameter, ID, range, preset, state,
latency or DSP-process code was touched.

**`Source/Look.h`**
- Grain tile: 512 px, long irregular horizontal fibres plus fine tooth, wrapped at the seam.
  The old tile was multiplied by an effective alpha of about 0.008 (0.025 x 0.32), i.e. invisible;
  measured plate texture was std 0.9 with zero horizontal variation vs 9-12 in the reference.
- `machinedCap`: replaced the `cos(a)`/`cos(2a)` lobe mix (the "fan" reflections) with a concentric
  lathe grain, soft directional gradient, groove with lit lower lip, chamfered bevel, dark edge.
  Dark caps get their own tint curve so form stays readable.
- New `knobBody(cap)`: cached per cap colour. Soft ground shadow, contact occlusion, dark
  52-flute knurled sidewall with lit/shaded flanks, rim highlight, seam, and the cap.
  `drawRotarySlider` now draws that image, then scale marks, then only the pointer. Same
  geometry (R = 0.385 x size), so hit areas, drag, fine-adjust, double-click reset, tooltips,
  discrete steps and automation are unaffected.
- Scale: three engraved ticks (0/50/100 %) plus dots, sized from the knob radius instead of 1-1.45 px.
- Pointer: rounded-cap inlaid bar with shadow and lit lower edge, width scales with knob.
- `paintScrew`: recess, three-stop head gradient, slot, specular.
- `paintVU` (Opto, Jet and every other meter): dark machined housing, recessed face, four bezel
  fasteners, bezel shadow on the face, face and glass clipped to the **rounded** face (the old
  glass was a square rectangle leaking into the rounded corners), curved lens reflection,
  thinner needle with shadow, counterweight tail and a shaded pivot hub.
  **Scale, ticks, labels, ranges and calibration are unchanged** (Opto/GR 0-20 dB;
  Jet peak -20..+4 dBFS).

**`Source/OriginalPanel.h`**
- `paintOriginalSurface`: near-flat body colour (the old +13 % / -14 % vertical ramp caused the
  broad banding), soft upper-left specular pool, edge vignette, visible fibre grain, lit
  top/left and shaded bottom/right bevel, engraved double panel line. Dark finishes are scaled to
  0.84 brightness (measured 20-50 levels brighter than the reference); light finishes
  (champagne Opto, measured within ~8/255) are untouched.
- `paintTapeDetail`: machined bezel with fasteners, dark cavity, wound-tape reels, machined flange
  with three curved pie-segment openings, brass hub, tape path round guide posts and a head
  block, curved glass. Still decorative; it does not depict tape physics.
- `paintOriginalMini`: distinct glyphs per family (EQ curve, Bell, Tilt, band-pass, saturation
  curve, clipped sine, chorus/phase, tremolo, reverb decay, Air chevrons, Tune steps, Stereo,
  Trim slider, Impact envelope) instead of one repeated cap icon. Meter and reel icons kept.

**`Source/PluginProcessor.cpp`** - one real defect fixed. `gainReduction(slot)` now returns 0 while
Bypass All is on. Previously `processBlock` returned before running modules, so each module kept
its last stored reduction and the Opto-style needles stayed deflected while bypassed.

**`tests/ProcessorRegression.h`** - regression for the above (drives Opto to > 1 dB GR, asserts 0
while bypassed). **Uncompiled.**

## Verified here (host-free, g++ 13, reproducible with `tools/dsp_audit/run_audit.sh`)
- `sh tests/run_tests.sh`: passes before and after (no DSP code changed).
- Syntax/type check of `Look.h` and `OriginalPanel.h` against a *hand-written minimal JUCE stub*
  (`g++ -fsyntax-only -Wall -Wextra`, zero diagnostics). This catches typos and scoping errors in
  my code only. **It proves nothing about real JUCE signatures or behaviour.**
- The `machinedCap` pixel shader was ported line-for-line to numpy to sanity-check its maths
  (no NaN, bounded output, no fan pattern, dark-cap form visible). `cap_shader_math_check.png`
  is that port, **not** a plugin screenshot.

## DSP audit findings (measured)
1. **Ember Clip aliases.** Production module vs an 8x-oversampled reference, alias energy re the
   fundamental: 1 kHz/12 dB drive -121 dB; 5 kHz/12 dB -38 dB; 7 kHz/12 dB -32 dB; at 24 dB drive
   -10 to -20 dB for 4-7 kHz. Real, but mostly relevant to bright or heavily driven material.
2. **Not fixed, deliberately.** First-order ADAA (zero latency) cuts alias by ~9-17 dB, but dulls
   the *idle* response (-2.0 dB at 10 kHz, -5.1 dB at 15 kHz) and is half a sample late versus the
   dry path at Mix < 100 %. A blend of base-rate and ADAA is worse than either (-18 vs -38 dB at
   5 kHz). Fixing it properly needs oversampling (latency/PDC change) or an opt-in mode (new
   parameter). Both are outside this pass's rules. Decision needed from the owner.
3. **Per-sample `sin`/`tan` in modulators are not a hotspot.** All 25 modules cost 2-262 ns per
   stereo frame at 48 kHz. Pulse 15, Jet 26, Phase Coil 46 ns (0.07-0.22 % of one core). Heaviest:
   Tune 1.26 %, Iron Gate 0.56 %. No change made.
4. **GR meters** return to 0 within 0.1 s when a slot is switched off or solo-muted, for all
   seven GR-reporting modules. Program Opto holds 6.8 dB after 1 s silence from an extreme 32 dB
   reduction, consistent with its documented slow memory; left as is. The only stale case found was
   Bypass All (fixed above).
5. **Jet peak** path reviewed: `getMagnitude(0,len)` is the JUCE all-channel overload, read once
   per slot after Jet, zeroed when off/solo-muted/bypassed/removed, non-blocking CAS. No defect.

Timings are one container core, host-free, single run of best-of-3; not FL Studio CPU.

## Not verified / blocked
- Compilation against JUCE 8.0.12, MSVC Windows x64 VST3, pluginval level 5, SkellCore sanitizers
  on the new code, and `ProcessorRegression.h` execution.
- Every visual claim above. The code is written to match measured differences from the reference,
  but no pixel of the real editor has been seen. Knob size/tick/pointer proportions, the 52-flute
  density at small sizes, tape-reel fit inside the 118 px window, glyph legibility at 40 px,
  and contrast on all four finishes **must be checked on real captures**.
- All 25 effects x 80/90/100/125/150 %, endpoints, bypass, empty/full rack, finishes, search/scroll,
  drawer interactions.
- Not attempted (needs compile-and-look iteration): header/library/signal-path restyling items 5-6
  of the brief (Bell value pills and separators, title tracking, serif model titles, drawer
  compaction, pill-shaped signal-path buttons). `PluginEditor.cpp` is untouched.
- FL Studio and real-vocal listening: unavailable; no listening claims.

## Suggested next step
Build with the Quick4 renderer, capture all scenes, and compare against the `Evidence/real-ui`
baselines. Revert any single function in `Look.h`/`OriginalPanel.h` independently if it looks wrong;
they are self-contained. Then do the PluginEditor items listed above.

## Integration note
Imported the returned Claude source ZIP atop the existing draft branch, preserving the concurrent Skell Air and Skell De-Esser additions (IDs 26 and 27). The seven catalog groups cover all 27 effects. Updated stale 25-module processor assertion and catalog labels. All twelve imported file hashes were verified before adapting the conflicting palette and count assertions. This import does not alter audio algorithms or host parameter IDs. Exact-head real editor captures and host testing remain required.

## Second Studio refinement (Nov 26)
Visually inspected exact 2dab75b native approved_studio screenshot and compared against the approved concept (not a working screenshot). Baseline 48k/512 8-module p95 processing 0.1938 ms (1.81688% block), p95 full snapshot 62.0607 ms, working set 130383872 bytes. Refined dark metal texture and edge contrast, brighter anisotropic cap highlights, narrower rim and pointer, warmer fasteners, enlarged tape reels, deeper meter bezel/scale, title and label tracking, real Bell parameter value pills, quieter chassis, hardware library search/cards and cached thumbnails, full-width signal strip reflecting bypass/solo, and I/O meter capture from real processed signal. GR accessor now suppresses stale disabled and solo-muted readouts without changing audio, with production regression. Added Graphite/Midnight/80% and eight-slot screenshots (21 total), dynamic exact-head source export, Windows-only plugin checks unchanged. No sound/parameter/preset modifications; actual new render and benchmark required before claiming visual improvement. FL Studio and real-audio listening unverified.
