# Adding a remote

v0.2 remotes are data packages, not C profiles.

## Fast path

Create an example/package scaffold:

```bash
python3 tools/new_profile.py living_tv "Living TV" TV
```

This creates:

```text
examples/living_tv/
├── remote.ur
└── signals.ir
```

Copy that directory to:

```text
/ext/apps_data/flipper_uni_remote/remotes/living_tv/
```

or use it as a template for a user-facing package.

## Using IR captured by Flipper

The FAP deliberately uses the standard Flipper `.ir` format. A file learned in the official Infrared app can therefore be copied into the remote package without converting address/command/raw timings to source code.

Set:

```text
SignalFile: my_remote.ir
```

and bind element actions to the exact `name:` entries inside that file.

Example:

```text
Element3Type: vstep
Element3Id: volume
Element3Rect: 0 4 1 2
Element3Label: VOL
Element3Up: Vol_up
Element3Down: Vol_dn
```

## Layout

Use `Rect: X Y W H` on the 3×6 logical grid. Do not store pixel positions.

## Bluetooth

You may assign a stable `BluetoothProfile` today. BLE HID and per-profile identity switching are not yet implemented, so a BT package is metadata-only in v0.2.

## Stateful appliances

Do not model a full-state air-conditioner protocol as fake independent button commands if the real remote sends an entire state frame. Use `Transport: STATE_IR` only as a placeholder until a matching state encoder/decoder driver exists.

## Validation

```bash
python3 tools/check_version.py
ufbt
ufbt lint
```

For IR changes, test actual hardware and document code provenance.
