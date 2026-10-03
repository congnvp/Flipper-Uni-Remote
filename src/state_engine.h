#pragma once

#include "remote.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <storage/storage.h>

typedef enum {
    UniAcModeCool = 0,
    UniAcModeDry,
    UniAcModeFan,
    UniAcModeAuto,
    UniAcModeHeat,
} UniAcMode;

typedef struct {
    bool power;
    UniAcMode mode;
    uint8_t temp_c;
    int8_t auto_bias;
    uint8_t fan;
    uint8_t swing_v;
    uint8_t swing_h;

    bool powerful;
    bool eco;
    bool comfort;
    bool auto_clean;
    bool purify;
    bool jet_dry;

    bool display_known;
    bool display_assumed_on;
    bool fahrenheit;

    bool timer_on_enabled;
    uint16_t timer_on_target_min;
    bool timer_off_enabled;
    uint16_t timer_off_target_min;
    uint16_t sleep_minutes;
} UniDeviceState;

typedef struct UniStateAdapter UniStateAdapter;

typedef struct {
    Storage* storage;
    const UniStateAdapter* adapter;
    UniDeviceState state;
    char remote_id[UNI_ID_MAX];
    char state_path[UNI_PATH_MAX];
    bool loaded;
    bool dirty;
} UniStateEngine;

void uni_state_engine_init(UniStateEngine* engine, Storage* storage);
bool uni_state_engine_load(UniStateEngine* engine, const UniRemote* remote);
void uni_state_engine_unload(UniStateEngine* engine);
bool uni_state_engine_execute(
    UniStateEngine* engine,
    const UniRemote* remote,
    const char* binding,
    bool repeat);
bool uni_state_engine_flush(UniStateEngine* engine);
void uni_state_engine_summary(const UniStateEngine* engine, char* out, size_t out_size);
