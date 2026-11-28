# Native rack controls — owner-source milestone

Base: `039422f3653955896f59c4da283b88f1a00cf8d9`. No upstream licensed engine source or private audio is included in this change.

## Changes

- Delay ENGINE panel: Classic/Modern/Bank, one to eight parallel staggered Bank taps, wet trim and active/preparing/budget status. Modern's true stereo/ping-pong modes are native; the rack handles stereo output exchange. Advanced engines are prepared by the non-audio service and exchanged using the native engine exchange. Reservations cover pending/current/fading/retired advanced engines; 128 MiB per engine, 384 MiB per rack instance for additional delay storage (not total RSS). The tempo floor is 30 BPM, and model preparation supports 32–192 kHz integer sample rates. Previous Classic path remains available. Long tails are not guaranteed to survive model changes. Preparation can cost message-thread time; DAW stress testing remains needed.
- Glitch PATTERN panel: 1–32 steps (Off/On/Accent/Reverse), step probability, 4 reverse styles, gate rate, swing, chaos, seed, seed lock, Auto/MIDI/Manual trigger, wet trim. Full-band only. MIDI note-on positions are honored within processing blocks. New Glitch slots use -1.3 dB wet trim; old sessions use 0 dB. This is not a peak limiter.
- 76 append-only normalized controls per slot; original IDs and ordering preserved. New controls participate in slot moves, undo and strip/host state recall. Owner-source availability remains opt-in; public catalog remains 31 effects.

## Verification (Linux, pinned JUCE 8.0.12)

- Legacy native reference suite: all 4 Flang modes (archived engine), Classic Delay, Glitch gate passed at 44.1, 48, 96 kHz. Maximum errors 2.98e-8, 1.49e-8, and 0 respectively.
- New rack-adapter test: Modern and two-tap Bank at 44.1/48/96/192 kHz; maximum errors 1.49e-8 and 7.45e-9. Callback allocation/deletion counters zero. Classic restoration releases advanced reservation. Pattern, probability and manual trigger checks passed. ASan/UBSan passed on both native test programs.
- Public portable regression passed. Real JUCE host-state regression and editor captures passed with re-linked cached JUCE objects; a redundant full renderer recompilation was killed by the memory limit. Inspect native-delay.png and native-glitch.png after rendering. No Linux VST3 release was produced.

## Windows owner build

With Visual Studio C++ build tools, CMake, Python, JUCE 8.0.12, and the exact owner-provided `flang/`, `delay/`, `glitch/` sources matching both `tools/native_*.sha256.json` manifests, run:

```powershell
powershell -ExecutionPolicy Bypass -File tools/build-owner-windows.ps1 -JucePath C:\Skell\JUCE-8.0.12 -NativeSourceRoot C:\Skell\native -PluginvalPath C:\Tools\pluginval.exe
```

The script is an unexecuted Windows build recipe, not a compiled VST3. It builds the plugin and native renderer, verifies moduleinfo, runs editor checks and optional pluginval, and packages a private test build. FL Studio validation, listening tests, and Windows execution are outstanding. Latest Flang revision `fad039cdcfc6118288168418b6ea16ab59d6cbb3` remains unintegrated because the exact upstream header was not transferred; the pinned compact archived Flang implementation is unchanged. Do not distribute owner-only dependencies or binaries as a public release.
