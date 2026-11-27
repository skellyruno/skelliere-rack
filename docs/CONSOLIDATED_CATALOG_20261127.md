# Consolidated rack catalog (2026-11-27)

32 selectable effects, 48 total historical/active processor identities. Existing IDs 1–34, host choice strings, knob automation and preset format are unchanged. The sixteen retired identities remain loadable, including factory and user presets; only the new-effect browser hides them. New IDs 35–48 are separate, not in-place algorithm replacements.

| Legacy | New catalog |
|---|---|
| Opto C (2), Program Opto (16) | Vox Rider (35), Opto C (16); the adaptive compressor already has the old threshold, ratio, attack, release, makeup plus sidechain HP |
| Air Lift (5), Air (26) | Air (41), static/dynamic |
| Bell 73 (19) | Dynamic EQ (36) |
| Sibilant (3), De-Esser (27) | De-Esser (42), classic/adaptive |
| Iron Gate (17), Smart Gate (34) | Noise Gate (43), manual/auto |
| Tape Echo (7), Slap 120 (25) | Tape Echo (44), free/synced, ping-pong, slap, ducking |
| Quad EQ (1), Tilt Tone (12) | Retired from catalog; SSL Strip and RubyQ remain |
| Plate Room (8), Warm Drive (4), Ember Clip (21), Stereo Stage (13) | New IDs 45–48, expanded modes |
| New | Multiband (37), Resonance (38), Doubler (39), Exciter (40) |

Multiband is a first six-band engine with shared controls, not full C6 parity or independent editable band settings. Resonance is an eight-detector relative-energy suppressor, not a commercial plugin clone. Doubler uses two modulated delay voices. Exciter uses high-pass harmonic residual. Room modes use differently scaled FDNs, not measured impulse responses. Clip has fixed 12-sample latency across modes and bypass; sample ceiling is not a certified true-peak guarantee. No commercial reference parity or DAW listening approval claimed.

Local verification on the modified source: cached JUCE 8.0.12 build, six real JUCE editor captures including 150% and edit modes, portable regression suite and targeted ASan/UBSan passed. All 34 legacy IDs rendered sample-exact versus the baseline on one three-second vocal excerpt at default settings. 42 host-free production DSP renders (14 new IDs × three private dry vocals, 44.1 kHz stereo, 257 sample blocks) finite, aligned, below 0 dBFS sample peak. Initial Doubler and Room wet-summing headroom failures corrected with linear blend and rerun. Ping-pong right-only input and exact repeat timing fixed. No private recordings are committed. Full 45-scene native audit, Windows VST3 CI and FL Studio audition remain pending. Existing factory presets remain unchanged.
