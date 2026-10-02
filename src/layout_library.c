#include "layout_library.h"

static const UniElementPreset element_presets[] = {
    {"btn11", "BUTTON 1X1", UniElementButton, 1, 1, "BTN", ""},
    {"btn12", "BUTTON 1X2", UniElementButton, 1, 2, "BTN", ""},
    {"h31", "LEFT RIGHT", UniElementHStep, 3, 1, "NAV", ""},
    {"v12", "UP DOWN", UniElementVStep, 1, 2, "VAL", ""},
    {"d33", "D-PAD", UniElementDpad, 3, 3, "", ""},
    {"sts31", "STATUS", UniElementStatus, 3, 1, "", ""},
    {"scr32", "SCREEN 3X2", UniElementScreen, 3, 2, "READY", ""},
    {"scr33", "SCREEN 3X3", UniElementScreen, 3, 3, "READY", ""},
};

static const UniElement tv_basic[] = {
    {.id="status", .type=UniElementStatus, .x=0,.y=0,.w=3,.h=1},
    {.id="main", .type=UniElementScreen, .x=0,.y=1,.w=3,.h=2, .label="READY"},
    {.id="channel", .type=UniElementHStep, .x=0,.y=3,.w=3,.h=1, .label="CH"},
    {.id="volume", .type=UniElementVStep, .x=0,.y=4,.w=1,.h=2, .label="VOL"},
    {.id="power", .type=UniElementButton, .x=1,.y=4,.w=1,.h=2, .label="PWR", .icon="pwr"},
    {.id="source", .type=UniElementVStep, .x=2,.y=4,.w=1,.h=2, .label="SRC"},
};

static const UniElement tv_dpad[] = {
    {.id="status", .type=UniElementStatus, .x=0,.y=0,.w=3,.h=1},
    {.id="main", .type=UniElementScreen, .x=0,.y=1,.w=3,.h=2, .label="READY"},
    {.id="nav", .type=UniElementDpad, .x=0,.y=3,.w=3,.h=3},
};

static const UniElement media[] = {
    {.id="status", .type=UniElementStatus, .x=0,.y=0,.w=3,.h=1},
    {.id="main", .type=UniElementScreen, .x=0,.y=1,.w=3,.h=2, .label="MEDIA"},
    {.id="prev", .type=UniElementButton, .x=0,.y=3,.w=1,.h=1, .label="PRV", .icon="prev"},
    {.id="play", .type=UniElementButton, .x=1,.y=3,.w=1,.h=1, .label="PLY", .icon="play"},
    {.id="next", .type=UniElementButton, .x=2,.y=3,.w=1,.h=1, .label="NXT", .icon="next"},
    {.id="rew", .type=UniElementButton, .x=0,.y=4,.w=1,.h=1, .label="REW", .icon="rew"},
    {.id="stop", .type=UniElementButton, .x=1,.y=4,.w=1,.h=1, .label="STP", .icon="stop"},
    {.id="ffwd", .type=UniElementButton, .x=2,.y=4,.w=1,.h=1, .label="FFD", .icon="ffwd"},
    {.id="mute", .type=UniElementButton, .x=0,.y=5,.w=1,.h=1, .label="MUT", .icon="mut"},
    {.id="home", .type=UniElementButton, .x=1,.y=5,.w=1,.h=1, .label="HOM", .icon="home"},
    {.id="back", .type=UniElementButton, .x=2,.y=5,.w=1,.h=1, .label="BCK", .icon="back"},
};

static const UniElement ac_basic[] = {
    {.id="status", .type=UniElementStatus, .x=0,.y=0,.w=3,.h=1},
    {.id="main", .type=UniElementScreen, .x=0,.y=1,.w=3,.h=2, .label="LOCAL"},
    {.id="mode", .type=UniElementHStep, .x=0,.y=3,.w=3,.h=1, .label="MOD"},
    {.id="temp", .type=UniElementVStep, .x=0,.y=4,.w=1,.h=2, .label="TMP"},
    {.id="power", .type=UniElementButton, .x=1,.y=4,.w=1,.h=2, .label="PWR", .icon="pwr"},
    {.id="fan", .type=UniElementVStep, .x=2,.y=4,.w=1,.h=2, .label="FAN"},
};

static const UniLayoutPreset layouts[] = {
    {"tv", "TV BASIC", tv_basic, sizeof(tv_basic)/sizeof(tv_basic[0])},
    {"dpad", "TV D-PAD", tv_dpad, sizeof(tv_dpad)/sizeof(tv_dpad[0])},
    {"media", "MEDIA", media, sizeof(media)/sizeof(media[0])},
    {"ac", "AC BASIC", ac_basic, sizeof(ac_basic)/sizeof(ac_basic[0])},
};

size_t uni_element_preset_count(void) {
    return sizeof(element_presets) / sizeof(element_presets[0]);
}

const UniElementPreset* uni_element_preset_get(size_t index) {
    if(index >= uni_element_preset_count()) return NULL;
    return &element_presets[index];
}

size_t uni_layout_preset_count(void) {
    return sizeof(layouts) / sizeof(layouts[0]);
}

const UniLayoutPreset* uni_layout_preset_get(size_t index) {
    if(index >= uni_layout_preset_count()) return NULL;
    return &layouts[index];
}
