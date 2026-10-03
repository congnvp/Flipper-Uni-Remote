# Flipper Uni Remote

Universal remote FAP for Flipper Zero. The current development branch combines the data-driven runtime, editable portrait layout, IR/stateful-IR engines, and BLE HID transport.

Current version: **v0.1.0 DEV**

Development branch:

```text
dev/full-v0.1.0
```

Development prerelease:

```text
dev-full-v0.1.0
```

## Current runtime

- English-only system UI.
- Portrait controls with the previously verified physical-key rotation.
- Fixed 3-column × 6-row logical layout.
- 19 px control regions on 20 px pitch below an 8 px status bar.
- Up to 8 pages per remote, with a compact page rail.
- D-pad is focused first and captured with OK; Back exits D-pad capture.
- Long Back is the global escape path.
- Only one remote layout is loaded at a time.

## Transports

### Normal IR

A remote owns its copied `signals.ir` file. The runtime supports parsed and raw Flipper IR signals, action sequences, repeat, and per-remote `IrBurst`.

Remote settings -> **IMPORT IR** opens the stock Flipper file browser at `/ext/infrared`, validates an IR signals file, then copies it into the selected remote package.

### Stateful IR

Built-in stateful engines currently cover:

- LG AKB75215401 family
- Daikin ARC433A73
- Panasonic RKR-style remote
- Carrier/Toshiba WC-UA4NE

State is persisted per remote. Supported controls include the applicable subset of power, mode, temperature, fan, swing, Eco, Powerful/Turbo, Nanoe and Carrier fixed/swing commands.

Remote settings -> **RESET STATE** deletes the local state snapshot and returns the profile to its default state on next use.

### Bluetooth HID

BLE HID remotes use Flipper's official HID BLE profile. Each `BluetoothProfile` gets its own persistent key file and derived Bluetooth identity so paired devices can be remembered independently.

Built-in bindings include navigation, Enter, Home, Back, Power, volume, mute and media controls. Numeric keyboard/consumer usages can also be mapped through `bt:kb:<usage>` and `bt:cc:<usage>`.

Remote settings -> **FORGET BT** removes the selected profile's pairing data and restarts that HID profile when active.

## Built-in development remotes

Create-only examples are installed when missing; existing user copies are not overwritten:

- Demo TV
- LG AC
- Daikin AC
- Panasonic AC
- Carrier AC
- Sony RM-PJ8 projector
- Optoma HR21G-YHGD03 projector
- Bluetooth TV

Sony and Optoma packages use the signal data from their dedicated repositories. Sony uses a three-frame IR burst to match the standalone RM-PJ8 implementation.

## Layout editor

The editor persists layout changes in each remote's `remote.ur`.

Supported elements:

- 1×1, 1×2 and 2×1 buttons
- horizontal and vertical step controls
- 3×3 cross-shaped D-pad; its four corner cells remain available
- screen/status element types
- add, replace, remove and move
- signal/sequence mapping
- icon assignment
- layout templates

The remote model supports up to 72 elements across up to 8 pages.

## Package layout

Each remote lives under:

```text
apps_data/flipper_uni_remote/remotes/<remote-id>/
├── remote.ur
├── signals.ir      # normal IR, when used
├── actions.ur      # optional sequences
└── state.bin       # stateful IR, when used
```

Bluetooth pairing keys are stored separately under the app's `bt/` directory.

## Build

Every push to `dev/**` is built using the official uFBT action. Development runs are serialized so the rolling prerelease tag cannot be updated concurrently.

Local build:

```bash
python3 tools/check_version.py
ufbt
```

## Current limitations

This development branch is functional rather than feature-complete relative to the longer UI/UX specification. Folder/Favourite library management, on-device text rename/create flows, and a full remote-package manager still need to be brought over. Hardware behavior must also be verified on a real Flipper Zero even when CI compilation and lint pass.

## License

MIT.
