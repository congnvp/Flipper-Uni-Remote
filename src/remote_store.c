#include "remote_store.h"

#include <flipper_format/flipper_format.h>
#include <furi.h>
#include <stdio.h>
#include <string.h>

#define UNI_REMOTE_FILETYPE "Flipper Uni Remote"
#define UNI_REMOTE_VERSION 1U
#define UNI_REMOTES_DIR APP_DATA_PATH("remotes")
#define UNI_DEFAULT_DIR APP_DATA_PATH("remotes/demo_tv")
#define UNI_DEFAULT_REMOTE APP_DATA_PATH("remotes/demo_tv/remote.ur")
#define UNI_DEFAULT_SIGNALS APP_DATA_PATH("remotes/demo_tv/signals.ir")

static bool write_text_file(Storage* storage, const char* path, const char* text) {
    File* file = storage_file_alloc(storage);
    if(!file) return false;

    bool ok = storage_file_open(file, path, FSAM_WRITE, FSOM_CREATE_ALWAYS);
    if(ok) {
        const size_t length = strlen(text);
        ok = storage_file_write(file, text, length) == length;
        storage_file_sync(file);
    }
    storage_file_close(file);
    storage_file_free(file);
    return ok;
}

static void ensure_default_package(Storage* storage) {
    storage_common_mkdir(storage, UNI_REMOTES_DIR);
    storage_common_mkdir(storage, UNI_DEFAULT_DIR);

    if(!storage_file_exists(storage, UNI_DEFAULT_REMOTE)) {
        static const char remote_text[] =
            "Filetype: Flipper Uni Remote\n"
            "Version: 1\n"
            "Id: demo_tv\n"
            "Name: Demo TV\n"
            "ShortName: TV\n"
            "Transport: IR\n"
            "Order: 10\n"
            "RepeatEnabled: true\n"
            "SignalFile: signals.ir\n"
            "BluetoothProfile: tv_demo\n"
            "ElementCount: 5\n"
            "#\n"
            "Element0Type: status\n"
            "Element0Id: status\n"
            "Element0Rect: 0 0 3 1\n"
            "#\n"
            "Element1Type: screen\n"
            "Element1Id: main\n"
            "Element1Rect: 0 1 3 2\n"
            "Element1Label: READY\n"
            "#\n"
            "Element2Type: hstep\n"
            "Element2Id: channel\n"
            "Element2Rect: 0 3 3 1\n"
            "Element2Label: CH\n"
            "Element2Left: Prev\n"
            "Element2Right: Next\n"
            "#\n"
            "Element3Type: vstep\n"
            "Element3Id: volume\n"
            "Element3Rect: 0 4 1 2\n"
            "Element3Label: VOL\n"
            "Element3Up: VolUp\n"
            "Element3Down: VolDown\n"
            "#\n"
            "Element4Type: button\n"
            "Element4Id: power\n"
            "Element4Rect: 1 4 1 2\n"
            "Element4Label: PWR\n"
            "Element4Tap: Power\n"
            "Element4Hold: Mute\n";
        write_text_file(storage, UNI_DEFAULT_REMOTE, remote_text);
    }

    if(!storage_file_exists(storage, UNI_DEFAULT_SIGNALS)) {
        static const char signal_text[] =
            "Filetype: IR signals file\n"
            "Version: 1\n"
            "#\n"
            "name: Power\n"
            "type: parsed\n"
            "protocol: NEC\n"
            "address: 00 00 00 00\n"
            "command: 45 00 00 00\n"
            "#\n"
            "name: Mute\n"
            "type: parsed\n"
            "protocol: NEC\n"
            "address: 00 00 00 00\n"
            "command: 47 00 00 00\n"
            "#\n"
            "name: VolUp\n"
            "type: parsed\n"
            "protocol: NEC\n"
            "address: 00 00 00 00\n"
            "command: 18 00 00 00\n"
            "#\n"
            "name: VolDown\n"
            "type: parsed\n"
            "protocol: NEC\n"
            "address: 00 00 00 00\n"
            "command: 52 00 00 00\n"
            "#\n"
            "name: Prev\n"
            "type: parsed\n"
            "protocol: NEC\n"
            "address: 00 00 00 00\n"
            "command: 08 00 00 00\n"
            "#\n"
            "name: Next\n"
            "type: parsed\n"
            "protocol: NEC\n"
            "address: 00 00 00 00\n"
            "command: 5A 00 00 00\n";
        write_text_file(storage, UNI_DEFAULT_SIGNALS, signal_text);
    }
}

