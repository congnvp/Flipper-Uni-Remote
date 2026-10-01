#!/usr/bin/env python3
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
version = (root / "VERSION").read_text(encoding="utf-8").strip()
match = re.fullmatch(r"(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)", version)
if not match:
    print(f"VERSION is not SemVer MAJOR.MINOR.PATCH: {version}", file=sys.stderr)
    raise SystemExit(1)

manifest = (root / "application.fam").read_text(encoding="utf-8")
manifest_match = re.search(r'fap_version\s*=\s*"([0-9]+\.[0-9]+)"', manifest)
if not manifest_match:
    print('application.fam does not contain fap_version="MAJOR.MINOR"', file=sys.stderr)
    raise SystemExit(1)

expected = f"{match.group(1)}.{match.group(2)}"
if manifest_match.group(1) != expected:
    print(
        f"Version mismatch: VERSION={version}, application.fam={manifest_match.group(1)}; expected {expected}",
        file=sys.stderr,
    )
    raise SystemExit(1)

print(f"Version OK: {version} (FAP manifest {expected})")
