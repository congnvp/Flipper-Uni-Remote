# UI/UX v0.1 DEV specification

Status: UI prototype baseline.

## Global rules

- FAP system UI is English only.
- User-provided remote names, folder names, and custom labels are not translated.
- Native LCD is 128×64; the app uses a logical portrait canvas of 64×128.
- Remote layout boundary is a 3-column × 6-row logical grid.
- Long Back is always reserved for system escape.

## Main Screen

Page order:

```text
FAVOURITE
-> user-defined folders
-> UNCATEGORIZED
-> FAVOURITE
```

LEFT/RIGHT loops through pages.

UP/DOWN moves through the vertical remote list and SETTINGS.

OK opens the highlighted remote or SETTINGS.

Remote rows are text only; no per-remote icons.

FAVOURITE is a system page, not a physical folder.

UNCATEGORIZED is a system page and is always the final page before wrapping to FAVOURITE.

## Naming

New remotes receive unique generated names:

```text
untitled
untitled_2
untitled_3
...
```

Automatic creation may add the suffix.

Manual rename to an existing name should warn and require a different name rather than silently adding a suffix.

## Management separation

Runtime and editing are separate user flows.

```text
MAIN
├── REMOTE RUNTIME
└── SETTINGS
    └── REMOTE MANAGER
        └── REMOTE
            ├── GENERAL
            ├── LAYOUT
            ├── CONTROLS
            └── IR FILES
```

Runtime is the fast path.

Layout, Controls, IR Files, Settings, and management are the slow path and may load/unload their own data to minimize RAM.

## RAM lifecycle requirement

Only the data needed for the current domain should be hot.

```text
HOME DOMAIN
-> RUNTIME DOMAIN
-> cleanup
-> HOME DOMAIN

HOME DOMAIN
-> MANAGEMENT DOMAIN
-> LAYOUT / CONTROLS / IR FILE DOMAIN
-> cleanup
-> MANAGEMENT DOMAIN
```

The final core should follow these requirements:

1. Main Screen keeps lightweight catalog metadata only.
2. Only one active remote is loaded for runtime.
3. Only one active page is hot.
4. Runtime control actions must be fast after the remote is visible.
5. Editor/management operations may trade latency for lower RAM.
6. Exiting a remote releases its runtime cache.
7. Exiting an editor releases editor-specific allocations.
8. No large surprise allocation should happen while pressing runtime controls.

## IR acquisition and ownership

Learning/capturing IR signals remains the responsibility of Flipper Zero's stock Infrared app.

This FAP should provide import/mapping only.

IR import choices:

```text
FLIPPER IR
BROWSE STORAGE
```

The selected source `.ir` file is copied into the selected remote's own directory. Runtime should use the owned copy, not depend on the original source path.

Each remote is intended to own a folder containing its configuration and copied IR source files.

The exact persistent file schema is deliberately not defined in this UI phase.
