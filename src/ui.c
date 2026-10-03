#include "ui.h"

#include "editor_model.h"
#include "icon_library.h"
#include "layout_library.h"

#include <furi.h>
#include <gui/canvas.h>
#include <stdio.h>
#include <string.h>

static const uint8_t grid_x[] = {0, 21, 42, 64};
static const uint8_t grid_y[] = {0, 21, 42, 64, 85, 106, 128};

static void pset(Canvas* canvas, int16_t x, int16_t y, Color color) {
    if(x < 0 || x >= 64 || y < 0 || y >= 128) return;
    canvas_set_color(canvas, color);
    canvas_draw_dot(canvas, y, 63 - x);
}

static void hline(Canvas* canvas, int16_t x, int16_t y, int16_t w, Color color) {
    for(int16_t i = 0; i < w; i++) pset(canvas, x + i, y, color);
}

static void vline(Canvas* canvas, int16_t x, int16_t y, int16_t h, Color color) {
    for(int16_t i = 0; i < h; i++) pset(canvas, x, y + i, color);
}

static void fill_rect(Canvas* canvas, int16_t x, int16_t y, int16_t w, int16_t h, Color color) {
    for(int16_t j = 0; j < h; j++) hline(canvas, x, y + j, w, color);
}

static void frame(Canvas* canvas, int16_t x, int16_t y, int16_t w, int16_t h, bool active) {
    if(w < 3 || h < 3) return;
    if(active) fill_rect(canvas, x, y, w, h, ColorBlack);
    hline(canvas, x + 1, y, w - 2, ColorBlack);
    hline(canvas, x + 1, y + h - 1, w - 2, ColorBlack);
    vline(canvas, x, y + 1, h - 2, ColorBlack);
    vline(canvas, x + w - 1, y + 1, h - 2, ColorBlack);
}

static const uint8_t font3x5[][5] = {
    [' ' - 32] = {0,0,0,0,0}, ['+' - 32] = {0,2,7,2,0}, ['-' - 32] = {0,0,7,0,0},
    ['.' - 32] = {0,0,0,0,2},
    ['0' - 32] = {7,5,5,5,7}, ['1' - 32] = {2,6,2,2,7}, ['2' - 32] = {6,1,2,4,7},
    ['3' - 32] = {6,1,2,1,6}, ['4' - 32] = {5,5,7,1,1}, ['5' - 32] = {7,4,6,1,6},
    ['6' - 32] = {3,4,6,5,2}, ['7' - 32] = {7,1,2,2,2}, ['8' - 32] = {2,5,2,5,2},
    ['9' - 32] = {2,5,3,1,6},
    ['A' - 32] = {2,5,7,5,5}, ['B' - 32] = {6,5,6,5,6}, ['C' - 32] = {3,4,4,4,3},
    ['D' - 32] = {6,5,5,5,6}, ['E' - 32] = {7,4,6,4,7}, ['F' - 32] = {7,4,6,4,4},
    ['G' - 32] = {3,4,5,5,3}, ['H' - 32] = {5,5,7,5,5}, ['I' - 32] = {7,2,2,2,7},
    ['J' - 32] = {1,1,1,5,2}, ['K' - 32] = {5,5,6,5,5}, ['L' - 32] = {4,4,4,4,7},
    ['M' - 32] = {5,7,7,5,5}, ['N' - 32] = {5,7,7,7,5}, ['O' - 32] = {2,5,5,5,2},
    ['P' - 32] = {6,5,6,4,4}, ['Q' - 32] = {2,5,5,7,3}, ['R' - 32] = {6,5,6,5,5},
    ['S' - 32] = {3,4,2,1,6}, ['T' - 32] = {7,2,2,2,2}, ['U' - 32] = {5,5,5,5,7},
    ['V' - 32] = {5,5,5,5,2}, ['W' - 32] = {5,5,7,7,5}, ['X' - 32] = {5,5,2,5,5},
    ['Y' - 32] = {5,5,2,2,2}, ['Z' - 32] = {7,1,2,4,7},
};

