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

## 2026-11-25 actual artwork correction

Prior exact-head captures showed miniature static faceplates with empty knob holes and unrelated labels beside the live knobs. The embedded PNGs were extracted directly from the verified Windows binary and matched SHA-256 hashes in the CI archive inventory. Four use packed 215-pixel rows; Blackbird instead uses 216-pixel rows, yielding 567 complete rows and a discarded partial final row. The 216-stride recovery visibly removes the diagonal corruption. Its printed microphone controls do not correspond to any rack DSP. Preserve all original files and saved art IDs 0–3. Use inspected label-free source material regions as integrated full-width control surfaces, not fake faceplate controls. Keep actual APVTS-bound controls and current DSP, use appropriate contrast on light surfaces, remove miniature art and redundant artwork descriptions. This is material-skin integration, not a complete sprite/filmstrip reconstruction or vendor effect implementation. Additional archive filmstrips and compressor art require separate asset inspection. Also fix Program Opto's clipped last control using available-height layout and add geometry checks for all captured occupied slots plus scale scenes. Local JUCE unavailable; exact new CI and actual image inspection remain release gates.

## Full asset pack recovery and live control sprites (Nov 25)
Both supplied Drive ZIPs materialized and passed ZIP integrity checks: 665 files, 620 PNGs (595 unique hashes), 12 module families. Every one of the 109 existing repo archive members matched the full pack path and SHA-256. Visually inspected first/mid/last frames and verified the unusual contiguous-frame packing (source apparent width = frame height * count, apparent height = frame width), not horizontal cropping. Preserve original archives. Actual normalized APVTS sliders select all 65 source frames; power buttons select two frames from their real attachment. Slider drag/reset/automation and parameter IDs are retained; Trim can display the supplied moving fader. No fake microphone or vendor DSP functionality. Default public build uses existing embedded AirEQ, CS-EQ, CS-Lift, Blackbird art. Optional private build with `-DSKELLIERE_FULL_ASSET_DIR=/path/containing/both/ZIPs` selects available 2x resources and adds Custom Opto, FG-2A, FG-116 classic/modern/vintage, FG-401 and Classic Instruments visuals mapped only to existing equivalent controls. Do not publish newly recovered third-party image bytes without redistribution rights. Full-pack mode has not been JUCE-compiled or visually verified locally (dependency download DNS blocked). All original XML, alternates, incompatible labeled switches, meter faces and mic panels retained in the original packs; not every unique image is a meaningful control in the rack. Added host-free frame math tests and real-editor endpoint/attachment scenes. Exact CI still required; screenshot success alone is not visual approval.

Local checks: exact SpriteFrames/test Git blobs c33bc61188be57c6ee60b46dd19e87cf47e06898 and 0c837ffed1fd8b0ca722012b784557ea21e30650 passed optimized C++17 warnings-as-errors and ASan/UBSan. All 24 selected resource IDs had both 1x and 2x variants, expected dimensions, and every frame in each variant had a distinct SHA-256 (2490 frame instances total). Representative CMake path resolution tested against local full archives and reconstructed hash-matched old archive members. Atlas storage now uses a bounded 9-column grid rather than a 65-frame vertical texture; fader track uses measured original handle centre/endpoints; preserve compressor GR readout beside the skin selector. Full JUCE/FL Studio interaction still unverified.

## Native full-faceplate layout reconstruction (Nov 25)
Recovered original staticcanvas XML for 11 of 12 supplied families (Vintage missing separate XML; adapted Modern geometry). Inspected all 48 1x full-panel candidates and selected actual full panel artwork for each family. Air/Earth/CS-EQ/CS-Lift/FG-116 classic/modern/vintage/FG-401 use stride 215; Blackbird, Classic Instruments, Custom Opto and FG-2A use 216 at 1x. The latter four have residual diagonal corruption in their supplied 2x conversion; use 1x, not an unverified 2x decoder. NativeFaceLayout holds separate panel-coordinate control, switch and meter geometry. Complete decoded panels render at natural aspect ratio in variable-width rack slots; original XML positions retained where compatible, and clearly labeled inserts remove unsupported scales/holes. No DSP or parameter IDs modified. Original meter enclosures retained, live GR scale overlaid (not yet original calibrated needle animation). Blackbird and Classic Instruments are appearance-only adaptations, not microphone models. Public CI uses previously embedded artwork only; additional seven full panels require local full-pack configuration. Full pack not locally JUCE compiled (DNS blocked). New 150% scene and existing min/max/80/100/125% checks; exact new screenshots still require inspection. The inserts and adapted control positions are first-pass reconstruction, not pixel-exact reproductions; no claim of finished all-asset integration.

