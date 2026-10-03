#include "editor_model.h"

size_t uni_editor_binding_count(const UniElement* e) {
    if(!e) return 0;
    switch(e->type) {
    case UniElementButton:
        return 2;
    case UniElementHStep:
        return 2;
    case UniElementVStep:
        return 2;
    case UniElementDpad:
        return 10;
    default:
        return 0;
    }
}

const char* uni_editor_binding_key(const UniElement* e, size_t i) {
    if(!e) return "";
    if(e->type == UniElementButton) {
        static const char* k[] = {"tap","hold"};
        return i < 2 ? k[i] : "";
    }
    if(e->type == UniElementHStep) {
        static const char* k[] = {"left","right"};
        return i < 2 ? k[i] : "";
    }
    if(e->type == UniElementVStep) {
        static const char* k[] = {"up","down"};
        return i < 2 ? k[i] : "";
    }
    if(e->type == UniElementDpad) {
        static const char* k[] = {
            "up","down","left","right","ok",
            "up_hold","down_hold","left_hold","right_hold","ok_hold"};
        return i < 10 ? k[i] : "";
    }
    return "";
}

const char* uni_editor_binding_label(const UniElement* e, size_t i) {
    if(!e) return "";
    if(e->type == UniElementButton) {
        static const char* l[] = {"TAP","HOLD"};
        return i < 2 ? l[i] : "";
    }
    if(e->type == UniElementHStep) {
        static const char* l[] = {"LEFT","RIGHT"};
        return i < 2 ? l[i] : "";
    }
    if(e->type == UniElementVStep) {
        static const char* l[] = {"UP","DOWN"};
        return i < 2 ? l[i] : "";
    }
    if(e->type == UniElementDpad) {
        static const char* l[] = {
            "UP","DOWN","LEFT","RIGHT","OK",
            "UP HOLD","DN HOLD","LT HOLD","RT HOLD","OK HOLD"};
        return i < 10 ? l[i] : "";
    }
    return "";
}

size_t uni_editor_icon_count(const UniElement* e) {
    if(!e) return 0;
    if(e->type == UniElementButton) return 2;
    if(e->type == UniElementDpad) return 5;
    return 0;
}

const char* uni_editor_icon_key(const UniElement* e, size_t i) {
    if(!e) return "";
    if(e->type == UniElementButton) {
        static const char* k[] = {"icon","hold_icon"};
        return i < 2 ? k[i] : "";
    }
    if(e->type == UniElementDpad) {
        static const char* k[] = {
            "up_hold_icon","down_hold_icon","left_hold_icon","right_hold_icon","ok_hold_icon"};
        return i < 5 ? k[i] : "";
    }
    return "";
}

const char* uni_editor_icon_label(const UniElement* e, size_t i) {
    if(!e) return "";
    if(e->type == UniElementButton) {
        static const char* l[] = {"ICON","HOLD ICON"};
        return i < 2 ? l[i] : "";
    }
    if(e->type == UniElementDpad) {
        static const char* l[] = {"UP ICON","DN ICON","LT ICON","RT ICON","OK ICON"};
        return i < 5 ? l[i] : "";
    }
    return "";
}

const char* uni_editor_hard_label(UniHardKeySlot slot) {
    static const char* labels[] = {"UP HOLD","DN HOLD","LT HOLD","RT HOLD","OK HOLD"};
    return slot < UniHardCount ? labels[slot] : "";
}

typedef struct {
    const char* binding;
    const char* label;
} UniEditorActionPreset;

