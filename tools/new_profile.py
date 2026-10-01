#!/usr/bin/env python3
from pathlib import Path
import re
import sys

if len(sys.argv) != 4:
    print('Usage: python3 tools/new_profile.py <remote_id> "Display Name" <ABC>')
    raise SystemExit(2)

remote_id, display_name, short_name = sys.argv[1], sys.argv[2], sys.argv[3].upper()
if not re.fullmatch(r"[a-z][a-z0-9_]*", remote_id):
    raise SystemExit("remote_id must match [a-z][a-z0-9_]*")
if not re.fullmatch(r"[A-Z0-9]{1,3}", short_name):
    raise SystemExit("short name must be 1-3 ASCII letters/digits")
if "
" in display_name or "" in display_name:
    raise SystemExit("display name must be one line")

root = Path(__file__).resolve().parents[1]
out = root / "examples" / remote_id
if out.exists():
    raise SystemExit(f"Already exists: {out}")
out.mkdir(parents=True)

(out / "remote.ur").write_text(
f'''Filetype: Flipper Uni Remote
Version: 1
Id: {remote_id}
Name: {display_name}
ShortName: {short_name}
Transport: IR
Order: 100
RepeatEnabled: true
SignalFile: signals.ir
BluetoothProfile: {remote_id}
ElementCount: 5
#
Element0Type: status
Element0Id: status
Element0Rect: 0 0 3 1
#
Element1Type: screen
Element1Id: main
Element1Rect: 0 1 3 2
Element1Label: READY
#
Element2Type: hstep
Element2Id: horizontal
Element2Rect: 0 3 3 1
Element2Label: NAV
Element2Left: Left
Element2Right: Right
#
Element3Type: vstep
Element3Id: vertical
Element3Rect: 0 4 1 2
Element3Label: VOL
Element3Up: Up
Element3Down: Down
#
Element4Type: button
Element4Id: power
Element4Rect: 1 4 1 2
Element4Label: PWR
Element4Tap: Power
Element4Hold: Mute
''', encoding='utf-8')

(out / "signals.ir").write_text(
'''Filetype: IR signals file
Version: 1
#
# Replace these placeholders with signals captured from the target remote.
''', encoding='utf-8')

print(f"Created {out.relative_to(root)}")
print("Copy real Flipper IR entries into signals.ir and keep element bindings matched to signal names.")
