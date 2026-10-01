#pragma once

#include "controller.h"
#include "remote_store.h"
#include <gui/canvas.h>
#include <stdbool.h>
#include <stddef.h>

#define UNI_LAST_SIGNAL_MAX UNI_SIGNAL_MAX

typedef enum {
    UniUiHome,
    UniUiRemote,
} UniUiPage;

typedef struct {
    UniUiPage page;
    const UniRemoteStore* store;
    size_t selected_remote;
    const UniRemote* remote;
    size_t focus_index;
    bool dpad_captured;
    bool tx_flash;
    bool tx_ok;
    char last_signal[UNI_LAST_SIGNAL_MAX];
} UniUiState;

void uni_ui_draw(Canvas* canvas, const UniUiState* state);
