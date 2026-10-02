# Remote package format

Each remote is a portable directory:

```text
/ext/apps_data/flipper_uni_remote/remotes/<remote-id>/
├── remote.ur
├── signals.ir
├── actions.ur
└── state.urs
```

`state.urs` is reserved for stateful-device drivers. Bluetooth bond keys are never part of a portable package.

## remote.ur

Header:

```text
Filetype: Flipper Uni Remote
Version: 1
```

Top-level fields:

```text
Id: living_tv
Name: Living TV
ShortName: TV
Transport: IR
Order: 10
RepeatEnabled: true
SignalFile: signals.ir
ActionFile: actions.ur
BluetoothProfile: living_tv

HardUpHold:
HardDownHold:
HardLeftHold:
HardRightHold:
HardOkHold:

ElementCount: 6
```

Hard-key bindings use the same binding syntax as elements. Long Back is not represented because it is permanently reserved by the engine.

## Binding syntax

```text
sig:Power
act:movie
```

For backwards compatibility a plain value such as `Power` is treated as a signal name.

- `sig:` calls one signal from `SignalFile`.
- `act:` resolves a named action from `ActionFile`.
- v0.4 named sequences do not repeat when the OS emits a repeat event.

## Elements

Common fields:

```text
Element0Type: button
Element0Id: power
Element0Rect: 1 4 1 2
Element0Label: PWR
Element0Icon: pwr
Element0Tap: sig:Power
Element0Hold: act:power_menu
```

Supported types:

- `status`
- `screen`
- `button`
- `hstep`
- `vstep`
- `dpad`

`Rect: X Y W H` uses the logical 3×6 grid.

### Button

```text
Element4Icon: pwr
Element4HoldIcon: menu
Element4Tap: sig:Power
Element4Hold: act:power_menu
```

### H-step

Its left/right triangles are fixed and do not need icon IDs.

```text
Element2Left: sig:ChDown
Element2Right: sig:ChUp
```

### V-step

Its up/down triangles are fixed.

```text
Element3Up: sig:VolUp
Element3Down: sig:VolDown
```

### D-pad

Normal layer:

```text
Element5Up: sig:Up
Element5Down: sig:Down
Element5Left: sig:Left
Element5Right: sig:Right
Element5Ok: sig:OK
```

Hold/ALT layer:

```text
Element5UpHold: sig:VolUp
Element5DownHold: sig:VolDown
Element5LeftHold: sig:Rewind
Element5RightHold: sig:FastForward
Element5OkHold: sig:Home

Element5UpHoldIcon: volp
Element5DownHoldIcon: volm
Element5LeftHoldIcon: rew
Element5RightHoldIcon: ffwd
Element5OkHoldIcon: home

Element5AltSticky: false
```

Double OK toggles ALT. In ALT, a short direction press runs the matching HOLD binding.

Short Back releases D-pad capture and restores the previous focus. Unless `AltSticky: true`, leaving D-pad resets ALT to NORMAL.

Long Back always performs system escape.

## Runtime editing

The v0.4 on-device editor rewrites `remote.ur` from the current data model. This means fields managed by the FAP remain internally consistent after Add/Remove/Move/Map/Icon/Template operations.

Files may still be edited directly on a PC or phone. After external changes use `MENU -> RELOAD`.

## signals.ir

This remains the official Flipper IR signal format. Parsed and raw entries are supported.

Signal names used by the on-device picker are limited to 31 characters.

## actions.ur

See `ACTIONS.md`.

## Icons

See `ICON_LIBRARY.md`.
