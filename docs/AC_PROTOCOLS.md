# Stateful IR / AC Protocol Contract

`STATE_IR` is implemented in the 0.9 integration candidate.

## Runtime flow

```text
UI element
  -> transport-aware binding (state:...)
  -> State Engine
  -> mutate LOCAL remembered state
  -> adapter normalize / validate
  -> vendor encoder / auxiliary command
  -> IR TX
```

UI/controller code contains no LG or Daikin protocol bytes.

## Common local state

The shared state model currently covers:

- power;
- mode;
- temperature;
- auto-mode bias;
- fan;
- vertical/horizontal swing;
- powerful/eco/comfort;
- auto-clean/purify/jet-dry;
- display/unit assumptions;
- on/off timers and sleep minutes.

Adapters normalize unsupported fields instead of inventing capabilities.

## Trust model

For one-way IR appliances, displayed values are `LOCAL`: the last state remembered/sent by this FAP. They are not `CONFIRMED` device state. A future feedback transport may add confirmed state, but current AC adapters do not.

## Persistence

Each STATE_IR package points to a local `state.urs`.

Rules implemented by the engine:

- defaults are adapter-owned;
- loaded values are normalized and validated before use;
- invalid state falls back to safe adapter defaults;
- state is marked dirty only after a successful mutation;
- dirty state is flushed on unload, reload and app shutdown rather than after every button press;
- transient page/focus state is stored in global settings, not `state.urs`;
- Bluetooth bond material is stored elsewhere and never enters this file.

## LG adapter

Adapter ID: `LG_AC`.

Ported from the known-working `congnvp/LG_AC_ir_flipper-zero` project.

Implemented protocol behavior includes:

- 28-bit LG frame construction and nibble checksum;
- 38 kHz raw transmission;
- 18–30 °C;
- Cool, Dry, Fan, Auto and Heat;
- Auto plus fan levels 1–5;
- eight vertical swing states;
- nine horizontal swing states;
- Power, Jet, Energy/Eco, Comfort, Display, Auto Clean, Purify, Jet Dry, C/F and Diagnosis;
- timer-on, timer-off, sleep and clear-timers operations.

Full-state changes such as temperature/mode/fan regenerate the state frame. Swing and supported auxiliary features use the exact auxiliary command families from the reference implementation.

## Daikin ARC433A73 adapter

Adapter ID: `DAIKIN_ARC433A73`.

Ported from `congnvp/daikin_ac_ir_flipper_zero`.

Implemented protocol behavior includes:

- 35-byte Daikin state frame;
- additive checksums for each section;
- three-section raw frame encoding;
- 584 raw timings at 38 kHz;
- Cool, Dry and Fan;
- 18–32 °C;
- Auto plus fan levels 1–5;
- vertical Swing;
- Powerful;
- on/off timers and clear-timers.

Unsupported generic state fields are normalized away.

## Binding examples

```text
state:power:toggle
state:temp:+
state:temp:-
state:mode:+
state:fan:-
state:swing_v:toggle
state:powerful:toggle
```

The on-device Map editor exposes only actions valid for the selected adapter.

## Evidence boundary

The encoders compile and link in the official release SDK and their package schemas pass CI validation. That is software evidence only. Physical acceptance by the actual LG/Daikin units remains a separate hardware gate in `TEST_MATRIX.md`.