static void text3(Canvas* canvas, const char* text, int16_t x, int16_t y, Color color) {
    while(*text) {
        char ch = *text++;
        if(ch >= 'a' && ch <= 'z') ch -= 32;
        if(ch < 32 || ch > 'Z') ch = ' ';
        const uint8_t* rows = font3x5[(uint8_t)ch - 32];
        for(uint8_t row = 0; row < 5; row++) {
            for(uint8_t col = 0; col < 3; col++) {
                if(rows[row] & (1U << (2 - col))) pset(canvas, x + col, y + row, color);
            }
        }
        x += 4;
    }
}

static int16_t text_width3(const char* text) {
    const size_t len = strlen(text);
    return len ? (int16_t)(len * 4 - 1) : 0;
}

static void text_center3(Canvas* canvas, const char* text, int16_t cx, int16_t y, Color color) {
    text3(canvas, text, cx - text_width3(text) / 2, y, color);
}

static void triangle(Canvas* canvas, int16_t cx, int16_t cy, UniKey direction, Color color) {
    if(direction == UniKeyUp || direction == UniKeyDown) {
        for(int16_t row = 0; row < 5; row++) {
            const int16_t n = direction == UniKeyUp ? row : 4 - row;
            const int16_t span = 1 + 2 * n;
            hline(canvas, cx - span / 2, cy - 2 + row, span, color);
        }
    } else {
        for(int16_t col = 0; col < 5; col++) {
            const int16_t n = direction == UniKeyLeft ? col : 4 - col;
            const int16_t span = 1 + 2 * n;
            vline(canvas, cx - 2 + col, cy - span / 2, span, color);
        }
    }
}

static void circle(Canvas* canvas, int16_t cx, int16_t cy, int16_t radius, Color color) {
    for(int16_t x = -radius; x <= radius; x++) {
        for(int16_t y = -radius; y <= radius; y++) {
            const int16_t d = x*x + y*y;
            if(d >= radius*radius - radius && d <= radius*radius + radius) {
                pset(canvas, cx + x, cy + y, color);
            }
        }
    }
}

