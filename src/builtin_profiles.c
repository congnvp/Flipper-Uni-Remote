#include "builtin_profiles.h"

#include <furi.h>
#include <stdio.h>
#include <string.h>

#define BUILTIN_ROOT APP_DATA_PATH("remotes")

typedef struct {
    uint8_t page;
    uint8_t x;
    uint8_t y;
    const char* id;
    const char* label;
    const char* icon;
    const char* tap;
    const char* hold;
} BuiltinButton;

typedef struct {
    const char* folder;
    const char* id;
    const char* name;
    const char* short_name;
    const char* transport;
    const char* state_profile;
    uint32_t order;
    uint8_t pages;
    const BuiltinButton* buttons;
    size_t button_count;
    const char* signals;
    bool sony_burst;
} BuiltinProfile;

#define B(P,X,Y,ID,L,I,T) {P,X,Y,ID,L,I,T,""}

static const BuiltinButton lg_buttons[] = {
    B(0,0,0,"power","PWR","pwr","state:power"),
    B(0,1,0,"mode_m","MOD-","mode","state:mode-"),
    B(0,2,0,"mode_p","MOD+","mode","state:mode+"),
    B(0,0,1,"temp_m","TMP-","volm","state:temp-"),
    B(0,1,1,"temp","TEMP","temp","state:resend"),
    B(0,2,1,"temp_p","TMP+","volp","state:temp+"),
    B(0,0,2,"fan_m","FAN-","fan","state:fan-"),
    B(0,1,2,"fan","FAN","fan","state:resend"),
    B(0,2,2,"fan_p","FAN+","fan","state:fan+"),
    B(0,0,3,"sv_m","SV-","swng","state:swing_v-"),
    B(0,1,3,"sv","SV","swng","state:resend"),
    B(0,2,3,"sv_p","SV+","swng","state:swing_v+"),
    B(0,0,4,"sh_m","SH-","swng","state:swing_h-"),
    B(0,1,4,"sh","SH","swng","state:resend"),
    B(0,2,4,"sh_p","SH+","swng","state:swing_h+"),
    B(0,0,5,"jet","JET","auto","state:jet"),
    B(0,1,5,"eco","ECO","eco","state:eco"),
    B(0,2,5,"light","LGT","lite","state:light"),
};

static const BuiltinButton daikin_buttons[] = {
    B(0,0,0,"power","PWR","pwr","state:power"),
    B(0,1,0,"mode_m","MOD-","mode","state:mode-"),
    B(0,2,0,"mode_p","MOD+","mode","state:mode+"),
    B(0,0,1,"temp_m","TMP-","volm","state:temp-"),
    B(0,1,1,"temp","TEMP","temp","state:resend"),
    B(0,2,1,"temp_p","TMP+","volp","state:temp+"),
    B(0,0,2,"fan_m","FAN-","fan","state:fan-"),
    B(0,1,2,"fan","FAN","fan","state:resend"),
    B(0,2,2,"fan_p","FAN+","fan","state:fan+"),
    B(0,0,3,"swing","SWING","swng","state:swing"),
    B(0,1,3,"powerful","PWRFL","auto","state:powerful"),
    B(0,2,3,"resend","SEND","src","state:resend"),
};

static const BuiltinButton carrier_buttons[] = {
    B(0,0,0,"power","PWR","pwr","state:power"),
    B(0,1,0,"mode_m","MOD-","mode","state:mode-"),
    B(0,2,0,"mode_p","MOD+","mode","state:mode+"),
    B(0,0,1,"temp_m","TMP-","volm","state:temp-"),
    B(0,1,1,"temp","TEMP","temp","state:resend"),
    B(0,2,1,"temp_p","TMP+","volp","state:temp+"),
    B(0,0,2,"fan_m","FAN-","fan","state:fan-"),
    B(0,1,2,"fan","FAN","fan","state:resend"),
    B(0,2,2,"fan_p","FAN+","fan","state:fan+"),
    B(0,0,3,"fix","FIX","swng","state:fix"),
    B(0,1,3,"swing","SWING","swng","state:swing"),
    B(0,2,3,"resend","SEND","src","state:resend"),
    B(0,0,4,"powerful","HI PWR","auto","state:powerful"),
    B(0,1,4,"eco","ECO","eco","state:eco"),
};

