# Contributing

## Workflow

1. Create a focused branch such as `feat/lg-ac-profile` or `fix/portrait-input`.
2. Keep changes small and scoped.
3. Use Conventional Commit-style messages where practical (`feat:`, `fix:`, `docs:`, `refactor:`, `ci:`).
4. Run:

```bash
python3 tools/check_version.py
ufbt
ufbt lint
```

5. Test hardware-facing changes on a real Flipper Zero and target device.
6. Open a pull request using the repository template.

## Version policy

The project uses Semantic Versioning.

- PATCH: fixes without changing profile/API behavior.
- MINOR: backwards-compatible features, profiles, UI elements or transports.
- MAJOR: incompatible profile format or public engine changes.

Do not bump `VERSION` in ordinary pull requests unless the pull request is explicitly preparing a release.

## Remote-code provenance

For every real-device profile, document where the codes came from and how they were verified. Prefer user-captured or otherwise redistributable data. Do not copy opaque proprietary databases without permission.

## Hardware safety

Do not add destructive or security-sensitive macros by default. Remote profiles should represent ordinary consumer-device controls.
