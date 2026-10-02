#include "stateful_ir.h"

#include <furi.h>
#include <furi_hal_rtc.h>
#include <infrared/worker/infrared_transmit.h>
#include <toolbox/saved_struct.h>
#include <stdio.h>
#include <string.h>

#define TAG "UniStateIr"
#define STATE_MAGIC 0x55U
#define STATE_VERSION 1U
#define IR_FREQ 38000U

#define F_POWERFUL 0x01U
#define F_ECO      0x02U
#define F_AUX      0x04U

static UniStateProfile parse_profile(const char* p) {
    if(!p) return UniStateProfileNone;
    if(strcmp(p,"lg_ac")==0) return UniStateProfileLgAc;
    if(strcmp(p,"daikin_arc433a73")==0) return UniStateProfileDaikinAc;
    if(strcmp(p,"carrier_wc_ua4ne")==0) return UniStateProfileCarrierAc;
    if(strcmp(p,"panasonic_rkr")==0) return UniStateProfilePanasonicAc;
    return UniStateProfileNone;
}

static void default_state(UniStatefulEngine* e) {
    memset(&e->state,0,sizeof(e->state));
    e->state.profile=(uint8_t)e->profile;
    switch(e->profile) {
    case UniStateProfileLgAc:
        e->state.mode=0; e->state.temp_half=46; e->state.fan=0;
        e->state.swing_v=0; e->state.swing_h=0; break;
    case UniStateProfileDaikinAc:
        e->state.mode=0; e->state.temp_half=46; e->state.fan=0;
        e->state.swing_v=0; break;
    case UniStateProfileCarrierAc:
        e->state.mode=0; e->state.temp_half=46; e->state.fan=0; break;
    case UniStateProfilePanasonicAc:
        e->state.mode=0; e->state.temp_half=50; e->state.fan=6;
        e->state.swing_v=1; break;
    default: break;
    }
}

static void make_state_path(UniStatefulEngine* e,const UniRemote* r) {
    snprintf(e->state_path,sizeof(e->state_path),"%s",r->config_path);
    char* slash=strrchr(e->state_path,'/');
    if(slash) snprintf(slash+1,(size_t)(e->state_path+sizeof(e->state_path)-(slash+1)),"state.bin");
}

static bool state_valid(const UniStatefulEngine* e,const UniStateData* s) {
    if(!s || s->profile!=(uint8_t)e->profile) return false;
    switch(e->profile) {
    case UniStateProfileLgAc:
        return s->mode<=4 && s->temp_half>=36 && s->temp_half<=60 && s->fan<=5 &&
               s->swing_v<=7 && s->swing_h<=8;
    case UniStateProfileDaikinAc:
        return s->mode<=2 && s->temp_half>=36 && s->temp_half<=64 && s->fan<=5 &&
               s->swing_v<=1;
    case UniStateProfileCarrierAc:
        return s->mode<=3 && s->temp_half>=34 && s->temp_half<=60 && s->fan<=5;
    case UniStateProfilePanasonicAc:
        return s->mode<=2 && s->temp_half>=32 && s->temp_half<=60 && s->fan<=6 &&
               s->swing_v<=5 && s->option<=2 && s->sleep<=10;
    default: return false;
    }
}

static void save_state(UniStatefulEngine* e) {
    if(!e->opened) return;
    if(!saved_struct_save(e->state_path,&e->state,sizeof(e->state),STATE_MAGIC,STATE_VERSION))
        FURI_LOG_W(TAG,"state save failed");
}

void uni_stateful_init(UniStatefulEngine* e,Storage* storage) {
    memset(e,0,sizeof(*e)); e->storage=storage;
}

bool uni_stateful_open(UniStatefulEngine* e,const UniRemote* r) {
    if(!e||!r||!e->storage) return false;
    e->profile=parse_profile(r->state_profile);
    if(e->profile==UniStateProfileNone) return false;
    make_state_path(e,r);
    default_state(e);
    UniStateData saved={0};
    if(saved_struct_load(e->state_path,&saved,sizeof(saved),STATE_MAGIC,STATE_VERSION) &&
       state_valid(e,&saved)) e->state=saved;
    e->opened=true;
    return true;
}

static void raw_send(const uint32_t* t,uint16_t n,float duty) {
    infrared_send_raw_ext(t,n,true,IR_FREQ,duty);
}

