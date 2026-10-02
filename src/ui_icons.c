#include "ui_icons.h"

#include <stdbool.h>
#include <stddef.h>

static void px(Canvas* canvas, int16_t x, int16_t y, Color color) {
    if(x < 0 || x >= 64 || y < 0 || y >= 128) return;
    canvas_set_color(canvas, color);
    canvas_draw_dot(canvas, y, 63 - x);
}

static void hl(Canvas* canvas, int16_t x, int16_t y, int16_t w, Color color) {
    for(int16_t i = 0; i < w; i++) px(canvas, x + i, y, color);
}

static void vl(Canvas* canvas, int16_t x, int16_t y, int16_t h, Color color) {
    for(int16_t i = 0; i < h; i++) px(canvas, x, y + i, color);
}

static void fill(Canvas* canvas, int16_t x, int16_t y, int16_t w, int16_t h, Color color) {
    for(int16_t r = 0; r < h; r++) hl(canvas, x, y + r, w, color);
}

static void rect(Canvas* canvas, int16_t x, int16_t y, int16_t w, int16_t h, Color color) {
    if(w < 2 || h < 2) return;
    hl(canvas, x, y, w, color);
    hl(canvas, x, y + h - 1, w, color);
    vl(canvas, x, y, h, color);
    vl(canvas, x + w - 1, y, h, color);
}

static void plus(Canvas* canvas, int16_t cx, int16_t cy, Color color) {
    hl(canvas, cx - 2, cy, 5, color);
    vl(canvas, cx, cy - 2, 5, color);
}

static void minus(Canvas* canvas, int16_t cx, int16_t cy, Color color) {
    hl(canvas, cx - 2, cy, 5, color);
}

static void arrow(Canvas* canvas, int16_t cx, int16_t cy, int8_t dx, int8_t dy, Color color) {
    if(dx) {
        hl(canvas, cx - 5, cy, 11, color);
        for(int8_t i = 0; i < 5; i++) {
            int16_t xx = cx + dx * (5 - i);
            px(canvas, xx, cy - i, color);
            px(canvas, xx, cy + i, color);
        }
    } else {
        vl(canvas, cx, cy - 5, 11, color);
        for(int8_t i = 0; i < 5; i++) {
            int16_t yy = cy + dy * (5 - i);
            px(canvas, cx - i, yy, color);
            px(canvas, cx + i, yy, color);
        }
    }
}

static void ring(Canvas* canvas, int16_t cx, int16_t cy, Color color) {
    hl(canvas, cx - 3, cy - 6, 7, color);
    hl(canvas, cx - 3, cy + 6, 7, color);
    vl(canvas, cx - 6, cy - 3, 7, color);
    vl(canvas, cx + 6, cy - 3, 7, color);
    px(canvas, cx - 5, cy - 4, color); px(canvas, cx - 4, cy - 5, color);
    px(canvas, cx + 5, cy - 4, color); px(canvas, cx + 4, cy - 5, color);
    px(canvas, cx - 5, cy + 4, color); px(canvas, cx - 4, cy + 5, color);
    px(canvas, cx + 5, cy + 4, color); px(canvas, cx + 4, cy + 5, color);
}

static const uint8_t font3x5[64][5] = {
    [' ' - 32]={0,0,0,0,0}, ['+' - 32]={0,2,7,2,0}, ['-' - 32]={0,0,7,0,0},
    ['0' - 32]={7,5,5,5,7}, ['1' - 32]={2,6,2,2,7}, ['2' - 32]={6,1,2,4,7},
    ['3' - 32]={6,1,2,1,6}, ['4' - 32]={5,5,7,1,1}, ['5' - 32]={7,4,6,1,6},
    ['6' - 32]={3,4,6,5,2}, ['7' - 32]={7,1,2,2,2}, ['8' - 32]={2,5,2,5,2},
    ['9' - 32]={2,5,3,1,6}, ['A' - 32]={2,5,7,5,5}, ['B' - 32]={6,5,6,5,6},
    ['C' - 32]={3,4,4,4,3}, ['D' - 32]={6,5,5,5,6}, ['G' - 32]={3,4,5,5,3},
    ['K' - 32]={5,5,6,5,5}, ['O' - 32]={2,5,5,5,2}, ['R' - 32]={6,5,6,5,5},
    ['U' - 32]={5,5,5,5,7}, ['X' - 32]={5,5,2,5,5}, ['Y' - 32]={5,5,2,2,2},
};