static bool ff_read_string(
    FlipperFormat* ff,
    const char* key,
    char* out,
    size_t out_size,
    bool required) {
    FuriString* value = furi_string_alloc();
    flipper_format_rewind(ff);
    const bool found = flipper_format_read_string(ff, key, value);
    if(found) snprintf(out, out_size, "%s", furi_string_get_cstr(value));
    furi_string_free(value);
    return found || !required;
}

static bool ff_read_u32(FlipperFormat* ff, const char* key, uint32_t* value, bool required) {
    flipper_format_rewind(ff);
    const bool found = flipper_format_read_uint32(ff, key, value, 1);
    return found || !required;
}

static bool ff_read_bool(FlipperFormat* ff, const char* key, bool* value, bool required) {
    flipper_format_rewind(ff);
    const bool found = flipper_format_read_bool(ff, key, value, 1);
    return found || !required;
}

static bool ff_read_rect(FlipperFormat* ff, const char* key, UniElement* element) {
    uint32_t rect[4] = {0};
    flipper_format_rewind(ff);
    if(!flipper_format_read_uint32(ff, key, rect, 4)) return false;
    if(rect[0] >= 3 || rect[1] >= 6 || rect[2] == 0 || rect[3] == 0) return false;
    if(rect[0] + rect[2] > 3 || rect[1] + rect[3] > 6) return false;

    element->x = (uint8_t)rect[0];
    element->y = (uint8_t)rect[1];
    element->w = (uint8_t)rect[2];
    element->h = (uint8_t)rect[3];
    return true;
}

static bool parse_transport(const char* text, UniTransport* transport) {
    if(strcmp(text, "IR") == 0) {
        *transport = UniTransportInfrared;
        return true;
    }
    if(strcmp(text, "BT") == 0 || strcmp(text, "BLE_HID") == 0) {
        *transport = UniTransportBluetoothHid;
        return true;
    }
    if(strcmp(text, "STATE_IR") == 0) {
        *transport = UniTransportStatefulIr;
        return true;
    }
    return false;
}

static bool parse_element_type(const char* text, UniElementType* type) {
    if(strcmp(text, "status") == 0) *type = UniElementStatus;
    else if(strcmp(text, "screen") == 0) *type = UniElementScreen;
    else if(strcmp(text, "button") == 0) *type = UniElementButton;
    else if(strcmp(text, "hstep") == 0) *type = UniElementHStep;
    else if(strcmp(text, "vstep") == 0) *type = UniElementVStep;
    else if(strcmp(text, "dpad") == 0) *type = UniElementDpad;
    else return false;
    return true;
}

static bool load_element(FlipperFormat* ff, uint32_t index, UniElement* element) {
    char key[32];
    char type_text[16] = {0};
    memset(element, 0, sizeof(UniElement));

#define READ_ELEMENT_STRING(SUFFIX, FIELD, REQUIRED) \
    do { \
        snprintf(key, sizeof(key), "Element%lu" SUFFIX, (unsigned long)index); \
        if(!ff_read_string(ff, key, element->FIELD, sizeof(element->FIELD), REQUIRED)) return false; \
    } while(0)

    snprintf(key, sizeof(key), "Element%luType", (unsigned long)index);
    if(!ff_read_string(ff, key, type_text, sizeof(type_text), true)) return false;
    if(!parse_element_type(type_text, &element->type)) return false;

    READ_ELEMENT_STRING("Id", id, true);
    snprintf(key, sizeof(key), "Element%luRect", (unsigned long)index);
    if(!ff_read_rect(ff, key, element)) return false;
    READ_ELEMENT_STRING("Label", label, false);
    READ_ELEMENT_STRING("Tap", tap, false);
    READ_ELEMENT_STRING("Hold", hold, false);
    READ_ELEMENT_STRING("Up", up, false);
    READ_ELEMENT_STRING("Down", down, false);
    READ_ELEMENT_STRING("Left", left, false);
    READ_ELEMENT_STRING("Right", right, false);
    READ_ELEMENT_STRING("Ok", ok, false);
    READ_ELEMENT_STRING("OkHold", ok_hold, false);

#undef READ_ELEMENT_STRING
    return true;
}

