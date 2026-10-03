#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
EXAMPLES = ROOT / "examples"
ICON_SOURCE = ROOT / "src" / "icon_library.c"

GRID_W = 3
GRID_H = 6
MAX_PAGES = 16
MAX_ELEMENTS = 18
MAX_SIGNALS = 32
MAX_ACTIONS = 16
MAX_SEQUENCE_STEPS = 8
MAX_SEQUENCE_DELAY_MS = 60000
MAX_ACTION_ID_LEN = 23
MAX_SIGNAL_NAME_LEN = 31
FOCUSABLE = {"button", "hstep", "vstep", "dpad"}

BINDING_SUFFIXES = (
    "Tap", "Hold", "Up", "Down", "Left", "Right", "Ok",
    "UpHold", "DownHold", "LeftHold", "RightHold", "OkHold",
)
HARD_KEYS = ("HardUpHold", "HardDownHold", "HardLeftHold", "HardRightHold", "HardOkHold")

LG_STATE_OPS = {
    "power:toggle", "mode:+", "mode:-", "temp:+", "temp:-", "fan:+", "fan:-",
    "swing_v:+", "swing_v:-", "swing_h:+", "swing_h:-", "jet:toggle", "eco:toggle",
    "comfort:toggle", "display:toggle", "auto_clean:toggle", "purify:toggle",
    "jet_dry:toggle", "timer_on:+", "timer_on:-", "timer_on:toggle",
    "timer_off:+", "timer_off:-", "timer_off:toggle", "sleep:+", "sleep:-",
    "timers:clear", "unit:toggle", "diagnosis:send",
}
DAIKIN_STATE_OPS = {
    "power:toggle", "mode:+", "mode:-", "temp:+", "temp:-", "fan:+", "fan:-",
    "swing_v:toggle", "swing:toggle", "powerful:toggle",
    "timer_on:+", "timer_on:-", "timer_on:toggle",
    "timer_off:+", "timer_off:-", "timer_off:toggle", "timers:clear",
}
STATE_OPS = {
    "LG_AC": LG_STATE_OPS,
    "DAIKIN_ARC433A73": DAIKIN_STATE_OPS,
}
BT_MEDIA = {
    "play_pause", "next", "prev", "stop", "mute", "vol_up", "vol_down",
    "home", "back", "forward",
}
BT_KEYS = {
    "up", "down", "left", "right", "enter", "escape", "space", "tab",
    "page_up", "page_down",
}


class ProfileError(Exception):
    pass


def parse_kv(path: Path) -> dict[str, str]:
    data: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or ":" not in line:
            continue
        key, value = line.split(":", 1)
        key, value = key.strip(), value.strip()
        if key in data:
            raise ProfileError(f"{path}: duplicate key {key}")
        data[key] = value
    return data


def safe_basename(value: str, where: str) -> None:
    if not value or value in {".", ".."} or "/" in value or "\\" in value:
        raise ProfileError(f"{where}: must be a simple file name")


def parse_signal_names(path: Path) -> set[str]:
    names: set[str] = set()
    if not path.exists():
        return names
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if line.startswith("name:"):
            name = line.split(":", 1)[1].strip()
            if not name:
                raise ProfileError(f"{path}: empty signal name")
            if len(name) > MAX_SIGNAL_NAME_LEN:
                raise ProfileError(
                    f"{path}: signal {name!r} exceeds runtime limit {MAX_SIGNAL_NAME_LEN}"
                )
            if len(names) >= MAX_SIGNALS:
                raise ProfileError(f"{path}: more than {MAX_SIGNALS} signals")
            if name in names:
                raise ProfileError(f"{path}: duplicate signal {name}")
            names.add(name)
    return names


