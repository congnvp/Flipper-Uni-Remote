#include "settings.h"

#include <flipper_format/flipper_format.h>
#include <furi.h>
#include <stdio.h>
#include <string.h>

#define UNI_SETTINGS_PATH APP_DATA_PATH("settings.ur")
#define UNI_SETTINGS_FILETYPE "Flipper Uni Remote Settings"
#define UNI_SETTINGS_VERSION 1U

bool uni_settings_save(Storage* storage, const UniSettings* settings) {
    if(!storage || !settings) return false;

    char text[384];
    const int length = snprintf(
        text,
        sizeof(text),
        "Filetype: Flipper Uni Remote Settings\n"
        "Version: 1\n"
        "RepeatEnabled: %s\n"
        "OpenDefault: %s\n"
        "DefaultRemote: %s\n"
        "LastRemote: %s\n"
        "LastPage: %lu\n"
        "LastFocus: %lu\n",
        settings->repeat_enabled ? "true" : "false",
        settings->open_default ? "true" : "false",
        settings->default_remote,
        settings->last_remote,
        (unsigned long)settings->last_page,
        (unsigned long)settings->last_focus);

    if(length <= 0 || (size_t)length >= sizeof(text)) return false;

    File* file = storage_file_alloc(storage);
    if(!file) return false;
    bool ok = storage_file_open(file, UNI_SETTINGS_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS);
    if(ok) {
        ok = storage_file_write(file, text, (size_t)length) == (size_t)length;
        storage_file_sync(file);
    }
    storage_file_close(file);
    storage_file_free(file);
    return ok;
}

static bool create_default(Storage* storage) {
    UniSettings settings = {
        .repeat_enabled = true,
        .open_default = false,
    };
    snprintf(settings.default_remote, sizeof(settings.default_remote), "demo_tv");
    settings.last_remote[0] = '\0';
    settings.last_page = 0;
    settings.last_focus = 0;
    return uni_settings_save(storage, &settings);
}

bool uni_settings_load_or_create(Storage* storage, UniSettings* settings) {
    if(!storage || !settings) return false;
    memset(settings, 0, sizeof(UniSettings));
    settings->repeat_enabled = true;
    settings->open_default = false;
    snprintf(settings->default_remote, sizeof(settings->default_remote), "demo_tv");
    settings->last_remote[0] = '\0';
    settings->last_page = 0;
    settings->last_focus = 0;

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
        flipper_format_read_bool(ff, "OpenDefault", &settings->open_default, 1);
        flipper_format_rewind(ff);
        if(flipper_format_read_string(ff, "DefaultRemote", value)) {
            snprintf(
                settings->default_remote,
                sizeof(settings->default_remote),
                "%s",
                furi_string_get_cstr(value));
        }
        flipper_format_rewind(ff);
        if(flipper_format_read_string(ff, "LastRemote", value)) {
            snprintf(
                settings->last_remote,
                sizeof(settings->last_remote),
                "%s",
                furi_string_get_cstr(value));
        }
        flipper_format_rewind(ff);
        flipper_format_read_uint32(ff, "LastPage", &settings->last_page, 1);
        flipper_format_rewind(ff);
        flipper_format_read_uint32(ff, "LastFocus", &settings->last_focus, 1);
        ok = true;
    } while(false);

    flipper_format_file_close(ff);
    furi_string_free(value);
    furi_string_free(filetype);
    flipper_format_free(ff);
    return ok;
}
