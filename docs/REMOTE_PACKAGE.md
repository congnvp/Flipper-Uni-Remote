# Remote package format (v1)

Each remote is a portable directory under:

```text
/ext/apps_data/flipper_uni_remote/remotes/<remote-id>/
```

The package is intentionally split by responsibility:

```text
remote.ur      layout, element bindings, transport settings
signals.ir     standard Flipper IR signals (parsed or raw)
state.urs      reserved for runtime/local state; not required in v0.2
```

Bluetooth pairing keys are **not** stored in the portable package. `BluetoothProfile` is only a stable profile/identity reference.

## remote.ur

The file is FlipperFormat text and can be edited on a computer or phone.

Required header:

```text
Filetype: Flipper Uni Remote
Version: 1
```

Top-level fields:

- `Id`: stable machine ID. Do not change merely because the display name changes.
- `Name`: display name.
- `ShortName`: 1–3 ASCII characters for compact status UI.
- `Transport`: `IR`, `BT`/`BLE_HID`, or `STATE_IR`.
- `Order`: remote order in the chooser.
- `RepeatEnabled`: enable/disable repeat behavior for this remote.
- `SignalFile`: relative `.ir` filename inside the package.
- `BluetoothProfile`: stable Bluetooth profile slot ID for future BLE identity isolation.
- `ElementCount`: number of numbered element records.

## Element fields

Each element uses numbered keys such as `Element0Type`.

Common fields:

```text
Element0Type: status
Element0Id: status
Element0Rect: X Y W H
Element0Label: TXT
```

`Rect` uses the logical 3×6 grid, not pixels.

Supported element types in v0.2:

- `status`
- `screen`
- `button`
- `hstep`
- `vstep`
- `dpad`

Signal-binding fields:

- `Tap`, `Hold`
- `Up`, `Down`
- `Left`, `Right`
- `Ok`, `OkHold`

Every value is the **signal name** in `signals.ir`. The UI never stores protocol bits itself.

## Focus rules

- `hstep`: Left/Right are captured; Up/Down leave the element.
- `vstep`: Up/Down are captured; Left/Right leave the element.
- `dpad`: auto-captures all four directions and OK when focused. Long Back releases capture.
- `button`: OK activates; directions move focus.
- Long Back is system-reserved and can never be remapped.

## IR files

`signals.ir` is the standard Flipper `IR signals file` format. Signals may be parsed or raw. This means a file learned with the official Infrared app can be copied into a package and referenced by signal name without converting protocol data into C source.

## Global settings

Global FAP settings are separate from remote packages:

```text
/ext/apps_data/flipper_uni_remote/settings.ur
```

v0.2 implements:

```text
Filetype: Flipper Uni Remote Settings
Version: 1
RepeatEnabled: true
DefaultRemote: living_tv
```

Precedence for repeat behavior is currently global permission AND per-remote `RepeatEnabled`. Future element-level overrides may further narrow that behavior.
