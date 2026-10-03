# Remote package format

Each portable remote is a directory under:

```text
/ext/apps_data/flipper_uni_remote/remotes/<remote-id>/
├── remote.ur
├── signals.ir
├── actions.ur
└── state.urs        # created/used by STATE_IR remotes
```

Bluetooth bond keys are app-private and are never part of a portable remote package.

## remote.ur

Header:

```text
Filetype: Flipper Uni Remote
Version: 1
```

Common top-level fields:

```text
Id: living_tv
Name: Living TV
ShortName: TV
Transport: IR
Order: 10
Favourite: false
Folder: LIVING
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

ElementCount: 6
```

`Favourite` is independent of `Folder`. A remote can appear in both its folder and FAVOURITE. An empty `Folder` places it in UNCATEGORIZED.

## Transports and bindings

### IR

```text
Transport: IR
Element0Tap: sig:Power
Element0Hold: act:movie
```

A plain signal name remains supported for older packages. `sig:` resolves one entry in `SignalFile`; `act:` resolves one alias/sequence in `ActionFile`.

`IrBurst` controls the number of initial frames for parsed IR signals. Default is 1. Sony RM-PJ8 uses 3. Raw signals are transmitted once because their timing body may already contain repetition.

### STATE_IR

```text
Transport: STATE_IR
StateAdapter: LG_AC
StateFile: state.urs
Element2Left: state:temp:-
Element2Right: state:temp:+
Element4Tap: state:power:toggle
```

Implemented adapters:

- `LG_AC`
- `DAIKIN_ARC433A73`

The binding mutates local remembered state, the adapter normalizes/validates it, then emits the vendor frame or auxiliary command. No fake per-button AC signal files are required.

`state.urs` is LOCAL remembered state only; it is not confirmation from the appliance.

### Bluetooth HID

```text
Transport: BT
BluetoothProfile: UniMedia
Element2Left: bt:media:prev
Element2Right: bt:media:next
Element6Up: bt:key:up
Element6Ok: bt:key:enter
```

Supported `bt:media:` actions include play/pause, previous/next, stop, mute, volume, Home, Back and Forward. Supported `bt:key:` actions include arrows, Enter, Escape, Space, Tab and Page Up/Down.

Each `BluetoothProfile` gets stable app-private bond storage and a stable profile-derived BLE identity. Leaving the BT remote restores the Flipper default Bluetooth profile.

## Elements

Common fields:

```text
Element0Type: button
Element0Id: power
Element0Page: 0
Element0Rect: 1 4 1 2
Element0Label: PWR
Element0Icon: pwr
Element0Tap: sig:Power
```

Supported types:

- `status`
- `screen`
- `button`
- `hstep`
- `vstep`
- `dpad`

`Rect: X Y W H` uses the logical 3×6 grid.

## Multiple pages

`PageCount` defaults to 1 and accepts 1..16. Each element may declare `ElementNPage`; omitted values default to page 0.

Elements on different pages may reuse the same cells. On one page, collision checking uses occupied cells rather than bounding rectangles. The 3×3 D-pad occupies its five cross cells, so its four corner cells remain usable.

Runtime navigation stays spatial inside the active page. If LEFT/RIGHT has no further focus target, the engine wraps to the previous/next non-empty page. D-pad and stepper directions retain their control semantics while captured.

A compact page indicator is drawn at the bottom for multi-page remotes.

## Button

```text
Element4Icon: pwr
Element4HoldIcon: menu
Element4Tap: sig:Power
Element4Hold: act:power_menu
```

## H-step / V-step

```text
Element2Left: sig:ChDown
Element2Right: sig:ChUp

Element3Up: sig:VolUp
Element3Down: sig:VolDown
```

Transport-specific bindings can be used in the same fields.

## D-pad

```text
Element5Up: sig:Up
Element5Down: sig:Down
Element5Left: sig:Left
Element5Right: sig:Right
Element5Ok: sig:OK

Element5UpHold: sig:VolUp
Element5DownHold: sig:VolDown
Element5LeftHold: sig:Rewind
Element5RightHold: sig:FastForward
Element5OkHold: sig:Home

Element5AltSticky: false
```

Double OK toggles ALT. Short Back releases D-pad capture. Long Back is a system action and cannot be remapped.

## On-device editor

The Layout Editor rewrites `remote.ur` from the current model. Move/Add/Replace/Remove/Icon remain transport-neutral.

Map is transport-aware:

- IR: Signal / Sequence / Clear.
- STATE_IR: adapter-valid Action / Clear.
- BT: Bluetooth Action / Clear.

This prevents the on-device editor from creating a binding that the selected transport cannot execute.

## Validation

CI runs `tools/check_profiles.py` before compilation. It checks:

- transport and adapter names;
- file-name/path traversal constraints;
- page/element bounds;
- occupied-cell collisions;
- signal/action references;
- stateful and Bluetooth action names;
- icon IDs;
- action sequence/alias signal references.

External edits can be loaded with `MENU -> RELOAD`.

See also `ACTIONS.md`, `ICON_LIBRARY.md`, and `AC_PROTOCOLS.md`.
