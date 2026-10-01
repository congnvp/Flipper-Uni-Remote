# Architecture

## Goals

The app separates **presentation**, **input semantics**, **device profiles**, and **transport**. This lets a contributor add a remote without having to understand every subsystem.

## Layers

### UI (`src/ui.c`)

- Logical resolution: 64×128 portrait.
- Native LCD: 128×64.
- Pixel transform: logical `(x, y)` → native `(y, 63 - x)`.
- Custom bitmap text/icons avoid rotated browser/firmware font assumptions.

### Application/input (`src/main.c`)

- Owns page state and the Flipper input queue.
- Maps physical keys to logical portrait directions.
- Maps UI intent to a semantic `UniAction`.
- Does not contain device-specific IR codes.

### Profiles (`profiles/`)

- One device/profile per C file.
- Profiles map semantic actions to transport-specific codes.
- `profiles/registry.c` is the current explicit registry.

### Transport (`src/ir_transport.c`)

- Converts a profile's IR code into an `InfraredMessage`.
- Sends through the public infrared signal library.
- Repeat is explicitly passed so protocols can encode repeat semantics.

Bluetooth HID will be added as a separate transport implementation rather than mixed into IR code.

## Planned evolution

The C profile registry is intentionally simple for v0.x. The target architecture is a versioned on-SD profile format with optional compiled profiles. The engine API should remain semantic: UI emits actions/state changes; a transport/profile encoder decides how to represent them on the wire.