/* ---------- LG ---------- */
#define LG_N 59U
static uint8_t lg_checksum16(uint16_t c){
    return (uint8_t)(((c>>0)&0xF)+((c>>4)&0xF)+((c>>8)&0xF)+((c>>12)&0xF))&0xF;
}
static uint32_t lg_raw(uint16_t c){return (0x88UL<<20)|((uint32_t)c<<4)|lg_checksum16(c);}
static uint8_t lg_fan(uint8_t f){static const uint8_t x[6]={0x5,0x0,0x9,0x2,0xA,0x4};return x[f<=5?f:0];}
static uint16_t lg_state_cmd(const UniStateData* s,bool onframe){
    uint16_t c=0; uint8_t tc=(uint8_t)(s->temp_half/2);
    if(s->mode==1)c=0x0990U;
    else if(s->mode==2)c=(uint16_t)(0x0A00U|(3U<<4)|lg_fan(s->fan));
    else if(s->mode==3)c=(uint16_t)(0x0B00U|((uint16_t)(s->bias+2)<<4)|0x5U);
    else if(s->mode==4)c=(uint16_t)(0x0C00U|((uint16_t)(tc-15U)<<4)|lg_fan(s->fan));
    else c=(uint16_t)(0x0800U|((uint16_t)(tc-15U)<<4)|lg_fan(s->fan));
    if(onframe)c&=(uint16_t)~0x0800U;
    return c;
}
static void lg_send_code(uint32_t code){
    uint32_t t[LG_N]; uint8_t i=0;t[i++]=3200;t[i++]=9900;
    for(int8_t b=27;b>=0;b--){t[i++]=480;t[i++]=(code&(1UL<<b))?1600:550;}
    t[i++]=480; raw_send(t,LG_N,0.33f);
}
static void lg_send_state(const UniStateData* s,bool on){lg_send_code(lg_raw(lg_state_cmd(s,on)));}
static const uint32_t lg_sv[8]={0x881315AUL,0x8813048UL,0x8813059UL,0x881306AUL,0x881307BUL,0x881308CUL,0x881309DUL,0x8813149UL};
static const uint32_t lg_sh[9]={0x881317CUL,0x88130BFUL,0x88130C0UL,0x88130D1UL,0x88130E2UL,0x88130F3UL,0x8813105UL,0x8813116UL,0x881316BUL};

/* ---------- Daikin ARC433A73 ---------- */
#define DK_LEN 35U
#define DK_N 584U
static uint8_t sum8(const uint8_t* d,uint8_t n){uint8_t s=0;for(uint8_t i=0;i<n;i++)s=(uint8_t)(s+d[i]);return s;}
static uint8_t dk_mode(uint8_t m){return m==1?0x2U:(m==2?0x6U:0x3U);}
static uint8_t dk_fan(uint8_t f){static const uint8_t a[6]={0xA,0x3,0x4,0x5,0x6,0x7};return a[f<=5?f:0];}
static void dk_state(const UniStateData* s,uint8_t o[DK_LEN]){
    memset(o,0,DK_LEN);o[0]=0x11;o[1]=0xDA;o[2]=0x27;o[4]=0xC5;
    o[8]=0x11;o[9]=0xDA;o[10]=0x27;o[12]=0x42;
    o[16]=0x11;o[17]=0xDA;o[18]=0x27;o[21]=0x08;o[31]=0xC0;
    DateTime dt;furi_hal_rtc_get_datetime(&dt);uint16_t now=(uint16_t)(dt.hour*60U+dt.minute);
    o[13]=(uint8_t)now;o[14]=(uint8_t)((now>>8)&0x07U);
    if(s->power)o[21]|=0x01U;o[21]|=(uint8_t)(dk_mode(s->mode)<<4);
    o[22]=s->mode==0?s->temp_half:(s->mode==1?0xC0U:50U);
    uint8_t fn=s->mode==1?0xAU:dk_fan(s->fan);
    o[24]=(uint8_t)((fn<<4)|(s->swing_v?0x0FU:0));
    o[26]=0;o[27]=0x60;o[28]=0x60;
    if((s->flags&F_POWERFUL)&&s->power)o[29]|=0x01U;
    o[7]=sum8(o,7);o[15]=sum8(o+8,7);o[34]=sum8(o+16,18);
}
static void dk_bit(uint32_t* t,uint16_t* p,bool one){t[(*p)++]=428;t[(*p)++]=one?1280:428;}
static void dk_section(uint32_t* t,uint16_t* p,const uint8_t* d,uint8_t n){
    t[(*p)++]=3650;t[(*p)++]=1623;
    for(uint8_t i=0;i<n;i++)for(uint8_t b=0;b<8;b++)dk_bit(t,p,(d[i]&(1U<<b))!=0);
    t[(*p)++]=428;t[(*p)++]=29428;
}
static void dk_send(const UniStateData* s){
    uint8_t d[DK_LEN];uint32_t t[DK_N];uint16_t p=0;dk_state(s,d);
    for(uint8_t i=0;i<5;i++)dk_bit(t,&p,false);t[p++]=428;t[p++]=29428;
    dk_section(t,&p,d,8);dk_section(t,&p,d+8,8);dk_section(t,&p,d+16,19);
    furi_check(p==DK_N);raw_send(t,DK_N,0.50f);
}

