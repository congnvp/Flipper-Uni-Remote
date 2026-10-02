#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UNI_MAX_REMOTES 12
#define UNI_MAX_ELEMENTS 18
#define UNI_ID_MAX 24
#define UNI_NAME_MAX 32
#define UNI_LABEL_MAX 12
#define UNI_BINDING_MAX 40
#define UNI_PATH_MAX 160
#define UNI_BT_PROFILE_MAX 24
#define UNI_ICON_ID_MAX 9

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

typedef enum {
    UniHardUpHold,
    UniHardDownHold,
    UniHardLeftHold,
    UniHardRightHold,
    UniHardOkHold,
    UniHardCount,
} UniHardKeySlot;

typedef struct {
    char id[UNI_ID_MAX];
    UniElementType type;
    uint8_t x;
    uint8_t y;
    uint8_t w;
    uint8_t h;
    char label[UNI_LABEL_MAX];
    char icon[UNI_ICON_ID_MAX];
    char hold_icon[UNI_ICON_ID_MAX];

    char tap[UNI_BINDING_MAX];
    char hold[UNI_BINDING_MAX];
    char up[UNI_BINDING_MAX];
    char down[UNI_BINDING_MAX];
    char left[UNI_BINDING_MAX];
    char right[UNI_BINDING_MAX];
    char ok[UNI_BINDING_MAX];

    char up_hold[UNI_BINDING_MAX];
    char down_hold[UNI_BINDING_MAX];
    char left_hold[UNI_BINDING_MAX];
    char right_hold[UNI_BINDING_MAX];
    char ok_hold[UNI_BINDING_MAX];

    char up_hold_icon[UNI_ICON_ID_MAX];
    char down_hold_icon[UNI_ICON_ID_MAX];
    char left_hold_icon[UNI_ICON_ID_MAX];
    char right_hold_icon[UNI_ICON_ID_MAX];
    char ok_hold_icon[UNI_ICON_ID_MAX];

    bool alt_sticky;
} UniElement;

typedef struct {
    char id[UNI_ID_MAX];
    char name[UNI_NAME_MAX];
    char short_name[4];
    UniTransport transport;
    uint32_t order;
    bool repeat_enabled;

    char config_path[UNI_PATH_MAX];
    char signal_file[64];
    char action_file[64];
    char signal_path[UNI_PATH_MAX];
    char action_path[UNI_PATH_MAX];

    char bluetooth_profile[UNI_BT_PROFILE_MAX];
    char hard_bindings[UniHardCount][UNI_BINDING_MAX];

    UniElement elements[UNI_MAX_ELEMENTS];
    size_t element_count;
} UniRemote;

bool uni_element_focusable(const UniElement* element);
const char* uni_transport_label(UniTransport transport);
const char* uni_element_type_name(UniElementType type);
