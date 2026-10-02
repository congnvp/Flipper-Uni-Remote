#include "remote_store.h"

#include <flipper_format/flipper_format.h>
#include <furi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNI_REMOTE_FILETYPE "Flipper Uni Remote"
#define UNI_REMOTE_VERSION 1U
#define UNI_REMOTES_DIR APP_DATA_PATH("remotes")
#define UNI_DEFAULT_DIR APP_DATA_PATH("remotes/demo_tv")
#define UNI_DEFAULT_REMOTE APP_DATA_PATH("remotes/demo_tv/remote.ur")
#define UNI_DEFAULT_SIGNALS APP_DATA_PATH("remotes/demo_tv/signals.ir")
#define UNI_DEFAULT_ACTIONS APP_DATA_PATH("remotes/demo_tv/actions.ur")

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
            "ActionFile: actions.ur\n"
            "BluetoothProfile: tv_demo\n"
            "HardUpHold: \n"
            "HardDownHold: \n"
            "HardLeftHold: \n"
            "HardRightHold: \n"
            "HardOkHold: \n"
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
            "Element2Left: sig:Prev\n"
            "Element2Right: sig:Next\n"
            "#\n"
            "Element3Type: vstep\n"
            "Element3Id: volume\n"
            "Element3Rect: 0 4 1 2\n"
            "Element3Label: VOL\n"
            "Element3Up: sig:VolUp\n"
            "Element3Down: sig:VolDown\n"
            "#\n"
            "Element4Type: button\n"
            "Element4Id: power\n"
            "Element4Rect: 1 4 1 2\n"
            "Element4Label: PWR\n"
            "Element4Icon: pwr\n"
            "Element4Tap: sig:Power\n"
            "Element4Hold: sig:Mute\n";
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

    if(!storage_file_exists(storage, UNI_DEFAULT_ACTIONS)) {
        static const char action_text[] =
            "Filetype: Flipper Uni Remote Actions\n"
            "Version: 1\n"
            "ActionCount: 1\n"
            "#\n"
            "Action0Id: quiet\n"
            "Action0Type: sequence\n"
            "Action0StepCount: 2\n"
            "Action0Step0: VolDown\n"
            "Action0Delay0: 120\n"
            "Action0Step1: VolDown\n"
            "Action0Delay1: 0\n";
        write_text_file(storage, UNI_DEFAULT_ACTIONS, action_text);
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

static void read_element_string(
    FlipperFormat* ff,
    uint32_t index,
    const char* suffix,
    char* out,
    size_t out_size) {
    char key[40];
    snprintf(key, sizeof(key), "Element%lu%s", (unsigned long)index, suffix);
    ff_read_string(ff, key, out, out_size, false);
}

static bool load_element(FlipperFormat* ff, uint32_t index, UniElement* element) {
    char key[40];
    char type_text[16] = {0};
    memset(element, 0, sizeof(UniElement));

    snprintf(key, sizeof(key), "Element%luType", (unsigned long)index);
    if(!ff_read_string(ff, key, type_text, sizeof(type_text), true)) return false;
    if(!parse_element_type(type_text, &element->type)) return false;

    snprintf(key, sizeof(key), "Element%luId", (unsigned long)index);
    if(!ff_read_string(ff, key, element->id, sizeof(element->id), true)) return false;
    snprintf(key, sizeof(key), "Element%luRect", (unsigned long)index);
    if(!ff_read_rect(ff, key, element)) return false;

    uint32_t page = 0U;
    snprintf(key, sizeof(key), "Element%luPage", (unsigned long)index);
    ff_read_u32(ff, key, &page, false);
    if(page >= UNI_MAX_PAGES) return false;
    element->page = (uint8_t)page;

    read_element_string(ff, index, "Label", element->label, sizeof(element->label));
    read_element_string(ff, index, "Icon", element->icon, sizeof(element->icon));
    read_element_string(ff, index, "HoldIcon", element->hold_icon, sizeof(element->hold_icon));

    read_element_string(ff, index, "Tap", element->tap, sizeof(element->tap));
    read_element_string(ff, index, "Hold", element->hold, sizeof(element->hold));
    read_element_string(ff, index, "Up", element->up, sizeof(element->up));
    read_element_string(ff, index, "Down", element->down, sizeof(element->down));
    read_element_string(ff, index, "Left", element->left, sizeof(element->left));
    read_element_string(ff, index, "Right", element->right, sizeof(element->right));
    read_element_string(ff, index, "Ok", element->ok, sizeof(element->ok));
    read_element_string(ff, index, "UpHold", element->up_hold, sizeof(element->up_hold));
    read_element_string(ff, index, "DownHold", element->down_hold, sizeof(element->down_hold));
    read_element_string(ff, index, "LeftHold", element->left_hold, sizeof(element->left_hold));
    read_element_string(ff, index, "RightHold", element->right_hold, sizeof(element->right_hold));
    read_element_string(ff, index, "OkHold", element->ok_hold, sizeof(element->ok_hold));

    read_element_string(ff, index, "UpHoldIcon", element->up_hold_icon, sizeof(element->up_hold_icon));
    read_element_string(ff, index, "DownHoldIcon", element->down_hold_icon, sizeof(element->down_hold_icon));
    read_element_string(ff, index, "LeftHoldIcon", element->left_hold_icon, sizeof(element->left_hold_icon));
    read_element_string(ff, index, "RightHoldIcon", element->right_hold_icon, sizeof(element->right_hold_icon));
    read_element_string(ff, index, "OkHoldIcon", element->ok_hold_icon, sizeof(element->ok_hold_icon));

    snprintf(key, sizeof(key), "Element%luAltSticky", (unsigned long)index);
    ff_read_bool(ff, key, &element->alt_sticky, false);
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
    uint32_t element_count = 0;

    memset(remote, 0, sizeof(UniRemote));
    remote->repeat_enabled = true;
    remote->page_count = 1U;
    snprintf(remote->id, sizeof(remote->id), "%.23s", folder);
    snprintf(remote->signal_file, sizeof(remote->signal_file), "signals.ir");
    snprintf(remote->action_file, sizeof(remote->action_file), "actions.ur");
    snprintf(remote->state_file, sizeof(remote->state_file), "state.urs");

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

        uint32_t page_count = 1U;
        ff_read_u32(ff, "PageCount", &page_count, false);
        if(page_count == 0U || page_count > UNI_MAX_PAGES) break;
        remote->page_count = (uint8_t)page_count;

        ff_read_string(ff, "SignalFile", remote->signal_file, sizeof(remote->signal_file), false);
        ff_read_string(ff, "ActionFile", remote->action_file, sizeof(remote->action_file), false);
        ff_read_string(ff, "StateFile", remote->state_file, sizeof(remote->state_file), false);
        ff_read_string(
            ff,
            "StateDriver",
            remote->state_driver,
            sizeof(remote->state_driver),
            false);
        ff_read_string(
            ff,
            "BluetoothProfile",
            remote->bluetooth_profile,
            sizeof(remote->bluetooth_profile),
            false);

        ff_read_string(ff, "HardUpHold", remote->hard_bindings[UniHardUpHold], UNI_BINDING_MAX, false);
        ff_read_string(ff, "HardDownHold", remote->hard_bindings[UniHardDownHold], UNI_BINDING_MAX, false);
        ff_read_string(ff, "HardLeftHold", remote->hard_bindings[UniHardLeftHold], UNI_BINDING_MAX, false);
        ff_read_string(ff, "HardRightHold", remote->hard_bindings[UniHardRightHold], UNI_BINDING_MAX, false);
        ff_read_string(ff, "HardOkHold", remote->hard_bindings[UniHardOkHold], UNI_BINDING_MAX, false);

        if(!ff_read_u32(ff, "ElementCount", &element_count, true)) break;
        if(element_count > UNI_MAX_ELEMENTS) break;
        remote->element_count = element_count;
        remote->elements = NULL;
        remote->elements_loaded = false;

        char signal_file_copy[64];
        char action_file_copy[64];
        char state_file_copy[64];
        snprintf(signal_file_copy, sizeof(signal_file_copy), "%s", remote->signal_file);
        snprintf(action_file_copy, sizeof(action_file_copy), "%s", remote->action_file);
        snprintf(state_file_copy, sizeof(state_file_copy), "%s", remote->state_file);

        snprintf(remote->config_path, sizeof(remote->config_path), "%s", config_path);
        snprintf(
            remote->signal_path,
            sizeof(remote->signal_path),
            UNI_REMOTES_DIR "/%s/%s",
            folder,
            signal_file_copy);
        snprintf(
            remote->action_path,
            sizeof(remote->action_path),
            UNI_REMOTES_DIR "/%s/%s",
            folder,
            action_file_copy);
        snprintf(
            remote->state_path,
            sizeof(remote->state_path),
            UNI_REMOTES_DIR "/%s/%s",
            folder,
            state_file_copy);
        ok = true;
    } while(false);

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
    for(size_t i = 0; i < UNI_MAX_REMOTES; i++) {
        if(store->remotes[i].elements) {
            free(store->remotes[i].elements);
            store->remotes[i].elements = NULL;
            store->remotes[i].elements_loaded = false;
        }
    }
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

static bool write_string(FlipperFormat* ff, const char* key, const char* value) {
    return flipper_format_write_string_cstr(ff, key, value ? value : "");
}

static bool write_element(FlipperFormat* ff, size_t index, const UniElement* e) {
    char key[40];
    uint32_t rect[4] = {e->x, e->y, e->w, e->h};

#define WRITE_STR(SUFFIX, VALUE) \
    do { \
        snprintf(key, sizeof(key), "Element%lu" SUFFIX, (unsigned long)index); \
        if(!write_string(ff, key, VALUE)) return false; \
    } while(0)

    WRITE_STR("Type", uni_element_type_name(e->type));
    WRITE_STR("Id", e->id);
    snprintf(key, sizeof(key), "Element%luRect", (unsigned long)index);
    if(!flipper_format_write_uint32(ff, key, rect, 4)) return false;
    uint32_t page = e->page;
    snprintf(key, sizeof(key), "Element%luPage", (unsigned long)index);
    if(!flipper_format_write_uint32(ff, key, &page, 1)) return false;
    WRITE_STR("Label", e->label);
    WRITE_STR("Icon", e->icon);
    WRITE_STR("HoldIcon", e->hold_icon);
    WRITE_STR("Tap", e->tap);
    WRITE_STR("Hold", e->hold);
    WRITE_STR("Up", e->up);
    WRITE_STR("Down", e->down);
    WRITE_STR("Left", e->left);
    WRITE_STR("Right", e->right);
    WRITE_STR("Ok", e->ok);
    WRITE_STR("UpHold", e->up_hold);
    WRITE_STR("DownHold", e->down_hold);
    WRITE_STR("LeftHold", e->left_hold);
    WRITE_STR("RightHold", e->right_hold);
    WRITE_STR("OkHold", e->ok_hold);
    WRITE_STR("UpHoldIcon", e->up_hold_icon);
    WRITE_STR("DownHoldIcon", e->down_hold_icon);
    WRITE_STR("LeftHoldIcon", e->left_hold_icon);
    WRITE_STR("RightHoldIcon", e->right_hold_icon);
    WRITE_STR("OkHoldIcon", e->ok_hold_icon);
    snprintf(key, sizeof(key), "Element%luAltSticky", (unsigned long)index);
    if(!flipper_format_write_bool(ff, key, &e->alt_sticky, 1)) return false;

#undef WRITE_STR
    return true;
}

bool uni_remote_store_save(UniRemoteStore* store, size_t remote_index) {
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    if(!store || !remote || !remote->config_path[0] || !remote->elements_loaded ||
       !remote->elements) {
        return false;
    }

    FlipperFormat* ff = flipper_format_file_alloc(store->storage);
    if(!ff) return false;

    bool ok = false;
    do {
        if(!flipper_format_file_open_always(ff, remote->config_path)) break;
        if(!flipper_format_write_header_cstr(ff, UNI_REMOTE_FILETYPE, UNI_REMOTE_VERSION)) break;
        if(!write_string(ff, "Id", remote->id)) break;
        if(!write_string(ff, "Name", remote->name)) break;
        if(!write_string(ff, "ShortName", remote->short_name)) break;

        const char* transport = remote->transport == UniTransportInfrared ?
                                    "IR" :
                                remote->transport == UniTransportBluetoothHid ?
                                    "BT" :
                                    "STATE_IR";
        if(!write_string(ff, "Transport", transport)) break;
        if(!flipper_format_write_uint32(ff, "Order", &remote->order, 1)) break;
        if(!flipper_format_write_bool(ff, "RepeatEnabled", &remote->repeat_enabled, 1)) break;
        uint32_t page_count = remote->page_count ? remote->page_count : 1U;
        if(!flipper_format_write_uint32(ff, "PageCount", &page_count, 1)) break;
        if(!write_string(ff, "SignalFile", remote->signal_file)) break;
        if(!write_string(ff, "ActionFile", remote->action_file)) break;
        if(!write_string(ff, "StateFile", remote->state_file)) break;
        if(!write_string(ff, "StateDriver", remote->state_driver)) break;
        if(!write_string(ff, "BluetoothProfile", remote->bluetooth_profile)) break;

        if(!write_string(ff, "HardUpHold", remote->hard_bindings[UniHardUpHold])) break;
        if(!write_string(ff, "HardDownHold", remote->hard_bindings[UniHardDownHold])) break;
        if(!write_string(ff, "HardLeftHold", remote->hard_bindings[UniHardLeftHold])) break;
        if(!write_string(ff, "HardRightHold", remote->hard_bindings[UniHardRightHold])) break;
        if(!write_string(ff, "HardOkHold", remote->hard_bindings[UniHardOkHold])) break;

        uint32_t count = remote->element_count;
        if(!flipper_format_write_uint32(ff, "ElementCount", &count, 1)) break;
        for(size_t i = 0; i < remote->element_count; i++) {
            if(!write_element(ff, i, &remote->elements[i])) goto done;
        }
        ok = true;
    } while(false);

done:
    flipper_format_file_close(ff);
    flipper_format_free(ff);
    return ok;
}


void uni_remote_store_unload_details(UniRemoteStore* store, size_t remote_index) {
    if(!store || remote_index >= store->count) return;
    UniRemote* remote = &store->remotes[remote_index];
    if(remote->elements) {
        free(remote->elements);
        remote->elements = NULL;
    }
    remote->elements_loaded = false;
}

bool uni_remote_store_load_details(UniRemoteStore* store, size_t remote_index) {
    if(!store || remote_index >= store->count) return false;

    UniRemote* remote = &store->remotes[remote_index];
    if(remote->elements_loaded && remote->elements) return true;

    /* Keep at most one full remote layout resident in RAM. */
    for(size_t i = 0; i < store->count; i++) {
        if(i != remote_index) uni_remote_store_unload_details(store, i);
    }

    UniElement* elements = calloc(UNI_MAX_ELEMENTS, sizeof(UniElement));
    if(!elements) return false;

    FlipperFormat* ff = flipper_format_file_alloc(store->storage);
    if(!ff) {
        free(elements);
        return false;
    }

    FuriString* filetype = furi_string_alloc();
    uint32_t version = 0;
    uint32_t element_count = 0;
    bool ok = false;

    do {
        if(!flipper_format_file_open_existing(ff, remote->config_path)) break;
        if(!flipper_format_read_header(ff, filetype, &version)) break;
        if(strcmp(furi_string_get_cstr(filetype), UNI_REMOTE_FILETYPE) != 0) break;
        if(version != UNI_REMOTE_VERSION) break;
        if(!ff_read_u32(ff, "ElementCount", &element_count, true)) break;
        if(element_count > UNI_MAX_ELEMENTS) break;

        bool legacy_status[UNI_MAX_PAGES] = {false};
        for(uint32_t i = 0; i < element_count; i++) {
            if(!load_element(ff, i, &elements[i])) goto done_details;
            if(elements[i].type == UniElementStatus && elements[i].page < UNI_MAX_PAGES) {
                legacy_status[elements[i].page] = true;
            }
        }

        /*
         * v0.5 owns the 8px status bar outside the 3x6 control grid.
         * Migrate old status elements in RAM and shift that page up one row.
         * The next editor save writes the clean v0.5 layout back to disk.
         */
        size_t write_index = 0;
        for(uint32_t i = 0; i < element_count; i++) {
            UniElement e = elements[i];
            if(e.type == UniElementStatus) continue;
            if(e.page < UNI_MAX_PAGES && legacy_status[e.page] && e.y > 0U) e.y--;
            elements[write_index++] = e;
        }

        remote->elements = elements;
        remote->element_count = write_index;
        remote->elements_loaded = true;
        elements = NULL;
        ok = true;
    } while(false);

done_details:
    flipper_format_file_close(ff);
    furi_string_free(filetype);
    flipper_format_free(ff);
    if(elements) free(elements);
    return ok;
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

bool uni_remote_store_reload(UniRemoteStore* store) {
    if(!store || !store->storage) return false;
    scan_remotes(store);
    return store->count > 0;
}

size_t uni_remote_store_count(const UniRemoteStore* store) {
    return store ? store->count : 0;
}

const UniRemote* uni_remote_store_get(const UniRemoteStore* store, size_t index) {
    if(!store || index >= store->count) return NULL;
    return &store->remotes[index];
}

UniRemote* uni_remote_store_get_mut(UniRemoteStore* store, size_t index) {
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

bool uni_remote_store_set_repeat(UniRemoteStore* store, size_t remote_index, bool enabled) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    if(!remote) return false;
    remote->repeat_enabled = enabled;
    return uni_remote_store_save(store, remote_index);
}

static bool elements_overlap_at(
    const UniElement* a,
    uint8_t ax,
    uint8_t ay,
    const UniElement* b,
    uint8_t bx,
    uint8_t by) {
    if(!a || !b) return false;
    if(a->page != b->page) return false;
    for(uint8_t y = 0; y < 6; y++) {
        for(uint8_t x = 0; x < 3; x++) {
            if(uni_element_occupies_cell_at(a, ax, ay, x, y) &&
               uni_element_occupies_cell_at(b, bx, by, x, y)) {
                return true;
            }
        }
    }
    return false;
}

static bool element_free(
    const UniRemote* remote,
    size_t ignore,
    const UniElement* candidate,
    uint8_t x,
    uint8_t y) {
    if(!remote || !candidate) return false;
    for(size_t i = 0; i < remote->element_count; i++) {
        if(i == ignore) continue;
        const UniElement* e = &remote->elements[i];
        if(elements_overlap_at(candidate, x, y, e, e->x, e->y)) return false;
    }
    return true;
}

static bool find_nearest_free_position(
    const UniRemote* remote,
    size_t ignore,
    const UniElement* candidate,
    uint8_t preferred_x,
    uint8_t preferred_y,
    uint8_t* out_x,
    uint8_t* out_y) {
    if(!candidate) return false;
    bool found = false;
    uint16_t best_score = UINT16_MAX;

    for(uint8_t y = 0; y + candidate->h <= 6; y++) {
        for(uint8_t x = 0; x + candidate->w <= 3; x++) {
            if(!element_free(remote, ignore, candidate, x, y)) continue;
            const uint16_t dx = x > preferred_x ? x - preferred_x : preferred_x - x;
            const uint16_t dy = y > preferred_y ? y - preferred_y : preferred_y - y;
            const uint16_t score = (uint16_t)(dx + dy);
            if(!found || score < best_score) {
                found = true;
                best_score = score;
                *out_x = x;
                *out_y = y;
            }
        }
    }
    return found;
}

static void preferred_position(
    const UniElementPreset* preset,
    uint8_t* x,
    uint8_t* y) {
    *x = 0;
    *y = 0;
    if(!preset) return;

    switch(preset->type) {
    case UniElementStatus:
        *x = 0;
        *y = 0;
        break;
    case UniElementScreen:
        *x = 0;
        *y = 1;
        break;
    case UniElementDpad:
        *x = 0;
        *y = 3;
        break;
    case UniElementHStep:
        *x = 0;
        *y = 3;
        break;
    case UniElementVStep:
        *x = 0;
        *y = preset->h <= 2 ? 4 : 0;
        break;
    case UniElementButton:
        *x = preset->w == 1 ? 1 : 0;
        *y = preset->h == 1 ? 5 : 4;
        break;
    }
    if(*x + preset->w > 3) *x = 3 - preset->w;
    if(*y + preset->h > 6) *y = 6 - preset->h;
}

static void remove_element_in_memory(UniRemote* remote, size_t index) {
    if(!remote || index >= remote->element_count) return;
    for(size_t i = index; i + 1 < remote->element_count; i++) {
        remote->elements[i] = remote->elements[i + 1];
    }
    remote->element_count--;
    memset(&remote->elements[remote->element_count], 0, sizeof(UniElement));
}

static void remove_overlaps(
    UniRemote* remote,
    size_t* protected_index,
    const UniElement* candidate,
    uint8_t x,
    uint8_t y) {
    if(!remote || !candidate) return;

    size_t i = 0;
    while(i < remote->element_count) {
        if(protected_index && i == *protected_index) {
            i++;
            continue;
        }

        const UniElement* e = &remote->elements[i];
        if(!elements_overlap_at(candidate, x, y, e, e->x, e->y)) {
            i++;
            continue;
        }

        remove_element_in_memory(remote, i);
        if(protected_index && i < *protected_index) (*protected_index)--;
    }
}

bool uni_remote_store_move_element(
    UniRemoteStore* store,
    size_t remote_index,
    size_t element_index,
    int8_t dx,
    int8_t dy) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    if(!remote || element_index >= remote->element_count) return false;

    UniElement* moving = &remote->elements[element_index];
    const int nx = (int)moving->x + dx;
    const int ny = (int)moving->y + dy;
    if(nx < 0 || ny < 0 || nx + moving->w > 3 || ny + moving->h > 6) return false;

    uint8_t old_x[UNI_MAX_ELEMENTS] = {0};
    uint8_t old_y[UNI_MAX_ELEMENTS] = {0};
    bool collided[UNI_MAX_ELEMENTS] = {false};
    const size_t count = remote->element_count;

    for(size_t i = 0; i < count; i++) {
        old_x[i] = remote->elements[i].x;
        old_y[i] = remote->elements[i].y;
        if(i != element_index &&
           elements_overlap_at(
               moving,
               (uint8_t)nx,
               (uint8_t)ny,
               &remote->elements[i],
               remote->elements[i].x,
               remote->elements[i].y)) {
            collided[i] = true;
        }
    }

    const uint8_t preferred_x = moving->x;
    const uint8_t preferred_y = moving->y;
    moving->x = (uint8_t)nx;
    moving->y = (uint8_t)ny;

    for(size_t i = 0; i < count; i++) {
        if(!collided[i]) continue;

        UniElement* displaced = &remote->elements[i];
        uint8_t rx = 0;
        uint8_t ry = 0;
        if(!find_nearest_free_position(
               remote,
               i,
               displaced,
               preferred_x,
               preferred_y,
               &rx,
               &ry)) {
            for(size_t j = 0; j < count; j++) {
                remote->elements[j].x = old_x[j];
                remote->elements[j].y = old_y[j];
            }
            return false;
        }
        displaced->x = rx;
        displaced->y = ry;
    }

    if(!uni_remote_store_save(store, remote_index)) {
        for(size_t j = 0; j < count; j++) {
            remote->elements[j].x = old_x[j];
            remote->elements[j].y = old_y[j];
        }
        return false;
    }
    return true;
}

static bool find_free_position(
    const UniRemote* remote,
    const UniElement* candidate,
    uint8_t* out_x,
    uint8_t* out_y) {
    if(!candidate) return false;
    for(uint8_t y = 0; y + candidate->h <= 6; y++) {
        for(uint8_t x = 0; x + candidate->w <= 3; x++) {
            if(element_free(remote, UNI_MAX_ELEMENTS, candidate, x, y)) {
                *out_x = x;
                *out_y = y;
                return true;
            }
        }
    }
    return false;
}

static void init_from_preset(
    UniElement* element,
    const UniElementPreset* preset,
    const char* id,
    uint8_t x,
    uint8_t y) {
    memset(element, 0, sizeof(UniElement));
    snprintf(element->id, sizeof(element->id), "%s", id ? id : preset->id);
    element->type = preset->type;
    element->x = x;
    element->y = y;
    element->w = preset->w;
    element->h = preset->h;
    snprintf(element->label, sizeof(element->label), "%s", preset->label);
    snprintf(element->icon, sizeof(element->icon), "%s", preset->icon);
}

bool uni_remote_store_add_element(
    UniRemoteStore* store,
    size_t remote_index,
    size_t preset_index,
    uint8_t page,
    size_t* new_index) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    const UniElementPreset* preset = uni_element_preset_get(preset_index);
    if(!remote || !preset) return false;
    if(page >= (remote->page_count ? remote->page_count : 1U)) return false;

    UniElement candidate = {0};
    candidate.type = preset->type;
    candidate.page = page;
    candidate.w = preset->w;
    candidate.h = preset->h;

    uint8_t x = 0;
    uint8_t y = 0;
    if(!find_free_position(remote, &candidate, &x, &y)) {
        /*
         * No valid placement: ADD becomes deliberate mask-region replacement.
         * A 3x3 D-pad removes only its five occupied cross cells; four corner
         * buttons survive because those cells are not part of the D-pad mask.
         */
        preferred_position(preset, &x, &y);
        remove_overlaps(remote, NULL, &candidate, x, y);
    }

    if(remote->element_count >= UNI_MAX_ELEMENTS) return false;
    const size_t index = remote->element_count++;
    UniElement* e = &remote->elements[index];
    char id[UNI_ID_MAX];
    snprintf(id, sizeof(id), "%.18s%lu", preset->id, (unsigned long)index);
    init_from_preset(e, preset, id, x, y);
    e->page = page;

    if(!uni_remote_store_save(store, remote_index)) {
        remote->element_count--;
        memset(&remote->elements[remote->element_count], 0, sizeof(UniElement));
        return false;
    }
    if(new_index) *new_index = index;
    return true;
}

