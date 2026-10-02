# Action file

`actions.ur` contains reusable named actions. v0.4 supports `signal` and `sequence`.

Header:

```text
Filetype: Flipper Uni Remote Actions
Version: 1
ActionCount: 2
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

For a direct signal, prefer `sig:Mute`; aliases are useful when profile semantics should stay stable while signal names change.

## Sequence action

```text
Action1Id: movie
Action1Type: sequence
Action1StepCount: 3

Action1Step0: Power
Action1Delay0: 700

Action1Step1: HDMI1
Action1Delay1: 300

Action1Step2: Play
Action1Delay2: 0
```

The delay belongs to the preceding step and is applied before the next step. The final delay is ignored.

Limits in v0.4:

- maximum 16 named actions per remote;
- maximum 8 steps per sequence;
- each step references one signal in `signals.ir`;
- nested sequences are intentionally not supported yet;
- sequence execution is synchronous;
- sequence bindings ignore repeat events to prevent accidental repeated macros.

The on-device Map editor can assign existing sequences. Creating/editing sequence steps is currently done by editing `actions.ur` on a phone or computer.
