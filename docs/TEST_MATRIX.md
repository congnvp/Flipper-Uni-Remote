# Test Matrix

Status values: `PASS`, `FAIL`, `TBD`, `N/A`.

A green build is not hardware evidence.

| Target | Build/CI | Package/schema | Physical TX / Pair | Persistence / reconnect | Notes |
| --- | --- | --- | --- | --- | --- |
| v0.4.3 baseline | PASS | PASS | TBD | N/A | Stable rollback baseline. |
| living_tv example | PASS | PASS | TBD | N/A | IR regression/example package. |
| Sony RM-PJ8 | PASS | PASS | TBD | N/A | 22 SIRC commands; 3 pages; parsed burst=3. |
| Optoma HR21G-YHGD03 | PASS | PASS | TBD | N/A | 18 NEC commands; 3 pages. |
| LG AC | PASS | PASS | TBD | TBD | STATE_IR encoder/state persistence compiled; physical AC confirmation pending. |
| Daikin ARC433A73 | PASS | PASS | TBD | TBD | 35-byte/584-timing STATE_IR adapter compiled; physical AC confirmation pending. |
| Bluetooth Media | PASS | PASS | TBD | TBD | BLE HID links through `ble_profile`; real pair/reconnect pending. |

Software integration evidence: PR #11. GitHub Actions run #209 passed version consistency, all six bundled profile validations, release-channel uFBT build, full-profile artifact upload and lint for the 0.9.0 candidate.

## Required software checks

| Check | Required |
| --- | --- |
| `python3 tools/check_version.py` | Yes |
| `python3 tools/check_profiles.py` | Yes |
| official release-channel `ufbt` | Yes |
| `ufbt lint` | Yes |
| artifact contains FAP + bundled profiles | Yes |

## Required hardware gate for v1.0.0

- Sony: open/navigate all pages; D-pad capture/release; all relevant IR commands accepted.
- Optoma: open/navigate all pages; all relevant NEC commands accepted.
- LG: Power, temp, mode, fan, supported swing; close/reopen LOCAL state restore; auxiliary commands sampled.
- Daikin: Power, temp, mode, fan, swing, Powerful; close/reopen LOCAL state restore.
- Bluetooth: first pair; reconnect after app exit/reopen; media keys; navigation keys; default Flipper BT profile restored after leaving the remote.
- Stress: repeated page switching, repeat/hold actions, remote switching and RELOAD without crash or stale state.

## Hardware test record

```text
Date:
Device / target:
Flipper firmware:
App commit:
Remote package:
Tested:
Result:
Known limitations:
```