bool uni_remote_store_replace_element(
    UniRemoteStore* store,
    size_t remote_index,
    size_t element_index,
    size_t preset_index,
    size_t* result_index) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    const UniElementPreset* preset = uni_element_preset_get(preset_index);
    if(!remote || !preset || element_index >= remote->element_count) return false;

    const UniElement old = remote->elements[element_index];
    uint8_t x = old.x;
    uint8_t y = old.y;
    if(x + preset->w > 3) x = 3 - preset->w;
    if(y + preset->h > 6) y = 6 - preset->h;

    UniElement candidate = old;
    candidate.type = preset->type;
    candidate.w = preset->w;
    candidate.h = preset->h;

    size_t protected_index = element_index;
    remove_overlaps(remote, &protected_index, &candidate, x, y);
    element_index = protected_index;

    if(preset->type == old.type) {
        remote->elements[element_index] = old;
        remote->elements[element_index].x = x;
        remote->elements[element_index].y = y;
        remote->elements[element_index].w = preset->w;
        remote->elements[element_index].h = preset->h;
    } else {
        init_from_preset(
            &remote->elements[element_index],
            preset,
            old.id,
            x,
            y);
        remote->elements[element_index].page = old.page;
    }

    const bool saved = uni_remote_store_save(store, remote_index);
    if(saved && result_index) *result_index = element_index;
    return saved;
}

