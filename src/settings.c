#include "settings.h"

#include <flipper_format/flipper_format.h>
#include <furi.h>
#include <stdio.h>
#include <string.h>

#define UNI_SETTINGS_PATH APP_DATA_PATH("settings.ur")
#define UNI_SETTINGS_FILETYPE "Flipper Uni Remote Settings"
#define UNI_SETTINGS_VERSION 1U

static bool create_default(Storage* storage) {
    static const char text[] =
        "Filetype: Flipper Uni Remote Settings
"
        "Version: 1
"
        "RepeatEnabled: true
"
        "DefaultRemote: demo_tv
";

    File* file = storage_file_alloc(storage);
    if(!file) return false;
    bool ok = storage_file_open(file, UNI_SETTINGS_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS);
    if(ok) {
        const size_t length = strlen(text);
        ok = storage_file_write(file, text, length) == length;
        storage_file_sync(file);
    }
    storage_file_close(file);
    storage_file_free(file);
    return ok;
}

bool uni_settings_load_or_create(Storage* storage, UniSettings* settings) {
    if(!storage || !settings) return false;
    memset(settings, 0, sizeof(UniSettings));
    settings->repeat_enabled = true;
    snprintf(settings->default_remote, sizeof(settings->default_remote), "demo_tv");

    if(!storage_file_exists(storage, UNI_SETTINGS_PATH) && !create_default(storage)) return false;

    FlipperFormat* ff = flipper_format_file_alloc(storage);
    if(!ff) return false;
    FuriString* filetype = furi_string_alloc();
    FuriString* value = furi_string_alloc();
    uint32_t version = 0;
    bool ok = false;

    do {
        if(!flipper_format_file_open_existing(ff, UNI_SETTINGS_PATH)) break;
        if(!flipper_format_read_header(ff, filetype, &version)) break;
        if(strcmp(furi_string_get_cstr(filetype), UNI_SETTINGS_FILETYPE) != 0) break;
        if(version != UNI_SETTINGS_VERSION) break;

        flipper_format_rewind(ff);
        flipper_format_read_bool(ff, "RepeatEnabled", &settings->repeat_enabled, 1);
        flipper_format_rewind(ff);
        if(flipper_format_read_string(ff, "DefaultRemote", value)) {
            snprintf(
                settings->default_remote,
                sizeof(settings->default_remote),
                "%s",
                furi_string_get_cstr(value));
        }
        ok = true;
    } while(false);

    flipper_format_file_close(ff);
    furi_string_free(value);
    furi_string_free(filetype);
    flipper_format_free(ff);
    return ok;
}
