#pragma once

#include "state_engine.h"

struct UniStateAdapter {
    const char* id;
    void (*defaults)(UniDeviceState* state);
    bool (*validate)(const UniDeviceState* state);
    void (*normalize)(UniDeviceState* state);
    bool (*execute)(UniDeviceState* state, const char* operation, bool repeat);
};

const UniStateAdapter* uni_state_adapter_lg(void);
const UniStateAdapter* uni_state_adapter_daikin(void);