static const BuiltinButton panasonic_buttons[] = {
    B(0,0,0,"power","PWR","pwr","state:power"),
    B(0,1,0,"mode_m","MOD-","mode","state:mode-"),
    B(0,2,0,"mode_p","MOD+","mode","state:mode+"),
    B(0,0,1,"temp_m","TMP-","volm","state:temp-"),
    B(0,1,1,"temp","TEMP","temp","state:resend"),
    B(0,2,1,"temp_p","TMP+","volp","state:temp+"),
    B(0,0,2,"fan_m","FAN-","fan","state:fan-"),
    B(0,1,2,"fan","FAN","fan","state:resend"),
    B(0,2,2,"fan_p","FAN+","fan","state:fan+"),
    B(0,0,3,"swing_m","SW-","swng","state:swing-"),
    B(0,1,3,"swing","SWING","swng","state:resend"),
    B(0,2,3,"swing_p","SW+","swng","state:swing+"),
    B(0,0,4,"nanoe","NANOE","auto","state:nanoe"),
    B(0,1,4,"extra_m","EXT-","eco","state:extra-"),
    B(0,2,4,"extra_p","EXT+","eco","state:extra+"),
};

static const BuiltinButton sony_buttons[] = {
    B(0,0,0,"power","PWR","pwr","act:Power"),
    B(0,1,0,"input","INPUT","src","act:Input"),
    B(0,2,0,"apa","APA","auto","act:APA"),
    B(0,0,1,"eco","ECO","eco","act:Eco"),
    B(0,1,1,"menu","MENU","menu","act:Menu"),
    B(0,2,1,"reset","RESET","back","act:Reset"),
    B(0,1,2,"up","UP","","act:Up"),
    B(0,0,3,"left","LEFT","","act:Left"),
    B(0,1,3,"enter","OK","","act:Enter"),
    B(0,2,3,"right","RIGHT","","act:Right"),
    B(0,1,4,"down","DOWN","","act:Down"),
    B(0,0,5,"return","BACK","back","act:Return"),
    B(1,0,0,"aspect","ASP","asp","act:Aspect"),
    B(1,1,0,"key","KEY","key","act:Keystone"),
    B(1,2,0,"pattern","PAT","app","act:Pattern"),
    B(1,0,1,"blank","BLANK","blank","act:Blank"),
    B(1,1,1,"freeze","FRZ","frz","act:Freeze"),
    B(1,0,2,"zoom_p","Z+","zoom","act:DZoomUp"),
    B(1,1,2,"zoom_m","Z-","zoom","act:DZoomDown"),
    B(1,0,3,"vol_p","VOL+","volp","act:VolUp"),
    B(1,1,3,"vol_m","VOL-","volm","act:VolDown"),
    B(1,2,3,"mute","MUTE","mut","act:Muting"),
};

static const BuiltinButton optoma_buttons[] = {
    B(0,0,0,"power","PWR","pwr","sig:Power"),
    B(0,1,0,"aspect","ASP","asp","sig:Aspect"),
    B(0,2,0,"source","SRC","src","sig:Source"),
    B(0,0,1,"mode","MODE","mode","sig:Mode"),
    B(0,1,1,"settings","SET","set","sig:Settings"),
    B(0,2,1,"menu","MENU","menu","sig:Menu"),
    B(0,1,2,"up","UP","","sig:Up"),
    B(0,0,3,"left","LEFT","","sig:Left"),
    B(0,1,3,"enter","OK","","sig:Enter"),
    B(0,2,3,"right","RIGHT","","sig:Right"),
    B(0,1,4,"down","DOWN","","sig:Down"),
    B(0,0,5,"return","BACK","back","sig:Return"),
    B(1,0,0,"vol_m","VOL-","volm","sig:VolDown"),
    B(1,1,0,"mute","MUTE","mut","sig:Mute"),
    B(1,2,0,"vol_p","VOL+","volp","sig:VolUp"),
    B(1,0,1,"freeze","FRZ","frz","sig:Freeze"),
    B(1,1,1,"key","KEY","key","sig:Keystone"),
    B(1,2,1,"avmute","AV MUT","mut","sig:AvMute"),
};

