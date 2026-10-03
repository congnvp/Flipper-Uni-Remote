#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <storage/storage.h>

typedef struct UniBtTransport UniBtTransport;

UniBtTransport* uni_bt_transport_alloc(Storage* storage);
void uni_bt_transport_free(UniBtTransport* transport);

bool uni_bt_transport_activate(UniBtTransport* transport, const char* profile_id);
void uni_bt_transport_deactivate(UniBtTransport* transport);

bool uni_bt_transport_execute(UniBtTransport* transport, const char* binding, bool repeat);
bool uni_bt_transport_is_active(const UniBtTransport* transport);
bool uni_bt_transport_is_connected(const UniBtTransport* transport);

bool uni_bt_transport_forget_profile(UniBtTransport* transport, const char* profile_id);
