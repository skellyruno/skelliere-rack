# Delay rendering continuity — 2026-11-27

Baseline: 614e0eb81952a92b878d83d9e42a7fcdc1983fba.

- Keep an already audible Classic delay while an advanced model is pending or rejected. On a cold start with no valid wet engine, leave the signal dry rather than claiming the selected engine is audible.
- Ignore stale advanced publications while Classic is audible; crossfade the live Classic signal into a ready advanced model over the existing 5 ms transition. Return the exact target once settled.
- Existing DSP kernels, module IDs, parameter mappings and default presets are unchanged. Advanced-to-advanced and return-to-Classic tail handling still require host listening; long tails are not guaranteed to survive model changes.
- Portable state-machine regression passed at 32/44.1/48/96/192 kHz with optimized strict-warning and ASan/UBSan builds locally. Native-owner regression added but not run against the exact pinned owner engines in this session. Full Windows VST3, DAW, real audio, UI capture and full current CI remain required. Offline JUCE kit was recovered and verified; no dependency download. No private source or recordings included.
