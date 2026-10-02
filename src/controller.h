#pragma once

#include "remote.h"
#include <input/input.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
    size_t focus_before_capture;
    bool focus_before_capture_valid;

    bool dpad_captured;
    bool dpad_alt;
    UniKey dpad_hold_key;

    bool ok_pending;
    uint32_t ok_pending_tick;

    bool nav_pending;
    UniKey pending_nav_key;

    bool request_home;
    bool action_ready;
    bool repeat;
    char binding[UNI_BINDING_MAX];
} UniController;

void uni_controller_reset(UniController* controller, const UniRemote* remote);
bool uni_controller_move_focus(UniController* controller, const UniRemote* remote, UniKey direction);
void uni_controller_handle(
    UniController* controller,
    const UniRemote* remote,
    UniKey key,
    InputType input_type,
    bool repeat_enabled);
void uni_controller_poll(UniController* controller, const UniRemote* remote);
UniKey uni_map_physical_key(InputKey key);
