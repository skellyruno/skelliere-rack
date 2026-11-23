# Skelliere Rack

A virtual mix rack for vocals: eight 500-series style slots (A to H), signal left to right, built on the SkellCore engine
from the Skell VST template. VST3 and Standalone on Windows, macOS and Linux, plus AU on macOS.

## What is in it

Eleven modules, any order, any slot:

| Module | What it is |
|---|---|
| Skelly Tune | SkellyTune real-time pitch correction with key and scale (adds fixed latency, the host compensates) |
| Quad EQ | High-pass, low shelf, two peaking mids, high shelf |
| Opto C | Compressor with needle gain-reduction meter |
| Sibilant | Phrase-aware split-band de-esser (SkellCore detector) with needle meter |
| Warm Drive | Tube-style saturator, tone and mix |
| Air Lift | High-shelf enhancer |
| Twin Chorus | Two-voice chorus |
| Tape Echo | Tone-shaped feedback delay |
| Plate Room | Reverb (SkellCore feedback delay network) with pre-delay and damping |
| Brick 1 | Zero-latency peak limiter with needle meter |
| Trim | Gain and pan |

Interface: module library (search, categories, double-click or drag into a slot), drag a slot header to move it
(Alt or Option-drag copies), power and solo per slot, rack presets, A/B snapshots, Dream Strips (your own saved racks,
right-click a strip to delete it), undo and redo for rack edits, Bypass All, 80/90/100/125% size, input and output
trim with meters.

Every slot is plain host parameters (module, on/off, six knobs), so DAW project recall and automation work.
Limits in this first version: only the first Skelly Tune slot is active. Bypass All now preserves the pitch engine's fixed reported latency
so bypassed and processed material stay aligned.

## Build

You need CMake 3.22 or newer and a C++17 compiler (Visual Studio 2022 Build Tools on Windows, Xcode command line
tools on macOS, gcc plus the JUCE Linux packages on Linux). The first configure downloads JUCE 8.0.12.

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --parallel

The VST3 appears under `build/SkellierRack_artefacts/Release/VST3/`. Copy it to your VST3 folder
(Windows: `C:\Program Files\Common Files\VST3`). To use a JUCE you already have, add `-DJUCE_PATH=/path/to/JUCE`.

No local toolchain? Push this folder to a GitHub repository. The workflow in `.github/workflows/build.yml` builds all
three platforms and attaches the plugins as downloads.

## Test the DSP without JUCE

    sh tests/run_tests.sh

Checks every module for finite output (mono and stereo, 44.1 and 96 kHz, block sizes from 1 to 4096, extreme knobs),
then EQ gain, compression amount, limiter ceiling, de-esser targeting, delay time, reverb decay, pan, and pitch snapping.

## Layout

- `Source/Modules.h` one table for every module: names, knobs, ranges, defaults, printed sections
- `Source/Dsp.h` the DSP, real-time safe (nothing allocates in the audio callback)
- `Source/PluginProcessor.*` slots, parameters, presets, undo, snapshots, Dream Strips
- `Source/Look.h` the whole look (colours, knobs, screws, meters) in one place
- `Source/PluginEditor.*` the interface
- `SkellCore/` DSP engine from the template (see its own licenses folder)

## Original DSP research / Program Opto (experimental)

[Read the TVB/VMR static-DSP deep dive and Skelliere signal-flow comparison](docs/DSP_REFERENCE_DEEP_DIVE_2026-11-23.md). This is a source-grounded architecture study: original vendor DSP implementations were **not** recovered or copied.

New optional **Program Opto** is an independent zero-latency linked compressor with six controls: Thresh, Ratio, Attack, Release, SC HP, Makeup. It uses RMS-style detection and program-dependent gain-reduction memory. Find it as extension choice 5 (s{slot}_extra=5) when the legacy choice is Empty. **The old Opto C, all legacy IDs and factory preset identities remain unchanged.** This remains experimental until real vocal listening, Windows pluginval and FL Studio validation.

Run host-free checks with sh tests/run_tests.sh. For the optional math-only alias study, install numpy/scipy, then run python tools/skelliere_saturation_probe.py. The oversampled math reference is not installed plugin DSP.
