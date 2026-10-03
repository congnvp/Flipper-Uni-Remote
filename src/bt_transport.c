#include "bt_transport.h"

#include <extra_profiles/hid_profile.h>
#include <bt/bt_service/bt.h>
#include <furi.h>
#include <furi_hal_bt.h>
#include <furi_hal_usb_hid.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNI_BT_DIR APP_DATA_PATH("bt")
#define UNI_BT_PREFIX "UniRem"

struct UniBtTransport {
    Storage* storage;
    Bt* bt;
    FuriHalBleProfileBase* profile;
    bool active;
    volatile bool connected;
    uint32_t profile_hash;
    char profile_id[32];
    char keys_path[160];
    BleProfileHidParams params;
};

static uint32_t hash_profile(const char* text) {
    uint32_t hash = 2166136261UL;
    if(!text || !text[0]) text = "default";
    while(*text) {
        hash ^= (uint8_t)*text++;
        hash *= 16777619UL;
    }
    return hash;
}

static void status_callback(BtStatus status, void* context) {
    UniBtTransport* transport = context;
    if(!transport) return;
    transport->connected = status == BtStatusConnected;
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
    if(transport->profile) {
        ble_profile_hid_kb_release_all(transport->profile);
        ble_profile_hid_consumer_key_release_all(transport->profile);
        ble_profile_hid_mouse_release_all(transport->profile);
    }

    bt_disconnect(transport->bt);
    furi_delay_ms(200);
    bt_keys_storage_set_default_path(transport->bt);
    bt_profile_restore_default(transport->bt);

    transport->profile = NULL;
    transport->active = false;
    transport->connected = false;
    transport->profile_id[0] = '\0';
    transport->profile_hash = 0;
}

void uni_bt_transport_free(UniBtTransport* transport) {
    if(!transport) return;
    uni_bt_transport_deactivate(transport);
    if(transport->bt) furi_record_close(RECORD_BT);
    free(transport);
}

bool uni_bt_transport_activate(UniBtTransport* transport, const char* profile_id) {
    if(!transport || !transport->bt) return false;
    if(!profile_id || !profile_id[0]) profile_id = "default";

    const uint32_t hash = hash_profile(profile_id);
    if(transport->active && transport->profile_hash == hash &&
       strcmp(transport->profile_id, profile_id) == 0) {
        return true;
    }

    uni_bt_transport_deactivate(transport);

    snprintf(transport->profile_id, sizeof(transport->profile_id), "%.31s", profile_id);
    transport->profile_hash = hash;
    snprintf(
        transport->keys_path,
        sizeof(transport->keys_path),
        APP_DATA_PATH("bt/%08lX.keys"),
        (unsigned long)hash);

    bt_disconnect(transport->bt);
    furi_delay_ms(200);
    bt_keys_storage_set_storage_path(transport->bt, transport->keys_path);

    transport->params.device_name_prefix = UNI_BT_PREFIX;
    transport->params.mac_xor = (uint16_t)((hash ^ (hash >> 16)) | 0x0100U);
    transport->profile =
        bt_profile_start(transport->bt, ble_profile_hid, (void*)&transport->params);

    if(!transport->profile) {
        bt_keys_storage_set_default_path(transport->bt);
        bt_profile_restore_default(transport->bt);
        transport->profile_id[0] = '\0';
        transport->profile_hash = 0;
        return false;
    }

    transport->active = true;
    transport->connected = false;
    bt_set_status_changed_callback(transport->bt, status_callback, transport);
    furi_hal_bt_start_advertising();
    return true;
}

static bool send_keyboard(UniBtTransport* transport, uint16_t key) {
    bool ok = ble_profile_hid_kb_press(transport->profile, key);
    ok = ble_profile_hid_kb_release(transport->profile, key) && ok;
    return ok;
}

static bool send_consumer(UniBtTransport* transport, uint16_t key) {
    bool ok = ble_profile_hid_consumer_key_press(transport->profile, key);
    ok = ble_profile_hid_consumer_key_release(transport->profile, key) && ok;
    return ok;
}

