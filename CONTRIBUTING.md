# Contributing

## Workflow

1. Create a focused branch such as `feat/layout-editor` or `fix/dpad-capture`.
2. Keep engine, transport and device-data changes separated where practical.
3. Use Conventional Commit-style messages (`feat:`, `fix:`, `docs:`, `refactor:`, `ci:`).
4. Run:

```bash
python3 tools/check_version.py
ufbt
ufbt lint
```

5. Test hardware-facing changes on a real Flipper Zero and target device.

## Adding IR remotes

Prefer adding a portable example/package instead of compiling codes into C. See `docs/REMOTE_PACKAGE.md`.

IR provenance still matters: document how codes were captured or obtained, and whether they were verified on hardware.

## Version policy

- PATCH: fixes with no incompatible package/schema change.
- MINOR: backwards-compatible features/elements/transports/schema extensions.
- MAJOR: incompatible remote-package or public-engine changes.

Do not bump `VERSION` in ordinary pull requests unless explicitly preparing a release.