bool uni_remote_store_remove_element(
    UniRemoteStore* store,
    size_t remote_index,
    size_t element_index) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    if(!remote || element_index >= remote->element_count) return false;
    for(size_t i = element_index; i + 1 < remote->element_count; i++) {
        remote->elements[i] = remote->elements[i + 1];
    }
    remote->element_count--;
    memset(&remote->elements[remote->element_count], 0, sizeof(UniElement));
    return uni_remote_store_save(store, remote_index);
}

bool uni_remote_store_apply_layout(
    UniRemoteStore* store,
    size_t remote_index,
    size_t layout_index) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    const UniLayoutPreset* layout = uni_layout_preset_get(layout_index);
    if(!remote || !layout || layout->count > UNI_MAX_ELEMENTS) return false;

    memset(remote->elements, 0, UNI_MAX_ELEMENTS * sizeof(UniElement));
    memcpy(remote->elements, layout->elements, layout->count * sizeof(UniElement));
    remote->element_count = layout->count;
    return uni_remote_store_save(store, remote_index);
}

static char* binding_field(UniElement* e, const char* field) {
    if(strcmp(field, "tap") == 0) return e->tap;
    if(strcmp(field, "hold") == 0) return e->hold;
    if(strcmp(field, "up") == 0) return e->up;
    if(strcmp(field, "down") == 0) return e->down;
    if(strcmp(field, "left") == 0) return e->left;
    if(strcmp(field, "right") == 0) return e->right;
    if(strcmp(field, "ok") == 0) return e->ok;
    if(strcmp(field, "up_hold") == 0) return e->up_hold;
    if(strcmp(field, "down_hold") == 0) return e->down_hold;
    if(strcmp(field, "left_hold") == 0) return e->left_hold;
    if(strcmp(field, "right_hold") == 0) return e->right_hold;
    if(strcmp(field, "ok_hold") == 0) return e->ok_hold;
    return NULL;
}

