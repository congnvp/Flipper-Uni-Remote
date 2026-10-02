#pragma once

#include "layout_library.h"
#include "remote.h"
#include <storage/storage.h>

typedef struct {
    Storage* storage;
    UniRemote remotes[UNI_MAX_REMOTES];
    size_t count;
} UniRemoteStore;

bool uni_remote_store_init(UniRemoteStore* store, Storage* storage);
bool uni_remote_store_reload(UniRemoteStore* store);
bool uni_remote_store_load_details(UniRemoteStore* store, size_t remote_index);
void uni_remote_store_unload_details(UniRemoteStore* store, size_t remote_index);
size_t uni_remote_store_count(const UniRemoteStore* store);
const UniRemote* uni_remote_store_get(const UniRemoteStore* store, size_t index);
UniRemote* uni_remote_store_get_mut(UniRemoteStore* store, size_t index);
size_t uni_remote_store_find_id(const UniRemoteStore* store, const char* id);

bool uni_remote_store_save(UniRemoteStore* store, size_t remote_index);
bool uni_remote_store_set_repeat(UniRemoteStore* store, size_t remote_index, bool enabled);
bool uni_remote_store_move_element(
    UniRemoteStore* store,
    size_t remote_index,
    size_t element_index,
    int8_t dx,
    int8_t dy);
bool uni_remote_store_add_element(
    UniRemoteStore* store,
    size_t remote_index,
    size_t preset_index,
    uint8_t page,
    size_t* new_index);
bool uni_remote_store_replace_element(
    UniRemoteStore* store,
    size_t remote_index,
    size_t element_index,
    size_t preset_index,
    size_t* result_index);
bool uni_remote_store_remove_element(
    UniRemoteStore* store,
    size_t remote_index,
    size_t element_index);
bool uni_remote_store_apply_layout(
    UniRemoteStore* store,
    size_t remote_index,
    size_t layout_index);
bool uni_remote_store_set_binding(
    UniRemoteStore* store,
    size_t remote_index,
    size_t element_index,
    const char* field,
    const char* binding);
bool uni_remote_store_set_icon(
    UniRemoteStore* store,
    size_t remote_index,
    size_t element_index,
    const char* field,
    const char* icon_id);
bool uni_remote_store_set_hard_binding(
    UniRemoteStore* store,
    size_t remote_index,
    UniHardKeySlot slot,
    const char* binding);
