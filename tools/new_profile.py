#!/usr/bin/env python3
from pathlib import Path
import re
import sys

if len(sys.argv) != 3:
    print("Usage: python3 tools/new_profile.py <profile_id> <ABC>")
    raise SystemExit(2)

profile_id, short_name = sys.argv[1], sys.argv[2].upper()
if not re.fullmatch(r"[a-z][a-z0-9_]*", profile_id):
    raise SystemExit("profile_id must match [a-z][a-z0-9_]*")
if not re.fullmatch(r"[A-Z0-9]{1,3}", short_name):
    raise SystemExit("short name must be 1-3 ASCII letters/digits")

root = Path(__file__).resolve().parents[1]
out = root / "profiles" / f"{profile_id}.c"
if out.exists():
    raise SystemExit(f"Already exists: {out}")

symbol = f"uni_profile_{profile_id}"
out.write_text(
    f'''#include "../src/profile.h"\n\n/* TODO: document device/model, code provenance and hardware verification. */\nconst UniRemoteProfile {symbol} = {{\n    .id = "{profile_id}",\n    .name = "TODO",\n    .short_name = "{short_name}",\n    .transport = UniTransportInfrared,\n    .ir = {{0}},\n    .has_action = {{0}},\n}};\n''',
    encoding="utf-8",
)
print(f"Created {out.relative_to(root)}")
print(f"Next: add `extern const UniRemoteProfile {symbol};` and `&{symbol},` to profiles/registry.c")
