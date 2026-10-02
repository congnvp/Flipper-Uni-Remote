#pragma once

#include "remote.h"
#include <stdbool.h>
#include <storage/storage.h>

typedef enum {
    UniAcModeAuto = 0,
    UniAcModeCool,
    UniAcModeDry,
    UniAcModeFan,
    UniAcModeHeat,
} UniAcMode;

typedef struct {
    bool power;
    UniAcMode mode;
    uint8_t temp_x2;
    uint8_t fan;
    uint8_t swing_v;
    uint8_t swing_h;
    bool eco;
    bool turbo;
    bool nanoe;
    uint8_t sleep_step;
} UniAcState;

bool uni_stateful_ir_execute(
    Storage* storage,
    const UniRemote* remote,
    const char* binding,
    bool repeat);

bool uni_stateful_ir_load_state(
    Storage* storage,
    const UniRemote* remote,
    UniAcState* out);

const char* uni_stateful_ir_mode_label(UniAcMode mode);
