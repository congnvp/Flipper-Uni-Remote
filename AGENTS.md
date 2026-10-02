# Coding-agent guide

Read `README.md`, `docs/ARCHITECTURE.md`, `docs/UI_SYSTEM.md`, `docs/REMOTE_PACKAGE.md`, `docs/ACTIONS.md`, and `docs/ICON_LIBRARY.md` before structural edits.

## Non-negotiable invariants

- Native LCD is 128×64; logical app canvas is 64×128 portrait.
- Long Back is a system escape and must never become configurable.
- Short Back is the D-pad capture escape; do not swap these two semantics.
- Existing icon IDs are a public profile API and must not be renamed casually.
- Element bindings use `sig:` or `act:`; plain signal names remain backwards-compatible.
- Transport code does not render UI.
- UI and controller do not embed IR protocol bytes.
- Real device signals belong in standard `.ir` files inside remote packages.
- Remote package geometry uses the 3×6 grid, not raw pixel coordinates.
- Bluetooth secrets/bond keys never go in portable remote packages.
- `VERSION` is SemVer source of truth; `application.fam` mirrors MAJOR.MINOR.
- Do not claim a build passed unless uFBT or CI actually built it.

## Expected validation

```bash
python3 tools/check_version.py
ufbt
ufbt lint
```

Hardware-facing behavior should also be tested on a physical Flipper Zero and the target device.
