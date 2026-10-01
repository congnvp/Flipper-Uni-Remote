#pragma once

#include "remote.h"
#include <input/input.h>
#include <stdbool.h>
#include <stddef.h>

#define UNI_CONTROLLER_SIGNAL_MAX UNI_SIGNAL_MAX

typedef enum {
    UniKeyUp,
    UniKeyDown,
    UniKeyLeft,
    UniKeyRight,
    UniKeyOk,
    UniKeyBack,
    UniKeyUnknown,
} UniKey;

typedef struct {
    size_t focus_index;
    bool dpad_captured;
    bool request_home;
    bool request_exit;
    bool action_ready;
    bool repeat;
    char signal[UNI_CONTROLLER_SIGNAL_MAX];
} UniController;

void uni_controller_reset(UniController* controller, const UniRemote* remote);
void uni_controller_handle(
    UniController* controller,
    const UniRemote* remote,
    UniKey key,
    InputType input_type,
    bool repeat_enabled);
UniKey uni_map_physical_key(InputKey key);
