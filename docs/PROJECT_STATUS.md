# Project Status

Last updated: 2026-10-03

## Known-good baseline

- Version: `0.4.3`
- Branch baseline: `main`
- Runtime rollback commit: `9eb46426953bfa993831caf0c5a6626584f7e155`
- GitHub Actions: FAP build run #40 completed successfully on 2026-10-02.
- Baseline-handoff PR #9 also passed version check, official uFBT build, artifact upload and advisory lint.
- Hardware verification is separate from CI and must never be inferred from a green build.

The runtime rollback point remains v0.4.3 commit `9eb46426953bfa993831caf0c5a6626584f7e155`.

## Current milestone

**M1.1 - Real stateless IR validation: Sony RM-PJ8**

M0 (baseline freeze and durable handoff) is complete. The repository is now the source of truth for project state and next actions.

## Working in v0.4.3

- SD-card remote packages under Apps Data.
- Metadata-only chooser scan plus lazy loading of one active remote layout to reduce memory pressure.
- Standard Flipper `.ir` files with parsed and raw IR signals.
- Direct signal bindings via `sig:<name>`.
- Named action bindings via `act:<id>`.
- Signal aliases and synchronous sequences with delays.
- Portrait 64x128 logical UI rendered on the native 128x64 LCD.
- 3x6 logical layout grid.
- On-device Layout Editor: Move, Add, Replace, Remove, Map, Icon and Template.
- Element presets including 1x1, 1x2, 2x1, steppers, D-pad, Status and Screen.
- Icon library with stable short IDs.
- D-pad NORMAL/HOLD/ALT behavior.
- Global and per-remote settings.
- Safe reload path.
- Cell-level occupancy masks, including usable D-pad corner cells.
- Runtime/editor spatial navigation using the same occupied-cell geometry.
- GitHub Actions build workflow using official uFBT action.

## Declared or reserved, but not implemented yet

- Executable `STATE_IR` transport.
- `state.urs` runtime state persistence/driver contract.
- AC protocol adapters and full-frame state encoders.
- BLE/HID transport.
- Bluetooth per-profile identity/bond handling.
- Favourite/folder/uncategorized remote library organization.
- On-device editing of sequence steps.
- A repository-level regression set built from the real Sony, Optoma, LG and Daikin remotes previously developed in separate repositories.

## Next task

**M1.1 - Real stateless IR validation: Sony RM-PJ8**

1. Import or recreate a portable Sony RM-PJ8 remote package using the existing data-driven package format.
2. Keep all protocol bytes in `signals.ir`.
3. Bind UI elements only through `sig:` / `act:`.
4. Validate parsed/raw send behavior, repeat policy, navigation and reload.
5. Run `python3 tools/check_version.py`, `ufbt` and `ufbt lint`.
6. Record CI result in this file.
7. Hardware test on a physical Flipper + Sony projector is required before marking TX as hardware-passed.

After Sony passes, repeat the same process for the Optoma projector before touching `STATE_IR`.

## Do not do next

- Do not implement Bluetooth yet.
- Do not add LG and Daikin encoders in the same change.
- Do not refactor UI/layout code while validating Sony unless a reproducible runtime bug requires it.
- Do not change public icon IDs or package semantics without a migration note.
- Do not mark hardware-facing behavior as passed from CI alone.

## Session handoff template

At the end of every development session, update this section.

```text
Current milestone:
Working:
Broken:
Not implemented:
Validation run:
Hardware test:
Next task:
Last known-good commit:
```