Follow-up: preserve original CS-EQ horizontal separators and the right-hand gain column; readouts occupy the former unused frequency-control positions. Mask only incompatible artwork areas with local matching material, retaining the complete original plate underneath. Corrected two source-canvas switch/control intersections (CS-Lift, Custom Opto) and added host-free all-layout geometry test for 14 type/appearance combinations, including private full-pack layouts. Native screen captures still required.

## Seamless restored faceplate batch (Nov 25)
- Removed the runtime rectangular-patch loop from `SlotComponent::paintNativeFace`; now renders one per-appearance cached full-size source-derived panel with matching brushed grain sampled from the original PNG, preserving perimeter hardware and original VU enclosure and erasing incompatible baked control holes, legends and vendor marks without changing the supplied archive. No new third-party bitmap bytes committed. Retains CS-EQ/CS-Lift hardware divider lines as native geometry and draws clear Skelliere identification and APVTS-bound control labels.
- Replaced library thumbnail source with the same restored panel. Screenshot regression checks ensure restored panel dimensions match original and the original center has actually changed. All UI-only; production DSP, parameters, state, IDs, factory presets and PDC untouched.
- Remaining: the source-derived texture generator must be visually inspected at 80/100/125/150% in real JUCE screenshots. Full-pack assets remain optional because this repository is public. Original calibrated meter filmstrips and private full-pack JUCE build are not complete.

Follow-up batch: fixed screenshot test false-positive for Blackbird's intentionally preserved upper chamber by sampling its cleaned lower half; simplified oversized empty slots to minimal recessed chassis panels and a restrained plus/drop target, removing giant slot letters. All original controls, DnD and processor state remain unchanged. Exact-head tests and screenshot verification required.

Real UI screenshot audit (bbe5cf0, 19 validated scenes): coherent single-material faceplates and minimalist empty modules confirmed on actual Windows JUCE captures; visible residual dark sample contamination above metal faces and unused lower Blackbird chamber found. Follow-up moves Blackbird's existing live mid/high/frequency controls into the lower source chamber without adding DSP/parameters, changes brushed-metal sampling to inspected clean side strips (no old logo pixels), and adds the actual animated source knob to each artwork-based library tile preview. New exact-head build and screenshot inspection required before approval.

Final header/faceplate cleanup after visually inspecting all 19 d8f95d6 real JUCE captures: preserved Blackbird two-chamber live controls and inspected source sprite thumbnails. Found 2-3px residual vendor-mark pixels at the top edges of both bright metal panels; the per-pixel source-derived restorer now cleans the complete height while leaving source corner screws and perimeter sides protected. Removed development/status text overlapping lower rack rail and updated initial LCD/help text for user-facing product. No parameter/DSP/state/binary identifier changes. Exact-head Windows build and editor screenshot inspection required before calling visual cleanup verified.

## Private full-pack runtime, scanline recovery and meters (Nov 25)
- All seven additional faceplate families can be loaded by the normal VST3 using a user-local artwork-v1 folder under the OS user application-data directory; optional SKELLIERE_ARTWORK_DIR override, or legacy private CMake embedding. No new source artwork committed. Restart the DAW after installing; absent assets fall back to the existing embedded/default appearance, without altering saved choices/IDs. Source artwork stays local.
- Recovered four skewed RGB families: original data includes four-byte row alignment. At 1x, row stride 648 bytes (215 RGB pixels +3 bytes); at 2x 1292 bytes (430 RGB pixels +2 bytes). Converted source truncates trailing data: 567 or 1138 complete rows. Visually inspected all eight decoded source images; no diagonal skew. Preserve only complete rows. RGBA families remain tightly packed. High-DPI panel restoration now scales samples and corner/chamber masks correctly.
- Recovered original GR scale, bezel and optical glass assets for six compressor appearances. New drawn needle (not a claimed original needle sprite) follows manually measured scale tick centres 0/1/3/5/7/10/20 dB, based on actual processor GR, with separate 45ms attack/240ms release UI smoothing. No DSP changes. Misleading original COMPRESS/LIMIT, HP and microphone selectors are not mapped to bypass; native generic power button remains where needed.
- Added portable packing/meter tests, public Windows pluginval strictness 5 CI step, measured production editor snapshot and audio block benchmark CSV (48k/512, eight modules; snapshot time is not actual display frame rate; memory is process-level). Full-pack renderer scenes selected at runtime. Actual full-pack host/visual verification requires local assets installed on Windows or private renderer; FL Studio testing not done by CI.
