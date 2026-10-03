# Bluetooth Media profile

BLE HID regression profile using the common `BT` transport.

- Stable bonding file per `BluetoothProfile`.
- Stable advertised-name prefix and MAC XOR derived from the profile ID.
- Page 1: media playback, track and volume.
- Page 2: keyboard navigation plus Home/Back/Escape/Space.

Bindings use `bt:media:<name>` or `bt:key:<name>`.

The BLE HID stack is linked with `fap_libs=["ble_profile"]`, matching external uFBT BLE HID applications. Physical pairing/reconnect remains a hardware validation item.
