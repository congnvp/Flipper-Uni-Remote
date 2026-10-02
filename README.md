# Flipper Uni Remote

A data-driven universal-remote FAP for Flipper Zero. Remote layout, signal bindings, icon IDs and action sequences live in editable text files on the SD card, while the engine stays transport-agnostic.

Current development baseline: **v0.4.0**.

## Runtime model

```text
Physical key
  -> system-reserved keys
  -> focus / captured element
  -> binding (sig: / act:)
  -> Action Engine
  -> IR / future BLE / future stateful IR
```

A remote package lives at:

```text
/ext/apps_data/flipper_uni_remote/remotes/<remote-id>/
├── remote.ur
├── signals.ir
├── actions.ur
└── state.urs        # reserved for stateful devices
```

## v0.4 features

- Multiple remote packages loaded from SD.
- Standard Flipper `.ir` files, parsed or raw.
- Single-signal binding: `sig:<signal-name>`.
- Sequence binding: `act:<action-id>`.
- Sequence steps support per-step delay.
- On-device Layout Editor with Move, Add, Remove and Template.
- Element library: button 1×1/1×2, H-step, V-step, D-pad, Status, Screen 3×2/3×3.
- Layout templates: TV Basic, TV D-pad, Media and AC Basic.
- Icon library with short stable IDs such as `pwr`, `mut`, `play`, `home`, `fan`, `cool`, `hdmi`.
- On-device Map editor: Signal / Sequence / Clear.
- Remote hard-key HOLD mapping uses the same Signal/Sequence picker.
- D-pad NORMAL layer plus HOLD actions/icons.
- Double OK toggles D-pad ALT layer. In ALT, a short direction press executes its HOLD binding.
- Short Back exits D-pad capture and restores the previous focus.
- Long Back is always a system escape and can never be remapped.
- Global and per-remote settings remain persisted in Apps Data.

## D-pad behavior

Normal layer:

```text
UP/DOWN/LEFT/RIGHT  -> normal bindings
hold direction      -> *_hold binding, if configured
OK                   -> delayed up to 240 ms for double-tap detection
double OK            -> toggle ALT
```

ALT layer displays the hold icons and maps a short direction press to the corresponding hold action. Double OK again returns to NORMAL.

If a direction has a separate hold binding, the normal action is delayed until a short press is confirmed so a long press does not send both actions.

## Editing on Flipper

Short Back outside captured D-pad opens Menu.

```text
MENU
├── GLOBAL
├── REMOTE
├── LAYOUT
├── RELOAD
└── BACK
```

Layout Editor:

- OK: Select ↔ Move.
- Short Back while not moving: Layout Tools.
- Layout Tools: Add / Remove / Map / Icon / Template / Done.
- Map: choose binding field, then Signal / Sequence / Clear.
- Icon: choose the icon field then an ID from the icon library.

Remote Settings includes KEYMAP for HOLD shortcuts. Directional hard-key HOLD mappings only apply when a focused element has not captured that direction.

## Build

```bash
python3 -m pip install --upgrade ufbt
ufbt update --channel release
ufbt
```

With Flipper attached:

```bash
ufbt launch
```

## Profile editing

A single signal:

```text
Element4Tap: sig:Power
```

A named sequence:

```text
Element4Hold: act:movie
```

A button icon:

```text
Element4Icon: pwr
```

D-pad hold layer:

```text
Element5UpHold: sig:VolUp
Element5UpHoldIcon: volp
Element5LeftHold: sig:Rewind
Element5LeftHoldIcon: rew
Element5AltSticky: false
```

See:

- `docs/REMOTE_PACKAGE.md`
- `docs/ACTIONS.md`
- `docs/ICON_LIBRARY.md`
- `docs/UI_SYSTEM.md`

## Versioning

Semantic Versioning: `vMAJOR.MINOR.PATCH`.

`VERSION` is the source of truth. `application.fam` mirrors MAJOR.MINOR.

## Invariants

- Long Back is never remappable.
- Native LCD is 128×64; logical UI is 64×128 portrait.
- Layout files store 3×6 grid geometry, not pixel coordinates.
- IR protocol bytes stay in `.ir` files, not engine/UI code.
- Bluetooth bond secrets never belong in portable remote packages.
- Stateful IR local state must not be presented as confirmed device state.

## License

MIT.
