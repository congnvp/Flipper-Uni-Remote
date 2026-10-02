# Layout library

The renderer uses a 3×6 logical grid.

## Element presets

| ID | Element | Size |
| --- | --- | --- |
| `btn11` | Button | 1×1 |
| `btn12` | Tall button | 1×2 |
| `btn21` | Wide button | 2×1 |
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


## Editor reflow rules

Runtime focus and layout selection are separate. Status and screen are non-focusable during remote use, but are selectable, movable, replaceable and removable in Layout Editor.

Move does not require equal element dimensions. If the destination overlaps another element, the displaced element is reflowed to the nearest valid rectangle, preferring the region just vacated.

ADD uses a free rectangle when available. If none exists, the preset's preferred region is used as an explicit region replacement. For example, D-pad `d33` prefers rows 4-6 (logical `0,3,3,3`) and removes controls occupying that region.

REPLACE transforms the selected element in place. If old and new presets are the same type, mappings/icons/labels are preserved while size changes. Overlapped neighboring elements are removed to keep the layout valid.


## Occupancy masks

The `Rect` remains the element's rendering/anchor box, but collision is evaluated cell-by-cell.

For a D-pad at `Rect: 0 3 3 3`, occupied cells are:

```text
. X .
X X X
. X .
```

The four `.` cells are real free layout slots. A 1×1 button can be placed in any of them.

A 1×2 or 2×1 element follows the same rule: it is legal whenever all cells it occupies are free, even if its bounding rectangle overlaps another masked element. The four D-pad corners themselves are not adjacent, so a 1×2 cannot fit wholly inside only those four corner cells; it can still partially overlap the D-pad bounding box when its second cell lies in another free adjacent slot.

Move, Add, Replace, reflow, runtime focus navigation and Layout Editor selection all use this same occupancy model.
