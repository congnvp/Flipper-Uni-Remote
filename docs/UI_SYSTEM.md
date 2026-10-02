# UI system

## Orientation

Hold Flipper Zero clockwise in portrait orientation: screen above, controls below.

Portrait hard-key mapping is positional: physical Left→logical Up, Right→Down, Down→Left, Up→Right. This keeps menu and focus movement aligned with what the user sees after rotating the device.

Logical resolution is 64×128. The renderer rotates logical pixels onto the native 128×64 LCD.

## Base grid

Layout files use 3 columns × 6 rows. Pixel cuts are:

- X: `0, 21, 42, 64`
- Y: `0, 21, 42, 64, 85, 106, 128`

Elements store grid rectangles, never absolute pixels.

## Elements

- `status`: transport / short remote name / TX status.
- `screen`: display-only state/last-action area.
- `button`: OK = Tap, Long OK = Hold.
- `hstep`: Left/Right actions; Up/Down exits to another focusable element.
- `vstep`: Up/Down actions; Left/Right exits to another focusable element.
- `dpad`: auto-captures directions and OK while focused. Long Back releases capture.

## Focus

Status and Screen are not focusable.

Focus traversal is spatial: the controller selects the nearest focusable element that lies fully in the requested direction. Element order in `remote.ur` no longer determines navigation.

## System escape

Long Back is evaluated before element or remote mappings:

```text
D-pad captured -> release D-pad capture
Remote page    -> remote chooser
Chooser        -> exit FAP
```

It must never be made configurable.

## Visual rules

- interactive borders: 1 pixel.
- icon clearance from inner border: at least 1 pixel.
- compact labels: English, preferably ≤3 characters.
- focused controls invert button background/glyph where applicable.
- display state should distinguish local/unconfirmed state from confirmed state when stateful transports are added.


## Menu and layout editor

Short Back opens the app Menu when not inside D-pad capture. Long Back remains system-reserved.

Layout Editor has two modes:
- Select: directions move selection spatially between interactive elements.
- Move: OK enters/leaves Move mode; directions move the selected element by one 3×6 grid cell.

If the destination is occupied by exactly one element with identical dimensions, the elements swap positions. Invalid overlaps are rejected.
