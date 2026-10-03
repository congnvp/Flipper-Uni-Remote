# Changelog

## [0.9.0] - 2026-10-03

Software-complete release candidate for the current v1 scope. Physical target validation is still required before v1.0.0.

### Added

- Generic 1..16 page remote layouts with page-aware runtime/editor navigation.
- Per-remote parsed-IR burst count.
- Sony RM-PJ8 regression profile with all 22 known SIRC commands.
- Optoma HR21G-YHGD03 regression profile with all 18 known NEC commands.
- Executable `STATE_IR` state engine and `state.urs` persistence.
- LG AC stateful adapter ported from the known-working LG project.
- Daikin ARC433A73 stateful adapter ported from the known-working Daikin project.
- Favourite, user folders and UNCATEGORIZED library views.
- Last remote/page/focus restore.
- BLE HID transport with profile-scoped bond storage and identity.
- Bluetooth Media regression profile.
- Transport-aware on-device Map editor for IR, stateful IR and Bluetooth actions.
- Static package validator for paths, bindings, actions, icons, pages, geometry and collisions.
- CI artifacts containing the FAP plus all bundled example profiles.
- 0.9 installation and hardware-gate documentation.

### Changed

- Remote layout page count is determined by usability, not by a fixed remote-template page count.
- Parsed IR transmission can reproduce device-specific initial frame bursts.
- Home chooser remains metadata-only while supporting categories.
- Remote Settings now expose Favourite and Folder.
- Bluetooth activates only for BT remotes and restores the default Flipper profile on exit.

### Fixed in final audit

- Aligned named signal action validation with the runtime `signal` action type and runtime size limits.
- Restored STATE_IR/Bluetooth transport state correctly after `RELOAD`.
- Replaced effectively invisible same-frame TX feedback with a timed 250 ms indicator.
- Preserved LG Jet state across power-off to match the working standalone LG implementation.
- Normalized Daikin Powerful off whenever the local unit state is off.
- Kept Favourite/Folder/Repeat metadata edits lazy so Home does not retain an unnecessary full layout.
- Re-synced the Home category after Favourite/Folder changes.
- Rejected unsafe package filenames, invalid Bluetooth profile IDs, missing IR signal files and colliding external layouts at runtime.
- Recovered automatically from malformed global settings and rebuilt the demo fallback if every remote package is invalid.
- Added malformed-profile regression tests to CI.

### Validation

- Package schema validation passes for living_tv, Sony, Optoma, LG, Daikin and Bluetooth Media.
- Official release-channel uFBT build, malformed-profile regressions and lint passed in PR #11 run #221 after the final software audit.
- Physical TX, AC state acceptance and BLE pair/reconnect remain tracked as TBD in `TEST_MATRIX.md`.

## [0.4.3] - 2026-10-02

### Fixed

- Replaced bounding-box collision with cell-level occupancy.
- D-pad now occupies only its five cross cells; four corner cells accept 1×1 buttons.
- Mixed-size elements may overlap another element's bounding rectangle when their occupied cells do not collide.
- Runtime and editor focus navigation now use occupied cells, keeping D-pad corner controls reachable.

### Added

- `btn21` 2×1 button preset.


## [0.4.2] - 2026-10-02

### Fixed

- Fixed NULL pointer reboot when RELOAD rescanned a previously loaded remote.
- Layout Editor now selects status and screen elements as editable layout objects.
- 1×1 elements can reflow/swap with different-size elements such as 1×2 or 2×1 where a valid placement exists.

### Added

- REPLACE tool for changing the selected element preset/size.
- Region replacement when adding a large element and no contiguous free rectangle exists.
- Adding a 3×3 D-pad can replace the occupied 3×3 region instead of requiring manual deletion of surrounding 1×1 controls.


## [0.4.1] - 2026-10-02

### Fixed

- Prevented out-of-memory reboot by lazily loading element/layout data for only the active remote.
- Remote chooser now keeps lightweight metadata for all remotes instead of 12 full 18-element profiles in RAM.
- Full element memory is released when returning to the chooser or when another remote is loaded.


## [0.4.0] - 2026-10-02

### Added

- Reusable Action Engine with direct signal and named sequence bindings.
- `actions.ur` with per-step sequence delays.
- On-device Layout Tools: Add, Remove, Map, Icon, Template.
- Element preset library and full layout template library.
- Central short-ID icon library for text-editable profiles.
- On-device hard-key HOLD mapping using Signal/Sequence picker.
- D-pad direction HOLD bindings and hold icons.
- Double-OK D-pad ALT lock; ALT turns hold bindings into short-press actions.
- Short Back exits D-pad capture and restores previous focus.

### Changed

- Remote edits now serialize the full `remote.ur` model.
- Bindings use explicit `sig:` and `act:` prefixes while retaining plain-signal compatibility.
- Long Back is a global system escape from all non-home pages.

### Fixed

- A D-pad long-direction action no longer emits the normal direction action first when a separate HOLD binding exists.


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