static void draw_icon_kind(
    Canvas* canvas,
    UniIconKind kind,
    const char* label,
    int16_t cx,
    int16_t cy,
    Color color) {
    switch(kind) {
    case UniIconPower:
        circle(canvas, cx, cy + 1, 5, color);
        vline(canvas, cx, cy - 6, 6, color);
        break;
    case UniIconMute:
        hline(canvas, cx - 5, cy - 2, 4, color);
        vline(canvas, cx - 1, cy - 4, 5, color);
        hline(canvas, cx + 2, cy - 3, 5, color);
        hline(canvas, cx + 2, cy + 3, 5, color);
        break;
    case UniIconPlay:
        for(int16_t x=0;x<6;x++) vline(canvas,cx-3+x,cy-x/2,1+x,color);
        break;
    case UniIconPause:
        fill_rect(canvas,cx-4,cy-4,3,9,color);
        fill_rect(canvas,cx+2,cy-4,3,9,color);
        break;
    case UniIconStop:
        fill_rect(canvas,cx-4,cy-4,9,9,color);
        break;
    case UniIconRecord:
        fill_rect(canvas,cx-3,cy-3,7,7,color);
        break;
    case UniIconPrev:
        triangle(canvas,cx+2,cy,UniKeyLeft,color); vline(canvas,cx-5,cy-4,9,color);
        break;
    case UniIconNext:
        triangle(canvas,cx-2,cy,UniKeyRight,color); vline(canvas,cx+5,cy-4,9,color);
        break;
    case UniIconRew:
        triangle(canvas,cx+3,cy,UniKeyLeft,color); triangle(canvas,cx-3,cy,UniKeyLeft,color);
        break;
    case UniIconFfwd:
        triangle(canvas,cx-3,cy,UniKeyRight,color); triangle(canvas,cx+3,cy,UniKeyRight,color);
        break;
    case UniIconHome:
        hline(canvas,cx-4,cy,9,color); vline(canvas,cx-3,cy,5,color); vline(canvas,cx+3,cy,5,color);
        triangle(canvas,cx,cy-3,UniKeyUp,color);
        break;
    case UniIconBack:
        triangle(canvas,cx-3,cy,UniKeyLeft,color); hline(canvas,cx-1,cy,7,color);
        break;
    case UniIconMenu:
        hline(canvas,cx-5,cy-4,11,color); hline(canvas,cx-5,cy,11,color); hline(canvas,cx-5,cy+4,11,color);
        break;
    case UniIconInfo:
        text_center3(canvas,"I",cx,cy-2,color);
        break;
    case UniIconGear:
        circle(canvas,cx,cy,4,color); pset(canvas,cx,cy,color);
        break;
    case UniIconPlus:
        hline(canvas,cx-4,cy,9,color); vline(canvas,cx,cy-4,9,color);
        break;
    case UniIconMinus:
        hline(canvas,cx-4,cy,9,color);
        break;
    case UniIconSun:
        circle(canvas,cx,cy,3,color); hline(canvas,cx-6,cy,3,color); hline(canvas,cx+4,cy,3,color);
        vline(canvas,cx,cy-6,3,color); vline(canvas,cx,cy+4,3,color);
        break;
    case UniIconMoon:
        circle(canvas,cx,cy,5,color); fill_rect(canvas,cx,cy-5,5,10,ColorWhite);
        break;
    case UniIconFan:
        text_center3(canvas,"FAN",cx,cy-2,color);
        break;
    case UniIconSnow:
        hline(canvas,cx-5,cy,11,color); vline(canvas,cx,cy-5,11,color);
        break;
    case UniIconHeat:
        text_center3(canvas,"HOT",cx,cy-2,color);
        break;
    case UniIconDrop:
        triangle(canvas,cx,cy-2,UniKeyUp,color); vline(canvas,cx,cy,5,color);
        break;
    case UniIconSource:
        text_center3(canvas,"SRC",cx,cy-2,color);
        break;
    case UniIconLock:
        frame(canvas,cx-4,cy,9,7,false); circle(canvas,cx,cy,4,color);
        break;
    case UniIconText:
    default:
        text_center3(canvas,label,cx,cy-2,color);
        break;
    }
}

static void draw_icon_id(
    Canvas* canvas,
    const char* id,
    int16_t cx,
    int16_t cy,
    Color color,
    const char* fallback) {
    const UniIconDef* icon = uni_icon_find(id);
    if(icon) draw_icon_kind(canvas, icon->kind, icon->short_label, cx, cy, color);
    else if(fallback && fallback[0]) text_center3(canvas, fallback, cx, cy-2, color);
}

static void grid_rect(const UniElement* e, int16_t* x, int16_t* y, int16_t* w, int16_t* h) {
    *x = grid_x[e->x];
    *y = grid_y[e->y];
    *w = grid_x[e->x + e->w] - *x;
    *h = grid_y[e->y + e->h] - *y;
}

static void draw_status(Canvas* canvas, const UniUiState* state, const UniElement* e) {
    int16_t x,y,w,h; grid_rect(e,&x,&y,&w,&h);
    frame(canvas,x+1,y+1,w-2,h-2,false);
    text3(canvas,uni_transport_label(state->remote->transport),x+4,y+8,ColorBlack);
    text_center3(canvas,state->remote->short_name,x+w/2,y+8,ColorBlack);
    const bool tx_flash = furi_get_tick() < state->tx_flash_until;
    const char* right = tx_flash ? "TX" : (state->tx_ok ? "--" : "ER");
    if(state->dpad_captured && state->dpad_alt) right = "AL";
    if(state->page == UniUiLayoutEditor) right = state->layout_moving ? "MV" : "ED";
    text3(canvas,right,x+w-12,y+8,ColorBlack);
}

