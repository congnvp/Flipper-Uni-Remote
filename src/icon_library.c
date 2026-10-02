#include "icon_library.h"
#include <string.h>

static const UniIconDef icons[] = {
    {"pwr","PWR",UniIconPower},{"mut","MUT",UniIconMute},{"play","PLY",UniIconPlay},
    {"paus","PAU",UniIconPause},{"stop","STP",UniIconStop},{"rec","REC",UniIconRecord},
    {"prev","PRV",UniIconPrev},{"next","NXT",UniIconNext},{"rew","REW",UniIconRew},
    {"ffwd","FFD",UniIconFfwd},{"home","HOM",UniIconHome},{"back","BCK",UniIconBack},
    {"menu","MNU",UniIconMenu},{"info","INF",UniIconInfo},{"set","SET",UniIconGear},
    {"volp","V+",UniIconPlus},{"volm","V-",UniIconMinus},{"chp","C+",UniIconPlus},
    {"chm","C-",UniIconMinus},{"src","SRC",UniIconSource},{"app","APP",UniIconText},
    {"guide","GDE",UniIconText},{"fav","FAV",UniIconText},{"sub","SUB",UniIconText},
    {"cc","CC",UniIconText},{"aud","AUD",UniIconText},{"num","NUM",UniIconText},
    {"red","RED",UniIconText},{"grn","GRN",UniIconText},{"blu","BLU",UniIconText},
    {"yel","YEL",UniIconText},{"temp","TMP",UniIconText},{"fan","FAN",UniIconFan},
    {"mode","MOD",UniIconText},{"swng","SWG",UniIconText},{"eco","ECO",UniIconText},
    {"slp","SLP",UniIconMoon},{"tmr","TMR",UniIconText},{"lite","LIT",UniIconSun},
    {"cool","CLD",UniIconSnow},{"heat","HOT",UniIconHeat},{"dry","DRY",UniIconDrop},
    {"auto","AUT",UniIconText},{"zoom","ZOM",UniIconText},{"focs","FOC",UniIconText},
    {"key","KEY",UniIconText},{"blank","BLK",UniIconText},{"frz","FRZ",UniIconText},
    {"asp","ASP",UniIconText},{"hdmi","HDM",UniIconText},{"usb","USB",UniIconText},
    {"bt","BT",UniIconText},{"wifi","WFI",UniIconText},{"lock","LCK",UniIconLock},
    {"mic","MIC",UniIconText},{"voice","VOC",UniIconText},{"eject","EJT",UniIconText},
    {"pip","PIP",UniIconText},{"sleep","SLP",UniIconMoon},{"input","INP",UniIconSource},
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
