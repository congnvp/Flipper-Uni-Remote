# Icon library

Profiles reference built-in icons by short, stable text IDs. H-step and V-step arrows are fixed and do not use this library.

Common IDs:

| ID | Meaning |
| --- | --- |
| `pwr` | Power |
| `mut` | Mute |
| `play` | Play |
| `paus` | Pause |
| `stop` | Stop |
| `rec` | Record |
| `prev` | Previous |
| `next` | Next |
| `rew` | Rewind |
| `ffwd` | Fast forward |
| `home` | Home |
| `back` | Back |
| `menu` | Menu |
| `info` | Info |
| `set` | Settings |
| `volp` / `volm` | Volume up/down |
| `chp` / `chm` | Channel up/down |
| `src` / `input` | Source/Input |
| `app` | Apps |
| `guide` | Guide |
| `fav` | Favorite |
| `sub` | Subtitle |
| `cc` | Closed caption |
| `aud` | Audio |
| `num` | Numeric keypad |
| `red`, `grn`, `blu`, `yel` | Color functions |
| `temp` | Temperature |
| `fan` | Fan |
| `mode` | Mode |
| `swng` | Swing |
| `eco` | Eco |
| `slp` / `sleep` | Sleep |
| `tmr` | Timer |
| `lite` | Light |
| `cool` | Cooling |
| `heat` | Heating |
| `dry` | Dry |
| `auto` | Auto |
| `zoom` | Zoom |
| `focs` | Focus |
| `key` | Keystone |
| `blank` | Blank screen |
| `frz` | Freeze |
| `asp` | Aspect ratio |
| `hdmi` | HDMI |
| `usb` | USB |
| `bt` | Bluetooth |
| `wifi` | Wi-Fi |
| `lock` | Lock / ALT state |
| `mic` / `voice` | Voice functions |
| `eject` | Eject |
| `pip` | Picture in picture |

Example:

```text
Element4Icon: pwr
Element4HoldIcon: menu
```

D-pad normal directions always use directional triangles. Only the HOLD/ALT layer takes icon IDs:

```text
Element5UpHoldIcon: volp
Element5LeftHoldIcon: rew
```

Unknown or empty icon IDs fall back to the element's short text label where possible. The ID list is an API: existing IDs should not be renamed casually because users can edit profiles as text.