static bool load_remote(Storage* storage, const char* folder, UniRemote* remote) {
    char config_path[UNI_PATH_MAX];
    snprintf(config_path, sizeof(config_path), UNI_REMOTES_DIR "/%s/remote.ur", folder);
    if(!storage_file_exists(storage, config_path)) return false;

    FlipperFormat* ff = flipper_format_file_alloc(storage);
    if(!ff) return false;

    bool ok = false;
    FuriString* filetype = furi_string_alloc();
    uint32_t version = 0;
    char transport_text[16] = {0};
    char signal_file[64] = "signals.ir";
    uint32_t element_count = 0;

    memset(remote, 0, sizeof(UniRemote));
    remote->repeat_enabled = true;
    snprintf(remote->id, sizeof(remote->id), "%s", folder);

    do {
        if(!flipper_format_file_open_existing(ff, config_path)) break;
        if(!flipper_format_read_header(ff, filetype, &version)) break;
        if(strcmp(furi_string_get_cstr(filetype), UNI_REMOTE_FILETYPE) != 0) break;
        if(version != UNI_REMOTE_VERSION) break;

        ff_read_string(ff, "Id", remote->id, sizeof(remote->id), false);
        if(!ff_read_string(ff, "Name", remote->name, sizeof(remote->name), true)) break;
        if(!ff_read_string(ff, "ShortName", remote->short_name, sizeof(remote->short_name), true))
            break;
        if(!ff_read_string(ff, "Transport", transport_text, sizeof(transport_text), true)) break;
        if(!parse_transport(transport_text, &remote->transport)) break;
        ff_read_u32(ff, "Order", &remote->order, false);
        ff_read_bool(ff, "RepeatEnabled", &remote->repeat_enabled, false);
        ff_read_string(ff, "SignalFile", signal_file, sizeof(signal_file), false);
        ff_read_string(
            ff,
            "BluetoothProfile",
            remote->bluetooth_profile,
            sizeof(remote->bluetooth_profile),
            false);
        if(!ff_read_u32(ff, "ElementCount", &element_count, true)) break;
        if(element_count > UNI_MAX_ELEMENTS) break;

        for(uint32_t i = 0; i < element_count; i++) {
            if(!load_element(ff, i, &remote->elements[i])) goto done;
        }
        remote->element_count = element_count;
        snprintf(
            remote->signal_path,
            sizeof(remote->signal_path),
            UNI_REMOTES_DIR "/%s/%s",
            folder,
            signal_file);
        ok = true;
    } while(false);

done:
    flipper_format_file_close(ff);
    furi_string_free(filetype);
    flipper_format_free(ff);
    return ok;
}

static void sort_remotes(UniRemoteStore* store) {
    for(size_t i = 0; i < store->count; i++) {
        for(size_t j = i + 1; j < store->count; j++) {
            if(store->remotes[j].order < store->remotes[i].order) {
                UniRemote tmp = store->remotes[i];
                store->remotes[i] = store->remotes[j];
                store->remotes[j] = tmp;
            }
        }
    }
}

static void scan_remotes(UniRemoteStore* store) {
    store->count = 0;
    File* dir = storage_file_alloc(store->storage);
    if(!dir) return;

    if(storage_dir_open(dir, UNI_REMOTES_DIR)) {
        FileInfo info;
        char name[64];
        while(store->count < UNI_MAX_REMOTES &&
              storage_dir_read(dir, &info, name, sizeof(name))) {
            if(!(info.flags & FSF_DIRECTORY)) continue;
            if(load_remote(store->storage, name, &store->remotes[store->count])) {
                store->count++;
            }
        }
    }

    storage_dir_close(dir);
    storage_file_free(dir);
    sort_remotes(store);
}

bool uni_remote_store_init(UniRemoteStore* store, Storage* storage) {
    if(!store || !storage) return false;
    memset(store, 0, sizeof(UniRemoteStore));
    store->storage = storage;

    storage_common_mkdir(storage, UNI_REMOTES_DIR);
    scan_remotes(store);
    if(store->count == 0) {
        ensure_default_package(storage);
        scan_remotes(store);
    }

    return store->count > 0;
}

size_t uni_remote_store_count(const UniRemoteStore* store) {
    return store ? store->count : 0;
}

const UniRemote* uni_remote_store_get(const UniRemoteStore* store, size_t index) {
    if(!store || index >= store->count) return NULL;
    return &store->remotes[index];
}

size_t uni_remote_store_find_id(const UniRemoteStore* store, const char* id) {
    if(!store || !id) return 0;
    for(size_t i = 0; i < store->count; i++) {
        if(strcmp(store->remotes[i].id, id) == 0) return i;
    }
    return 0;
}
