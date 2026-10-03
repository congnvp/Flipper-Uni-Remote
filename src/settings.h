#pragma once

#include "remote.h"
#include <stdbool.h>
#include <storage/storage.h>

typedef struct {
    bool repeat_enabled;
    bool open_default;
    char default_remote[UNI_ID_MAX];
    char last_remote[UNI_ID_MAX];
    uint32_t last_page;
    uint32_t last_focus;
} UniSettings;

bool uni_settings_load_or_create(Storage* storage, UniSettings* settings);
bool uni_settings_save(Storage* storage, const UniSettings* settings);
