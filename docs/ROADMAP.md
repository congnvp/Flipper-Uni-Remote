# Roadmap to v1.0

Implementation status as of 2026-10-03:

- M0 baseline/handoff: COMPLETE.
- M1 stateless Sony + Optoma: SOFTWARE COMPLETE; hardware evidence TBD.
- M2 STATE_IR + LG + Daikin: SOFTWARE COMPLETE; hardware evidence TBD.
- M3 library/persistence UX: SOFTWARE COMPLETE.
- M4 BLE HID: SOFTWARE COMPLETE; physical pair/reconnect evidence TBD.
- M5 hardening/release candidate: SOFTWARE COMPLETE.
- M6 editor/macro completion: SOFTWARE COMPLETE in 0.10.0.

The code is intentionally not tagged v1.0.0 until the required physical rows in `TEST_MATRIX.md` are PASS.

## Completed architecture

```text
Element
  -> binding
  -> transport dispatcher
     -> IR Action Engine -> signal / nested sequence -> parsed/raw IR
     -> STATE_IR Engine -> adapter -> full/aux IR
     -> BLE HID Transport -> media/keyboard report
```

The UI/layout editor remains transport-agnostic. The on-device authoring layer now covers layout position/type, icon, label, page, binding, folder and IR sequence macros.

## v1.0 hardware gate

1. Sony RM-PJ8 physical TX/navigation.
2. Optoma HR21G-YHGD03 physical TX/navigation.
3. LG AC Power/temp/mode/fan/swing + state restore.
4. Daikin ARC433A73 Power/temp/mode/fan/swing + state restore.
5. Bluetooth Media pair/reconnect and default-profile restoration.
6. Switch/reload/repeat/hold stress on physical Flipper.
7. Update test matrix and release notes.
8. Tag `v1.0.0`.

No further software feature expansion is required before the current v1 hardware gate.
