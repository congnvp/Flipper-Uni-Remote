#include "ir_transport.h"

#include <lib/infrared/signal/infrared_signal.h>
#include <stdlib.h>

struct UniIrTransport {
    InfraredSignal* signal;
};

UniIrTransport* uni_ir_transport_alloc(void) {
    UniIrTransport* transport = malloc(sizeof(UniIrTransport));
    if(!transport) return NULL;

    transport->signal = infrared_signal_alloc();
    if(!transport->signal) {
        free(transport);
        return NULL;
    }

    return transport;
}

void uni_ir_transport_free(UniIrTransport* transport) {
    if(!transport) return;
    infrared_signal_free(transport->signal);
    free(transport);
}

bool uni_ir_transport_send(UniIrTransport* transport, const UniIrCode* code, bool repeat) {
    if(!transport || !transport->signal || !code) return false;
    if(!infrared_is_protocol_valid(code->protocol)) return false;

    const InfraredMessage message = {
        .protocol = code->protocol,
        .address = code->address,
        .command = code->command,
        .repeat = repeat,
    };

    infrared_signal_set_message(transport->signal, &message);
    infrared_signal_transmit(transport->signal);
    return true;
}