static void draw_screen(Canvas* canvas, const UniUiState* state, const UniElement* e) {
    int16_t x,y,w,h; grid_rect(e,&x,&y,&w,&h);
    frame(canvas,x+1,y+1,w-2,h-2,false);
    char title[13]={0}; snprintf(title,sizeof(title),"%.12s",state->remote->name);
    text_center3(canvas,title,x+w/2,y+6,ColorBlack);
    const char* middle=state->last_signal[0]?state->last_signal:e->label;
    if(!middle[0]) middle="READY";
    char value[13]={0}; snprintf(value,sizeof(value),"%.12s",middle);
    text_center3(canvas,value,x+w/2,y+h/2-2,ColorBlack);
    if(state->remote->transport==UniTransportStatefulIr)
        text_center3(canvas,"LOCAL",x+w/2,y+h-9,ColorBlack);
    else if(state->remote->transport==UniTransportBluetoothHid)
        text_center3(canvas,"BT PROFILE",x+w/2,y+h-9,ColorBlack);
    else
        text_center3(canvas,state->tx_ok?"READY":"NO SIGNAL",x+w/2,y+h-9,ColorBlack);
}

static void draw_button(
    Canvas* canvas,const UniElement* e,int16_t x,int16_t y,int16_t w,int16_t h,bool focused) {
    frame(canvas,x+1,y+1,w-2,h-2,focused);
    const Color color=focused?ColorWhite:ColorBlack;
    if(e->icon[0]) draw_icon_id(canvas,e->icon,x+w/2,y+h/2,color,e->label);
    else text_center3(canvas,e->label,x+w/2,y+h/2-2,color);
}

static void draw_hstep(
    Canvas* canvas,const UniElement* e,int16_t x,int16_t y,int16_t w,int16_t h,bool focused) {
    const int16_t side=(w-4)/3, rx=x+w-side;
    frame(canvas,x+1,y+1,side,h-2,focused);
    frame(canvas,rx-1,y+1,side,h-2,focused);
    const Color color=focused?ColorWhite:ColorBlack;
    triangle(canvas,x+side/2+1,y+h/2,UniKeyLeft,color);
    triangle(canvas,rx+side/2-1,y+h/2,UniKeyRight,color);
    text_center3(canvas,e->label,x+w/2,y+h/2-2,ColorBlack);
}

static void draw_vstep(
    Canvas* canvas,const UniElement* e,int16_t x,int16_t y,int16_t w,int16_t h,bool focused) {
    int16_t bh=15; if(h<39) bh=(h-9)/2;
    const int16_t by=y+h-bh-1;
    frame(canvas,x+1,y+1,w-2,bh,focused);
    frame(canvas,x+1,by,w-2,bh,focused);
    const Color color=focused?ColorWhite:ColorBlack;
    triangle(canvas,x+w/2,y+1+bh/2,UniKeyUp,color);
    triangle(canvas,x+w/2,by+bh/2,UniKeyDown,color);
    text_center3(canvas,e->label,x+w/2,y+h/2-2,ColorBlack);
}

static const char* dpad_hold_icon(const UniElement* e, UniKey key) {
    if(key==UniKeyUp) return e->up_hold_icon;
    if(key==UniKeyDown) return e->down_hold_icon;
    if(key==UniKeyLeft) return e->left_hold_icon;
    if(key==UniKeyRight) return e->right_hold_icon;
    return "";
}

static void draw_dpad_direction(
    Canvas* canvas,const UniElement* e,UniKey key,int16_t cx,int16_t cy,Color color,
    bool alternate,bool held) {
    const char* icon=dpad_hold_icon(e,key);
    if((alternate||held) && icon[0]) draw_icon_id(canvas,icon,cx,cy,color,"");
    else triangle(canvas,cx,cy,key,color);
}

