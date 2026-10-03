#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <storage/storage.h>

typedef struct UniIrTransport UniIrTransport;

UniIrTransport* uni_ir_transport_alloc(Storage* storage);
void uni_ir_transport_free(UniIrTransport* transport);
bool uni_ir_transport_send(
    UniIrTransport* transport,
    const char* signal_file,
    const char* signal_name,
    bool repeat,
    uint8_t burst_count);
