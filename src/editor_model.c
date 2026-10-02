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
