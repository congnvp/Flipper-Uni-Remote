#pragma once

#include "profile.h"
#include <stdbool.h>

typedef struct UniIrTransport UniIrTransport;

UniIrTransport* uni_ir_transport_alloc(void);
void uni_ir_transport_free(UniIrTransport* transport);
bool uni_ir_transport_send(UniIrTransport* transport, const UniIrCode* code, bool repeat);
