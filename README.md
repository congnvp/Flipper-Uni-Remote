# Flipper Uni Remote

Universal-remote FAP for Flipper Zero with data-driven layouts and three executable transports:

- ordinary IR: parsed/raw Flipper `.ir` signals and named sequences;
- stateful IR: local device state + vendor adapter + full-frame/auxiliary IR;
- Bluetooth HID: media and keyboard/navigation controls.

Current software candidate: **v0.9.0**. The v1.0.0 tag is intentionally held until the physical regression matrix passes.

## Architecture

```text
Physical key
  -> focus / captured element
  -> binding
  -> transport dispatcher
       IR       -> Action Engine -> parsed/raw IR
       STATE_IR -> State Engine -> LG/Daikin adapter -> IR
       BT       -> BLE HID -> media/keyboard report
```

The layout engine does not contain protocol bytes.

## Remote packages

```text
/ext/apps_data/flipper_uni_remote/remotes/<remote-id>/
├── remote.ur
├── signals.ir
├── actions.ur
└── state.urs
```

Included regression/reference profiles:

- Sony RM-PJ8 projector;
- Optoma HR21G-YHGD03 projector;
- LG AC;
- Daikin ARC433A73;
- Bluetooth Media;
- living_tv example.

See `docs/REMOTE_PACKAGE.md`.

## UI / layouts

- Portrait 64×128 logical UI rendered on the native 128×64 display.
- 3×6 logical grid.
- 1..16 pages per remote.
- Spatial focus navigation.
- LEFT/RIGHT at the edge switches page.
- D-pad uses a five-cell cross occupancy mask, leaving its four bounding-box corners usable.
- Compact page indicator for multi-page remotes.
- Focus inversion and press feedback.
- Long Back is always reserved for system escape.

The number of pages is a layout decision, not a fixed remote type rule. A remote may use 1, 2, 3, 4 or more pages when that produces a cleaner interface.

## On-device editing

Layout Editor supports:

- Move;
- Add;
- Replace;
- Remove;
- Map;
- Icon;
- Template.

Map is transport-aware:

- IR -> Signal / Sequence;
- STATE_IR -> valid adapter actions;
- BT -> valid Bluetooth media/key actions.

## Remote library

Home is metadata-only and preserves lazy loading.

Categories:

```text
FAVOURITE -> user folders -> UNCATEGORIZED
```

Favourite is independent of folders. The app also remembers the last remote/page/focus when safe.

## Stateful AC

LG and Daikin use `STATE_IR`; TEMP/FAN/MODE are not fake independent IR codes.

`state.urs` is local remembered state. The display therefore represents what this FAP last sent/remembers, not confirmed appliance state.

See `docs/AC_PROTOCOLS.md`.

## Bluetooth HID

A BT remote supplies `BluetoothProfile`.

The app:

- starts the BLE HID profile only when a BT remote is opened;
- uses stable profile-derived BLE identity;
- stores bond keys separately per profile in app-private storage;
- restores the Flipper default Bluetooth profile when leaving the BT remote.

Portable remote packages never contain bond secrets.

## Build

```bash
python3 -m pip install --upgrade ufbt
ufbt update --channel release
python3 tools/check_version.py
python3 tools/check_profiles.py
ufbt
ufbt lint
```

With a connected Flipper:

```bash
ufbt launch
```

CI performs the same version/profile validation before the release-channel build and bundles the FAP with all example profiles.

## Installation

See `docs/INSTALL.md`.

## Validation boundary

The 0.9.0 codebase and bundled profiles are intended to be software-complete for the current v1 scope. A successful build does not prove physical IR acceptance or BLE pairing/reconnect.

Required real-device evidence is tracked in `docs/TEST_MATRIX.md`.

## Documentation

- `docs/INSTALL.md`
- `docs/REMOTE_PACKAGE.md`
- `docs/AC_PROTOCOLS.md`
- `docs/ACTIONS.md`
- `docs/ICON_LIBRARY.md`
- `docs/UI_SYSTEM.md`
- `docs/TEST_MATRIX.md`
- `docs/PROJECT_STATUS.md`

## Versioning

Semantic Versioning: `vMAJOR.MINOR.PATCH`.

`VERSION` is the source of truth; `application.fam` mirrors MAJOR.MINOR.

## License

MIT.
