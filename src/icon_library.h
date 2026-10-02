#pragma once

#include <stddef.h>

typedef enum {
    UniIconText,
    UniIconPower,
    UniIconMute,
    UniIconPlay,
    UniIconPause,
    UniIconStop,
    UniIconRecord,
    UniIconPrev,
    UniIconNext,
    UniIconRew,
    UniIconFfwd,
    UniIconHome,
    UniIconBack,
    UniIconMenu,
    UniIconInfo,
    UniIconGear,
    UniIconPlus,
    UniIconMinus,
    UniIconSun,
    UniIconMoon,
    UniIconFan,
    UniIconSnow,
    UniIconHeat,
    UniIconDrop,
    UniIconSource,
    UniIconLock,
} UniIconKind;

typedef struct {
    const char* id;
    const char* short_label;
    UniIconKind kind;
} UniIconDef;

size_t uni_icon_count(void);
const UniIconDef* uni_icon_get(size_t index);
const UniIconDef* uni_icon_find(const char* id);
