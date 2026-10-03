#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
EXAMPLES = ROOT / "examples"

GRID_W = 3
GRID_H = 6
MAX_PAGES = 16
MAX_ELEMENTS = 18

BINDING_SUFFIXES = (
    "Tap", "Hold", "Up", "Down", "Left", "Right", "Ok",
    "UpHold", "DownHold", "LeftHold", "RightHold", "OkHold",
)
HARD_KEYS = ("HardUpHold", "HardDownHold", "HardLeftHold", "HardRightHold", "HardOkHold")


class ProfileError(Exception):
    pass


def parse_kv(path: Path) -> dict[str, str]:
    data: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        key = key.strip()
        value = value.strip()
        if key in data:
            raise ProfileError(f"{path}: duplicate key {key}")
        data[key] = value
    return data


def parse_signal_names(path: Path) -> set[str]:
    names: set[str] = set()
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if line.startswith("name:"):
            name = line.split(":", 1)[1].strip()
            if not name:
                raise ProfileError(f"{path}: empty signal name")
            if name in names:
                raise ProfileError(f"{path}: duplicate signal {name}")
            names.add(name)
    return names


def parse_action_ids(path: Path) -> set[str]:
    if not path.exists():
        return set()
    data = parse_kv(path)
    try:
        count = int(data.get("ActionCount", "0"))
    except ValueError as exc:
        raise ProfileError(f"{path}: invalid ActionCount") from exc
    ids: set[str] = set()
    for i in range(count):
        key = f"Action{i}Id"
        action_id = data.get(key, "")
        if not action_id:
            raise ProfileError(f"{path}: missing {key}")
        if action_id in ids:
            raise ProfileError(f"{path}: duplicate action id {action_id}")
        ids.add(action_id)
    return ids


@dataclass(frozen=True)
class Element:
    index: int
    kind: str
    page: int
    x: int
    y: int
    w: int
    h: int

    def occupies(self, cell_x: int, cell_y: int) -> bool:
        if cell_x < self.x or cell_y < self.y:
            return False
        lx = cell_x - self.x
        ly = cell_y - self.y
        if lx >= self.w or ly >= self.h:
            return False
        if self.kind == "dpad" and self.w >= 3 and self.h >= 3:
            return lx == self.w // 2 or ly == self.h // 2
        return True


def parse_rect(text: str, where: str) -> tuple[int, int, int, int]:
    try:
        values = tuple(int(v) for v in text.split())
    except ValueError as exc:
        raise ProfileError(f"{where}: Rect must contain integers") from exc
    if len(values) != 4:
        raise ProfileError(f"{where}: Rect must contain four integers")
    x, y, w, h = values
    if x < 0 or y < 0 or w <= 0 or h <= 0:
        raise ProfileError(f"{where}: invalid Rect {values}")
    if x + w > GRID_W or y + h > GRID_H:
        raise ProfileError(f"{where}: Rect {values} exceeds {GRID_W}x{GRID_H} grid")
    return x, y, w, h


def check_binding(binding: str, where: str, signals: set[str], actions: set[str]) -> None:
    if not binding:
        return
    if binding.startswith("sig:"):
        name = binding[4:]
        if name not in signals:
            raise ProfileError(f"{where}: unknown signal {name}")
        return
    if binding.startswith("act:"):
        action_id = binding[4:]
        if action_id not in actions:
            raise ProfileError(f"{where}: unknown action {action_id}")
        return
    # Backward-compatible plain signal name.
    if binding not in signals:
        raise ProfileError(f"{where}: unknown plain signal {binding}")


def validate_profile(folder: Path) -> list[str]:
    remote_path = folder / "remote.ur"
    if not remote_path.exists():
        return []

    data = parse_kv(remote_path)
    if data.get("Filetype") != "Flipper Uni Remote":
        raise ProfileError(f"{remote_path}: wrong Filetype")
    if data.get("Version") != "1":
        raise ProfileError(f"{remote_path}: unsupported Version")

    try:
        page_count = int(data.get("PageCount", "1"))
        ir_burst = int(data.get("IrBurst", "1"))
        element_count = int(data["ElementCount"])
    except (KeyError, ValueError) as exc:
        raise ProfileError(f"{remote_path}: invalid PageCount/IrBurst/ElementCount") from exc

    if not 1 <= page_count <= MAX_PAGES:
        raise ProfileError(f"{remote_path}: PageCount must be 1..{MAX_PAGES}")
    if not 1 <= ir_burst <= 8:
        raise ProfileError(f"{remote_path}: IrBurst must be 1..8")
    if not 0 <= element_count <= MAX_ELEMENTS:
        raise ProfileError(f"{remote_path}: ElementCount must be 0..{MAX_ELEMENTS}")

    signal_file = folder / data.get("SignalFile", "signals.ir")
    action_file = folder / data.get("ActionFile", "actions.ur")
    if not signal_file.exists():
        raise ProfileError(f"{remote_path}: missing signal file {signal_file.name}")

    signals = parse_signal_names(signal_file)
    actions = parse_action_ids(action_file)

    for key in HARD_KEYS:
        check_binding(data.get(key, ""), f"{remote_path}:{key}", signals, actions)

    elements: list[Element] = []
    for i in range(element_count):
        prefix = f"Element{i}"
        kind = data.get(prefix + "Type", "")
        if kind not in {"status", "screen", "button", "hstep", "vstep", "dpad"}:
            raise ProfileError(f"{remote_path}:{prefix}Type invalid: {kind!r}")
        if not data.get(prefix + "Id", ""):
            raise ProfileError(f"{remote_path}:{prefix}Id missing")
        try:
            page = int(data.get(prefix + "Page", "0"))
        except ValueError as exc:
            raise ProfileError(f"{remote_path}:{prefix}Page invalid") from exc
        if not 0 <= page < page_count:
            raise ProfileError(f"{remote_path}:{prefix}Page {page} outside PageCount {page_count}")

        rect_text = data.get(prefix + "Rect", "")
        if not rect_text:
            raise ProfileError(f"{remote_path}:{prefix}Rect missing")
        x, y, w, h = parse_rect(rect_text, f"{remote_path}:{prefix}")
        element = Element(i, kind, page, x, y, w, h)

        for other in elements:
            if other.page != element.page:
                continue
            for cy in range(GRID_H):
                for cx in range(GRID_W):
                    if element.occupies(cx, cy) and other.occupies(cx, cy):
                        raise ProfileError(
                            f"{remote_path}: Element{element.index} overlaps Element{other.index} "
                            f"on page {page} at cell {cx},{cy}"
                        )
        elements.append(element)

        for suffix in BINDING_SUFFIXES:
            key = prefix + suffix
            check_binding(data.get(key, ""), f"{remote_path}:{key}", signals, actions)

    used_pages = {e.page for e in elements}
    empty_pages = [str(p) for p in range(page_count) if p not in used_pages]
    notes = []
    if empty_pages:
        notes.append("empty pages: " + ", ".join(empty_pages))
    return notes


def main() -> int:
    if not EXAMPLES.exists():
        print("No examples directory")
        return 0

    folders = sorted(p for p in EXAMPLES.iterdir() if p.is_dir())
    checked = 0
    try:
        for folder in folders:
            if not (folder / "remote.ur").exists():
                continue
            notes = validate_profile(folder)
            checked += 1
            suffix = f" ({'; '.join(notes)})" if notes else ""
            print(f"PASS {folder.name}{suffix}")
    except ProfileError as exc:
        print(f"FAIL {exc}", file=sys.stderr)
        return 1

    print(f"Validated {checked} profile(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
