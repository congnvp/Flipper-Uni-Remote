# UI system

## Orientation

Hold Flipper Zero **clockwise in portrait orientation**: screen above, controls below.

Logical display size is 64×128 pixels. The UI renderer rotates logical pixels onto the native 128×64 LCD.

## Base grid

The design language uses a conceptual **3 columns × 6 rows** grid. Pixel cuts are approximately:

- columns: 21 / 21 / 22 px
- rows: 21 / 21 / 22 / 21 / 21 / 22 px

Elements may span cells.

## Element rules

- Button border: 1 pixel.
- At least 1 pixel clearance between border and icon.
- Active/pressed buttons invert background and glyph.
- English labels are preferred and typically limited to three characters inside compact controls.
- Vertical steppers use taller up/down buttons and keep the center value about 1–2 pixels from the button borders.

## Current remote page

- status: 3×1
- display: 3×2
- horizontal left/right controller: 3×1
- bottom controls: two rows, with vertical controller and center power/OK block

The right bottom block is intentionally reserved for future enum/state controls.
