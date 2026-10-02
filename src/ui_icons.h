#pragma once

#include <gui/canvas.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    UiIconPower, UiIconMute, UiIconInput, UiIconHome, UiIconMenu, UiIconBack,
    UiIconSettings, UiIconInfo, UiIconGuide, UiIconExit,
    UiIconUp, UiIconDown, UiIconLeft, UiIconRight, UiIconOk,
    UiIconVolUp, UiIconVolDown, UiIconChUp, UiIconChDown,
    UiIconPlay, UiIconPause, UiIconStop, UiIconRecord,
    UiIconRewind, UiIconFastForward, UiIconPrevious, UiIconNext,
    UiIconSubtitle, UiIconAudio, UiIconMic, UiIconVoice,
    UiIconSearch, UiIconApps, UiIconKeyboard, UiIconMouse,
    UiIconPair, UiIconBluetooth,
    UiIconNum0, UiIconNum1, UiIconNum2, UiIconNum3, UiIconNum4,
    UiIconNum5, UiIconNum6, UiIconNum7, UiIconNum8, UiIconNum9,
    UiIconDot, UiIconDash,
    UiIconRed, UiIconGreen, UiIconYellow, UiIconBlue,
    UiIconLight, UiIconSleep, UiIconTimer, UiIconFan,
    UiIconTempUp, UiIconTempDown, UiIconMode, UiIconAuto,
    UiIconCool, UiIconHeat, UiIconDry, UiIconSwing,
    UiIconEco, UiIconTurbo, UiIconFreeze,
    UiIconZoomIn, UiIconZoomOut, UiIconFocusIn, UiIconFocusOut,
    UiIconAspect, UiIconProjector, UiIconTv,
    UiIconGamepad, UiIconA, UiIconB, UiIconX, UiIconY,
    UiIconBlank, UiIconRepeat, UiIconShuffle, UiIconFavorite,
    UiIconBrightnessUp, UiIconBrightnessDown, UiIconLastChannel, UiIconEject,
    UiIconCount,
} UiIcon;

uint8_t ui_icon_count(void);
const char* ui_icon_name(UiIcon icon);
bool ui_icon_lookup_id(const char* id, UiIcon* out);
void ui_icon_draw(Canvas* canvas, UiIcon icon, int16_t x, int16_t y, Color color);
