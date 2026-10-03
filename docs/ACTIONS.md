# Action file

`actions.ur` contains reusable named IR actions. Version 1 supports `signal` aliases and `sequence` macros.

Header:

```text
Filetype: Flipper Uni Remote Actions
Version: 1
ActionCount: 3
```

## Signal action

A named alias for one IR signal:

```text
Action0Id: mute_now
Action0Type: signal
Action0Signal: Mute
```

Bind it with:

```text
Element4Tap: act:mute_now
```

Direct `sig:Mute` remains the simplest form. Aliases are useful when a stable semantic action ID should survive signal-name changes.

## Sequence / macro

A sequence can call signals directly:

```text
Action1Id: movie
Action1Type: sequence
Action1StepCount: 3
Action1Step0: sig:Power
Action1Delay0: 700
Action1Step1: HDMI1
Action1Delay1: 300
Action1Step2: sig:Play
Action1Delay2: 0
```

The delay belongs to the preceding step and is applied before the next step. The final delay is ignored by execution.

## Nested macros

A sequence step may call another action:

```text
Action2Id: quiet_movie
Action2Type: sequence
Action2StepCount: 2
Action2Step0: act:quiet
Action2Delay0: 150
Action2Step1: act:movie
Action2Delay1: 0
```

The validator and runtime both reject recursive cycles such as `A -> B -> A`.

## Limits

- maximum 16 named actions per remote;
- maximum 8 steps per sequence;
- action IDs use `A-Z`, `a-z`, `0-9`, `_` and `-`;
- action ID length: maximum 23 characters;
- signal-name length: maximum 31 characters;
- per-step delay: 0..60000 ms;
- sequence repeat events are ignored to prevent accidental macro storms.

## On-device Macro Editor

For ordinary IR remotes:

```text
MENU -> REMOTE -> MACROS
```

The editor can:

- create a new sequence;
- rename a sequence;
- add, replace or remove steps;
- select a signal or another sequence as a step;
- adjust the delay of a step with Left/Right in 100 ms increments;
- delete a sequence when it is not referenced.

The engine refuses edits that create an invalid reference or a recursive macro cycle. Delete and rename operations that would break live references are rejected rather than silently corrupting the package.

The Layout/Keymap Map picker can bind existing sequences with `act:<id>`.
