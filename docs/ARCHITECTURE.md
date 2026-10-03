# Architecture

## Core rule

The UI owns geometry and presentation. Transports own device communication. Protocol bytes and bond secrets do not belong in layout code.

```text
Physical key
  -> Controller
  -> binding
  -> transport dispatcher
       IR       -> Action Engine -> IR transport
       STATE_IR -> State Engine -> adapter -> IR TX
       BT       -> BLE HID transport
```

## Remote model

`src/remote.h` defines transport-neutral UI elements plus portable package metadata:

- ID/name/order;
- Favourite/folder;
- page count and element page;
- IR burst policy;
- signal/action filenames;
- state adapter/state filename;
- Bluetooth profile identity.

The model does not embed vendor IR protocol frames.

## Store and memory

`src/remote_store.c` scans `APP_DATA_PATH("remotes")`.

Chooser/library views retain lightweight remote metadata only. Full element arrays are lazy-loaded for the active remote and released when no longer needed. Category calculations therefore do not reintroduce the pre-v0.4.1 memory problem.

The store applies fixed upper bounds to hostile/corrupt SD data.

## Controller

`src/controller.c` owns key policy, focus and capture.

Long Back is reserved globally. Runtime focus navigation is page-aware and uses logical occupied cells rather than raw bounding boxes.

D-pad occupies a five-cell cross inside a 3×3 bound, allowing corner controls.

## Ordinary IR

`src/action_engine.c` resolves `sig:` and `act:` bindings.

`src/ir_transport.c` reads standard Flipper IR signal files and transmits parsed/raw signals. Parsed signals can use the remote's `IrBurst`; raw signals retain their captured timing payload.

## Stateful IR

`src/state_engine.c` owns LOCAL remembered state and `state.urs` persistence.

`src/state_adapter_lg.c` and `src/state_adapter_daikin.c` own vendor encoding and normalization. TEMP/FAN/MODE mutate state and regenerate the appropriate frame rather than pretending to be independent learned buttons.

Displayed AC state is LOCAL, not confirmed appliance feedback.

## Bluetooth HID

`src/bt_transport.c` starts a BLE HID profile only while a BT remote is active.

Each `BluetoothProfile` receives:

- a stable profile-derived BLE identity;
- a separate app-private key-storage file;
- media/keyboard HID report execution.

Leaving the BT remote releases HID keys, disconnects, restores the default key path and restores the default Flipper Bluetooth profile.

## UI

The logical canvas is 64×128 portrait, mapped onto the native 128×64 LCD.

Packages store logical 3×6 rectangles, not pixels. Multi-page layouts reuse the same logical grid per page. The editor operates on the same geometry and transport-valid action model as runtime.

## Validation

`tools/check_profiles.py` statically validates portable package structure before the FAP compiles. Hardware acceptance remains outside static validation and is recorded in `TEST_MATRIX.md`.
