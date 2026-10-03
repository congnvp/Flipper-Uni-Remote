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
if "\n" in display_name or "\r" in display_name:
    raise SystemExit("display name must be one line")

root = Path(__file__).resolve().parents[1]
out = root / "examples" / remote_id
if out.exists():
    raise SystemExit(f"Already exists: {out}")
out.mkdir(parents=True)

(out / "remote.ur").write_text(
f"""Filetype: Flipper Uni Remote
Version: 1
Id: {remote_id}
Name: {display_name}
ShortName: {short_name}
Transport: IR
Order: 100
Favourite: false
Folder:
RepeatEnabled: true
PageCount: 1
IrBurst: 1
SignalFile: signals.ir
ActionFile: actions.ur
BluetoothProfile:
StateAdapter:
StateFile: state.urs
HardUpHold:
HardDownHold:
HardLeftHold:
HardRightHold:
HardOkHold:
ElementCount: 5
#
Element0Type: status
Element0Id: status
Element0Page: 0
Element0Rect: 0 0 3 1
#
Element1Type: screen
Element1Id: main
Element1Page: 0
Element1Rect: 0 1 3 2
Element1Label: READY
#
Element2Type: hstep
Element2Id: horizontal
Element2Page: 0
Element2Rect: 0 3 3 1
Element2Label: NAV
Element2Left: sig:Left
Element2Right: sig:Right
#
Element3Type: vstep
Element3Id: vertical
Element3Page: 0
Element3Rect: 0 4 1 2
Element3Label: VOL
Element3Up: sig:Up
Element3Down: sig:Down
#
Element4Type: button
Element4Id: power
Element4Page: 0
Element4Rect: 1 4 1 2
Element4Label: PWR
Element4Icon: pwr
Element4Tap: sig:Power
Element4Hold: sig:Mute
""", encoding="utf-8")

(out / "signals.ir").write_text(
"""Filetype: IR signals file
Version: 1
#
# Paste standard Flipper IR signal entries here.
""", encoding="utf-8")

(out / "actions.ur").write_text(
"""Filetype: Flipper Uni Remote Actions
Version: 1
ActionCount: 0
""", encoding="utf-8")

print(f"Created {out.relative_to(root)}")
print("Map single signals as sig:<name> and sequences as act:<id>.")
