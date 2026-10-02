#pragma once

#include "remote.h"
#include <stddef.h>

typedef struct {
    const char* id;
    const char* name;
    UniElementType type;
    uint8_t w;
    uint8_t h;
    const char* label;
    const char* icon;
} UniElementPreset;

typedef struct {
    const char* id;
    const char* name;
    const UniElement* elements;
    size_t count;
} UniLayoutPreset;

size_t uni_element_preset_count(void);
const UniElementPreset* uni_element_preset_get(size_t index);
size_t uni_layout_preset_count(void);
const UniLayoutPreset* uni_layout_preset_get(size_t index);
