#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
from tempfile import TemporaryDirectory

import check_profiles as cp


BASE_REMOTE = """Filetype: Flipper Uni Remote
Version: 1
Id: test_remote
Name: Test Remote
ShortName: TST
Transport: IR
Order: 1
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
ElementCount: 1
#
Element0Type: button
Element0Id: power
Element0Page: 0
Element0Rect: 0 0 1 1
Element0Label: PWR
Element0Tap: sig:Power
"""

SIGNALS = """Filetype: IR signals file
Version: 1
#
name: Power
type: parsed
protocol: NEC
address: 00 00 00 00
command: 45 00 00 00
"""

ACTIONS_SIGNAL = """Filetype: Flipper Uni Remote Actions
Version: 1
ActionCount: 1
Action0Id: power_alias
Action0Type: signal
Action0Signal: Power
"""


def write_profile(root: Path, remote: str, actions: str = ACTIONS_SIGNAL) -> Path:
    root.mkdir(parents=True, exist_ok=True)
    (root / "remote.ur").write_text(remote, encoding="utf-8")
    (root / "signals.ir").write_text(SIGNALS, encoding="utf-8")
    (root / "actions.ur").write_text(actions, encoding="utf-8")
    return root


def expect_pass(name: str, remote: str, actions: str = ACTIONS_SIGNAL) -> None:
    with TemporaryDirectory() as td:
        folder = write_profile(Path(td) / name, remote, actions)
        cp.validate_profile(folder)


def expect_fail(name: str, remote: str, actions: str = ACTIONS_SIGNAL) -> None:
    with TemporaryDirectory() as td:
        folder = write_profile(Path(td) / name, remote, actions)
        try:
            cp.validate_profile(folder)
        except cp.ProfileError:
            return
        raise AssertionError(f"{name}: malformed profile unexpectedly passed")


def main() -> int:
    alias_remote = BASE_REMOTE.replace("sig:Power", "act:power_alias")
    expect_pass("signal_action", alias_remote)

    alias_actions = ACTIONS_SIGNAL.replace("Action0Type: signal", "Action0Type: alias")
    expect_fail("legacy_alias_type", alias_remote, alias_actions)

    unsafe_path = BASE_REMOTE.replace("SignalFile: signals.ir", "SignalFile: ../signals.ir")
    expect_fail("unsafe_path", unsafe_path)

    overlapping = BASE_REMOTE.replace(
        "ElementCount: 1",
        "ElementCount: 2",
    ) + """
#
Element1Type: button
Element1Id: collide
Element1Page: 0
Element1Rect: 0 0 1 1
Element1Tap: sig:Power
"""
    expect_fail("overlap", overlapping)

    too_many_steps = """Filetype: Flipper Uni Remote Actions
Version: 1
ActionCount: 1
Action0Id: too_long
Action0Type: sequence
Action0StepCount: 9
""" + "".join(
        f"Action0Step{i}: Power\nAction0Delay{i}: 0\n" for i in range(9)
    )
    expect_fail("too_many_steps", BASE_REMOTE, too_many_steps)

    nested_actions = """Filetype: Flipper Uni Remote Actions
Version: 1
ActionCount: 2
Action0Id: inner
Action0Type: sequence
Action0StepCount: 1
Action0Step0: Power
Action0Delay0: 0
Action1Id: outer
Action1Type: sequence
Action1StepCount: 2
Action1Step0: act:inner
Action1Delay0: 50
Action1Step1: sig:Power
Action1Delay1: 0
"""
    nested_remote = BASE_REMOTE.replace("sig:Power", "act:outer")
    expect_pass("nested_sequence", nested_remote, nested_actions)

    cyclic_actions = """Filetype: Flipper Uni Remote Actions
Version: 1
ActionCount: 2
Action0Id: a
Action0Type: sequence
Action0StepCount: 1
Action0Step0: act:b
Action0Delay0: 0
Action1Id: b
Action1Type: sequence
Action1StepCount: 1
Action1Step0: act:a
Action1Delay0: 0
"""
    expect_fail("sequence_cycle", nested_remote.replace("act:outer", "act:a"), cyclic_actions)

    long_delay = """Filetype: Flipper Uni Remote Actions
Version: 1
ActionCount: 1
Action0Id: slow
Action0Type: sequence
Action0StepCount: 1
Action0Step0: Power
Action0Delay0: 60001
"""
    expect_fail("excessive_delay", BASE_REMOTE.replace("sig:Power", "act:slow"), long_delay)

    bt_remote = BASE_REMOTE.replace("Transport: IR", "Transport: BT").replace(
        "BluetoothProfile:", "BluetoothProfile: bad profile!"
    ).replace("Element0Tap: sig:Power", "Element0Tap: bt:media:play_pause")
    expect_fail("bad_bt_profile", bt_remote)

    print("Validator regression cases PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