/* ---------- Carrier/Toshiba ---------- */
#define CR_MAX 327U
static uint8_t xor8(const uint8_t* d,uint8_t n){uint8_t v=0;for(uint8_t i=0;i<n;i++)v^=d[i];return v;}
static uint8_t cr_mode(uint8_t m){return m==1?1U:(m==2?2U:(m==3?4U:0U));}
static uint8_t cr_fan(uint8_t f){return f==0?0U:(uint8_t)(f+1U);}
static uint8_t cr_msg(const UniStateData* s,uint8_t o[10]){
    bool ext=s->power&&((s->flags&F_POWERFUL)||(s->flags&F_ECO));uint8_t n=ext?10:9;
    memset(o,0,10);o[0]=0xF2;o[1]=0x0D;o[2]=(uint8_t)(n-6);o[3]=(uint8_t)~o[2];
    o[4]=ext?0x09:0x01;o[5]=(uint8_t)(((s->temp_half/2)-17U)<<4);
    o[6]=(uint8_t)((cr_fan(s->fan)<<5)|(s->power?cr_mode(s->mode):0x07U));
    if(ext)o[8]=(s->flags&F_POWERFUL)?0x01U:0x03U;
    o[n-1]=xor8(o,(uint8_t)(n-1));return n;
}
static void cr_append(uint32_t* t,uint16_t* p,const uint8_t* d,uint8_t n){
    t[(*p)++]=4400;t[(*p)++]=4395;
    for(uint8_t i=0;i<n;i++)for(uint8_t b=0;b<8;b++){bool one=(d[i]&(0x80U>>b))!=0;t[(*p)++]=540;t[(*p)++]=one?1627:543;}
    t[(*p)++]=540;
}
static void cr_send_bytes(const uint8_t* d,uint8_t n,uint8_t frames){
    uint32_t t[CR_MAX];uint16_t p=0;for(uint8_t f=0;f<frames;f++){if(f)t[p++]=6712;cr_append(t,&p,d,n);}raw_send(t,p,0.33f);
}
static void cr_send_state(const UniStateData* s){uint8_t d[10];uint8_t n=cr_msg(s,d);cr_send_bytes(d,n,2);}
static void cr_swing(uint8_t val,uint8_t frames){uint8_t d[7]={0xF2,0x0D,0x01,0xFE,0x21,val,0};d[6]=xor8(d,6);cr_send_bytes(d,7,frames);}

/* ---------- Panasonic RKR ---------- */
#define PN_MAX 460U
static uint8_t pn_fan(uint8_t f){if(f>=1&&f<=5)return (uint8_t)(f+2);return 0xA;}
static uint8_t pn_swing(uint8_t s){return s<=4?(uint8_t)(s+1):0xF;}
static uint8_t pn_sum(const uint8_t* d,size_t a,size_t b){uint8_t v=0;for(size_t i=a;i<b;i++)v=(uint8_t)(v+d[i]);return v;}
static void pn_frame(const UniStateData* s,uint8_t o[27]){
    static const uint8_t fixed[27]={0x02,0x20,0xE0,0x04,0,0,0,0x06,0x02,0x20,0xE0,0x04,0,0x09,0x32,0x80,0xA2,0,0,0x0E,0xE0,0,0,0x89,0,0,0};
    memcpy(o,fixed,27);uint8_t mb=0x08;if(s->power)mb|=1;if(s->mode==1)mb|=0x30;else if(s->mode==2)mb|=0x20;
    o[13]=mb;o[14]=s->temp_half;o[16]=(uint8_t)((pn_fan(s->fan)<<4)|pn_swing(s->swing_v));
    if(s->fan==0)o[21]|=0x20;else if(s->option==1)o[21]|=1;
    if(s->option==2)o[25]|=0x10;if(s->flags&F_AUX)o[25]|=0x06;
    o[26]=pn_sum(o,8,26);
}
static void pn_append_byte(uint32_t* t,size_t* p,uint8_t v){
    for(uint8_t b=0;b<8;b++){t[(*p)++]=432;t[(*p)++]=(v&(1U<<b))?1296:432;}
}
static size_t pn_sections(const uint8_t* a,size_t an,const uint8_t* b,size_t bn,uint32_t* t){
    size_t p=0;t[p++]=3456;t[p++]=1728;for(size_t i=0;i<an;i++)pn_append_byte(t,&p,a[i]);t[p++]=432;t[p++]=10000;
    t[p++]=3456;t[p++]=1728;for(size_t i=0;i<bn;i++)pn_append_byte(t,&p,b[i]);t[p++]=432;return p;
}
static void pn_send(const UniStateData* s){uint8_t d[27];uint32_t t[PN_MAX];pn_frame(s,d);size_t n=pn_sections(d,8,d+8,19,t);raw_send(t,(uint16_t)n,0.33f);}