static const char sony_signals[] =
"Filetype: IR signals file\nVersion: 1\n#\n"
"name: Power\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 15 00 00 00\n#\n"
"name: Input\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 57 00 00 00\n#\n"
"name: APA\ntype: parsed\nprotocol: SIRC20\naddress: 5A 05 00 00\ncommand: 60 00 00 00\n#\n"
"name: Eco\ntype: parsed\nprotocol: SIRC20\naddress: FA 04 00 00\ncommand: 11 00 00 00\n#\n"
"name: Menu\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 29 00 00 00\n#\n"
"name: Reset\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 7B 00 00 00\n#\n"
"name: Up\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 35 00 00 00\n#\n"
"name: Down\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 36 00 00 00\n#\n"
"name: Left\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 34 00 00 00\n#\n"
"name: Right\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 33 00 00 00\n#\n"
"name: Enter\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 5A 00 00 00\n#\n"
"name: Return\ntype: parsed\nprotocol: SIRC20\naddress: FA 04 00 00\ncommand: 6F 00 00 00\n#\n"
"name: Aspect\ntype: parsed\nprotocol: SIRC20\naddress: 5A 05 00 00\ncommand: 6E 00 00 00\n#\n"
"name: Keystone\ntype: parsed\nprotocol: SIRC20\naddress: 5A 05 00 00\ncommand: 3A 00 00 00\n#\n"
"name: Pattern\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 7E 00 00 00\n#\n"
"name: Blank\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 24 00 00 00\n#\n"
"name: Freeze\ntype: parsed\nprotocol: SIRC20\naddress: 5A 05 00 00\ncommand: 67 00 00 00\n#\n"
"name: DZoomUp\ntype: parsed\nprotocol: SIRC20\naddress: 5A 05 00 00\ncommand: 6A 00 00 00\n#\n"
"name: DZoomDown\ntype: parsed\nprotocol: SIRC20\naddress: 5A 05 00 00\ncommand: 6B 00 00 00\n#\n"
"name: VolUp\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 12 00 00 00\n#\n"
"name: VolDown\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 13 00 00 00\n#\n"
"name: Muting\ntype: parsed\nprotocol: SIRC15\naddress: 54 00 00 00\ncommand: 14 00 00 00\n";

static const char optoma_signals[] =
"Filetype: IR signals file\nVersion: 1\n#\n"
"name: Power\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 02 00 00 00\n#\n"
"name: Aspect\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 64 00 00 00\n#\n"
"name: Source\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: C3 00 00 00\n#\n"
"name: Mode\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 05 00 00 00\n#\n"
"name: Settings\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: A8 00 00 00\n#\n"
"name: Menu\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 0E 00 00 00\n#\n"
"name: Up\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 11 00 00 00\n#\n"
"name: Left\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 10 00 00 00\n#\n"
"name: Enter\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 0F 00 00 00\n#\n"
"name: Right\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 12 00 00 00\n#\n"
"name: Down\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 14 00 00 00\n#\n"
"name: Return\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 0D 00 00 00\n#\n"
"name: VolDown\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 8F 00 00 00\n#\n"
"name: Mute\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 52 00 00 00\n#\n"
"name: VolUp\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 8C 00 00 00\n#\n"
"name: Freeze\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 06 00 00 00\n#\n"
"name: Keystone\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 07 00 00 00\n#\n"
"name: AvMute\ntype: parsed\nprotocol: NEC\naddress: 32 00 00 00\ncommand: 03 00 00 00\n";

static const BuiltinProfile profiles[] = {
    {"lg_ac","lg_ac","LG AC","LG","STATE_IR","lg_ac",20,1,lg_buttons,sizeof(lg_buttons)/sizeof(lg_buttons[0]),NULL,false},
    {"daikin_ac","daikin_ac","Daikin ARC433A73","DKN","STATE_IR","daikin_arc433a73",30,1,daikin_buttons,sizeof(daikin_buttons)/sizeof(daikin_buttons[0]),NULL,false},
    {"carrier_ac","carrier_ac","Carrier WC-UA4NE","CAR","STATE_IR","carrier_wc_ua4ne",40,1,carrier_buttons,sizeof(carrier_buttons)/sizeof(carrier_buttons[0]),NULL,false},
    {"panasonic_ac","panasonic_ac","Panasonic AC","PAN","STATE_IR","panasonic_rkr",50,1,panasonic_buttons,sizeof(panasonic_buttons)/sizeof(panasonic_buttons[0]),NULL,false},
    {"sony_pj8","sony_pj8","Sony RM-PJ8","SON","IR","",60,2,sony_buttons,sizeof(sony_buttons)/sizeof(sony_buttons[0]),sony_signals,true},
    {"optoma_hr21g","optoma_hr21g","Optoma HR21G","OPT","IR","",70,2,optoma_buttons,sizeof(optoma_buttons)/sizeof(optoma_buttons[0]),optoma_signals,false},
};

