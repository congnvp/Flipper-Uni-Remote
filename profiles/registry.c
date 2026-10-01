#include "../src/profile.h"

extern const UniRemoteProfile uni_profile_demo_nec;

static const UniRemoteProfile* const profiles[] = {
    &uni_profile_demo_nec,
};

const UniRemoteProfile* uni_profiles_get(size_t index) {
    if(index >= uni_profiles_count()) return NULL;
    return profiles[index];
}

size_t uni_profiles_count(void) {
    return sizeof(profiles) / sizeof(profiles[0]);
}