static int wrap(int v,int n){while(v<0)v+=n;while(v>=n)v-=n;return v;}
static int clampi(int v,int lo,int hi){return v<lo?lo:(v>hi?hi:v);}

static bool execute_lg(UniStatefulEngine* e,const char* a){
    UniStateData* s=&e->state;
    if(strcmp(a,"power")==0){if(s->power){lg_send_code(0x88C0051UL);s->power=0;}else{s->power=1;lg_send_state(s,true);}return true;}
    if(strcmp(a,"mode+")==0||strcmp(a,"mode-")==0){s->mode=(uint8_t)wrap((int)s->mode+(a[4]=='+'?1:-1),5);if(s->power)lg_send_state(s,false);return true;}
    if(strcmp(a,"temp+")==0||strcmp(a,"temp-")==0){int t=(int)s->temp_half+(a[4]=='+'?2:-2);s->temp_half=(uint8_t)clampi(t,36,60);if(s->power)lg_send_state(s,false);return true;}
    if(strcmp(a,"fan+")==0||strcmp(a,"fan-")==0){s->fan=(uint8_t)wrap((int)s->fan+(a[3]=='+'?1:-1),6);if(s->power)lg_send_state(s,false);return true;}
    if(strcmp(a,"swing_v+")==0||strcmp(a,"swing_v-")==0){s->swing_v=(uint8_t)wrap((int)s->swing_v+(a[7]=='+'?1:-1),8);if(s->power)lg_send_code(lg_sv[s->swing_v]);return true;}
    if(strcmp(a,"swing_h+")==0||strcmp(a,"swing_h-")==0){s->swing_h=(uint8_t)wrap((int)s->swing_h+(a[7]=='+'?1:-1),9);if(s->power)lg_send_code(lg_sh[s->swing_h]);return true;}
    if(strcmp(a,"jet")==0){s->flags^=F_POWERFUL;if(s->power){if(s->flags&F_POWERFUL)lg_send_code(0x8810089UL);else lg_send_state(s,false);}return true;}
    if(strcmp(a,"eco")==0){s->flags^=F_ECO;if(s->power)lg_send_code((s->flags&F_ECO)?0x8810045UL:0x8810056UL);return true;}
    if(strcmp(a,"light")==0){lg_send_code(0x88C00A6UL);return true;}
    if(strcmp(a,"resend")==0){lg_send_state(s,false);return true;}
    return false;
}

static bool execute_daikin(UniStatefulEngine* e,const char* a){
    UniStateData* s=&e->state;
    bool changed=true;
    if(strcmp(a,"power")==0){s->power=!s->power;if(!s->power)s->flags&=(uint8_t)~F_POWERFUL;}
    else if(strcmp(a,"mode+")==0||strcmp(a,"mode-")==0)s->mode=(uint8_t)wrap((int)s->mode+(a[4]=='+'?1:-1),3);
    else if(strcmp(a,"temp+")==0||strcmp(a,"temp-")==0)s->temp_half=(uint8_t)clampi((int)s->temp_half+(a[4]=='+'?2:-2),36,64);
    else if(strcmp(a,"fan+")==0||strcmp(a,"fan-")==0)s->fan=(uint8_t)wrap((int)s->fan+(a[3]=='+'?1:-1),6);
    else if(strcmp(a,"swing")==0)s->swing_v=!s->swing_v;
    else if(strcmp(a,"powerful")==0){if(s->power)s->flags^=F_POWERFUL;else changed=false;}
    else if(strcmp(a,"resend")==0){}
    else changed=false;
    if(changed)dk_send(s);return changed;
}

