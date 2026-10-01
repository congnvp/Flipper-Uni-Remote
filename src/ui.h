#pragma once

#include "profile.h"
#include <gui/canvas.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    UniUiHome,
    UniUiRemote,
} UniUiPage;

typedef struct {
    UniUiPage page;
    size_t selected_profile;
    const UniRemoteProfile* profile;
    UniAction last_action;
    bool last_action_valid;
    bool tx_flash;
} UniUiState;

void uni_ui_draw(Canvas* canvas, const UniUiState* state);
