#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UNI_MAX_REMOTES 12
#define UNI_MAX_ELEMENTS 12
#define UNI_ID_MAX 24
#define UNI_NAME_MAX 32
#define UNI_LABEL_MAX 12
#define UNI_SIGNAL_MAX 32
#define UNI_PATH_MAX 160
#define UNI_BT_PROFILE_MAX 24

typedef enum {
    UniTransportInfrared,
    UniTransportBluetoothHid,
    UniTransportStatefulIr,
} UniTransport;

typedef enum {
    UniElementStatus,
    UniElementScreen,
    UniElementButton,
    UniElementHStep,
    UniElementVStep,
    UniElementDpad,
} UniElementType;

typedef struct {
    char id[UNI_ID_MAX];
    UniElementType type;
    uint8_t x;
    uint8_t y;
    uint8_t w;
    uint8_t h;
    char label[UNI_LABEL_MAX];
    char tap[UNI_SIGNAL_MAX];
    char hold[UNI_SIGNAL_MAX];
    char up[UNI_SIGNAL_MAX];
    char down[UNI_SIGNAL_MAX];
    char left[UNI_SIGNAL_MAX];
    char right[UNI_SIGNAL_MAX];
    char ok[UNI_SIGNAL_MAX];
    char ok_hold[UNI_SIGNAL_MAX];
} UniElement;

typedef struct {
    char id[UNI_ID_MAX];
    char name[UNI_NAME_MAX];
    char short_name[4];
    UniTransport transport;
    uint32_t order;
    bool repeat_enabled;
    char signal_path[UNI_PATH_MAX];
    char bluetooth_profile[UNI_BT_PROFILE_MAX];
    UniElement elements[UNI_MAX_ELEMENTS];
    size_t element_count;
} UniRemote;

bool uni_element_focusable(const UniElement* element);
const char* uni_transport_label(UniTransport transport);
