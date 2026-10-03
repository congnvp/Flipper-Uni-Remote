# Coding-agent guide

Read these first, in order:

1. `docs/PROJECT_STATUS.md`
2. `docs/ROADMAP.md`
3. `docs/DEVELOPMENT_WORKFLOW.md`
4. `README.md`
5. `docs/ARCHITECTURE.md`
6. The subsystem docs relevant to the current task: `docs/UI_SYSTEM.md`, `docs/REMOTE_PACKAGE.md`, `docs/ACTIONS.md`, `docs/ICON_LIBRARY.md`, `docs/LAYOUT_LIBRARY.md`, `docs/AC_PROTOCOLS.md`.

The repository is the durable source of truth. Do not depend on prior chat history for project state.

## Work-in-progress rule

- Work on one milestone slice, one protocol adapter, or one reproducible bug at a time.
- Do not mix unrelated UI, transport, protocol and persistence changes.
- Do not start the next milestone until the current change has validation evidence.
- Update `docs/PROJECT_STATUS.md` before ending an unfinished session.

## Non-negotiable invariants

- Native LCD is 128×64; logical app canvas is 64×128 portrait.
- Long Back is a system escape and must never become configurable.
- Short Back is the D-pad capture escape; do not swap these two semantics.
- Existing icon IDs are a public profile API and must not be renamed casually.
- Element bindings use `sig:` or `act:`; plain signal names remain backwards-compatible.
- Transport code does not render UI.
- UI and controller do not embed IR protocol bytes.
- Real device signals belong in standard `.ir` files inside remote packages.
- Stateful AC protocols use local state + a full-frame encoder; do not fake them as unrelated per-button IR commands.
- Remote package geometry uses the 3×6 grid, not raw pixel coordinates.
- Bluetooth secrets/bond keys never go in portable remote packages.
- `VERSION` is SemVer source of truth; `application.fam` mirrors MAJOR.MINOR.
- Do not claim a build passed unless uFBT or CI actually built it.
- Do not claim hardware behavior passed unless it was physically tested and recorded.

## Expected validation

```bash
python3 tools/check_version.py
ufbt
ufbt lint
```

After pushing, check GitHub Actions. Hardware-facing behavior must also be tested on a physical Flipper Zero and the target device.

Update `docs/TEST_MATRIX.md` when validation evidence changes and `docs/PROJECT_STATUS.md` with the exact next task and last known-good commit.