static bool execute_carrier(UniStatefulEngine* e,const char* a){
    UniStateData* s=&e->state;
    if(strcmp(a,"power")==0){s->power=!s->power;if(!s->power)s->flags=0;cr_send_state(s);return true;}
    if(strcmp(a,"mode+")==0||strcmp(a,"mode-")==0){s->mode=(uint8_t)wrap((int)s->mode+(a[4]=='+'?1:-1),4);s->flags&=(uint8_t)~(F_POWERFUL|F_ECO);if(s->power)cr_send_state(s);return true;}
    if(strcmp(a,"temp+")==0||strcmp(a,"temp-")==0){s->temp_half=(uint8_t)clampi((int)s->temp_half+(a[4]=='+'?2:-2),34,60);if(s->power)cr_send_state(s);return true;}
    if(strcmp(a,"fan+")==0||strcmp(a,"fan-")==0){if(s->mode!=2)s->fan=(uint8_t)wrap((int)s->fan+(a[3]=='+'?1:-1),6);if(s->power)cr_send_state(s);return true;}
    if(strcmp(a,"fix")==0){cr_swing(0,1);return true;}
    if(strcmp(a,"swing")==0){cr_swing(4,2);return true;}
    if(strcmp(a,"powerful")==0){if(!s->power)return true;s->flags^=F_POWERFUL;if(s->flags&F_POWERFUL)s->flags&=(uint8_t)~F_ECO;cr_send_state(s);return true;}
    if(strcmp(a,"eco")==0){if(!s->power)return true;s->flags^=F_ECO;if(s->flags&F_ECO)s->flags&=(uint8_t)~F_POWERFUL;cr_send_state(s);return true;}
    if(strcmp(a,"resend")==0){cr_send_state(s);return true;}
    return false;
}

static bool execute_panasonic(UniStatefulEngine* e,const char* a){
    UniStateData* s=&e->state;
    bool send=true;
    if(strcmp(a,"power")==0)s->power=!s->power;
    else if(strcmp(a,"mode+")==0||strcmp(a,"mode-")==0)s->mode=(uint8_t)wrap((int)s->mode+(a[4]=='+'?1:-1),3);
    else if(strcmp(a,"temp+")==0||strcmp(a,"temp-")==0)s->temp_half=(uint8_t)clampi((int)s->temp_half+(a[4]=='+'?1:-1),32,60);
    else if(strcmp(a,"fan+")==0||strcmp(a,"fan-")==0)s->fan=(uint8_t)wrap((int)s->fan+(a[3]=='+'?1:-1),7);
    else if(strcmp(a,"swing+")==0||strcmp(a,"swing-")==0)s->swing_v=(uint8_t)wrap((int)s->swing_v+(a[5]=='+'?1:-1),6);
    else if(strcmp(a,"nanoe")==0)s->flags^=F_AUX;
    else if(strcmp(a,"extra+")==0||strcmp(a,"extra-")==0)s->option=(uint8_t)wrap((int)s->option+(a[5]=='+'?1:-1),3);
    else if(strcmp(a,"resend")==0){}
    else send=false;
    if(send)pn_send(s);return send;
}

bool uni_stateful_execute(UniStatefulEngine* e,const UniRemote* r,const char* binding,bool repeat){
    UNUSED(r);if(!e||!e->opened||!binding||repeat)return false;
    const char* a=strncmp(binding,"state:",6)==0?binding+6:binding;
    bool ok=false;
    switch(e->profile){
    case UniStateProfileLgAc:ok=execute_lg(e,a);break;
    case UniStateProfileDaikinAc:ok=execute_daikin(e,a);break;
    case UniStateProfileCarrierAc:ok=execute_carrier(e,a);break;
    case UniStateProfilePanasonicAc:ok=execute_panasonic(e,a);break;
    default:break;
    }
    if(ok)save_state(e);return ok;
}

void uni_stateful_format_status(const UniStatefulEngine* e,char* out,size_t n){
    if(!out||n==0){return;}out[0]='\0';if(!e||!e->opened)return;
    const UniStateData* s=&e->state;
    const unsigned whole=s->temp_half/2U;const unsigned half=(s->temp_half&1U)?5U:0U;
    if(e->profile==UniStateProfilePanasonicAc)
        snprintf(out,n,"%s %u.%uC F%u",s->power?"ON":"OFF",whole,half,(unsigned)s->fan);
    else snprintf(out,n,"%s %uC F%u",s->power?"ON":"OFF",whole,(unsigned)s->fan);
}
