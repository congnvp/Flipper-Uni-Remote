# Test Matrix

Use this file to distinguish compile/CI evidence from real hardware evidence.

Status values: `PASS`, `FAIL`, `TBD`, `N/A`.

## Baseline evidence

| Target | Build/CI | Runtime/package | Physical TX | Stateful persistence | Notes |
| --- | --- | --- | --- | --- | --- |
| v0.4.3 engine baseline | PASS | PASS* | TBD | N/A | GitHub Actions run #40 passed. Runtime features are present in code/docs; hardware evidence is tracked per device below. |
| Example living_tv | PASS | PASS* | TBD | N/A | Example package exists; not a substitute for target-device verification. |
| Sony RM-PJ8 | TBD | TBD | TBD | N/A | M1.1 |
| Optoma HR21G-YHGD03 | TBD | TBD | TBD | N/A | M1.2 |
| LG AC | TBD | TBD | TBD | TBD | M2 adapter 1 |
| Daikin ARC433A73 | TBD | TBD | TBD | TBD | M2 adapter 2 |
| Bluetooth HID media profile | TBD | TBD | TBD | N/A | M4 |

`PASS*` means the capability is present in the known-good baseline and the repository CI builds it, but this file does not yet contain a fresh dedicated regression record for that package.

## Required checks per code change

| Check | Required before merge |
| --- | --- |
| `python3 tools/check_version.py` | Yes |
| `ufbt` | Yes |
| `ufbt lint` | Yes; review warnings even if CI treats lint as advisory |
| GitHub Actions build | Yes |
| Package parse/open test | When package/store code changes |
| Navigation/layout regression | When controller/UI/layout code changes |
| Physical IR TX | When signal/transport/protocol behavior changes |
| State restore | When stateful persistence changes |
| Pair/reconnect | When Bluetooth code changes |

## Hardware test record format

Append concise evidence here instead of relying on chat memory.

```text
Date:
Device:
Flipper firmware:
App commit:
Remote package:
Tested:
Result:
Known limitations:
```