bool uni_remote_store_set_binding(
    UniRemoteStore* store,
    size_t remote_index,
    size_t element_index,
    const char* field,
    const char* binding) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    if(!remote || element_index >= remote->element_count || !field) return false;
    char* target = binding_field(&remote->elements[element_index], field);
    if(!target) return false;
    snprintf(target, UNI_BINDING_MAX, "%s", binding ? binding : "");
    return uni_remote_store_save(store, remote_index);
}

static char* icon_field(UniElement* e, const char* field) {
    if(strcmp(field, "icon") == 0) return e->icon;
    if(strcmp(field, "hold_icon") == 0) return e->hold_icon;
    if(strcmp(field, "up_hold_icon") == 0) return e->up_hold_icon;
    if(strcmp(field, "down_hold_icon") == 0) return e->down_hold_icon;
    if(strcmp(field, "left_hold_icon") == 0) return e->left_hold_icon;
    if(strcmp(field, "right_hold_icon") == 0) return e->right_hold_icon;
    if(strcmp(field, "ok_hold_icon") == 0) return e->ok_hold_icon;
    return NULL;
}

bool uni_remote_store_set_icon(
    UniRemoteStore* store,
    size_t remote_index,
    size_t element_index,
    const char* field,
    const char* icon_id) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    if(!remote || element_index >= remote->element_count || !field) return false;
    char* target = icon_field(&remote->elements[element_index], field);
    if(!target) return false;
    snprintf(target, UNI_ICON_ID_MAX, "%s", icon_id ? icon_id : "");
    return uni_remote_store_save(store, remote_index);
}

bool uni_remote_store_set_hard_binding(
    UniRemoteStore* store,
    size_t remote_index,
    UniHardKeySlot slot,
    const char* binding) {
    if(!uni_remote_store_load_details(store, remote_index)) return false;
    UniRemote* remote = uni_remote_store_get_mut(store, remote_index);
    if(!remote || slot >= UniHardCount) return false;
    snprintf(
        remote->hard_bindings[slot],
        UNI_BINDING_MAX,
        "%s",
        binding ? binding : "");
    return uni_remote_store_save(store, remote_index);
}