def parse_actions(path: Path, signals: set[str]) -> set[str]:
    if not path.exists():
        return set()
    data = parse_kv(path)
    if data.get("Filetype") != "Flipper Uni Remote Actions":
        raise ProfileError(f"{path}: wrong Filetype")
    try:
        count = int(data.get("ActionCount", "0"))
    except ValueError as exc:
        raise ProfileError(f"{path}: invalid ActionCount") from exc
    if not 0 <= count <= MAX_ACTIONS:
        raise ProfileError(f"{path}: ActionCount must be 0..{MAX_ACTIONS}")

    actions: dict[str, tuple[str, list[str]]] = {}
    for i in range(count):
        action_id = data.get(f"Action{i}Id", "")
        action_type = data.get(f"Action{i}Type", "")
        if not action_id:
            raise ProfileError(f"{path}: missing Action{i}Id")
        if len(action_id) > MAX_ACTION_ID_LEN:
            raise ProfileError(
                f"{path}: Action{i}Id exceeds runtime limit {MAX_ACTION_ID_LEN}"
            )
        if not re.fullmatch(r"[A-Za-z0-9_-]+", action_id):
            raise ProfileError(f"{path}: Action{i}Id contains unsupported characters")
        if action_id in actions:
            raise ProfileError(f"{path}: duplicate action id {action_id}")

        refs: list[str] = []
        if action_type == "sequence":
            try:
                steps = int(data.get(f"Action{i}StepCount", "0"))
            except ValueError as exc:
                raise ProfileError(f"{path}: invalid Action{i}StepCount") from exc
            if not 0 <= steps <= MAX_SEQUENCE_STEPS:
                raise ProfileError(
                    f"{path}: Action{i}StepCount must be 0..{MAX_SEQUENCE_STEPS}"
                )
            for step in range(steps):
                ref = data.get(f"Action{i}Step{step}", "")
                if not ref:
                    raise ProfileError(f"{path}: Action{i}Step{step} is empty")
                refs.append(ref)
                try:
                    delay = int(data.get(f"Action{i}Delay{step}", "0"))
                except ValueError as exc:
                    raise ProfileError(f"{path}: invalid Action{i}Delay{step}") from exc
                if not 0 <= delay <= MAX_SEQUENCE_DELAY_MS:
                    raise ProfileError(
                        f"{path}: Action{i}Delay{step} must be 0..{MAX_SEQUENCE_DELAY_MS}"
                    )
        elif action_type == "signal":
            signal = data.get(f"Action{i}Signal", "")
            if signal not in signals:
                raise ProfileError(
                    f"{path}: Action{i}Signal references unknown signal {signal!r}"
                )
        else:
            raise ProfileError(f"{path}: unsupported Action{i}Type {action_type!r}")

        actions[action_id] = (action_type, refs)

    ids = set(actions)
    graph: dict[str, list[str]] = {action_id: [] for action_id in ids}
    for action_id, (action_type, refs) in actions.items():
        if action_type != "sequence":
            continue
        for step, ref in enumerate(refs):
            if ref.startswith("sig:"):
                signal = ref[4:]
                if signal not in signals:
                    raise ProfileError(
                        f"{path}: action {action_id} step {step} references unknown signal {signal!r}"
                    )
            elif ref.startswith("act:"):
                nested = ref[4:]
                if nested not in ids:
                    raise ProfileError(
                        f"{path}: action {action_id} step {step} references unknown action {nested!r}"
                    )
                graph[action_id].append(nested)
            elif ":" in ref:
                raise ProfileError(
                    f"{path}: action {action_id} step {step} has invalid binding {ref!r}"
                )
            elif ref not in signals:
                raise ProfileError(
                    f"{path}: action {action_id} step {step} references unknown signal {ref!r}"
                )

    visiting: set[str] = set()
    visited: set[str] = set()

    def visit(action_id: str) -> None:
        if action_id in visited:
            return
        if action_id in visiting:
            raise ProfileError(f"{path}: recursive action cycle includes {action_id!r}")
        visiting.add(action_id)
        for nested in graph[action_id]:
            visit(nested)
        visiting.remove(action_id)
        visited.add(action_id)

    for action_id in ids:
        visit(action_id)

    return ids


def icon_ids() -> set[str]:
    if not ICON_SOURCE.exists():
        return set()
    text = ICON_SOURCE.read_text(encoding="utf-8")
    return set(re.findall(r'\{"([^"]+)"\s*,', text))


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
        lx, ly = cell_x - self.x, cell_y - self.y
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