static const UniEditorActionPreset lg_actions[] = {
    {"state:power:toggle","POWER"},
    {"state:temp:+","TEMP +"},{"state:temp:-","TEMP -"},
    {"state:mode:+","MODE +"},{"state:mode:-","MODE -"},
    {"state:fan:+","FAN +"},{"state:fan:-","FAN -"},
    {"state:swing_v:+","SW V +"},{"state:swing_v:-","SW V -"},
    {"state:swing_h:+","SW H +"},{"state:swing_h:-","SW H -"},
    {"state:jet:toggle","JET"},{"state:eco:toggle","ECO"},
    {"state:comfort:toggle","COMFORT"},{"state:display:toggle","DISPLAY"},
    {"state:auto_clean:toggle","AUTO CLEAN"},{"state:purify:toggle","PURIFY"},
    {"state:jet_dry:toggle","JET DRY"},{"state:unit:toggle","C/F"},
    {"state:diagnosis:send","DIAG"},
    {"state:timer_on:+","ON +10"},{"state:timer_on:-","ON -10"},
    {"state:timer_on:toggle","ON TIMER"},
    {"state:timer_off:+","OFF +10"},{"state:timer_off:-","OFF -10"},
    {"state:timer_off:toggle","OFF TIMER"},
    {"state:sleep:+","SLEEP +"},{"state:sleep:-","SLEEP -"},
    {"state:timers:clear","CLR TIMER"},
};

static const UniEditorActionPreset daikin_actions[] = {
    {"state:power:toggle","POWER"},
    {"state:temp:+","TEMP +"},{"state:temp:-","TEMP -"},
    {"state:mode:+","MODE +"},{"state:mode:-","MODE -"},
    {"state:fan:+","FAN +"},{"state:fan:-","FAN -"},
    {"state:swing_v:toggle","SWING"},
    {"state:powerful:toggle","POWERFUL"},
    {"state:timer_on:+","ON +10"},{"state:timer_on:-","ON -10"},
    {"state:timer_on:toggle","ON TIMER"},
    {"state:timer_off:+","OFF +10"},{"state:timer_off:-","OFF -10"},
    {"state:timer_off:toggle","OFF TIMER"},
    {"state:timers:clear","CLR TIMER"},
};

static const UniEditorActionPreset bt_actions[] = {
    {"bt:media:play_pause","PLAY/PAUSE"},
    {"bt:media:next","NEXT"},{"bt:media:prev","PREV"},
    {"bt:media:stop","STOP"},{"bt:media:mute","MUTE"},
    {"bt:media:vol_up","VOL +"},{"bt:media:vol_down","VOL -"},
    {"bt:media:home","HOME"},{"bt:media:back","BACK"},
    {"bt:media:forward","FORWARD"},
    {"bt:key:up","KEY UP"},{"bt:key:down","KEY DOWN"},
    {"bt:key:left","KEY LEFT"},{"bt:key:right","KEY RIGHT"},
    {"bt:key:enter","ENTER"},{"bt:key:escape","ESC"},
    {"bt:key:space","SPACE"},{"bt:key:tab","TAB"},
    {"bt:key:page_up","PAGE UP"},{"bt:key:page_down","PAGE DOWN"},
};

static const UniEditorActionPreset*
    transport_actions(const UniRemote* remote, size_t* count) {
    if(count) *count = 0;
    if(!remote) return NULL;
    if(remote->transport == UniTransportBluetoothHid) {
        if(count) *count = sizeof(bt_actions) / sizeof(bt_actions[0]);
        return bt_actions;
    }
    if(remote->transport != UniTransportStatefulIr) return NULL;
    if(strcmp(remote->state_adapter, "LG_AC") == 0) {
        if(count) *count = sizeof(lg_actions) / sizeof(lg_actions[0]);
        return lg_actions;
    }
    if(strcmp(remote->state_adapter, "DAIKIN_ARC433A73") == 0) {
        if(count) *count = sizeof(daikin_actions) / sizeof(daikin_actions[0]);
        return daikin_actions;
    }
    return NULL;
}

size_t uni_editor_transport_action_count(const UniRemote* remote) {
    size_t count = 0;
    transport_actions(remote, &count);
    return count;
}

const char* uni_editor_transport_action_binding(const UniRemote* remote, size_t index) {
    size_t count = 0;
    const UniEditorActionPreset* actions = transport_actions(remote, &count);
    return actions && index < count ? actions[index].binding : "";
}

const char* uni_editor_transport_action_label(const UniRemote* remote, size_t index) {
    size_t count = 0;
    const UniEditorActionPreset* actions = transport_actions(remote, &count);
    return actions && index < count ? actions[index].label : "";
}
