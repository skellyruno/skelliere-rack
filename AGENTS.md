# Skelliere Rack agent instructions

Follow the shared [`SkellWorld/AGENTS.md`](https://github.com/skellyruno/SkellWorld/blob/main/AGENTS.md) baseline. This document is additive and applies to this plugin.

- Preserve the current VST3 plugin ID (`SkAu` / `SkRk`), bundle ID, existing APVTS parameter IDs (`in`, `out`, `bypass`, `s{slot}_type`, `s{slot}_on`, `s{slot}_k{knob}`), type indices and factory preset names. Migration tests are required before incompatible changes.
- The rack contains eight slots with 27 module types (11 legacy and 16 extensions); only the first Tune module is processed. Do not silently reassign extra Tune instances or change the one-Tune latency contract.
- Any bypass path must match the pitch engine's reported latency when a Tune slot exists. Check mode transitions and both mono/stereo with odd block sizes.
- Run `sh tests/run_tests.sh` plus ASan/UBSan whenever DSP is modified. Test against real vocals and a DAW before making any sound-quality or release claims.
- Check JUCE dependency availability, real VST3 builds, pluginval/FL Studio, and SkellCore/Quick4 real editor screenshots where supported. No mockup screenshots.
- Use feature branches and draft PRs; never merge an unverified change. Only cancel superseded pull-request CI runs. Preserve successful main, release and manually triggered jobs.
- Check `SkellCore/SOURCE_PROVENANCE.md` and license terms before distributing bundled upstream source in a public repository. Do not assume that all included engines are MIT just because the Tune files are.

- Build only Windows VST3 in routine plugin CI. Retain portable DSP/sanitizer tests on Ubuntu and real Windows editor screenshot verification; do not add macOS or Linux plugin builds unless requested.
