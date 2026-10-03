# Sony RM-PJ8 profile

Portable Universal Remote package for the Sony Projector Remote Commander RM-PJ8.

## Source

The IR table is copied without modification from the known-working `congnvp/sony_projector_RM-PJ8` project. That project documents the codes as captures from a physical RM-PJ8 remote.

The standalone FAP sends Sony SIRC commands as a burst of three frames. This profile therefore sets:

```text
IrBurst: 3
```

so Universal Remote preserves the same press behavior instead of using its default one-frame parsed IR transmit.

## Layout

The 22 original commands are represented by 16 UI elements across three pages:

- Page 1: Power, Input, Menu, APA, Eco, Reset.
- Page 2: D-pad/Enter plus Aspect, Keystone, Pattern, Blank, Freeze and Return.
- Page 3: D.Zoom stepper, Volume stepper and Mute.

This keeps the interface sparse while preserving every command from the original IR file.

## Validation status

- File/protocol provenance: known working standalone project.
- Universal Remote parse/render/build: pending CI on this branch.
- Physical TX from Universal Remote: pending test on a Flipper Zero + Sony projector.
