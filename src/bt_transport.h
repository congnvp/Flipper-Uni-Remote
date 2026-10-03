#pragma once

#include "remote.h"
#include <stdbool.h>
#include <storage/storage.h>

typedef struct UniBtTransport UniBtTransport;

UniBtTransport* uni_bt_transport_alloc(Storage* storage);
void uni_bt_transport_free(UniBtTransport* transport);
bool uni_bt_transport_activate(UniBtTransport* transport, const UniRemote* remote);
void uni_bt_transport_deactivate(UniBtTransport* transport);
bool uni_bt_transport_send(
    UniBtTransport* transport,
    const UniRemote* remote,
    const char* binding,
    bool repeat);
bool uni_bt_transport_connected(const UniBtTransport* transport);
