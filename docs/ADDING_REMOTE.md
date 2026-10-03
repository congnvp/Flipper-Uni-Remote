# Adding a remote

Remote definitions are data packages, not compiled UI profiles.

## Scaffold an ordinary IR remote

```bash
python3 tools/new_profile.py living_tv "Living TV" TV
```

This creates:

```text
examples/living_tv/
├── remote.ur
├── signals.ir
└── actions.ur
```

Copy the directory to:

```text
/ext/apps_data/flipper_uni_remote/remotes/living_tv/
```

## Ordinary IR

Use a standard Flipper `.ir` file and bind exact signal names:

```text
Transport: IR
Element3Up: sig:Vol_up
Element3Down: sig:Vol_dn
```

Named aliases/sequences use `act:<id>`.

If a parsed protocol needs multiple initial frames, set `IrBurst`.

## Layout and pages

Geometry uses a 3×6 logical grid:

```text
Element3Page: 0
Element3Rect: 0 4 1 2
```

Choose `PageCount` based on readability. There is no preferred fixed number of pages. Elements on separate pages can reuse cells.

## Library metadata

```text
Favourite: false
Folder: LIVING
```

An empty Folder goes to UNCATEGORIZED. Favourite is independent of folder membership.

## Stateful AC

Use `STATE_IR` only when a matching adapter exists.

```text
Transport: STATE_IR
StateAdapter: LG_AC
StateFile: state.urs
Element2Left: state:temp:-
Element2Right: state:temp:+
```

Current adapters:

- `LG_AC`
- `DAIKIN_ARC433A73`

Do not convert full-state AC behavior into fake per-button learned files.

## Bluetooth HID

```text
Transport: BT
BluetoothProfile: MyMedia
Element2Tap: bt:media:play_pause
Element3Up: bt:key:up
```

Bond keys are generated/stored in app-private data and must never be copied into the package.

## On-device edits

The Map picker is transport-aware, so IR remotes see signals/sequences while STATE_IR and BT remotes see only valid actions for their transport.

## Validation

Before committing:

```bash
python3 tools/check_version.py
python3 tools/check_profiles.py
ufbt
ufbt lint
```

The profile validator catches invalid bindings, unknown actions/icons, path traversal, page/grid errors and collisions.

Physical-device behavior still requires real hardware evidence.