def check_binding(
    binding: str,
    where: str,
    transport: str,
    adapter: str,
    signals: set[str],
    actions: set[str],
) -> None:
    if not binding:
        return

    if transport == "IR":
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
        if ":" in binding:
            raise ProfileError(f"{where}: binding {binding!r} is not valid for IR")
        if binding not in signals:
            raise ProfileError(f"{where}: unknown plain signal {binding}")
        return

    if transport == "STATE_IR":
        if not binding.startswith("state:"):
            raise ProfileError(f"{where}: STATE_IR binding must start with state:")
        op = binding[6:]
        allowed = STATE_OPS.get(adapter)
        if allowed is None:
            raise ProfileError(f"{where}: unknown StateAdapter {adapter!r}")
        if op not in allowed:
            raise ProfileError(f"{where}: unsupported {adapter} operation {op!r}")
        return

    if transport == "BT":
        if binding.startswith("bt:media:"):
            name = binding[9:]
            if name not in BT_MEDIA:
                raise ProfileError(f"{where}: unknown BT media binding {name!r}")
            return
        if binding.startswith("bt:key:"):
            name = binding[7:]
            if name not in BT_KEYS:
                raise ProfileError(f"{where}: unknown BT key binding {name!r}")
            return
        raise ProfileError(f"{where}: BT binding must use bt:media: or bt:key:")

    raise ProfileError(f"{where}: unknown Transport {transport!r}")


def validate_profile(folder: Path) -> list[str]:
    remote_path = folder / "remote.ur"
    if not remote_path.exists():
        return []

    data = parse_kv(remote_path)
    if data.get("Filetype") != "Flipper Uni Remote":
        raise ProfileError(f"{remote_path}: wrong Filetype")
    if data.get("Version") != "1":
        raise ProfileError(f"{remote_path}: unsupported Version")

    transport = data.get("Transport", "")
    if transport not in {"IR", "STATE_IR", "BT"}:
        raise ProfileError(f"{remote_path}: unsupported Transport {transport!r}")

    folder_name = data.get("Folder", "")
    if len(folder_name) >= 24 or "/" in folder_name or "\\" in folder_name:
        raise ProfileError(f"{remote_path}: invalid Folder")

    adapter = data.get("StateAdapter", "")
    if transport == "STATE_IR" and adapter not in STATE_OPS:
        raise ProfileError(f"{remote_path}: unsupported StateAdapter {adapter!r}")

    bt_profile = data.get("BluetoothProfile", "")
    if transport == "BT":
        if not re.fullmatch(r"[A-Za-z0-9_-]{1,23}", bt_profile):
            raise ProfileError(f"{remote_path}: invalid BluetoothProfile")

    for field, default in (("SignalFile", "signals.ir"), ("ActionFile", "actions.ur"), ("StateFile", "state.urs")):
        safe_basename(data.get(field, default), f"{remote_path}:{field}")

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
    if transport == "IR" and not signal_file.exists():
        raise ProfileError(f"{remote_path}: missing signal file {signal_file.name}")

    signals = parse_signal_names(signal_file)
    actions = parse_actions(action_file, signals)
    icons = icon_ids()

    for key in HARD_KEYS:
        check_binding(data.get(key, ""), f"{remote_path}:{key}", transport, adapter, signals, actions)

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
            check_binding(
                data.get(key, ""), f"{remote_path}:{key}", transport, adapter, signals, actions
            )

        for key, value in data.items():
            if key.startswith(prefix) and key.endswith("Icon") and value and value not in icons:
                raise ProfileError(f"{remote_path}:{key} references unknown icon {value!r}")

    used_pages = {e.page for e in elements}
    focus_pages = {e.page for e in elements if e.kind in FOCUSABLE}
    notes: list[str] = []
    empty_pages = [str(p) for p in range(page_count) if p not in used_pages]
    dead_pages = [str(p) for p in sorted(used_pages) if p not in focus_pages]
    if empty_pages:
        notes.append("empty pages: " + ", ".join(empty_pages))
    if dead_pages:
        notes.append("no focusable control on pages: " + ", ".join(dead_pages))
    return notes


def main() -> int:
    if not EXAMPLES.exists():
        print("No examples directory")
        return 0

    checked = 0
    try:
        for folder in sorted(p for p in EXAMPLES.iterdir() if p.is_dir()):
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
