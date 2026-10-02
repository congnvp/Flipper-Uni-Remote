#include "icon_library.h"
#include <string.h>

static const UniIconDef icons[] = {
    {"pwr","POWER",UniIconPower},{"mut","MUTE",UniIconMute},{"input","INPUT",UniIconSource},
    {"home","HOME",UniIconHome},{"menu","MENU",UniIconMenu},{"back","BACK",UniIconBack},
    {"set","SET",UniIconGear},{"info","INFO",UniIconInfo},{"guide","GUIDE",UniIconText},
    {"exit","EXIT",UniIconText},{"up","UP",UniIconText},{"down","DOWN",UniIconText},
    {"left","LEFT",UniIconText},{"right","RIGHT",UniIconText},{"ok","OK",UniIconText},
    {"volp","VOL+",UniIconPlus},{"volm","VOL-",UniIconMinus},{"chp","CH+",UniIconPlus},
    {"chm","CH-",UniIconMinus},{"play","PLAY",UniIconPlay},{"paus","PAUSE",UniIconPause},
    {"stop","STOP",UniIconStop},{"rec","RECORD",UniIconRecord},{"rew","REW",UniIconRew},
    {"ffwd","FWD",UniIconFfwd},{"prev","PREV",UniIconPrev},{"next","NEXT",UniIconNext},
    {"sub","SUB",UniIconText},{"aud","AUDIO",UniIconText},{"mic","MIC",UniIconText},
    {"voice","VOICE",UniIconText},{"search","SEARCH",UniIconText},{"apps","APPS",UniIconText},
    {"key","KEYBOARD",UniIconText},{"mouse","MOUSE",UniIconText},{"pair","PAIR",UniIconText},
    {"bt","BT",UniIconText},{"n0","0",UniIconText},{"n1","1",UniIconText},
    {"n2","2",UniIconText},{"n3","3",UniIconText},{"n4","4",UniIconText},
    {"n5","5",UniIconText},{"n6","6",UniIconText},{"n7","7",UniIconText},
    {"n8","8",UniIconText},{"n9","9",UniIconText},{"dot","DOT",UniIconText},
    {"dash","DASH",UniIconText},{"red","RED",UniIconText},{"grn","GREEN",UniIconText},
    {"yel","YELLOW",UniIconText},{"blu","BLUE",UniIconText},{"lite","LIGHT",UniIconSun},
    {"slp","SLEEP",UniIconMoon},{"tmr","TIMER",UniIconText},{"fan","FAN",UniIconFan},
    {"tempup","TEMP+",UniIconPlus},{"tempdn","TEMP-",UniIconMinus},{"mode","MODE",UniIconText},
    {"auto","AUTO",UniIconText},{"cool","COOL",UniIconSnow},{"heat","HEAT",UniIconHeat},
    {"dry","DRY",UniIconDrop},{"swng","SWING",UniIconText},{"eco","ECO",UniIconText},
    {"turbo","TURBO",UniIconText},{"frz","FREEZE",UniIconText},{"zmin","ZOOM+",UniIconText},
    {"zout","ZOOM-",UniIconText},{"focin","FOCUS+",UniIconText},{"focout","FOCUS-",UniIconText},
    {"asp","ASPECT",UniIconText},{"proj","PROJECTOR",UniIconText},{"tv","TV",UniIconText},
    {"game","GAMEPAD",UniIconText},{"a","A",UniIconText},{"b","B",UniIconText},
    {"x","X",UniIconText},{"y","Y",UniIconText},{"blank","BLANK",UniIconText},
    {"repeat","REPEAT",UniIconText},{"shuf","SHUFFLE",UniIconText},{"fav","FAVORITE",UniIconText},
    {"brip","BRIGHT+",UniIconText},{"brim","BRIGHT-",UniIconText},{"lastch","LAST CH",UniIconText},
    {"eject","EJECT",UniIconText},
};

size_t uni_icon_count(void) {
    return sizeof(icons) / sizeof(icons[0]);
}

const UniIconDef* uni_icon_get(size_t index) {
    if(index >= uni_icon_count()) return NULL;
    return &icons[index];
}

const UniIconDef* uni_icon_find(const char* id) {
    if(!id || !id[0]) return NULL;
    for(size_t i = 0; i < uni_icon_count(); i++) {
        if(strcmp(icons[i].id, id) == 0) return &icons[i];
    }
    return NULL;
}
