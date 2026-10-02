# Flipper Uni Remote

Development rewrite of the Flipper Zero universal-remote FAP.

Current baseline: **v0.1.0 DEV**.

This branch intentionally starts from the UI/UX layer before the new runtime core is added. The FAP currently provides an interactive prototype for validating navigation, the 3×6 portrait grid, Main Screen categories, Settings, Remote Manager, per-remote management, layout/control mapping flows, and IR-file import flows.

## UI model

```text
MAIN SCREEN
├── FAVOURITE
├── user folders...
├── UNCATEGORIZED
└── SETTINGS
    ├── REMOTE MANAGER
    │   └── <remote>
    │       ├── GENERAL
    │       ├── LAYOUT
    │       ├── CONTROLS
    │       ├── IR FILES
    │       ├── DUPLICATE
    │       └── DELETE
    ├── FOLDER MANAGER
    ├── APP SETTINGS
    ├── DATA
    └── ABOUT
```

Main Screen uses a vertical text-only remote list. LEFT/RIGHT switches pages in a loop:

```text
FAVOURITE -> user folders -> UNCATEGORIZED -> FAVOURITE
```

Runtime remote screens use the fixed logical **3 columns × 6 rows** grid.

## DEV build

The development branch is:

```text
dev/ui-v0.1.0
```

GitHub Actions builds the branch with official uFBT and publishes an artifact whose name begins:

```text
flipper-uni-remote-dev-v0.1.0-
```

The on-device About screen also identifies the build as `V0.1.0 DEV`.

## Scope of v0.1.0 DEV

Implemented for UI testing:

- Main Screen with FAVOURITE first.
- User-folder examples and system UNCATEGORIZED page.
- Circular LEFT/RIGHT page navigation.
- Vertical text-only remote list.
- SETTINGS entry on Main Screen.
- Remote Manager and per-remote management flow.
- Separate Layout, Controls, and IR Files management screens.
- 3×6 grid runtime previews for TV, AC, Projector, and Generic remotes.
- Mapping-flow mock UI.
- IR import-flow mock UI with FLIPPER IR and BROWSE STORAGE choices.
- English-only system UI.
- Long Back remains system escape.
- Physical portrait direction mapping retained from the previously hardware-verified build.

Not implemented yet:

- IR transmission.
- Persistent storage.
- Real Flipper file browser.
- Copying `.ir` files.
- Text input / rename.
- Layout persistence.
- Real control mapping.
- Folder persistence.
- Runtime/editor RAM-domain implementation.

These are intentionally deferred until the UI/UX is approved.

## Build locally

```bash
python3 tools/check_version.py
ufbt
```

## License

MIT.
