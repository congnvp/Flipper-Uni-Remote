# Flipper Uni Remote

A data-driven universal-remote FAP for Flipper Zero, designed so humans and coding agents can extend remotes without recompiling protocol data into the application.

Current development baseline: **v0.2.0**.

## v0.2 architecture

The core flow is:

```text
Element -> semantic interaction -> signal/state action -> transport
State   -> display elements
```

Global settings live at `/ext/apps_data/flipper_uni_remote/settings.ur`.

A remote is a portable package on the SD card:

```text
/ext/apps_data/flipper_uni_remote/remotes/<remote-id>/
├── remote.ur      # layout, element bindings, per-remote settings
├── signals.ir     # standard Flipper IR signals, parsed or raw
└── state.urs      # reserved for local/stateful-device state
```

Bluetooth bond keys are deliberately kept outside portable remote packages. `BluetoothProfile` is a stable slot/reference that will be used by the BLE transport layer.

## What works in v0.2

- Official uFBT external-FAP build.
- Portrait 64×128 logical UI mapped onto the physical 128×64 LCD.
- Multiple remotes loaded by scanning Apps Data on the SD card.
- Layout loaded from `remote.ur` using a 3×6 logical grid.
- Standard `.ir` file is read by signal name at runtime. Parsed and raw signals stay in Flipper's native format.
- `button`, `hstep`, `vstep`, `dpad`, `status`, and `screen` element types.
- H-step captures Left/Right; Up/Down leaves the element.
- V-step captures Up/Down; Left/Right leaves the element.
- D-pad auto-captures directions and OK; **Long Back** releases it.
- **Long Back is system-reserved and cannot be remapped.** Outside D-pad capture, Long Back leaves the remote; on the remote chooser it exits the FAP.
- Global `settings.ur` with `RepeatEnabled` and `DefaultRemote`.
- Per-remote `Order`, `RepeatEnabled`, `Transport`, and `BluetoothProfile` fields.
- A demo remote package is created automatically on first run if no valid remote exists.

Not yet implemented: on-device layout editor, IR learning/assignment UI, stateful AC decoders/encoders, BLE HID transport, Bluetooth identity switching, and settings menus. The v0.2 schema is designed so these features do not require rewriting the renderer or IR files.

## Build

```bash
python3 -m pip install --upgrade ufbt
ufbt update --channel release
ufbt
```

With a Flipper connected:

```bash
ufbt launch
```

## Editing a remote without recompiling

1. Run the app once so Apps Data is created.
2. Copy or edit a remote directory under `/ext/apps_data/flipper_uni_remote/remotes/`.
3. Put any standard Flipper `.ir` file in the remote directory.
4. In `remote.ur`, map element bindings to the `name:` fields in that `.ir` file.
5. Restart the FAP to reload packages.

See `docs/REMOTE_PACKAGE.md` and `examples/living_tv/`.

## Repository layout

```text
.
├── application.fam
├── VERSION
├── src/
│   ├── controller.*      # hard-key policy and focus capture
│   ├── ir_transport.*    # signal-name -> .ir -> transmitter
│   ├── remote.*          # data model
│   ├── remote_store.*    # Apps Data scanner/parser
│   ├── settings.*        # global settings
│   ├── ui.*              # 3×6 data-driven renderer
│   └── main.c            # orchestration only
├── examples/
├── docs/
├── tools/
└── .github/workflows/
```

## Versioning

Semantic Versioning is used: `vMAJOR.MINOR.PATCH`.

- `VERSION` is the source of truth.
- Git release tags use `v`, for example `v0.2.0`.
- `application.fam` mirrors `MAJOR.MINOR` in `fap_version`.

## Safety/invariants

- Long Back is always reserved as a system escape.
- Device-specific signal bytes do not belong in UI/engine C code.
- UI stores signal names, never decoded protocol payloads.
- Bluetooth keys/secrets must never be placed in portable remote packages.
- IR-only local state must not be presented as confirmed device state.

## License

MIT. See `LICENSE`.
