# Roadmap to v1.0

This roadmap starts from the real v0.4.3 codebase. Runtime UI, action sequences and the layout editor already exist, so they are not rebuilt as separate milestones.

## M0 - Baseline freeze and handoff

Deliverables:

- `PROJECT_STATUS.md` records the known-good commit and exact next task.
- `TEST_MATRIX.md` separates CI, simulator/runtime and physical-device evidence.
- `AC_PROTOCOLS.md` defines the stateful-IR contract before implementation.
- `DEVELOPMENT_WORKFLOW.md` limits work-in-progress and requires an end-of-session handoff.
- Coding-agent instructions point to these documents.

Definition of done:

- Documentation merged.
- Existing source behavior unchanged.
- Baseline remains v0.4.3.

## M1 - Prove the existing engine with real stateless IR remotes

Order:

1. Sony RM-PJ8 projector.
2. Optoma HR21G-YHGD03 projector.
3. One additional ordinary/raw IR package if needed to cover a missing path.

Scope:

- Portable remote package.
- Parsed and raw IR loading.
- `sig:` and `act:` mappings.
- Repeat/hold behavior.
- Reload and lazy-load regression.
- Layout/navigation regression.

Definition of done:

- CI passes for each change.
- The target package opens and navigates correctly.
- TX is physically verified on the target device before the hardware column is marked PASS.
- No protocol bytes are moved into UI/controller code.

## M2 - Stateful IR framework and AC adapters

Implement framework first, adapter second.

Order:

1. Generic local state model and state persistence contract.
2. Adapter interface: validate state, normalize state, encode full frame, send.
3. LG AC adapter using the existing LG AC repository as reference.
4. Daikin ARC433A73 adapter.
5. A third AC family only after LG and Daikin pass.

Core rule:

```text
Element
  -> Action
  -> mutate local state
  -> protocol adapter
  -> full IR frame
  -> IR transport
```

A temperature button must not pretend to be an independent IR signal when the original remote sends a complete state frame.

Definition of done:

- UI remains protocol-agnostic.
- Stateful transport does not reuse fake per-button `.ir` signals.
- Local state survives app close/reopen.
- UI labels local state as local/remembered, never confirmed device state unless a real feedback channel exists.
- LG and Daikin each pass physical hardware tests.

## M3 - Remote library and persistence UX

Scope:

- Favourite list.
- User folders.
- Uncategorized bucket.
- One remote may be both in a folder and Favourite.
- Last remote/page/focus restore where safe.
- Preserve lazy loading; chooser must not eagerly load every element layout.

Definition of done:

- Large remote libraries do not regress the v0.4.1 memory fix.
- Favourite/folder changes persist.
- Corrupt metadata cannot prevent app startup.

## M4 - Bluetooth HID transport

Scope:

- Add Bluetooth/HID as a transport below the Action Engine.
- Keep UI elements independent of transport.
- Keep bond/identity secrets in app-private storage, never portable remote packages.
- Establish what the exported Flipper firmware APIs actually permit before promising per-remote isolated identities.

Definition of done:

- At least one media-control profile works with a real paired host.
- Reconnect behavior is documented and physically tested.
- IR-only remotes remain unaffected.

## M5 - Hardening and v1.0 release candidate

No feature expansion in this milestone.

Required work:

- Malformed package handling.
- Missing signal/action/icon handling.
- Empty pages/layouts.
- Out-of-range state values.
- Long-press/repeat stress.
- Reload and remote-switch stress.
- Memory/heap regression checks.
- Version consistency.
- Clean uFBT build.
- Lint review.
- Physical regression for Sony, Optoma, LG and Daikin.
- Release notes and install/package documentation.

Definition of done:

- `TEST_MATRIX.md` has no unknown blockers for required v1.0 devices.
- All CI checks pass.
- Required physical-device rows are marked PASS with test notes.
- Tag `v1.0.0` only after the release candidate meets the matrix.
