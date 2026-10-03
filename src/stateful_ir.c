#include "stateful_ir.h"

#include <furi.h>
#include <infrared/worker/infrared_transmit.h>
#include <storage/storage.h>
#include <stdio.h>
#include <string.h>

#define UNI_STATE_MAGIC 0x55414353UL
#define UNI_STATE_VERSION 1U

#define IR_FREQ 38000U

typedef struct {
    uint32_t magic;
    uint8_t version;
    uint8_t power;
    uint8_t mode;
    uint8_t temp_x2;
    uint8_t fan;
    uint8_t swing_v;
    uint8_t swing_h;
    uint8_t eco;
    uint8_t turbo;
    uint8_t nanoe;
    uint8_t sleep_step;
} PersistedState;

static bool file_read_all(Storage* storage, const char* path, void* data, size_t size) {
    File* file = storage_file_alloc(storage);
    if(!file) return false;
    bool ok = false;
    if(storage_file_open(file, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        ok = storage_file_read(file, data, size) == size;
        storage_file_close(file);
    }
    storage_file_free(file);
    return ok;
}

static bool file_write_all(Storage* storage, const char* path, const void* data, size_t size) {
    File* file = storage_file_alloc(storage);
    if(!file) return false;
    bool ok = false;
    if(storage_file_open(file, path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        ok = storage_file_write(file, data, size) == size;
        storage_file_sync(file);
        storage_file_close(file);
    }
    storage_file_free(file);
    return ok;
}

static void default_state(const UniRemote* remote, UniAcState* s) {
    memset(s, 0, sizeof(*s));
    s->mode = UniAcModeCool;
    s->fan = 0;
    s->swing_v = 0;
    s->swing_h = 0;
    s->temp_x2 = 46; /* 23 C */

    if(strcmp(remote->state_profile, "PANASONIC_RKR") == 0) {
        s->mode = UniAcModeAuto;
        s->temp_x2 = 50;
        s->fan = 6;     /* Auto */
        s->swing_v = 1; /* physical position 2 */
    } else if(strcmp(remote->state_profile, "CARRIER_WC_UA4NE") == 0) {
        s->mode = UniAcModeAuto;
        s->temp_x2 = 48;
    }
}

bool uni_stateful_ir_load_state(Storage* storage, const UniRemote* remote, UniAcState* out) {
    if(!storage || !remote || !out) return false;

    PersistedState p = {0};
    if(file_read_all(storage, remote->state_path, &p, sizeof(p)) &&
       p.magic == UNI_STATE_MAGIC && p.version == UNI_STATE_VERSION &&
       p.mode <= UniAcModeHeat) {
        out->power = p.power != 0;
        out->mode = (UniAcMode)p.mode;
        out->temp_x2 = p.temp_x2;
        out->fan = p.fan;
        out->swing_v = p.swing_v;
        out->swing_h = p.swing_h;
        out->eco = p.eco != 0;
        out->turbo = p.turbo != 0;
        out->nanoe = p.nanoe != 0;
        out->sleep_step = p.sleep_step;
        return true;
    }

    default_state(remote, out);
    return true;
}

static bool save_state(Storage* storage, const UniRemote* remote, const UniAcState* s) {
    PersistedState p = {
        .magic = UNI_STATE_MAGIC,
        .version = UNI_STATE_VERSION,
        .power = s->power ? 1U : 0U,
        .mode = (uint8_t)s->mode,
        .temp_x2 = s->temp_x2,
        .fan = s->fan,
        .swing_v = s->swing_v,
        .swing_h = s->swing_h,
        .eco = s->eco ? 1U : 0U,
        .turbo = s->turbo ? 1U : 0U,
        .nanoe = s->nanoe ? 1U : 0U,
        .sleep_step = s->sleep_step,
    };
    return file_write_all(storage, remote->state_path, &p, sizeof(p));
}

const char* uni_stateful_ir_mode_label(UniAcMode mode) {
    static const char* labels[] = {"AUTO", "COOL", "DRY", "FAN", "HEAT"};
    return mode <= UniAcModeHeat ? labels[mode] : "--";
}

static uint8_t clamp_u8(int value, int lo, int hi) {
    if(value < lo) return (uint8_t)lo;
    if(value > hi) return (uint8_t)hi;
    return (uint8_t)value;
}

static uint8_t wrap_u8(int value, uint8_t count) {
    while(value < 0) value += count;
    while(value >= count) value -= count;
    return (uint8_t)value;
}

/* ---------------- LG AKB75215401 / LG2 28-bit ---------------- */

#define LG_DUTY 0.33f
#define LG_TIMINGS 59U
#define LG_POWER_OFF 0x88C0051UL
#define LG_ENERGY_ON 0x8810045UL
#define LG_ENERGY_OFF 0x8810056UL
#define LG_JET_ON 0x8810089UL
#define LG_JET_OFF_STATE 0U

static const uint8_t lg_fan_native[6] = {0x5,0x0,0x9,0x2,0xA,0x4};
static const uint32_t lg_swing_v[8] = {
    0x881315AUL,0x8813048UL,0x8813059UL,0x881306AUL,
    0x881307BUL,0x881308CUL,0x881309DUL,0x8813149UL
};
static const uint32_t lg_swing_h[9] = {
    0x881317CUL,0x88130BFUL,0x88130C0UL,0x88130D1UL,0x88130E2UL,
    0x88130F3UL,0x8813105UL,0x8813116UL,0x881316BUL
};

static uint8_t lg_checksum16(uint16_t command) {
    uint8_t sum = 0;
    for(uint8_t shift = 0; shift < 16; shift += 4) sum += (command >> shift) & 0xFU;
    return sum & 0xFU;
}

static uint32_t lg_raw_from_command(uint16_t command) {
    return (0x88UL << 20) | ((uint32_t)command << 4) | lg_checksum16(command);
}

static uint16_t lg_state_command(const UniAcState* s, bool power_on_frame) {
    uint16_t command;
    const uint8_t temp_c = s->temp_x2 / 2U;
    const uint8_t fan = s->fan < 6 ? s->fan : 0;

    switch(s->mode) {
    case UniAcModeDry:
        command = 0x0990U;
        break;
    case UniAcModeFan:
        command = (uint16_t)(0x0A00U | (3U << 4) | lg_fan_native[fan]);
        break;
    case UniAcModeAuto:
        command = 0x0B25U;
        break;
    case UniAcModeHeat:
        command = (uint16_t)(0x0C00U | ((temp_c - 15U) << 4) | lg_fan_native[fan]);
        break;
    case UniAcModeCool:
    default:
        command = (uint16_t)(0x0800U | ((temp_c - 15U) << 4) | lg_fan_native[fan]);
        break;
    }
    if(power_on_frame) command &= (uint16_t)~0x0800U;
    return command;
}

static void lg_encode(uint32_t code, uint32_t timings[LG_TIMINGS]) {
    uint8_t i = 0;
    timings[i++] = 3200;
    timings[i++] = 9900;
    for(int8_t bit = 27; bit >= 0; bit--) {
        timings[i++] = 480;
        timings[i++] = (code & (1UL << bit)) ? 1600 : 550;
    }
    timings[i++] = 480;
}

static void lg_send_code(uint32_t code) {
    uint32_t timings[LG_TIMINGS];
    lg_encode(code, timings);
    infrared_send_raw_ext(timings, LG_TIMINGS, true, IR_FREQ, LG_DUTY);
}

static void lg_send_state(const UniAcState* s, bool power_on) {
    lg_send_code(lg_raw_from_command(lg_state_command(s, power_on)));
}

static bool lg_send_action(UniAcState* s, const char* action) {
    if(strcmp(action, "power") == 0) {
        if(s->power) {
            lg_send_code(LG_POWER_OFF);
            s->power = false;
        } else {
            s->power = true;
            lg_send_state(s, true);
        }
        return true;
    }
    if(strcmp(action, "swingv+") == 0) {
        s->swing_v = wrap_u8((int)s->swing_v + 1, 8);
        if(s->power) lg_send_code(lg_swing_v[s->swing_v]);
        return true;
    }
    if(strcmp(action, "swingh+") == 0) {
        s->swing_h = wrap_u8((int)s->swing_h + 1, 9);
        if(s->power) lg_send_code(lg_swing_h[s->swing_h]);
        return true;
    }
    if(strcmp(action, "eco") == 0) {
        s->eco = !s->eco;
        if(s->power) lg_send_code(s->eco ? LG_ENERGY_ON : LG_ENERGY_OFF);
        return true;
    }
    if(strcmp(action, "turbo") == 0) {
        s->turbo = !s->turbo;
        if(s->power) {
            if(s->turbo) lg_send_code(LG_JET_ON);
            else lg_send_state(s, false);
        }
        return true;
    }
    return false;
}

/* ---------------- Daikin ARC433A73 ---------------- */

#define DAIKIN_DUTY 0.50f
#define DAIKIN_LEN 35U
#define DAIKIN_COUNT 584U
#define DAIKIN_HDR_MARK 3650U
#define DAIKIN_HDR_SPACE 1623U
#define DAIKIN_BIT_MARK 428U
#define DAIKIN_ZERO 428U
#define DAIKIN_ONE 1280U
#define DAIKIN_GAP 29000U

static const uint8_t daikin_fan_native[6] = {0xA,0x3,0x4,0x5,0x6,0x7};

static uint8_t sum_bytes(const uint8_t* d, uint8_t n) {
    uint8_t sum = 0;
    for(uint8_t i = 0; i < n; i++) sum = (uint8_t)(sum + d[i]);
    return sum;
}

static uint8_t daikin_mode(const UniAcState* s) {
    if(s->mode == UniAcModeDry) return 0x2U;
    if(s->mode == UniAcModeFan) return 0x6U;
    return 0x3U;
}

static void daikin_build(const UniAcState* s, uint8_t out[DAIKIN_LEN]) {
    memset(out, 0, DAIKIN_LEN);
    out[0]=0x11; out[1]=0xDA; out[2]=0x27; out[4]=0xC5;
    out[8]=0x11; out[9]=0xDA; out[10]=0x27; out[12]=0x42;
    out[16]=0x11; out[17]=0xDA; out[18]=0x27; out[21]=0x08; out[31]=0xC0;

    if(s->power) out[21] |= 0x01U;
    out[21] |= (uint8_t)(daikin_mode(s) << 4);
    out[22] = s->mode == UniAcModeCool ? s->temp_x2 :
              s->mode == UniAcModeDry ? 0xC0U : 50U;
    const uint8_t fan = s->mode == UniAcModeDry ? 0xAU :
        daikin_fan_native[s->fan < 6 ? s->fan : 0];
    out[24] = (uint8_t)((fan << 4) | (s->swing_v ? 0x0FU : 0x00U));
    if(s->turbo && s->power) out[29] |= 0x01U;
    out[26]=0x00; out[27]=0x06; out[28]=0x60; /* unused timers */
    out[7]=sum_bytes(out,7);
    out[15]=sum_bytes(out+8,7);
    out[34]=sum_bytes(out+16,18);
}

static void daikin_bit(uint32_t* t, uint16_t* pos, bool one) {
    t[(*pos)++] = DAIKIN_BIT_MARK;
    t[(*pos)++] = one ? DAIKIN_ONE : DAIKIN_ZERO;
}
static void daikin_section(uint32_t* t,uint16_t* pos,const uint8_t* d,uint8_t n) {
    t[(*pos)++]=DAIKIN_HDR_MARK; t[(*pos)++]=DAIKIN_HDR_SPACE;
    for(uint8_t i=0;i<n;i++) for(uint8_t bit=0;bit<8;bit++) daikin_bit(t,pos,(d[i]&(1U<<bit))!=0);
    t[(*pos)++]=DAIKIN_BIT_MARK;
    t[(*pos)++]=DAIKIN_ZERO+DAIKIN_GAP;
}
static void daikin_send(const UniAcState* s) {
    uint8_t state[DAIKIN_LEN];
    uint32_t timings[DAIKIN_COUNT];
    daikin_build(s,state);
    uint16_t pos=0;
    for(uint8_t i=0;i<5;i++) daikin_bit(timings,&pos,false);
    timings[pos++]=DAIKIN_BIT_MARK; timings[pos++]=DAIKIN_ZERO+DAIKIN_GAP;
    daikin_section(timings,&pos,state,8);
    daikin_section(timings,&pos,state+8,8);
    daikin_section(timings,&pos,state+16,19);
    infrared_send_raw_ext(timings,pos,true,IR_FREQ,DAIKIN_DUTY);
}

/* ---------------- Panasonic RKR-style ---------------- */

#define PANA_DUTY 0.33f
#define PANA_HDR_MARK 3456U
#define PANA_HDR_SPACE 1728U
#define PANA_BIT_MARK 432U
#define PANA_ONE 1296U
#define PANA_ZERO 432U
#define PANA_GAP 10000U
#define PANA_MAX 460U

static void pana_append(uint32_t* t,size_t* n,uint32_t v){if(*n<PANA_MAX)t[(*n)++]=v;}
static void pana_byte(uint32_t* t,size_t* n,uint8_t v){
    for(uint8_t bit=0;bit<8;bit++){pana_append(t,n,PANA_BIT_MARK);pana_append(t,n,(v&(1U<<bit))?PANA_ONE:PANA_ZERO);}
}
static size_t pana_sections(const uint8_t* a,size_t an,const uint8_t* b,size_t bn,uint32_t* t){
    size_t n=0;
    pana_append(t,&n,PANA_HDR_MARK);
    pana_append(t,&n,PANA_HDR_SPACE);
    for(size_t i=0;i<an;i++) {
        pana_byte(t,&n,a[i]);
    }
    pana_append(t,&n,PANA_BIT_MARK);
    pana_append(t,&n,PANA_GAP);
    pana_append(t,&n,PANA_HDR_MARK);
    pana_append(t,&n,PANA_HDR_SPACE);
    for(size_t i=0;i<bn;i++) {
        pana_byte(t,&n,b[i]);
    }
    pana_append(t,&n,PANA_BIT_MARK);
    return n;
}
static uint8_t pana_checksum(const uint8_t* f,size_t s,size_t e){uint8_t v=0;for(size_t i=s;i<e;i++)v=(uint8_t)(v+f[i]);return v;}
static uint8_t pana_mode(const UniAcState* s){
    uint8_t v=0x08U;if(s->power)v|=0x01U;if(s->mode==UniAcModeCool)v|=0x30U;else if(s->mode==UniAcModeDry)v|=0x20U;return v;
}
static uint8_t pana_fan(uint8_t fan){
    if(fan==0||fan>=6)return 0xAU;
    return (uint8_t)(fan+2U);
}
static uint8_t pana_swing(uint8_t swing){return swing<5?(uint8_t)(swing+1U):0xFU;}
static void pana_build(const UniAcState* s,uint8_t out[27]){
    static const uint8_t fixed[27]={0x02,0x20,0xE0,0x04,0,0,0,0x06,0x02,0x20,0xE0,0x04,0,0x09,0x32,0x80,0xA2,0,0,0x0E,0xE0,0,0,0x89,0,0,0};
    memcpy(out,fixed,27);
    out[13]=pana_mode(s); out[14]=s->temp_x2;
    out[16]=(uint8_t)((pana_fan(s->fan)<<4)|pana_swing(s->swing_v));
    out[21]=0;
    if(s->turbo) out[21]|=0x01U;
    out[25]=0;
    if(s->eco) out[25]|=0x10U;
    if(s->nanoe) out[25]|=0x06U;
    out[26]=pana_checksum(out,8,26);
}
static void pana_send(const UniAcState* s){
    uint8_t frame[27];uint32_t timings[PANA_MAX];pana_build(s,frame);
    size_t n=pana_sections(frame,8,frame+8,19,timings);
    infrared_send_raw_ext(timings,n,true,IR_FREQ,PANA_DUTY);
}

/* ---------------- Carrier/Toshiba WC-UA4NE ---------------- */

#define TOSHIBA_DUTY 0.33f
#define T_HDR_MARK 4400U
#define T_HDR_SPACE 4395U
#define T_BIT 540U
#define T_ZERO 543U
#define T_ONE 1627U
#define T_GAP 6712U
#define T_MAX 327U

static uint8_t xor_sum(const uint8_t* d,uint8_t n){uint8_t v=0;for(uint8_t i=0;i<n;i++)v^=d[i];return v;}
static uint8_t carrier_mode(const UniAcState* s){
    if(s->mode==UniAcModeCool)return 1;
    if(s->mode==UniAcModeDry)return 2;
    if(s->mode==UniAcModeFan)return 4;
    return 0;
}
static uint8_t carrier_build(const UniAcState* s,uint8_t out[10]){
    const bool ext=s->power&&(s->turbo||s->eco);const uint8_t n=ext?10:9;memset(out,0,10);
    out[0]=0xF2;out[1]=0x0D;out[2]=(uint8_t)(n-6);out[3]=(uint8_t)~out[2];out[4]=ext?0x09:0x01;
    uint8_t temp=clamp_u8(s->temp_x2/2,17,30);out[5]=(uint8_t)((temp-17)<<4);
    uint8_t mode=s->power?carrier_mode(s):7;uint8_t fan=s->fan==0?0:clamp_u8((int)s->fan+1,2,6);
    out[6]=(uint8_t)((fan<<5)|mode);out[7]=0;if(ext)out[8]=s->turbo?1:3;out[n-1]=xor_sum(out,(uint8_t)(n-1));return n;
}
static void carrier_frame(const uint8_t* d,uint8_t n,uint32_t* t,uint16_t* p){
    t[(*p)++]=T_HDR_MARK;
    t[(*p)++]=T_HDR_SPACE;
    for(uint8_t i=0;i<n;i++) {
        for(uint8_t bit=0;bit<8;bit++) {
            bool one=(d[i]&(0x80U>>bit))!=0;
            t[(*p)++]=T_BIT;
            t[(*p)++]=one?T_ONE:T_ZERO;
        }
    }
    t[(*p)++]=T_BIT;
}
static void carrier_send_bytes(const uint8_t* d,uint8_t n,uint8_t frames){
    uint32_t t[T_MAX];uint16_t p=0;for(uint8_t f=0;f<frames;f++){if(f)t[p++]=T_GAP;carrier_frame(d,n,t,&p);}infrared_send_raw_ext(t,p,true,IR_FREQ,TOSHIBA_DUTY);
}
static void carrier_send(const UniAcState* s){uint8_t d[10];uint8_t n=carrier_build(s,d);carrier_send_bytes(d,n,2);}
static void carrier_swing(uint8_t val,uint8_t frames){uint8_t d[7]={0xF2,0x0D,0x01,0xFE,0x21,val,0};d[6]=xor_sum(d,6);carrier_send_bytes(d,7,frames);}

/* ---------------- Generic state mutation ---------------- */

static bool profile_supports_heat(const UniRemote* r){return strcmp(r->state_profile,"LG_AKB75215401")==0;}
static uint8_t mode_count(const UniRemote* r){return profile_supports_heat(r)?5U:4U;}

static bool mutate_common(const UniRemote* r, UniAcState* s, const char* action) {
    if(strcmp(action,"temp+")==0){s->temp_x2=(uint8_t)(s->temp_x2+2);return true;}
    if(strcmp(action,"temp-")==0){s->temp_x2=(uint8_t)(s->temp_x2-2);return true;}
    if(strcmp(action,"mode+")==0){s->mode=(UniAcMode)wrap_u8((int)s->mode+1,mode_count(r));return true;}
    if(strcmp(action,"mode-")==0){s->mode=(UniAcMode)wrap_u8((int)s->mode-1,mode_count(r));return true;}
    if(strcmp(action,"fan+")==0){s->fan=wrap_u8((int)s->fan+1,6);return true;}
    if(strcmp(action,"fan-")==0){s->fan=wrap_u8((int)s->fan-1,6);return true;}
    if(strcmp(action,"swing")==0){s->swing_v=s->swing_v?0:1;return true;}
    if(strcmp(action,"swing+")==0){
        const uint8_t count =
            strcmp(r->state_profile,"PANASONIC_RKR")==0 ? 6U : 2U;
        s->swing_v=wrap_u8((int)s->swing_v+1,count);
        return true;
    }
    if(strcmp(action,"eco")==0){
        if(strcmp(r->state_profile,"DAIKIN_ARC433A73")==0) return false;
        s->eco=!s->eco;if(s->eco)s->turbo=false;return true;
    }
    if(strcmp(action,"turbo")==0){s->turbo=!s->turbo;if(s->turbo)s->eco=false;return true;}
    if(strcmp(action,"nanoe")==0){
        if(strcmp(r->state_profile,"PANASONIC_RKR")!=0) return false;
        s->nanoe=!s->nanoe;return true;
    }
    return false;
}

static void normalize(const UniRemote* r, UniAcState* s) {
    int lo=18, hi=30;
    if(strcmp(r->state_profile,"DAIKIN_ARC433A73")==0) hi=32;
    if(strcmp(r->state_profile,"PANASONIC_RKR")==0){lo=16;hi=30;}
    if(strcmp(r->state_profile,"CARRIER_WC_UA4NE")==0){lo=17;hi=30;}
    s->temp_x2=clamp_u8(s->temp_x2,lo*2,hi*2);

    if(strcmp(r->state_profile,"DAIKIN_ARC433A73")==0) {
        if(s->mode==UniAcModeAuto||s->mode==UniAcModeHeat) s->mode=UniAcModeCool;
        if(s->mode==UniAcModeDry) s->fan=0;
    } else if(strcmp(r->state_profile,"PANASONIC_RKR")==0) {
        if(s->mode==UniAcModeFan||s->mode==UniAcModeHeat) s->mode=UniAcModeAuto;
        if(s->fan>6)s->fan=6;
        if(s->swing_v>5)s->swing_v=5;
    } else if(strcmp(r->state_profile,"CARRIER_WC_UA4NE")==0) {
        if(s->mode==UniAcModeHeat)s->mode=UniAcModeAuto;
        if(s->mode==UniAcModeDry)s->fan=0;
    }
}

static bool send_profile(const UniRemote* r, const UniAcState* s) {
    if(strcmp(r->state_profile,"LG_AKB75215401")==0) {
        if(!s->power) lg_send_code(LG_POWER_OFF); else lg_send_state(s,false);
        return true;
    }
    if(strcmp(r->state_profile,"DAIKIN_ARC433A73")==0) {daikin_send(s);return true;}
    if(strcmp(r->state_profile,"PANASONIC_RKR")==0) {pana_send(s);return true;}
    if(strcmp(r->state_profile,"CARRIER_WC_UA4NE")==0) {carrier_send(s);return true;}
    return false;
}

bool uni_stateful_ir_execute(
    Storage* storage,
    const UniRemote* remote,
    const char* binding,
    bool repeat) {
    if(!storage || !remote || !binding || strncmp(binding,"st:",3)!=0) return false;
    UNUSED(repeat);

    UniAcState s;
    if(!uni_stateful_ir_load_state(storage,remote,&s)) return false;
    const char* action=binding+3;

    bool handled=false;
    if(strcmp(remote->state_profile,"LG_AKB75215401")==0) {
        handled=lg_send_action(&s,action);
        if(handled){normalize(remote,&s);return save_state(storage,remote,&s);}
    }

    if(strcmp(remote->state_profile,"CARRIER_WC_UA4NE")==0) {
        if(strcmp(action,"fix")==0){carrier_swing(0,1);return true;}
        if(strcmp(action,"swing")==0){carrier_swing(4,2);return true;}
    }

    if(strcmp(action,"power")==0){s.power=!s.power;handled=true;}
    else handled=mutate_common(remote,&s,action);
    if(!handled) return false;

    normalize(remote,&s);
    if(!send_profile(remote,&s)) return false;
    return save_state(storage,remote,&s);
}

bool uni_stateful_ir_reset_state(Storage* storage, const UniRemote* remote) {
    if(!storage || !remote || !remote->state_path[0]) return false;
    const FS_Error status = storage_common_remove(storage, remote->state_path);
    return status == FSE_OK || status == FSE_NOT_EXIST;
}