static bool write_text(Storage* storage,const char* path,const char* text) {
    File* file=storage_file_alloc(storage);
    if(!file) return false;
    bool ok=storage_file_open(file,path,FSAM_WRITE,FSOM_CREATE_ALWAYS);
    if(ok) {
        size_t n=strlen(text);
        ok=storage_file_write(file,text,n)==n;
        storage_file_sync(file);
    }
    storage_file_close(file);
    storage_file_free(file);
    return ok;
}

static void add_button(FuriString* s,size_t i,const BuiltinButton* b) {
    furi_string_cat_printf(
        s,
        "#\nElement%luType: button\nElement%luId: %s\nElement%luPage: %u\n"
        "Element%luRect: %u %u 1 1\nElement%luLabel: %s\nElement%luIcon: %s\n"
        "Element%luTap: %s\nElement%luHold: %s\n",
        (unsigned long)i,(unsigned long)i,b->id,(unsigned long)i,(unsigned)b->page,
        (unsigned long)i,(unsigned)b->x,(unsigned)b->y,
        (unsigned long)i,b->label,(unsigned long)i,b->icon,
        (unsigned long)i,b->tap,(unsigned long)i,b->hold);
}

static bool write_remote(Storage* storage,const BuiltinProfile* p,const char* dir) {
    char path[256];
    snprintf(path,sizeof(path),"%s/remote.ur",dir);
    if(storage_file_exists(storage,path)) return true;

    FuriString* text=furi_string_alloc();
    furi_string_printf(
        text,
        "Filetype: Flipper Uni Remote\nVersion: 1\nId: %s\nName: %s\n"
        "ShortName: %s\nTransport: %s\nStateProfile: %s\nOrder: %lu\n"
        "PageCount: %u\nRepeatEnabled: true\nSignalFile: signals.ir\n"
        "ActionFile: actions.ur\nBluetoothProfile: \nHardUpHold: \n"
        "HardDownHold: \nHardLeftHold: \nHardRightHold: \nHardOkHold: \n"
        "ElementCount: %lu\n",
        p->id,p->name,p->short_name,p->transport,p->state_profile,
        (unsigned long)p->order,(unsigned)p->pages,(unsigned long)p->button_count);
    for(size_t i=0;i<p->button_count;i++) add_button(text,i,&p->buttons[i]);
    bool ok=write_text(storage,path,furi_string_get_cstr(text));
    furi_string_free(text);
    return ok;
}

static bool write_sony_actions(Storage* storage,const BuiltinProfile* p,const char* dir) {
    if(!p->sony_burst) return true;
    char path[256];
    snprintf(path,sizeof(path),"%s/actions.ur",dir);
    if(storage_file_exists(storage,path)) return true;

    FuriString* text=furi_string_alloc();
    furi_string_printf(
        text,
        "Filetype: Flipper Uni Remote Actions\nVersion: 1\nActionCount: %lu\n",
        (unsigned long)p->button_count);
    for(size_t i=0;i<p->button_count;i++) {
        const char* binding=p->buttons[i].tap;
        const char* signal=strncmp(binding,"act:",4)==0?binding+4:binding;
        furi_string_cat_printf(
            text,
            "#\nAction%luId: %s\nAction%luType: sequence\nAction%luStepCount: 3\n"
            "Action%luStep0: %s\nAction%luDelay0: 25\n"
            "Action%luStep1: %s\nAction%luDelay1: 25\n"
            "Action%luStep2: %s\nAction%luDelay2: 0\n",
            (unsigned long)i,signal,(unsigned long)i,(unsigned long)i,
            (unsigned long)i,signal,(unsigned long)i,
            (unsigned long)i,signal,(unsigned long)i,
            (unsigned long)i,signal,(unsigned long)i);
    }
    bool ok=write_text(storage,path,furi_string_get_cstr(text));
    furi_string_free(text);
    return ok;
}

bool uni_builtin_profiles_seed(Storage* storage) {
    if(!storage) return false;
    storage_common_mkdir(storage,BUILTIN_ROOT);
    bool ok=true;
    for(size_t i=0;i<sizeof(profiles)/sizeof(profiles[0]);i++) {
        const BuiltinProfile* p=&profiles[i];
        char dir[192];
        snprintf(dir,sizeof(dir),BUILTIN_ROOT "/%s",p->folder);
        storage_common_mkdir(storage,dir);
        if(!write_remote(storage,p,dir)) ok=false;
        if(p->signals) {
            char sp[256];
            snprintf(sp,sizeof(sp),"%s/signals.ir",dir);
            if(!storage_file_exists(storage,sp) && !write_text(storage,sp,p->signals)) ok=false;
        }
        if(!write_sony_actions(storage,p,dir)) ok=false;
    }
    return ok;
}
