# Development Workflow

The repository is the durable source of truth. Chat history is not.

## Work-in-progress limit

One development session should contain exactly one of:

- one milestone slice;
- one protocol adapter;
- one reproducible bug;
- one documentation/refactor task with no feature behavior mixed in.

Do not combine unrelated UI, transport, protocol and persistence changes in one session.

## Start of session

1. Read `AGENTS.md`.
2. Read `docs/PROJECT_STATUS.md`.
3. Read the relevant milestone in `docs/ROADMAP.md`.
4. Read only the subsystem documents needed for that task.
5. Start from the `Last known-good commit` or a newer verified commit.
6. State the single acceptance criterion before editing code.

## During the session

- Preserve the invariants in `AGENTS.md`.
- Prefer the smallest change that proves the acceptance criterion.
- Do not opportunistically refactor unrelated modules.
- Keep transport, UI, protocol and persistence boundaries explicit.
- If a new bug is discovered outside scope, record it and continue the current task unless it blocks validation.

## End of session

A coding session is not complete until:

1. `python3 tools/check_version.py` has run.
2. `ufbt` has run.
3. `ufbt lint` has run or its failure is recorded.
4. GitHub Actions result is checked after push/PR.
5. Hardware tests are recorded when hardware behavior changed.
6. `docs/TEST_MATRIX.md` is updated if evidence changed.
7. `docs/PROJECT_STATUS.md` is updated with the exact next task and last known-good commit.

Never say "build passed" from code inspection alone.

## Branch strategy

Suggested names:

```text
feat/m1-sony-profile
feat/m1-optoma-profile
feat/m2-stateful-core
feat/m2-lg-ac
feat/m2-daikin-ac
feat/m3-library
feat/m4-bluetooth-hid
fix/<reproducible-bug>
docs/<topic>
```

Keep `main` as a known-good integration branch.

## Commit strategy

Prefer commits that each preserve a buildable state.

A useful commit message explains the verified behavior, not only the files changed.

## Handoff rule

If work must stop mid-task, do not leave the next agent to infer state from the diff. Update `PROJECT_STATUS.md` with:

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

That handoff is mandatory for unfinished work.
