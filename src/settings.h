#pragma once

#include "remote.h"
#include <stdbool.h>
#include <storage/storage.h>

typedef struct {
    bool repeat_enabled;
    char default_remote[UNI_ID_MAX];
} UniSettings;

bool uni_settings_load_or_create(Storage* storage, UniSettings* settings);