static void draw_dpad(
    Canvas* canvas,const UniUiState* state,const UniElement* e,
    int16_t x,int16_t y,int16_t w,int16_t h,bool focused) {
    const int16_t cw=w/3,ch=h/3,cx=x+cw,cy=y+ch;
    const bool active=focused&&state->dpad_captured;
    const Color color=active?ColorWhite:ColorBlack;

    frame(canvas,cx+1,y+1,cw-2,ch-2,active);
    frame(canvas,x+1,cy+1,cw-2,ch-2,active);
    frame(canvas,cx+1,cy+1,cw-2,ch-2,active);
    frame(canvas,x+2*cw+1,cy+1,w-2*cw-2,ch-2,active);
    frame(canvas,cx+1,y+2*ch+1,cw-2,h-2*ch-2,active);

    draw_dpad_direction(canvas,e,UniKeyUp,cx+cw/2,y+ch/2,color,state->dpad_alt,state->dpad_hold_key==UniKeyUp);
    draw_dpad_direction(canvas,e,UniKeyLeft,x+cw/2,cy+ch/2,color,state->dpad_alt,state->dpad_hold_key==UniKeyLeft);
    draw_dpad_direction(canvas,e,UniKeyRight,x+2*cw+(w-2*cw)/2,cy+ch/2,color,state->dpad_alt,state->dpad_hold_key==UniKeyRight);
    draw_dpad_direction(canvas,e,UniKeyDown,cx+cw/2,y+2*ch+(h-2*ch)/2,color,state->dpad_alt,state->dpad_hold_key==UniKeyDown);

    if(state->dpad_alt && e->ok_hold_icon[0])
        draw_icon_id(canvas,e->ok_hold_icon,cx+cw/2,cy+ch/2,color,"OK");
    else
        circle(canvas,cx+cw/2,cy+ch/2,4,color);
}

static void draw_element(Canvas* canvas,const UniUiState* state,size_t index) {
    const UniElement* e=&state->remote->elements[index];
    const bool focused=uni_element_focusable(e)&&index==state->focus_index;
    int16_t x,y,w,h; grid_rect(e,&x,&y,&w,&h);
    switch(e->type) {
    case UniElementStatus: draw_status(canvas,state,e); break;
    case UniElementScreen: draw_screen(canvas,state,e); break;
    case UniElementButton: draw_button(canvas,e,x,y,w,h,focused); break;
    case UniElementHStep: draw_hstep(canvas,e,x,y,w,h,focused); break;
    case UniElementVStep: draw_vstep(canvas,e,x,y,w,h,focused); break;
    case UniElementDpad: draw_dpad(canvas,state,e,x,y,w,h,focused); break;
    }
}

static void draw_page_indicator(Canvas* canvas,const UniUiState* state) {
    if(!state->remote || state->remote->page_count <= 1) return;
    const uint8_t count = state->remote->page_count;
    const int16_t total = (int16_t)count * 3 - 1;
    const int16_t start = (64 - total) / 2;
    for(uint8_t i = 0; i < count; i++) {
        const int16_t x = start + (int16_t)i * 3;
        if(i == state->remote_page) hline(canvas, x, 126, 2, ColorBlack);
        else pset(canvas, x, 126, ColorBlack);
    }
}

static void draw_remote(Canvas* canvas,const UniUiState* state) {
    if(!state->remote) return;
    for(size_t i=0;i<state->remote->element_count;i++) {
        if(state->remote->elements[i].page == state->remote_page) draw_element(canvas,state,i);
    }
    draw_page_indicator(canvas,state);
}

static void draw_menu_header(Canvas* canvas,const char* title) {
    frame(canvas,1,1,62,19,false); text_center3(canvas,title,32,8,ColorBlack);
}

static void draw_menu_row(Canvas* canvas,int16_t y,const char* label,const char* value,bool active) {
    frame(canvas,3,y,58,17,active);
    const Color color=active?ColorWhite:ColorBlack;
    text3(canvas,label,7,y+6,color);
    if(value&&value[0]) {
        const int16_t tw=text_width3(value);
        text3(canvas,value,57-tw,y+6,color);
    }
}

static size_t scroll_start(size_t selected,size_t count) {
    if(count<=5) return 0;
    if(selected<2) return 0;
    if(selected+3>=count) return count-5;
    return selected-2;
}

