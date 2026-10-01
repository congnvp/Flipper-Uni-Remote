# Architecture

## Core rule

The UI does not know IR protocol bits and transports do not know layout geometry.

```text
Element -> Controller -> Action name -> Transport
State   -> UI display elements
```

## Remote model

`src/remote.h` defines portable remote concepts: transport, elements, grid rectangle, signal bindings, ordering and future Bluetooth profile identity.

## Store

`src/remote_store.c` scans `APP_DATA_PATH("remotes")`. Each valid directory contains `remote.ur`; the signal file referenced by that configuration remains a standard Flipper `.ir` file.

The store intentionally keeps a fixed upper bound in v0.x so corrupted SD data cannot cause uncontrolled allocations. The limits can be raised or replaced by a dynamic vector later without changing the file format.

## Controller

`src/controller.c` owns hard-key policy and focus capture.

Priority:

```text
SYSTEM RESERVED
    -> FOCUS CAPTURE
    -> ELEMENT BINDING
    -> future REMOTE KEYMAP
    -> future GLOBAL DEFAULT
```

Long Back is evaluated first and is never exposed as a configurable binding.

## IR transport

`src/ir_transport.c` opens the remote's `.ir` file, finds a signal by `name`, and transmits it through the firmware infrared signal API. Parsed signals can receive protocol repeat semantics; raw signals are retransmitted as captured.

## UI

The logical canvas is 64×128 portrait and maps to the physical 128×64 LCD using:

```text
logical (x, y) -> native (y, 63 - x)
```

The file format stores grid rectangles, not pixel positions. Renderer changes therefore do not require rewriting user remote packages.

## Stateful IR

`STATE_IR` is a declared transport type but not executed in v0.2. Stateful appliances such as many air conditioners will use a state encoder/decoder driver, not a fake collection of independent button frames.

The future state file should distinguish at least:

- `LOCAL`: state last sent/remembered by the FAP.
- `CONFIRMED`: state verified through a feedback channel.
- `UNKNOWN`: state cannot be trusted.

## Bluetooth

`BluetoothProfile` is already part of the remote package, but pairing keys are not. The BLE transport must keep identity/bond storage in app-private storage and test whether firmware-exported APIs permit per-remote identity switching before claiming strict isolation between two identical nearby hosts.
