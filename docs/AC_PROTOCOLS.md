# Stateful IR / AC Protocol Contract

This document is the design gate for `STATE_IR`. It does not mean stateful IR is implemented.

## Problem

Many air-conditioner remotes transmit a complete state frame. Treating TEMP+, FAN or SWING as unrelated captured button signals produces an incorrect abstraction and makes state display unreliable.

The Universal Remote must model these devices as local state plus a protocol adapter.

## Required flow

```text
UI element
  -> Action Engine
  -> state mutation
  -> normalize / validate
  -> protocol adapter
  -> complete encoded frame
  -> IR TX
```

UI/controller code must never contain vendor protocol bytes.

## Generic state

The common layer should be able to represent fields such as:

- power
- mode
- temperature
- fan
- swing_vertical
- swing_horizontal

Adapters may expose extra vendor-specific fields, but the common state must not invent features unsupported by the real remote.

## Trust model

Every displayed state value must carry an implicit trust level:

- `LOCAL`: last value sent/remembered by this FAP.
- `CONFIRMED`: verified by a real feedback channel.
- `UNKNOWN`: not trustworthy.

For one-way IR ACs, the normal state is `LOCAL`, not `CONFIRMED`.

## Adapter responsibilities

A protocol adapter should own:

1. Supported ranges/enums.
2. State normalization.
3. Vendor-specific defaults.
4. Full-frame encoding.
5. Checksums/parity if required.
6. Required repeated frames/timing.
7. Mapping between portable local state and vendor frame fields.

The generic Action Engine should only request state mutations such as:

```text
STATE_SET(power, on)
STATE_STEP(temperature, +1)
STATE_CYCLE(fan)
STATE_CYCLE(swing_vertical)
```

## Persistence

`state.urs` is reserved for portable/local state data.

Rules:

- Do not store Bluetooth bonds or unrelated secrets in it.
- Validate every loaded value through the active adapter.
- Clamp or replace invalid values with protocol-safe defaults.
- Save after a successful local state mutation according to a write policy that avoids excessive SD writes.
- Persist local state separately from transient UI focus/navigation state.

## Adapter rollout

Implement one adapter at a time.

### Adapter 1: LG AC

Reference the existing LG AC project and preserve its known semantics, including:

- temperature range used by that remote;
- fan levels;
- vertical swing positions;
- horizontal swing positions;
- omitted functions that were intentionally unsupported in the original project.

Do not copy UI assumptions into the encoder.

### Adapter 2: Daikin ARC433A73

Add only after the LG framework is stable.

Use the same generic state/action interface. Vendor differences belong behind the adapter boundary.

## Definition of done for an adapter

- Encoded frames match the protocol behavior derived from the known working project/data.
- Generic UI and controller require no vendor-specific branches.
- App state can close/reopen and restore to a valid local configuration.
- Physical target device accepts POWER, temperature, mode/fan and supported swing operations.
- Unsupported functions are omitted rather than guessed.
