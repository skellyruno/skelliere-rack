#!/usr/bin/env python3
"""Normalize JUCE-generated VST3 moduleinfo.json without changing string literals.

Some JUCE versions emit trailing commas. Strict JSON parsers reject those files.
Only fix commas immediately preceding a closing bracket/brace, outside strings.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path


def without_trailing_commas(source: str) -> str:
    out = []
    quoted = escaped = False
    for i, ch in enumerate(source):
        if quoted:
            out.append(ch)
            if escaped:
                escaped = False
            elif ch == "\\":
                escaped = True
            elif ch == '"':
                quoted = False
        elif ch == '"':
            quoted = True
            out.append(ch)
        elif ch == ',':
            j = i + 1
            while j < len(source) and source[j].isspace():
                j += 1
            if j < len(source) and source[j] in "]}":
                continue
            out.append(ch)
        else:
            out.append(ch)
    return "".join(out)


def normalize(path: Path) -> bool:
    original = path.read_text(encoding="utf-8-sig")
    fixed = without_trailing_commas(original)
    json.loads(fixed)  # Fail the build if something else is invalid.
    if fixed != original:
        path.write_text(fixed, encoding="utf-8")
    return fixed != original


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build_dir", type=Path)
    args = parser.parse_args()
    files = [p for p in args.build_dir.rglob("moduleinfo.json")
             if any(segment.lower().endswith(".vst3") for segment in p.parts)]
    changed = sum(normalize(path) for path in files)
    print(f"Validated {len(files)} VST3 moduleinfo files; normalized {changed}")


if __name__ == "__main__":
    main()
