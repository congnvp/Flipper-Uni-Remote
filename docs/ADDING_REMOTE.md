# Adding a remote profile

## Current v0.x format

Profiles are compiled C data. This keeps the first releases deterministic while the dynamic SD format is designed.

### 1. Create a profile file

Copy `profiles/demo_nec.c` to a descriptive name, e.g.:

```text
profiles/samsung_tv_example.c
```

Expose one symbol:

```c
const UniRemoteProfile uni_profile_samsung_tv_example = {
    .id = "samsung_tv_example",
    .name = "Samsung TV Example",
    .short_name = "SAM",
    .transport = UniTransportInfrared,
    ...
};
```

`short_name` should be three characters where practical because the portrait status bar is compact.

### 2. Map semantic actions

The baseline engine knows:

- `UniActionPower`
- `UniActionMute`
- `UniActionUp`
- `UniActionDown`
- `UniActionLeft`
- `UniActionRight`
- `UniActionOk`

For parsed IR protocols provide protocol, address and command. Mark only supported actions as `true` in `has_action`.

### 3. Register it

Add an `extern` and pointer entry in `profiles/registry.c`.

### 4. Document provenance

Add a comment in the profile file describing the remote/model and where the codes came from. Record whether each command was verified on hardware.

### 5. Validate

```bash
python3 tools/check_version.py
ufbt
ufbt lint
```

## Repeat behavior

Directional keys send an initial parsed IR message on `InputTypePress` and repeat messages on `InputTypeRepeat`. OK uses short press for Power and long press for Mute in the baseline UI.

Profiles for protocols or appliances requiring full-state frames (many air conditioners) should not be forced into this simple key-code model. They will use a state encoder interface in a later milestone.
