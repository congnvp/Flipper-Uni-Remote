# Flipper Uni Remote

A modular universal-remote FAP for Flipper Zero. The project is designed so both human contributors and coding agents can add remote profiles, UI elements, transports, and tests without rewriting the whole app.

Current baseline: **v0.1.0**.

## What works now

- Builds as a standalone `.fap` with official **uFBT**.
- Portrait UI: hold Flipper Zero clockwise, with the screen above the controls.
- 64×128 logical pixel canvas mapped to the native 128×64 LCD.
- Built-in 6×3 UI system with 1-pixel borders and compact bitmap text/icons.
- IR transport using the public Flipper infrared library.
- PRESS/REPEAT behavior for directional commands.
- Short OK = Power; long OK = Mute.
- One explicit **NEC Demo** profile for validating the engine and IR path.
- CI builds on the official release SDK.

> The bundled NEC profile is a reference profile, not a universal TV code set. Add a profile for the target device before relying on it as a daily remote.

## Build

Install uFBT:

```bash
python3 -m pip install --upgrade ufbt
```

From the repository root:

```bash
ufbt update --channel release
ufbt
```

The resulting FAP is placed in `dist/`.

With a Flipper connected by USB:

```bash
ufbt launch
```

Useful shortcuts:

```bash
make build
make lint
make launch
```

Official uFBT documentation: https://github.com/flipperdevices/flipperzero-ufbt

## Install on Flipper Zero

Copy the generated `.fap` to an application directory on the SD card, for example:

```text
/apps/Infrared/flipper_uni_remote.fap
```

Then launch it from **Apps → Infrared**.

## Repository layout

```text
.
├── application.fam       # Flipper app manifest
├── VERSION               # SemVer source of truth
├── src/                  # engine, UI and transport code
├── profiles/             # remote profiles + registry
├── docs/                 # architecture and contribution docs
├── tools/                # validation/helper scripts
└── .github/workflows/    # CI build and tagged release
```

## Add a remote

Start with `docs/ADDING_REMOTE.md`. In the current C-profile format, most additions require:

1. Add `profiles/<device>.c`.
2. Register it in `profiles/registry.c`.
3. Run `python3 tools/check_version.py` and `ufbt`.
4. Test on the actual device and document the source of the IR codes.

A future milestone will add SD-card profile loading so most device additions can be data-only.

## Versioning

This repository follows **Semantic Versioning**:

```text
vMAJOR.MINOR.PATCH
```

- `VERSION` stores the full version, e.g. `0.1.0`.
- Git tags use a `v` prefix, e.g. `v0.1.0`.
- `application.fam` stores the corresponding `MAJOR.MINOR`, because the Flipper app manifest/catalog version field currently uses that form.
- Tagged releases are built automatically by GitHub Actions.

See `CONTRIBUTING.md` and `AGENTS.md` for collaboration rules.

## Roadmap

- Dynamic `.ir` / profile loading from SD card.
- Stateful AC profiles and full-frame encoders.
- BLE HID transport.
- Editable element layouts and macros.
- More built-in profiles only where licensing/provenance is clear.

## License

MIT. See `LICENSE`.
