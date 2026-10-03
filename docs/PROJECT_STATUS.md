# Project Status

Last updated: 2026-10-03

## Baseline and integration candidate

- Stable rollback baseline: `v0.4.3`.
- Baseline runtime rollback commit: `9eb46426953bfa993831caf0c5a6626584f7e155`.
- Integration branch: `feat/v1-integration`.
- Integration PR: #11.
- Software release-candidate version: `0.9.0`.
- Final pre-hardware software validation: GitHub Actions run #221 PASS (version, six-profile validation, malformed-profile regression tests, release-channel uFBT build, artifact upload and lint).
- Hardware verification remains separate from CI and must not be inferred from a green build.

## Implemented in 0.9.0

### M1 - Real stateless IR

- Generic multi-page engine: 1..16 pages; old packages default to page 0.
- Page-aware runtime focus, editor navigation and compact page indicator.
- Per-remote parsed IR burst count.
- Sony RM-PJ8: all 22 known SIRC15/SIRC20 commands, 3-page sparse layout, `IrBurst: 3`.
- Optoma HR21G-YHGD03: all 18 known NEC commands, 3-page sparse layout.

### M2 - Stateful IR

- Executable `STATE_IR` transport.
- Adapter-owned defaults, normalization and validation.
- `state.urs` local-state persistence with deferred writes.
- LG AC adapter ported from the working LG project.
- Daikin ARC433A73 adapter ported from the working Daikin project.
- Transport-aware on-device action mapping for both adapters.

### M3 - Remote library / persistence

- FAVOURITE category.
- User-folder categories.
- UNCATEGORIZED bucket.
- Favourite remains independent of folder membership.
- Metadata-only category scans preserve lazy loading.
- Last remote/page/focus restore when the saved target is still safe/valid.
- Remote Settings can toggle Favourite and move among existing folders.

### M4 - Bluetooth HID

- Real BLE HID transport linked with `ble_profile`.
- `bt:media:` and `bt:key:` bindings.
- Profile-scoped app-private bond-key files.
- Stable profile-derived advertised identity.
- BT profile activates only while a BT remote is open and restores the default Flipper BT profile on exit.
- Bluetooth Media regression package included.

### M5 - Hardening

- Static package validator runs before every CI build.
- Validator checks paths, transport bindings, action references, icon IDs, grid bounds, page bounds and collisions.
- Version consistency check.
- Official release-channel uFBT build.
- Advisory lint.
- Artifact bundles FAP plus all validated example profiles.

## Remaining before v1.0.0

Only evidence/release-gate work remains:

1. Physical Sony RM-PJ8 TX and navigation test.
2. Physical Optoma TX and navigation test.
3. Physical LG AC state/restore/TX regression.
4. Physical Daikin AC state/restore/TX regression.
5. BLE HID pair/reconnect/media/navigation test against a real host.
6. Stress test switching/reloading remotes and long/repeat actions on a real Flipper.
7. Record results in `TEST_MATRIX.md`.
8. Tag `v1.0.0` only after required hardware rows pass.

## Current handoff

```text
Current milestone: M5 / 0.9.0 software release candidate
Working: M1-M4 implementation; package validation; uFBT build; lint
Broken: no known software-side blocker after final audit
Not implemented: no planned v1 feature remains
Validation: run #221 PASS after the final parser/runtime audit
Hardware test: TBD
Next task: final 0.9.0 CI, then physical regression matrix
Rollback baseline: v0.4.3 / 9eb46426953bfa993831caf0c5a6626584f7e155
```
