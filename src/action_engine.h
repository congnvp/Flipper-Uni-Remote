#pragma once

#include "ir_transport.h"
#include "remote.h"
#include <stdbool.h>
#include <stddef.h>
#include <storage/storage.h>

#define UNI_MAX_SIGNALS 32
#define UNI_MAX_ACTIONS 16
#define UNI_MAX_SEQUENCE_STEPS 8
#define UNI_MAX_SEQUENCE_DELAY_MS 60000U
#define UNI_ACTION_ID_MAX 24
#define UNI_SIGNAL_NAME_MAX 32

typedef struct {
    char names[UNI_MAX_SIGNALS][UNI_SIGNAL_NAME_MAX];
    size_t count;
} UniSignalCatalog;

typedef enum {
    UniActionSignal,
    UniActionSequence,
} UniActionType;

typedef struct {
    char id[UNI_ACTION_ID_MAX];
    UniActionType type;
    char signal[UNI_SIGNAL_NAME_MAX];
    char steps[UNI_MAX_SEQUENCE_STEPS][UNI_SIGNAL_NAME_MAX];
    uint32_t delays_ms[UNI_MAX_SEQUENCE_STEPS];
    size_t step_count;
} UniNamedAction;

typedef struct {
    UniNamedAction actions[UNI_MAX_ACTIONS];
    size_t count;
} UniActionCatalog;

typedef struct {
    Storage* storage;
    UniIrTransport* ir;
    UniSignalCatalog signals;
    UniActionCatalog actions;
} UniActionEngine;

void uni_action_engine_init(UniActionEngine* engine, Storage* storage, UniIrTransport* ir);
bool uni_action_engine_load(UniActionEngine* engine, const UniRemote* remote);
bool uni_action_engine_execute(
    UniActionEngine* engine,
    const UniRemote* remote,
    const char* binding,
    bool repeat);

const UniNamedAction* uni_action_find(const UniActionCatalog* catalog, const char* id);

size_t uni_action_sequence_count(const UniActionCatalog* catalog);
size_t uni_action_sequence_index(const UniActionCatalog* catalog, size_t position);
bool uni_action_engine_save(UniActionEngine* engine, const UniRemote* remote);
bool uni_action_engine_add_sequence(
    UniActionEngine* engine,
    const UniRemote* remote,
    size_t* action_index);
bool uni_action_engine_remove_sequence(
    UniActionEngine* engine,
    const UniRemote* remote,
    size_t action_index);
bool uni_action_engine_rename_sequence(
    UniActionEngine* engine,
    const UniRemote* remote,
    size_t action_index,
    const char* id);
bool uni_action_engine_set_step(
    UniActionEngine* engine,
    const UniRemote* remote,
    size_t action_index,
    size_t step_index,
    const char* binding);
bool uni_action_engine_append_step(
    UniActionEngine* engine,
    const UniRemote* remote,
    size_t action_index,
    const char* binding);
bool uni_action_engine_remove_step(
    UniActionEngine* engine,
    const UniRemote* remote,
    size_t action_index,
    size_t step_index);
bool uni_action_engine_set_delay(
    UniActionEngine* engine,
    const UniRemote* remote,
    size_t action_index,
    size_t step_index,
    uint32_t delay_ms);
bool uni_action_engine_action_in_use(
    const UniActionEngine* engine,
    const UniRemote* remote,
    const char* id);
