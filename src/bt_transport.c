#include "bt_transport.h"

#include <bt/bt_service/bt.h>
#include <extra_profiles/hid_profile.h>
#include <furi.h>
#include <furi_hal_bt.h>
#include <furi_hal_usb_hid.h>
#include <stdio.h>
#include <string.h>

#define UNI_BT_DIR APP_DATA_PATH("bt")

struct UniBtTransport {
    Storage* storage;
    Bt* bt;
    FuriHalBleProfileBase* hid;
    BleProfileHidParams params;
    char active_profile[UNI_BT_PROFILE_MAX];
    char prefix[8];
    char keys_path[UNI_PATH_MAX];
    volatile bool connected;
    bool active;
};

static void bt_status_changed(BtStatus status, void* context) {
    UniBtTransport* transport = context;
    if(transport) transport->connected = status == BtStatusConnected;
}

static uint16_t profile_hash(const char* text) {
    uint16_t hash = 0x5A5AU;
    if(!text) return hash;
    while(*text) {
        hash = (uint16_t)((hash << 5) ^ (hash >> 3) ^ (uint8_t)*text++);
    }
    return hash ? hash : 1U;
}

static void sanitize_profile_name(
    const char* input,
    char* output,
    size_t output_size,
    bool prefix_mode) {
    if(!output || output_size == 0) return;
    size_t out = 0;
    if(input) {
        for(size_t i = 0; input[i] && out + 1 < output_size; i++) {
            char ch = input[i];
            const bool valid =
                (ch >= 'a' && ch <= 'z') ||
                (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9') ||
                (!prefix_mode && (ch == '_' || ch == '-'));
            if(valid) output[out++] = ch;
        }
    }
    output[out] = '\0';
    if(out == 0) snprintf(output, output_size, "%s", prefix_mode ? "UniRem" : "remote");
}

UniBtTransport* uni_bt_transport_alloc(Storage* storage) {
    if(!storage) return NULL;
    UniBtTransport* transport = calloc(1, sizeof(UniBtTransport));
    if(!transport) return NULL;
    transport->storage = storage;
    transport->bt = furi_record_open(RECORD_BT);
    if(!transport->bt) {
        free(transport);
        return NULL;
    }
    storage_common_mkdir(storage, UNI_BT_DIR);
    return transport;
}

void uni_bt_transport_deactivate(UniBtTransport* transport) {
    if(!transport || !transport->bt || !transport->active) return;

    bt_set_status_changed_callback(transport->bt, NULL, NULL);
    if(transport->hid) {
        ble_profile_hid_kb_release_all(transport->hid);
        ble_profile_hid_consumer_key_release_all(transport->hid);
    }

    bt_disconnect(transport->bt);
    furi_delay_ms(200);
    bt_keys_storage_set_default_path(transport->bt);
    bt_profile_restore_default(transport->bt);

    transport->hid = NULL;
    transport->connected = false;
    transport->active = false;
    transport->active_profile[0] = '\0';
}

void uni_bt_transport_free(UniBtTransport* transport) {
    if(!transport) return;
    uni_bt_transport_deactivate(transport);
    if(transport->bt) furi_record_close(RECORD_BT);
    free(transport);
}

bool uni_bt_transport_activate(UniBtTransport* transport, const UniRemote* remote) {
    if(!transport || !remote || remote->transport != UniTransportBluetoothHid ||
       !remote->bluetooth_profile[0]) {
        return false;
    }

    if(transport->active &&
       strcmp(transport->active_profile, remote->bluetooth_profile) == 0) {
        return true;
    }

    uni_bt_transport_deactivate(transport);

    char safe[UNI_BT_PROFILE_MAX] = {0};
    sanitize_profile_name(remote->bluetooth_profile, safe, sizeof(safe), false);
    sanitize_profile_name(remote->bluetooth_profile, transport->prefix, sizeof(transport->prefix), true);
    snprintf(
        transport->keys_path,
        sizeof(transport->keys_path),
        UNI_BT_DIR "/%s.keys",
        safe);

    transport->params.device_name_prefix = transport->prefix;
    transport->params.mac_xor = profile_hash(remote->bluetooth_profile);

    bt_disconnect(transport->bt);
    furi_delay_ms(200);
    bt_keys_storage_set_storage_path(transport->bt, transport->keys_path);

    transport->hid = bt_profile_start(
        transport->bt,
        ble_profile_hid,
        (FuriHalBleProfileParams)&transport->params);
    if(!transport->hid) {
        bt_keys_storage_set_default_path(transport->bt);
        bt_profile_restore_default(transport->bt);
        return false;
    }

    snprintf(
        transport->active_profile,
        sizeof(transport->active_profile),
        "%s",
        remote->bluetooth_profile);
    transport->connected = false;
    transport->active = true;
    bt_set_status_changed_callback(transport->bt, bt_status_changed, transport);
    furi_hal_bt_start_advertising();
    return true;
}

static bool send_consumer(FuriHalBleProfileBase* hid, uint16_t code) {
    if(!hid) return false;
    if(!ble_profile_hid_consumer_key_press(hid, code)) return false;
    return ble_profile_hid_consumer_key_release(hid, code);
}

static bool send_keyboard(FuriHalBleProfileBase* hid, uint16_t code) {
    if(!hid) return false;
    if(!ble_profile_hid_kb_press(hid, code)) return false;
    return ble_profile_hid_kb_release(hid, code);
}

static bool media_code(const char* name, uint16_t* code) {
    if(strcmp(name, "play_pause") == 0) *code = HID_CONSUMER_PLAY_PAUSE;
    else if(strcmp(name, "next") == 0) *code = HID_CONSUMER_SCAN_NEXT_TRACK;
    else if(strcmp(name, "prev") == 0) *code = HID_CONSUMER_SCAN_PREVIOUS_TRACK;
    else if(strcmp(name, "stop") == 0) *code = HID_CONSUMER_STOP;
    else if(strcmp(name, "mute") == 0) *code = HID_CONSUMER_MUTE;
    else if(strcmp(name, "vol_up") == 0) *code = HID_CONSUMER_VOLUME_INCREMENT;
    else if(strcmp(name, "vol_down") == 0) *code = HID_CONSUMER_VOLUME_DECREMENT;
    else if(strcmp(name, "home") == 0) *code = HID_CONSUMER_AC_HOME;
    else if(strcmp(name, "back") == 0) *code = HID_CONSUMER_AC_BACK;
    else if(strcmp(name, "forward") == 0) *code = HID_CONSUMER_AC_FORWARD;
    else return false;
    return true;
}

static bool keyboard_code(const char* name, uint16_t* code) {
    if(strcmp(name, "up") == 0) *code = HID_KEYBOARD_UP_ARROW;
    else if(strcmp(name, "down") == 0) *code = HID_KEYBOARD_DOWN_ARROW;
    else if(strcmp(name, "left") == 0) *code = HID_KEYBOARD_LEFT_ARROW;
    else if(strcmp(name, "right") == 0) *code = HID_KEYBOARD_RIGHT_ARROW;
    else if(strcmp(name, "enter") == 0) *code = HID_KEYBOARD_RETURN;
    else if(strcmp(name, "escape") == 0) *code = HID_KEYBOARD_ESCAPE;
    else if(strcmp(name, "space") == 0) *code = HID_KEYBOARD_SPACEBAR;
    else if(strcmp(name, "tab") == 0) *code = HID_KEYBOARD_TAB;
    else if(strcmp(name, "page_up") == 0) *code = HID_KEYBOARD_PAGE_UP;
    else if(strcmp(name, "page_down") == 0) *code = HID_KEYBOARD_PAGE_DOWN;
    else return false;
    return true;
}

bool uni_bt_transport_send(
    UniBtTransport* transport,
    const UniRemote* remote,
    const char* binding,
    bool repeat) {
    UNUSED(repeat);
    if(!transport || !remote || !binding || strncmp(binding, "bt:", 3) != 0) return false;
    if(!uni_bt_transport_activate(transport, remote)) return false;
    if(!transport->connected) return false;

    const char* operation = binding + 3;
    uint16_t code = 0;
    if(strncmp(operation, "media:", 6) == 0) {
        if(!media_code(operation + 6, &code)) return false;
        return send_consumer(transport->hid, code);
    }
    if(strncmp(operation, "key:", 4) == 0) {
        if(!keyboard_code(operation + 4, &code)) return false;
        return send_keyboard(transport->hid, code);
    }
    return false;
}

bool uni_bt_transport_connected(const UniBtTransport* transport) {
    return transport && transport->active && transport->connected;
}