static void draw_home(Canvas* canvas,const UniUiState* state) {
    char category[13]={0};
    if(!uni_remote_store_category_name(
           state->store,
           state->home_category,
           category,
           sizeof(category))) {
        snprintf(category,sizeof(category),"UNI REMOTE");
    }
    draw_menu_header(canvas,category);

    const size_t count=
        uni_remote_store_category_remote_count(state->store,state->home_category);
    const size_t selected_pos=
        uni_remote_store_category_position(
            state->store,state->home_category,state->selected_remote);
    const size_t start=scroll_start(selected_pos,count);
    for(size_t row=0;row<5 && start+row<count;row++) {
        const size_t position=start+row;
        const size_t index=
            uni_remote_store_category_remote_at(
                state->store,state->home_category,position);
        const UniRemote* r=uni_remote_store_get(state->store,index);
        if(!r) continue;
        char right[5]={0};
        snprintf(right,sizeof(right),"%s%.2s",r->favorite?"*":"",uni_transport_label(r->transport));
        draw_menu_row(
            canvas,
            24+(int16_t)row*19,
            r->name,
            right,
            index==state->selected_remote);
    }
    text_center3(canvas,"< CAT >  BACK MENU",32,119,ColorBlack);
}

static void draw_main_menu(Canvas* canvas,const UniUiState* state) {
    static const char* labels[]={"GLOBAL","REMOTE","LAYOUT","RELOAD","BACK"};
    draw_menu_header(canvas,"MENU");
    for(size_t i=0;i<5;i++) draw_menu_row(canvas,24+(int16_t)i*19,labels[i],"",i==state->menu_index);
}

static void draw_global(Canvas* canvas,const UniUiState* state) {
    draw_menu_header(canvas,"GLOBAL");
    const size_t di=uni_remote_store_find_id(state->store,state->settings->default_remote);
    const UniRemote* dr=uni_remote_store_get(state->store,di);
    draw_menu_row(canvas,26,"REPEAT",state->settings->repeat_enabled?"ON":"OFF",state->menu_index==0);
    draw_menu_row(canvas,47,"AUTO",state->settings->open_default?"ON":"OFF",state->menu_index==1);
    draw_menu_row(canvas,68,"DEFAULT",dr?dr->short_name:"---",state->menu_index==2);
    draw_menu_row(canvas,89,"BACK","",state->menu_index==3);
}

static void draw_remote_settings(Canvas* canvas,const UniUiState* state) {
    const UniRemote* r=state->remote?state->remote:uni_remote_store_get(state->store,state->selected_remote);
    draw_menu_header(canvas,"REMOTE");
    static const char* labels[]={
        "REPEAT","FAV","FOLDER","DEFAULT","LAYOUT","KEYMAP","BT ID","BACK"};
    const size_t count=8,start=scroll_start(state->menu_index,count);
    for(size_t row=0;row<5;row++) {
        const size_t i=start+row; if(i>=count) break;
        const char* value="";
        char temp[10]={0};
        if(i==0) value=r&&r->repeat_enabled?"ON":"OFF";
        else if(i==1) value=r&&r->favorite?"ON":"OFF";
        else if(i==2) {
            if(r&&r->folder[0]) snprintf(temp,sizeof(temp),"%.9s",r->folder);
            else snprintf(temp,sizeof(temp),"NONE");
            value=temp;
        }
        else if(i==3) value="SET";
        else if(i==4) value="EDIT";
        else if(i==5) value="EDIT";
        else if(i==6) { if(r) snprintf(temp,sizeof(temp),"%.9s",r->bluetooth_profile); value=temp; }
        draw_menu_row(canvas,24+(int16_t)row*19,labels[i],value,i==state->menu_index);
    }
}

static void draw_layout_editor(Canvas* canvas,const UniUiState* state) {
    draw_remote(canvas,state);
    if(!state->remote||state->layout_element>=state->remote->element_count) return;
    const UniElement* e=&state->remote->elements[state->layout_element];
    int16_t x,y,w,h; grid_rect(e,&x,&y,&w,&h);
    pset(canvas,x+1,y+1,ColorBlack); pset(canvas,x+w-2,y+1,ColorBlack);
    pset(canvas,x+1,y+h-2,ColorBlack); pset(canvas,x+w-2,y+h-2,ColorBlack);
}

