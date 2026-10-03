#include "ir_transport.h"

#include <flipper_format/flipper_format.h>
#include <lib/infrared/signal/infrared_signal.h>
#include <infrared/worker/infrared_transmit.h>
#include <furi.h>
#include <stdlib.h>

struct UniIrTransport {
    Storage* storage;
    InfraredSignal* signal;
};

UniIrTransport* uni_ir_transport_alloc(Storage* storage) {
    if(!storage) return NULL;
    UniIrTransport* transport = malloc(sizeof(UniIrTransport));
    if(!transport) return NULL;

    transport->storage = storage;
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

bool uni_ir_transport_send(
    UniIrTransport* transport,
    const char* signal_file,
    const char* signal_name,
    bool repeat,
    uint8_t burst_count) {
    if(!transport || !signal_file || !signal_name || signal_name[0] == '\0') return false;

    FlipperFormat* ff = flipper_format_file_alloc(transport->storage);
    if(!ff) return false;

    bool ok = false;
    do {
        if(!flipper_format_file_open_existing(ff, signal_file)) break;
        if(infrared_signal_search_by_name_and_read(transport->signal, ff, signal_name) !=
           InfraredErrorCodeNone) {
            break;
        }

        if(burst_count == 0) burst_count = 1;

        if(!infrared_signal_is_raw(transport->signal)) {
            InfraredMessage message = *infrared_signal_get_message(transport->signal);
            message.repeat = repeat;
            infrared_send(&message, burst_count);
            ok = true;
        } else {
            for(uint8_t i = 0; i < burst_count; i++) {
                infrared_signal_transmit(transport->signal);
                if(i + 1 < burst_count) furi_delay_ms(20);
            }
            ok = true;
        }
    } while(false);

    flipper_format_file_close(ff);
    flipper_format_free(ff);
    return ok;
}
