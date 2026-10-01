# Changelog

All notable changes are documented here. Versions follow Semantic Versioning.

## [0.2.0] - 2026-10-01

### Added

- SD-card remote packages under Apps Data.
- Versioned `remote.ur` configuration format.
- Runtime loading of standard Flipper `.ir` files by signal name.
- Data-driven 3×6 element renderer.
- Multiple remote chooser with per-remote ordering.
- H-step, V-step, button and D-pad focus behavior.
- Absolute Long Back escape policy.
- Global settings file plus per-remote repeat and Bluetooth-profile metadata.
- Auto-created demo package for first run.

### Changed

- Replaced compiled IR code profiles with runtime remote packages.
- Separated controller policy from transport and UI.

## [0.1.0] - 2026-10-01

### Added

- Initial buildable FAP scaffold.
- Portrait 64×128 logical UI on the 128×64 Flipper LCD.
- Public-library IR transport.
- Compiled NEC demo profile.
- uFBT CI and contributor documentation.