static void glyph(Canvas* canvas, char ch, int16_t x, int16_t y, uint8_t scale, Color color) {
    if(ch < 32 || ch > '_') return;
    const uint8_t* rows=font3x5[(uint8_t)ch-32];
    for(uint8_t r=0;r<5;r++) for(uint8_t cc=0;cc<3;cc++) if(rows[r]&(1U<<(2-cc)))
        fill(canvas,x+cc*scale,y+r*scale,scale,scale,color);
}

static void speaker(Canvas* canvas, int16_t x, int16_t y, Color color) {
    fill(canvas,x,y+4,3,5,color);
    hl(canvas,x+3,y+3,2,color); hl(canvas,x+3,y+9,2,color);
    vl(canvas,x+5,y+4,5,color);
}

static void snow(Canvas* canvas,int16_t cx,int16_t cy,Color color) {
    hl(canvas,cx-6,cy,13,color); vl(canvas,cx,cy-6,13,color);
    for(int8_t d=-4;d<=4;d+=2){px(canvas,cx+d,cy+d,color);px(canvas,cx+d,cy-d,color);}
}

static void sun(Canvas* canvas,int16_t cx,int16_t cy,Color color) {
    rect(canvas,cx-2,cy-2,5,5,color);
    px(canvas,cx,cy-6,color);px(canvas,cx,cy+6,color);px(canvas,cx-6,cy,color);px(canvas,cx+6,cy,color);
    px(canvas,cx-4,cy-4,color);px(canvas,cx+4,cy-4,color);px(canvas,cx-4,cy+4,color);px(canvas,cx+4,cy+4,color);
}

static void fan(Canvas* canvas,int16_t cx,int16_t cy,Color color) {
    px(canvas,cx,cy,color);
    hl(canvas,cx-1,cy-5,3,color);vl(canvas,cx+5,cy-1,3,color);
    hl(canvas,cx-1,cy+5,3,color);vl(canvas,cx-5,cy-1,3,color);
    hl(canvas,cx+1,cy-4,3,color);vl(canvas,cx+4,cy+1,3,color);
    hl(canvas,cx-3,cy+4,3,color);vl(canvas,cx-4,cy-3,3,color);
}

static void lens(Canvas* canvas,int16_t cx,int16_t cy,bool add,Color color) {
    ring(canvas,cx-2,cy-2,color); hl(canvas,cx+3,cy+3,4,color); vl(canvas,cx+6,cy+3,4,color);
    if(add) plus(canvas,cx-2,cy-2,color); else minus(canvas,cx-2,cy-2,color);
}

static void focus_box(Canvas* canvas,int16_t cx,int16_t cy,bool add,Color color) {
    hl(canvas,cx-6,cy-6,4,color);vl(canvas,cx-6,cy-6,4,color);
    hl(canvas,cx+3,cy-6,4,color);vl(canvas,cx+6,cy-6,4,color);
    hl(canvas,cx-6,cy+6,4,color);vl(canvas,cx-6,cy+3,4,color);
    hl(canvas,cx+3,cy+6,4,color);vl(canvas,cx+6,cy+3,4,color);
    if(add) plus(canvas,cx,cy,color); else minus(canvas,cx,cy,color);
}

static const char* const names[UiIconCount]={
"POWER","MUTE","INPUT","HOME","MENU","BACK","SETTINGS","INFO","GUIDE","EXIT",
"UP","DOWN","LEFT","RIGHT","OK","VOL+","VOL-","CH+","CH-","PLAY","PAUSE","STOP","RECORD",
"REWIND","FAST FWD","PREVIOUS","NEXT","SUBTITLE","AUDIO","MIC","VOICE","SEARCH","APPS",
"KEYBOARD","MOUSE","PAIR","BLUETOOTH","0","1","2","3","4","5","6","7","8","9","DOT","DASH",
"RED","GREEN","YELLOW","BLUE","LIGHT","SLEEP","TIMER","FAN","TEMP+","TEMP-","MODE","AUTO",
"COOL","HEAT","DRY","SWING","ECO","TURBO","FREEZE","ZOOM+","ZOOM-","FOCUS+","FOCUS-",
"ASPECT","PROJECTOR","TV","GAMEPAD","A","B","X","Y","BLANK","REPEAT","SHUFFLE","FAVORITE",
"BRIGHT+","BRIGHT-","LAST CH","EJECT"};

uint8_t ui_icon_count(void){return (uint8_t)UiIconCount;}
const char* ui_icon_name(UiIcon icon){return icon<UiIconCount?names[icon]:"UNKNOWN";}

