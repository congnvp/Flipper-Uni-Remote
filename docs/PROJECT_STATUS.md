# Project Status

Last updated: 2026-10-03

## Baseline and integration candidate

- Stable rollback baseline: `v0.4.3`.
- Baseline runtime rollback commit: `9eb46426953bfa993831caf0c5a6626584f7e155`.
- Integration branch: `feat/v1-integration`.
- Integration PR: #11.
- Software release-candidate version: `0.10.0`.
- Hardware verification remains separate from CI and must not be inferred from a green build.

## Implemented

### M1 - Real stateless IR

- Generic multi-page engine: 1..16 pages.
- Sony RM-PJ8: 22 known SIRC commands, 3-page layout, parsed burst=3.
- Optoma HR21G-YHGD03: 18 known NEC commands, 3-page layout.

### M2 - Stateful IR

- Executable `STATE_IR` transport and deferred `state.urs` persistence.
- LG AC adapter.
- Daikin ARC433A73 adapter.
- Transport-aware on-device state-action mapping.

### M3 - Library / persistence

- FAVOURITE, user folders and UNCATEGORIZED.
- Favourite independent of folder membership.
- Metadata-only Home with lazy full-layout loading.
- Last remote/page/focus restore.
- On-device folder text entry.

### M4 - Bluetooth HID

- BLE HID media/navigation bindings.
- Profile-scoped private bond files and stable profile identity.
- Default Flipper Bluetooth profile restored when leaving BT remote.

### M5 - Hardening

- Runtime and CI package validation.
- Unsafe path, invalid identity, missing signal, geometry/collision and malformed-settings recovery.
- Release-channel uFBT build, lint and complete artifact packaging.
- Malformed-profile regression suite.

### M6 - Editing / macro completion

- Home remote rows are borderless text rows with selected-row inversion.
- Successful control actions get a second short inversion as press feedback.
- Layout Editor can edit Label and Page and append new pages.
- Empty trailing pages are trimmed.
- Folder names can be created/edited directly on-device.
- IR Macro Editor can create, rename, delete and edit sequences.
- Macro steps can select signals or nested macros.
- Step delay editing in 100 ms increments, capped at 60000 ms.
- Nested macro execution has runtime cycle protection.
- Validator rejects recursive macro graphs and invalid action IDs/delays.
- Runtime validates externally edited action graphs before accepting them.
- Returning from editors to Home restores strict metadata-only memory state.

## Remaining before v1.0.0

Only physical evidence/release-gate work remains:

1. Sony RM-PJ8 physical TX/navigation.
2. Optoma physical TX/navigation.
3. LG AC state/restore/TX regression.
4. Daikin AC state/restore/TX regression.
5. BLE HID pair/reconnect/media/navigation against a real host.
6. Stress switching/reloading remotes and long/repeat actions on a real Flipper.
7. Record results in `TEST_MATRIX.md`.
8. Tag `v1.0.0` only after required hardware rows pass.

## Current handoff

```text
Current milestone: M6 / 0.10.0 software completion candidate
Working: M1-M6 implementation and validation pipeline
Broken: no known software-side blocker; 0.10 code/test head passed GitHub Actions run #255
Not implemented: no known software feature from the agreed design remains
Hardware test: TBD
Next task: physical regression matrix; keep v1.0.0 gated until required hardware rows pass
Rollback baseline: v0.4.3 / 9eb46426953bfa993831caf0c5a6626584f7e155
```