static void draw_layout_tools(Canvas* canvas,const UniUiState* state) {
    static const char* labels[]={"ADD","REPLACE","REMOVE","MAP","ICON","TEMPLATE","DONE"};
    draw_menu_header(canvas,"LAYOUT TOOLS");
    const size_t start=scroll_start(state->menu_index,7);
    for(size_t row=0;row<5;row++) {
        const size_t i=start+row; if(i>=7) break;
        draw_menu_row(canvas,24+(int16_t)row*19,labels[i],"",i==state->menu_index);
    }
}

static void draw_add_element(Canvas* canvas,const UniUiState* state) {
    draw_menu_header(canvas,state->layout_replace_mode?"REPLACE WITH":"ADD ELEMENT");
    const size_t count=uni_element_preset_count(),start=scroll_start(state->menu_index,count);
    for(size_t row=0;row<5 && start+row<count;row++) {
        const size_t i=start+row;
        const UniElementPreset* p=uni_element_preset_get(i);
        draw_menu_row(canvas,24+(int16_t)row*19,p->name,p->id,i==state->menu_index);
    }
}

static void draw_layout_preset(Canvas* canvas,const UniUiState* state) {
    draw_menu_header(canvas,"TEMPLATE");
    const size_t count=uni_layout_preset_count(),start=scroll_start(state->menu_index,count);
    for(size_t row=0;row<5 && start+row<count;row++) {
        const size_t i=start+row;
        const UniLayoutPreset* p=uni_layout_preset_get(i);
        draw_menu_row(canvas,24+(int16_t)row*19,p->name,p->id,i==state->menu_index);
    }
}

static const UniElement* edited_element(const UniUiState* state) {
    if(!state->remote||state->layout_element>=state->remote->element_count) return NULL;
    return &state->remote->elements[state->layout_element];
}

static void draw_map_field(Canvas* canvas,const UniUiState* state) {
    const UniElement* e=edited_element(state);
    draw_menu_header(canvas,"MAP FIELD");
    const size_t count=uni_editor_binding_count(e),start=scroll_start(state->menu_index,count);
    for(size_t row=0;row<5 && start+row<count;row++) {
        const size_t i=start+row;
        draw_menu_row(canvas,24+(int16_t)row*19,uni_editor_binding_label(e,i),"",i==state->menu_index);
    }
}

static void draw_map_kind(Canvas* canvas,const UniUiState* state) {
    const UniRemote* remote=state->remote?state->remote:uni_remote_store_get(state->store,state->selected_remote);
    draw_menu_header(canvas,"ACTION TYPE");
    if(remote&&remote->transport!=UniTransportInfrared) {
        static const char* labels[]={"ACTION","CLEAR","BACK"};
        for(size_t i=0;i<3;i++)
            draw_menu_row(canvas,26+(int16_t)i*21,labels[i],"",i==state->menu_index);
    } else {
        static const char* labels[]={"SIGNAL","SEQUENCE","CLEAR","BACK"};
        for(size_t i=0;i<4;i++)
            draw_menu_row(canvas,26+(int16_t)i*21,labels[i],"",i==state->menu_index);
    }
}

static size_t sequence_count(const UniActionCatalog* catalog) {
    size_t n=0;
    for(size_t i=0;i<catalog->count;i++) if(catalog->actions[i].type==UniActionSequence) n++;
    return n;
}

static const UniNamedAction* sequence_at(const UniActionCatalog* catalog,size_t index) {
    size_t n=0;
    for(size_t i=0;i<catalog->count;i++) {
        if(catalog->actions[i].type!=UniActionSequence) continue;
        if(n++==index) return &catalog->actions[i];
    }
    return NULL;
}

