# Coding-agent guide

This file is the compact contract for AI coding agents working in this repository.

## Before editing

Read, in order:

1. `README.md`
2. `docs/ARCHITECTURE.md`
3. `docs/UI_SYSTEM.md`
4. `docs/ADDING_REMOTE.md` when changing profiles

## Non-negotiable invariants

- The physical LCD remains 128×64. The app uses a logical 64×128 portrait coordinate system and rotates pixels in `src/ui.c`.
- Portrait input mapping lives in `src/main.c`; do not silently change it.
- Keep transport code out of UI code.
- Keep device-specific codes out of engine code; they belong in `profiles/`.
- Prefer public SDK APIs. Do not depend on private firmware internals without documenting why.
- `VERSION` is the SemVer source of truth. `application.fam` mirrors only MAJOR.MINOR.
- A profile addition should not require architectural changes.

## Validation before committing

```bash
python3 tools/check_version.py
ufbt
ufbt lint
```

If uFBT is unavailable, make that limitation explicit in the pull request; never claim a build passed when it was not run.

## Editing rules for agents

- Preserve existing public names unless a breaking change is intentional.
- Prefer one profile per file.
- Update docs when behavior or profile format changes.
- Do not commit `dist/`, `.ufbt/`, generated `.fap`, `.elf`, or build caches.
- Give concrete test notes for IR/BLE behavior.
