#pragma once

#include "controller.h"
#include "remote_store.h"
#include "settings.h"
#include <gui/canvas.h>
#include <stdbool.h>
#include <stddef.h>

#define UNI_LAST_SIGNAL_MAX UNI_SIGNAL_MAX

typedef enum {
    UniUiHome,
    UniUiRemote,
    UniUiMenu,
    UniUiGlobalSettings,
    UniUiRemoteSettings,
    UniUiLayoutEditor,
} UniUiPage;

typedef struct {
    UniUiPage page;
    const UniRemoteStore* store;
    const UniSettings* settings;
    size_t selected_remote;
    const UniRemote* remote;
    size_t focus_index;
    bool dpad_captured;
    bool tx_flash;
    bool tx_ok;
    size_t menu_index;
    size_t layout_element;
    bool layout_moving;
    char last_signal[UNI_LAST_SIGNAL_MAX];
} UniUiState;

void uni_ui_draw(Canvas* canvas, const UniUiState* state);
