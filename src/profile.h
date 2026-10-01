#pragma once

#include <infrared.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    UniTransportInfrared,
    UniTransportBluetoothHid,
} UniTransport;

typedef enum {
    UniActionPower,
    UniActionMute,
    UniActionUp,
    UniActionDown,
    UniActionLeft,
    UniActionRight,
    UniActionOk,
    UniActionCount,
} UniAction;

typedef struct {
    InfraredProtocol protocol;
    uint32_t address;
    uint32_t command;
} UniIrCode;

typedef struct {
    const char* id;
    const char* name;
    const char* short_name;
    UniTransport transport;
    UniIrCode ir[UniActionCount];
    bool has_action[UniActionCount];
} UniRemoteProfile;

const UniRemoteProfile* uni_profiles_get(size_t index);
size_t uni_profiles_count(void);
