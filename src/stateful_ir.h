#pragma once

#include "remote.h"

#include <stdbool.h>
#include <stddef.h>
#include <storage/storage.h>

typedef struct UniStatefulIr UniStatefulIr;

UniStatefulIr* uni_stateful_ir_alloc(Storage* storage);
void uni_stateful_ir_free(UniStatefulIr* stateful);

bool uni_stateful_ir_load(UniStatefulIr* stateful, const UniRemote* remote);
void uni_stateful_ir_unload(UniStatefulIr* stateful);

bool uni_stateful_ir_execute(
    UniStatefulIr* stateful,
    const char* action,
    bool repeat);

size_t uni_stateful_ir_action_count(const UniStatefulIr* stateful);
const char* uni_stateful_ir_action_name(const UniStatefulIr* stateful, size_t index);

/* Compact LOCAL/last-sent state intended for the 64x8 runtime status bar. */
void uni_stateful_ir_status(
    const UniStatefulIr* stateful,
    char* out,
    size_t out_size);
