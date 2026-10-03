# Install / test the 0.9 candidate

## From a GitHub Actions artifact

The CI artifact contains:

- the built `.fap`;
- all directories under `examples/`.

Install the FAP with qFlipper/uFBT or copy it to the appropriate Apps directory for the firmware build.

Copy the remote directories you want to use into:

```text
/ext/apps_data/flipper_uni_remote/remotes/
```

Example:

```text
/ext/apps_data/flipper_uni_remote/remotes/sony_rm_pj8/
/ext/apps_data/flipper_uni_remote/remotes/optoma_hr21g_yhgd03/
/ext/apps_data/flipper_uni_remote/remotes/lg_ac/
/ext/apps_data/flipper_uni_remote/remotes/daikin_arc433a73/
/ext/apps_data/flipper_uni_remote/remotes/bluetooth_media/
```

Each directory must keep its own `remote.ur`, `signals.ir`, `actions.ur` and README. `state.urs` is created/updated locally for stateful remotes.

## IR remotes

No additional setup is required after copying the package.

The Sony package uses `IrBurst: 3` to preserve the three-frame SIRC behavior of the known-working standalone implementation.

## Stateful AC remotes

The first open uses adapter defaults if no valid `state.urs` exists.

Default temperature:

- LG: 23 °C;
- Daikin: 23 °C.

Remember: the displayed state is LOCAL. If somebody uses another physical remote, the appliance state and this FAP's remembered state can diverge.

## Bluetooth Media

Open the Bluetooth Media remote. The FAP activates its BLE HID profile and advertises using the selected `BluetoothProfile` identity.

Pair it from the target host.

Bond data is stored under the app's private data area, separately per profile. It is not stored inside `remotes/bluetooth_media`.

Leaving the BT remote restores the default Flipper Bluetooth profile.

## Updating a profile

If `remote.ur`, `signals.ir` or `actions.ur` is edited externally, use:

```text
MENU -> RELOAD
```

CI-valid example packages can be checked locally with:

```bash
python3 tools/check_profiles.py
```

## 0.9 hardware checklist

Before calling the project v1.0.0, test:

1. Sony IR.
2. Optoma IR.
3. LG AC state + TX + restore.
4. Daikin AC state + TX + restore.
5. Bluetooth first pair + reconnect + media/navigation.
6. Repeat/hold, page switching, remote switching and RELOAD stress.

Record evidence in `docs/TEST_MATRIX.md`.
