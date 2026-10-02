# Layout library

The renderer uses a 3×6 logical grid.

## Element presets

| ID | Element | Size |
| --- | --- | --- |
| `btn11` | Button | 1×1 |
| `btn12` | Tall button | 1×2 |
| `h31` | Horizontal stepper | 3×1 |
| `v12` | Vertical stepper | 1×2 |
| `d33` | D-pad | 3×3 |
| `sts31` | Status | 3×1 |
| `scr32` | Screen | 3×2 |
| `scr33` | Large screen | 3×3 |

`Layout Tools -> Add` chooses from this library and places the new element into the first valid free rectangle. It can then be moved normally.

## Full templates

- `tv` — TV Basic
- `dpad` — TV D-pad
- `media` — Media controls
- `ac` — AC Basic

Applying a full template replaces the current element list. Device-specific mappings should then be assigned through Map or by editing `remote.ur`.

The layout file stores grid geometry rather than pixels, so renderer changes do not require rewriting profiles.
