#include "remote.h"

bool uni_element_focusable(const UniElement* element) {
    if(!element) return false;
    return element->type == UniElementButton || element->type == UniElementHStep ||
           element->type == UniElementVStep || element->type == UniElementDpad;
}

const char* uni_transport_label(UniTransport transport) {
    switch(transport) {
    case UniTransportInfrared:
        return "IR";
    case UniTransportBluetoothHid:
        return "BT";
    case UniTransportStatefulIr:
        return "ST";
    default:
        return "--";
    }
}
