# Changelog

## [0.3.1] - 2026-10-02

### Fixed

- Corrected portrait hard-key mapping: menu and focus navigation now match the visual direction of the D-pad when the LCD is above the controls.
- Restored the intended focus-capture rule: normal D-pad input moves focus; H/V steppers capture only their own axis; D-pad elements capture all directions and OK until Long Back.


All notable changes are documented here. Versions follow Semantic Versioning.

## [0.3.0] - 2026-10-02

### Added

- On-device main Menu via Short Back.
- Functional Global Settings and Remote Settings.
- Persistent global Repeat, Auto-open default, and Default remote settings.
- Persistent per-remote Repeat setting.
- On-device Layout Editor with grid movement and same-size element swapping.
- Spatial focus navigation based on 3×6 geometry.

### Fixed

- Corrected portrait hard-key rotation for the physical orientation with LCD above controls.
- Fixed focus jumping from vertical steppers to unrelated earlier elements.
- Directional Press/Short double-processing risk removed.

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
