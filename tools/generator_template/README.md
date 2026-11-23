# Portable quickstart fixture for new_plugin.py

This directory contains only the minimal **original SkellCore quickstart gain-plugin files** required by `tools/new_plugin.py` and its generator regression tests.

Copied byte-for-byte from `skellyruno/skellcore-vst-template` at reference commit `1588dc3b9592b19c21ace4eaf8fd49599ba503d0` (2026-11-23). The Skelliere Rack implementation in the top-level `Source/` is **not** a generator template. Keeping them separate prevents generated plugins from accidentally inheriting rack processor names or overwriting project identities.

When updating this fixture, compare against the upstream template and run `python tools/test_new_plugin.py` on Linux and Windows. Do not change Skelliere's top-level processor, CMake IDs, or published parameter mappings for generator compatibility.
