#pragma once

#include "remote.h"
#include <storage/storage.h>

typedef struct {
    Storage* storage;
    UniRemote remotes[UNI_MAX_REMOTES];
    size_t count;
} UniRemoteStore;

bool uni_remote_store_init(UniRemoteStore* store, Storage* storage);
bool uni_remote_store_reload(UniRemoteStore* store);
size_t uni_remote_store_count(const UniRemoteStore* store);
const UniRemote* uni_remote_store_get(const UniRemoteStore* store, size_t index);
UniRemote* uni_remote_store_get_mut(UniRemoteStore* store, size_t index);
size_t uni_remote_store_find_id(const UniRemoteStore* store, const char* id);
bool uni_remote_store_set_repeat(UniRemoteStore* store, size_t remote_index, bool enabled);
bool uni_remote_store_move_element(
    UniRemoteStore* store,
    size_t remote_index,
    size_t element_index,
    int8_t dx,
    int8_t dy);
