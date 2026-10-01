#pragma once

#include "remote.h"
#include <storage/storage.h>

typedef struct {
    Storage* storage;
    UniRemote remotes[UNI_MAX_REMOTES];
    size_t count;
} UniRemoteStore;

bool uni_remote_store_init(UniRemoteStore* store, Storage* storage);
size_t uni_remote_store_count(const UniRemoteStore* store);
const UniRemote* uni_remote_store_get(const UniRemoteStore* store, size_t index);
size_t uni_remote_store_find_id(const UniRemoteStore* store, const char* id);