void ui_icon_draw(Canvas* canvas,UiIcon icon,int16_t x,int16_t y,Color color){
    const int16_t cx=x+9, cy=y+9;
    switch(icon){
    case UiIconPower: ring(canvas,cx,cy+1,color);vl(canvas,cx,cy-6,7,color);break;
    case UiIconMute: speaker(canvas,x+2,y+2,color);hl(canvas,x+11,y+5,6,color);for(int8_t i=0;i<6;i++)px(canvas,x+11+i,y+11-i,color);break;
    case UiIconInput: rect(canvas,x+3,y+4,10,11,color);arrow(canvas,x+13,cy,1,0,color);break;
    case UiIconHome: for(int8_t i=0;i<5;i++)hl(canvas,cx-i,y+8-i,1+2*i,color);vl(canvas,x+5,y+8,8,color);vl(canvas,x+13,y+8,8,color);break;
    case UiIconMenu: hl(canvas,x+3,y+4,13,color);hl(canvas,x+3,y+9,13,color);hl(canvas,x+3,y+14,13,color);break;
    case UiIconBack: arrow(canvas,cx,cy,-1,0,color);break;
    case UiIconSettings: ring(canvas,cx,cy,color);plus(canvas,cx,cy,color);px(canvas,cx,cy-7,color);px(canvas,cx,cy+7,color);px(canvas,cx-7,cy,color);px(canvas,cx+7,cy,color);break;
    case UiIconInfo: ring(canvas,cx,cy,color);px(canvas,cx,cy-4,color);vl(canvas,cx,cy-1,6,color);break;
    case UiIconGuide: rect(canvas,x+4,y+3,11,13,color);hl(canvas,x+6,y+6,7,color);hl(canvas,x+6,y+9,7,color);hl(canvas,x+6,y+12,7,color);break;
    case UiIconExit: rect(canvas,x+3,y+3,8,13,color);arrow(canvas,x+13,cy,1,0,color);break;
    case UiIconUp: arrow(canvas,cx,cy,0,-1,color);break;
    case UiIconDown: arrow(canvas,cx,cy,0,1,color);break;
    case UiIconLeft: arrow(canvas,cx,cy,-1,0,color);break;
    case UiIconRight: arrow(canvas,cx,cy,1,0,color);break;
    case UiIconOk: ring(canvas,cx,cy,color);glyph(canvas,'O',x+5,y+7,1,color);glyph(canvas,'K',x+10,y+7,1,color);break;
    case UiIconVolUp: speaker(canvas,x+2,y+2,color);plus(canvas,x+14,cy,color);break;
    case UiIconVolDown: speaker(canvas,x+2,y+2,color);minus(canvas,x+14,cy,color);break;
    case UiIconChUp: arrow(canvas,cx,cy,0,-1,color);plus(canvas,x+14,y+14,color);break;
    case UiIconChDown: arrow(canvas,cx,cy,0,1,color);minus(canvas,x+14,y+14,color);break;
    case UiIconPlay: for(int8_t i=0;i<7;i++)vl(canvas,x+5+i,cy-i,1+2*i,color);break;
    case UiIconPause: fill(canvas,x+5,y+4,3,11,color);fill(canvas,x+11,y+4,3,11,color);break;
    case UiIconStop: fill(canvas,x+5,y+5,9,9,color);break;
    case UiIconRecord: ring(canvas,cx,cy,color);fill(canvas,cx-2,cy-2,5,5,color);break;
    case UiIconRewind: arrow(canvas,x+7,cy,-1,0,color);arrow(canvas,x+12,cy,-1,0,color);break;
    case UiIconFastForward: arrow(canvas,x+7,cy,1,0,color);arrow(canvas,x+12,cy,1,0,color);break;
    case UiIconPrevious: vl(canvas,x+4,y+4,11,color);arrow(canvas,x+11,cy,-1,0,color);break;
    case UiIconNext: vl(canvas,x+14,y+4,11,color);arrow(canvas,x+8,cy,1,0,color);break;
    case UiIconSubtitle: rect(canvas,x+2,y+5,15,9,color);glyph(canvas,'C',x+4,y+7,1,color);glyph(canvas,'C',x+11,y+7,1,color);break;
    case UiIconAudio: glyph(canvas,'A',x+3,y+6,1,color);glyph(canvas,'U',x+8,y+6,1,color);glyph(canvas,'D',x+13,y+6,1,color);break;
    case UiIconMic: rect(canvas,cx-2,y+3,5,8,color);vl(canvas,cx-5,y+7,5,color);vl(canvas,cx+5,y+7,5,color);hl(canvas,cx-5,y+11,11,color);vl(canvas,cx,y+11,4,color);hl(canvas,cx-3,y+15,7,color);break;
    case UiIconVoice: speaker(canvas,x+2,y+2,color);px(canvas,x+11,y+6,color);px(canvas,x+13,y+5,color);px(canvas,x+15,y+4,color);px(canvas,x+11,y+12,color);px(canvas,x+13,y+13,color);px(canvas,x+15,y+14,color);break;
    case UiIconSearch: ring(canvas,cx-2,cy-2,color);hl(canvas,cx+3,cy+3,4,color);vl(canvas,cx+6,cy+3,4,color);break;
    case UiIconApps: for(int8_t r=0;r<3;r++)for(int8_t cc=0;cc<3;cc++)fill(canvas,x+3+cc*5,y+3+r*5,3,3,color);break;
    case UiIconKeyboard: rect(canvas,x+2,y+5,15,9,color);for(int8_t r=0;r<2;r++)for(int8_t cc=0;cc<5;cc++)px(canvas,x+4+cc*2,y+7+r*3,color);hl(canvas,x+5,y+12,9,color);break;
    case UiIconMouse: rect(canvas,x+5,y+2,9,15,color);vl(canvas,cx,y+2,5,color);px(canvas,cx-2,y+5,color);px(canvas,cx+2,y+5,color);break;
    case UiIconPair: ring(canvas,x+6,cy,color);ring(canvas,x+13,cy,color);hl(canvas,x+8,cy,4,color);break;
    case UiIconBluetooth:
        vl(canvas,cx,y+2,15,color);
        hl(canvas,cx,y+2,2,color); px(canvas,cx+2,y+3,color); px(canvas,cx+3,y+4,color);
        px(canvas,cx+2,y+5,color); px(canvas,cx+1,y+6,color);
        hl(canvas,cx,y+16,2,color); px(canvas,cx+2,y+15,color); px(canvas,cx+3,y+14,color);
        px(canvas,cx+2,y+13,color); px(canvas,cx+1,y+12,color);
        for(int8_t i=0;i<5;i++){px(canvas,cx-i,y+5+i,color);px(canvas,cx-i,y+13-i,color);}
        break;
    case UiIconNum0: case UiIconNum1: case UiIconNum2: case UiIconNum3: case UiIconNum4:
    case UiIconNum5: case UiIconNum6: case UiIconNum7: case UiIconNum8: case UiIconNum9:
        glyph(canvas,(char)('0'+(icon-UiIconNum0)),x+6,y+4,2,color);break;
    case UiIconDot: fill(canvas,cx-1,cy-1,3,3,color);break;
    case UiIconDash: hl(canvas,x+5,cy,9,color);break;
    case UiIconRed: glyph(canvas,'R',x+6,y+4,2,color);break;
    case UiIconGreen: glyph(canvas,'G',x+6,y+4,2,color);break;
    case UiIconYellow: glyph(canvas,'Y',x+6,y+4,2,color);break;
    case UiIconBlue: glyph(canvas,'B',x+6,y+4,2,color);break;
    case UiIconLight: ring(canvas,cx,y+7,color);hl(canvas,cx-3,y+14,7,color);hl(canvas,cx-2,y+16,5,color);break;
    case UiIconSleep:
        px(canvas,x+11,y+3,color);px(canvas,x+9,y+4,color);px(canvas,x+7,y+5,color);
        px(canvas,x+6,y+7,color);px(canvas,x+6,y+9,color);px(canvas,x+7,y+11,color);
        px(canvas,x+9,y+13,color);px(canvas,x+11,y+14,color);px(canvas,x+13,y+14,color);
        px(canvas,x+15,y+13,color);px(canvas,x+13,y+12,color);px(canvas,x+12,y+11,color);
        px(canvas,x+11,y+10,color);px(canvas,x+10,y+8,color);px(canvas,x+10,y+6,color);
        px(canvas,x+11,y+5,color);
        break;
    case UiIconTimer: ring(canvas,cx,cy+1,color);hl(canvas,cx-2,y+2,5,color);vl(canvas,cx,cy-4,5,color);hl(canvas,cx,cy,4,color);break;
    case UiIconFan: fan(canvas,cx,cy,color);break;
    case UiIconTempUp: vl(canvas,x+6,y+3,11,color);ring(canvas,x+6,y+14,color);arrow(canvas,x+13,cy,0,-1,color);break;
    case UiIconTempDown: vl(canvas,x+6,y+3,11,color);ring(canvas,x+6,y+14,color);arrow(canvas,x+13,cy,0,1,color);break;
    case UiIconMode: ring(canvas,cx,cy,color);arrow(canvas,cx+2,y+4,1,0,color);arrow(canvas,cx-2,y+14,-1,0,color);break;
    case UiIconAuto: ring(canvas,cx,cy,color);glyph(canvas,'A',x+6,y+7,1,color);break;
    case UiIconCool: snow(canvas,cx,cy,color);break;
    case UiIconHeat: sun(canvas,cx,cy,color);break;
    case UiIconDry: for(int8_t r=0;r<12;r++){int8_t w=(r<6)?1+r:11-r;hl(canvas,cx-w/2,y+3+r,w,color);}break;
    case UiIconSwing: arrow(canvas,cx,y+5,1,0,color);arrow(canvas,cx,y+13,-1,0,color);vl(canvas,cx,y+6,7,color);break;
    case UiIconEco: for(int8_t r=0;r<11;r++){int8_t w=(r<6)?r+2:12-r;hl(canvas,cx-w/2,y+4+r,w,color);}hl(canvas,x+5,y+14,9,color);break;
    case UiIconTurbo: fan(canvas,cx,cy,color);ring(canvas,cx,cy,color);break;
    case UiIconFreeze: snow(canvas,cx,cy,color);rect(canvas,x+2,y+2,15,15,color);break;
    case UiIconZoomIn: lens(canvas,cx,cy,true,color);break;
    case UiIconZoomOut: lens(canvas,cx,cy,false,color);break;
    case UiIconFocusIn: focus_box(canvas,cx,cy,true,color);break;
    case UiIconFocusOut: focus_box(canvas,cx,cy,false,color);break;
    case UiIconAspect: rect(canvas,x+2,y+4,15,11,color);rect(canvas,x+5,y+6,9,7,color);break;
    case UiIconProjector: rect(canvas,x+2,y+5,15,9,color);ring(canvas,x+12,cy,color);px(canvas,x+4,y+14,color);px(canvas,x+14,y+14,color);break;
    case UiIconTv: rect(canvas,x+2,y+3,15,11,color);hl(canvas,x+5,y+16,9,color);vl(canvas,cx,y+14,3,color);break;
    case UiIconGamepad: rect(canvas,x+2,y+6,15,8,color);plus(canvas,x+6,cy,color);px(canvas,x+13,y+8,color);px(canvas,x+15,y+10,color);break;
    case UiIconA: case UiIconB: case UiIconX: case UiIconY: ring(canvas,cx,cy,color);glyph(canvas,icon==UiIconA?'A':icon==UiIconB?'B':icon==UiIconX?'X':'Y',x+8,y+7,1,color);break;
    case UiIconBlank: rect(canvas,x+3,y+3,13,13,color);break;
    case UiIconRepeat: ring(canvas,cx,cy,color);arrow(canvas,cx+3,y+4,1,0,color);break;
    case UiIconShuffle: arrow(canvas,x+13,y+5,1,0,color);arrow(canvas,x+13,y+13,1,0,color);hl(canvas,x+3,y+5,4,color);hl(canvas,x+3,y+13,4,color);for(int8_t i=0;i<6;i++){px(canvas,x+7+i,y+5+i,color);px(canvas,x+7+i,y+13-i,color);}break;
    case UiIconFavorite: for(int8_t r=0;r<7;r++){int8_t w=(r<3)?5+r*2:13-(r-3)*2;hl(canvas,cx-w/2,y+4+r,w,color);}vl(canvas,cx,y+11,4,color);break;
    case UiIconBrightnessUp: sun(canvas,cx,cy,color);plus(canvas,x+15,y+15,color);break;
    case UiIconBrightnessDown: sun(canvas,cx,cy,color);minus(canvas,x+15,y+15,color);break;
    case UiIconLastChannel: arrow(canvas,cx,cy,-1,0,color);glyph(canvas,'C',x+12,y+12,1,color);break;
    case UiIconEject: for(int8_t i=0;i<7;i++)hl(canvas,cx-i,y+4+i,1+2*i,color);hl(canvas,x+3,y+14,13,color);break;
    default: rect(canvas,x+3,y+3,13,13,color);break;
    }
}
