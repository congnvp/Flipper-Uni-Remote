#include "../src/profile.h"

/*
 * Reference profile for the common 21-key NEC learning remote.
 * It is intentionally labelled DEMO: replace/add profiles for real devices.
 */
const UniRemoteProfile uni_profile_demo_nec = {
    .id = "demo_nec",
    .name = "NEC Demo",
    .short_name = "NEC",
    .transport = UniTransportInfrared,
    .ir = {
        [UniActionPower] = {InfraredProtocolNEC, 0x00, 0x45},
        [UniActionMute] = {InfraredProtocolNEC, 0x00, 0x47},
        [UniActionUp] = {InfraredProtocolNEC, 0x00, 0x18},
        [UniActionDown] = {InfraredProtocolNEC, 0x00, 0x52},
        [UniActionLeft] = {InfraredProtocolNEC, 0x00, 0x08},
        [UniActionRight] = {InfraredProtocolNEC, 0x00, 0x5A},
        [UniActionOk] = {InfraredProtocolNEC, 0x00, 0x1C},
    },
    .has_action = {
        [UniActionPower] = true,
        [UniActionMute] = true,
        [UniActionUp] = true,
        [UniActionDown] = true,
        [UniActionLeft] = true,
        [UniActionRight] = true,
        [UniActionOk] = true,
    },
};
