# Craft Anything — independent Skelliere Rack lab

Status: **builder blocked in ChatGPT session**. Checked local PATH: craft-anything-cli, claude, codex and copilot absent. Attempted official v0.6.1 release URL (curl HEAD): DNS could not resolve github.com. JUCE/Quick4 were not present in checked local paths. No AI builder invocation or AI-generated candidate is claimed. Prior standalone Craft metadata CI passed on owned fixtures, not AI rebuilding. The current Rack draft's prior exact head a9cf5c333b27a6c6b427c2ce4dd437ac48d3dd8e passed build 45284224805, portable tests 45284224813, editor renderer 45284224882; no fresh screenshot inspection claim.

The existing research is now present in an isolated C++ lab under experiments/independent_dsp: four byte-identical independent source/test files from skellyruno/skell-vst-decompiler at ae2c64e307658a249b90167619c9671500bb9111. Their original Git blob IDs are:
- OriginalDynamics.h: 251f394b8fce8d0430398e80833e59ff10fa382f
- OriginalAdaptiveCompressor.h: db2dee3a75fad8732e2103e59b042f1b8fc9060b
- test_original.cpp: a2914f1223ce78b29f0fa5ea2ff306206d1c4b9c
- test_adaptive.cpp: bfdd8b22dd6c044ce2e3d7fa2cf6ef497cebc5fb
These are independent research implementations only; no vendor code, binaries, data, UI or reconstructed commercial DSP. New compare_dynamics.cpp measures the original adaptive, its fixed-timing control and current production ProgramOptoCore under the same input, at four sample rates. Different detectors/timing make opto a descriptive comparator, not a level-matched contest. No plugin integration, automation/state, type IDs, Tune latency, or UI changes. CMake supports a standalone lab; new tests added to existing Rack CI with sanitizers.

To use an authorized local agent after installing Craft Anything v0.6.1 and signing into the intended backend:

    sh tools/run_craft_dsp_lab.sh "Claude Code"

Use the exact configured backend name (Codex adapter may be unverified upstream). Set CRAFT_CLI to an executable path if necessary. Script copies only allowlisted original files into a fresh temporary folder, builds/tests baseline, runs Craft with target '-' and clean-room mode, existing edit, two rounds, then builds/tests candidate. It does not pass --allow-run or --unattended. Clean-room mode is not an OS sandbox: run with least privileges in an isolated environment; an agent may still access other files according to its permissions. No private references or credentials should be present there. Candidate stays in isolated folder and must be reviewed before integration. No automatic commits or merges. The local builder uses the user's configured agent and may consume their provider usage.

Current priorities: original FET-style transient handling, program-dependent release, dynamic bell frequency selectivity, low-alias color, sample-peak limiting with explicit delay, CPU and level-matched listening. Production Skelliere remains untouched until measured and approved.