static bool parse_u16(const char* text, uint16_t* out) {
    if(!text || !text[0] || !out) return false;
    char* end = NULL;
    unsigned long value = strtoul(text, &end, 0);
    if(end == text || *end != '\0' || value > 0xFFFFUL) return false;
    *out = (uint16_t)value;
    return true;
}

bool uni_bt_transport_execute(UniBtTransport* transport, const char* binding, bool repeat) {
    UNUSED(repeat);
    if(!transport || !transport->active || !transport->profile ||
       !binding || strncmp(binding, "bt:", 3) != 0) {
        return false;
    }

    const char* action = binding + 3;

    if(strcmp(action, "up") == 0) return send_keyboard(transport, HID_KEYBOARD_UP_ARROW);
    if(strcmp(action, "down") == 0) return send_keyboard(transport, HID_KEYBOARD_DOWN_ARROW);
    if(strcmp(action, "left") == 0) return send_keyboard(transport, HID_KEYBOARD_LEFT_ARROW);
    if(strcmp(action, "right") == 0) return send_keyboard(transport, HID_KEYBOARD_RIGHT_ARROW);
    if(strcmp(action, "ok") == 0 || strcmp(action, "enter") == 0)
        return send_keyboard(transport, HID_KEYBOARD_RETURN);
    if(strcmp(action, "space") == 0) return send_keyboard(transport, HID_KEYBOARD_SPACEBAR);
    if(strcmp(action, "esc") == 0) return send_keyboard(transport, HID_KEYBOARD_ESCAPE);

    if(strcmp(action, "vol+") == 0)
        return send_consumer(transport, HID_CONSUMER_VOLUME_INCREMENT);
    if(strcmp(action, "vol-") == 0)
        return send_consumer(transport, HID_CONSUMER_VOLUME_DECREMENT);
    if(strcmp(action, "mute") == 0) return send_consumer(transport, HID_CONSUMER_MUTE);
    if(strcmp(action, "play") == 0 || strcmp(action, "playpause") == 0)
        return send_consumer(transport, HID_CONSUMER_PLAY_PAUSE);
    if(strcmp(action, "next") == 0)
        return send_consumer(transport, HID_CONSUMER_SCAN_NEXT_TRACK);
    if(strcmp(action, "prev") == 0)
        return send_consumer(transport, HID_CONSUMER_SCAN_PREVIOUS_TRACK);
    if(strcmp(action, "stop") == 0) return send_consumer(transport, HID_CONSUMER_STOP);
    if(strcmp(action, "home") == 0) return send_consumer(transport, HID_CONSUMER_AC_HOME);
    if(strcmp(action, "back") == 0) return send_consumer(transport, HID_CONSUMER_AC_BACK);
    if(strcmp(action, "power") == 0) return send_consumer(transport, HID_CONSUMER_POWER);

    if(strncmp(action, "kb:", 3) == 0) {
        uint16_t key = 0;
        return parse_u16(action + 3, &key) && send_keyboard(transport, key);
    }
    if(strncmp(action, "cc:", 3) == 0) {
        uint16_t key = 0;
        return parse_u16(action + 3, &key) && send_consumer(transport, key);
    }

    return false;
}

bool uni_bt_transport_is_active(const UniBtTransport* transport) {
    return transport && transport->active;
}

bool uni_bt_transport_is_connected(const UniBtTransport* transport) {
    return transport && transport->active && transport->connected;
}

bool uni_bt_transport_forget_profile(UniBtTransport* transport, const char* profile_id) {
    if(!transport || !transport->bt) return false;
    if(!profile_id || !profile_id[0]) profile_id = "default";

    const uint32_t hash = hash_profile(profile_id);
    const bool was_active =
        transport->active && transport->profile_hash == hash &&
        strcmp(transport->profile_id, profile_id) == 0;

    if(was_active) uni_bt_transport_deactivate(transport);

    char keys_path[160];
    snprintf(
        keys_path,
        sizeof(keys_path),
        APP_DATA_PATH("bt/%08lX.keys"),
        (unsigned long)hash);
    const FS_Error status = storage_common_remove(transport->storage, keys_path);
    const bool removed = status == FSE_OK || status == FSE_NOT_EXIST;

    if(was_active) {
        return removed && uni_bt_transport_activate(transport, profile_id);
    }
    return removed;
}
