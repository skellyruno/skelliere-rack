# Tall strip hardware milestone

Baseline: draft PR #3, c09914a789cd6e2b7bb084d379fe58d2c4c925de. The previous Windows, native screenshot and portable CI workflows passed. All changes are editor geometry, materials, artwork and capture tooling; no audio processing equations, state schema, effect IDs or parameter IDs changed.

- Logical editor 1260 x 870 (previously 1260 x 790); rack panels 654 instead of 574 logical pixels. Retain four/five complete slots at the existing widths, zoom controls and paging. Scale the panel control grid and engravings together; preserve footer/fastener bounds. The screen size is larger; 80% scale remains available.
- Dope horizontal recessed A-Type badge at the left, separate left mode rocker, exact 105px vertical control pitch, output peak at right. The first render caught one-pixel rounding drift; corrected the grid rather than relaxing its test. MixHead's LED track uses the available taller meter window.
- SSL keeps its dedicated console-cap family with smaller caps and more vertical separation. Its compact two-column face and all 27 real controls are retained; this is not a variable-width slot implementation.
- Demon original procedural nine-frame skeletal relief: enlarged socket subtraction, hotter eyes, longer canines, greater jaw opening and claw reach, refined specular lighting. Larger window and frame overlap; stylized original geometry, not photographic reference parity.
- Shared panel wear and rail geometry tuned to the taller enclosure. Existing artwork/knob designs retained.

Validation: offline kit checksum check, cached native Linux JUCE editor build twice, final color and engine Quick4 sets 4/4 each, portable C++ suite exit 0 (including 24 identical SSL audio comparisons and 48 bypass cases). Final full editor audit tracked separately. No new Windows/FL Studio verification claimed before exact-head CI.
