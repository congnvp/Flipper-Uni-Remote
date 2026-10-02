#pragma once
#include "remote.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <storage/storage.h>

typedef enum {
    UniStateProfileNone = 0,
    UniStateProfileLgAc,
    UniStateProfileDaikinAc,
    UniStateProfileCarrierAc,
    UniStateProfilePanasonicAc,
} UniStateProfile;

typedef struct {
    uint8_t profile;
    uint8_t power;
    uint8_t mode;
    uint8_t temp_half;
    uint8_t fan;
    uint8_t swing_v;
    uint8_t swing_h;
    int8_t bias;
    uint8_t option;
    uint8_t sleep;
    uint8_t flags;
} UniStateData;

typedef struct {
    Storage* storage;
    UniStateProfile profile;
    UniStateData state;
    char state_path[UNI_PATH_MAX];
    bool opened;
} UniStatefulEngine;

void uni_stateful_init(UniStatefulEngine* engine, Storage* storage);
bool uni_stateful_open(UniStatefulEngine* engine, const UniRemote* remote);
bool uni_stateful_execute(
    UniStatefulEngine* engine,
    const UniRemote* remote,
    const char* binding,
    bool repeat);
void uni_stateful_format_status(
    const UniStatefulEngine* engine,
    char* out,
    size_t out_size);