static void draw_map_pick(Canvas* canvas,const UniUiState* state) {
    const UniRemote* remote=state->remote?state->remote:uni_remote_store_get(state->store,state->selected_remote);
    const bool transport_action=remote&&remote->transport!=UniTransportInfrared;
    const bool signal=state->picker_kind==UniPickSignal;
    draw_menu_header(canvas,transport_action?"ACTION":(signal?"SIGNAL":"SEQUENCE"));
    const size_t count=transport_action?
        uni_editor_transport_action_count(remote):
        (signal?state->action_engine->signals.count:sequence_count(&state->action_engine->actions));
    if(count==0) { text_center3(canvas,"EMPTY",32,58,ColorBlack); return; }
    const size_t start=scroll_start(state->menu_index,count);
    for(size_t row=0;row<5 && start+row<count;row++) {
        const size_t i=start+row;
        const char* name="";
        const char* right="";
        if(transport_action) {
            name=uni_editor_transport_action_label(remote,i);
            right=remote->transport==UniTransportBluetoothHid?"BT":"ST";
        } else if(signal) {
            name=state->action_engine->signals.names[i];
            right="SIG";
        } else {
            const UniNamedAction* a=sequence_at(&state->action_engine->actions,i);
            name=a?a->id:"";
            right="SEQ";
        }
        draw_menu_row(canvas,24+(int16_t)row*19,name,right,i==state->menu_index);
    }
}

static void draw_icon_field(Canvas* canvas,const UniUiState* state) {
    const UniElement* e=edited_element(state);
    draw_menu_header(canvas,"ICON FIELD");
    const size_t count=uni_editor_icon_count(e),start=scroll_start(state->menu_index,count);
    for(size_t row=0;row<5 && start+row<count;row++) {
        const size_t i=start+row;
        draw_menu_row(canvas,24+(int16_t)row*19,uni_editor_icon_label(e,i),"",i==state->menu_index);
    }
}

static void draw_icon_pick(Canvas* canvas,const UniUiState* state) {
    draw_menu_header(canvas,"ICON LIBRARY");
    const size_t count=uni_icon_count()+1,start=scroll_start(state->menu_index,count);
    for(size_t row=0;row<5 && start+row<count;row++) {
        const size_t i=start+row;
        if(i==0) draw_menu_row(canvas,24+(int16_t)row*19,"NONE","",state->menu_index==0);
        else {
            const UniIconDef* icon=uni_icon_get(i-1);
            draw_menu_row(canvas,24+(int16_t)row*19,icon->short_label,icon->id,i==state->menu_index);
        }
    }
}

static void draw_keymap(Canvas* canvas,const UniUiState* state) {
    draw_menu_header(canvas,"HARD KEYMAP");
    const size_t count=UniHardCount+1,start=scroll_start(state->menu_index,count);
    for(size_t row=0;row<5 && start+row<count;row++) {
        const size_t i=start+row;
        if(i<UniHardCount) draw_menu_row(canvas,24+(int16_t)row*19,uni_editor_hard_label((UniHardKeySlot)i),"MAP",i==state->menu_index);
        else draw_menu_row(canvas,24+(int16_t)row*19,"BACK","",i==state->menu_index);
    }
}

void uni_ui_draw(Canvas* canvas,const UniUiState* state) {
    canvas_clear(canvas); canvas_set_color(canvas,ColorBlack);
    switch(state->page) {
    case UniUiHome: draw_home(canvas,state); break;
    case UniUiRemote: draw_remote(canvas,state); break;
    case UniUiMenu: draw_main_menu(canvas,state); break;
    case UniUiGlobalSettings: draw_global(canvas,state); break;
    case UniUiRemoteSettings: draw_remote_settings(canvas,state); break;
    case UniUiLayoutEditor: draw_layout_editor(canvas,state); break;
    case UniUiLayoutTools: draw_layout_tools(canvas,state); break;
    case UniUiAddElement: draw_add_element(canvas,state); break;
    case UniUiLayoutPreset: draw_layout_preset(canvas,state); break;
    case UniUiMapField: draw_map_field(canvas,state); break;
    case UniUiMapKind: draw_map_kind(canvas,state); break;
    case UniUiMapPick: draw_map_pick(canvas,state); break;
    case UniUiIconField: draw_icon_field(canvas,state); break;
    case UniUiIconPick: draw_icon_pick(canvas,state); break;
    case UniUiKeymap: draw_keymap(canvas,state); break;
    }
    canvas_set_color(canvas,ColorBlack);
}
