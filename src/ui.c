#include "ui.h"

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
    const Color color = ColorBlack;
    hline(canvas, x + 1, y, w - 2, color);
    hline(canvas, x + 1, y + h - 1, w - 2, color);
    vline(canvas, x, y + 1, h - 2, color);
    vline(canvas, x + w - 1, y + 1, h - 2, color);
}

static const uint8_t font3x5[][5] = {
    [' ' - 32] = {0, 0, 0, 0, 0},
    ['+' - 32] = {0, 2, 7, 2, 0},
    ['-' - 32] = {0, 0, 7, 0, 0},
    ['.' - 32] = {0, 0, 0, 0, 2},
    ['0' - 32] = {7, 5, 5, 5, 7},
    ['1' - 32] = {2, 6, 2, 2, 7},
    ['2' - 32] = {6, 1, 2, 4, 7},
    ['3' - 32] = {6, 1, 2, 1, 6},
    ['4' - 32] = {5, 5, 7, 1, 1},
    ['5' - 32] = {7, 4, 6, 1, 6},
    ['6' - 32] = {3, 4, 6, 5, 2},
    ['7' - 32] = {7, 1, 2, 2, 2},
    ['8' - 32] = {2, 5, 2, 5, 2},
    ['9' - 32] = {2, 5, 3, 1, 6},
    ['A' - 32] = {2, 5, 7, 5, 5},
    ['B' - 32] = {6, 5, 6, 5, 6},
    ['C' - 32] = {3, 4, 4, 4, 3},
    ['D' - 32] = {6, 5, 5, 5, 6},
    ['E' - 32] = {7, 4, 6, 4, 7},
    ['F' - 32] = {7, 4, 6, 4, 4},
    ['G' - 32] = {3, 4, 5, 5, 3},
    ['H' - 32] = {5, 5, 7, 5, 5},
    ['I' - 32] = {7, 2, 2, 2, 7},
    ['J' - 32] = {1, 1, 1, 5, 2},
    ['K' - 32] = {5, 5, 6, 5, 5},
    ['L' - 32] = {4, 4, 4, 4, 7},
    ['M' - 32] = {5, 7, 7, 5, 5},
    ['N' - 32] = {5, 7, 7, 7, 5},
    ['O' - 32] = {2, 5, 5, 5, 2},
    ['P' - 32] = {6, 5, 6, 4, 4},
    ['Q' - 32] = {2, 5, 5, 7, 3},
    ['R' - 32] = {6, 5, 6, 5, 5},
    ['S' - 32] = {3, 4, 2, 1, 6},
    ['T' - 32] = {7, 2, 2, 2, 2},
    ['U' - 32] = {5, 5, 5, 5, 7},
    ['V' - 32] = {5, 5, 5, 5, 2},
    ['W' - 32] = {5, 5, 7, 7, 5},
    ['X' - 32] = {5, 5, 2, 5, 5},
    ['Y' - 32] = {5, 5, 2, 2, 2},
    ['Z' - 32] = {7, 1, 2, 4, 7},
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
            const int16_t d = x * x + y * y;
            if(d >= radius * radius - radius && d <= radius * radius + radius) {
                pset(canvas, cx + x, cy + y, color);
            }
        }
    }
}

static void grid_rect(
    const UniElement* element,
    int16_t* x,
    int16_t* y,
    int16_t* w,
    int16_t* h) {
    *x = grid_x[element->x];
    *y = grid_y[element->y];
    *w = grid_x[element->x + element->w] - *x;
    *h = grid_y[element->y + element->h] - *y;
}

static void draw_status(Canvas* canvas, const UniUiState* state, const UniElement* element) {
    int16_t x, y, w, h;
    grid_rect(element, &x, &y, &w, &h);
    frame(canvas, x + 1, y + 1, w - 2, h - 2, false);
    text3(canvas, uni_transport_label(state->remote->transport), x + 4, y + 8, ColorBlack);
    text_center3(canvas, state->remote->short_name, x + w / 2, y + 8, ColorBlack);
    const char* status_right = state->tx_flash ? "TX" : (state->tx_ok ? "--" : "ER");
    if(state->page == UniUiLayoutEditor) status_right = state->layout_moving ? "MV" : "ED";
    text3(canvas, status_right, x + w - 12, y + 8, ColorBlack);
}

static void draw_screen(Canvas* canvas, const UniUiState* state, const UniElement* element) {
    int16_t x, y, w, h;
    grid_rect(element, &x, &y, &w, &h);
    frame(canvas, x + 1, y + 1, w - 2, h - 2, false);

    char title[13] = {0};
    snprintf(title, sizeof(title), "%.12s", state->remote->name);
    text_center3(canvas, title, x + w / 2, y + 6, ColorBlack);

    const char* middle = state->last_signal[0] ? state->last_signal : element->label;
    if(!middle[0]) middle = "READY";
    char signal[13] = {0};
    snprintf(signal, sizeof(signal), "%.12s", middle);
    text_center3(canvas, signal, x + w / 2, y + h / 2 - 2, ColorBlack);

    if(state->remote->transport == UniTransportBluetoothHid) {
        char profile[13] = {0};
        snprintf(profile, sizeof(profile), "%.12s", state->remote->bluetooth_profile);
        text_center3(canvas, profile, x + w / 2, y + h - 9, ColorBlack);
    } else if(state->remote->transport == UniTransportStatefulIr) {
        text_center3(canvas, "LOCAL STATE", x + w / 2, y + h - 9, ColorBlack);
    } else {
        text_center3(
            canvas,
            state->tx_ok ? "READY" : "NO SIGNAL",
            x + w / 2,
            y + h - 9,
            ColorBlack);
    }
}

static void draw_button(
    Canvas* canvas,
    const UniElement* element,
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h,
    bool focused) {
    frame(canvas, x + 1, y + 1, w - 2, h - 2, focused);
    const Color color = focused ? ColorWhite : ColorBlack;
    text_center3(canvas, element->label, x + w / 2, y + h / 2 - 2, color);
}

static void draw_hstep(
    Canvas* canvas,
    const UniElement* element,
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h,
    bool focused) {
    const int16_t side = (w - 4) / 3;
    const int16_t left_w = side;
    const int16_t right_x = x + w - side;
    frame(canvas, x + 1, y + 1, left_w, h - 2, focused);
    frame(canvas, right_x - 1, y + 1, side, h - 2, focused);
    const Color color = focused ? ColorWhite : ColorBlack;
    triangle(canvas, x + left_w / 2 + 1, y + h / 2, UniKeyLeft, color);
    triangle(canvas, right_x + side / 2 - 1, y + h / 2, UniKeyRight, color);
    text_center3(canvas, element->label, x + w / 2, y + h / 2 - 2, ColorBlack);
}

static void draw_vstep(
    Canvas* canvas,
    const UniElement* element,
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h,
    bool focused) {
    int16_t button_h = 15;
    if(h < 39) button_h = (h - 9) / 2;
    const int16_t bottom_y = y + h - button_h - 1;
    frame(canvas, x + 1, y + 1, w - 2, button_h, focused);
    frame(canvas, x + 1, bottom_y, w - 2, button_h, focused);
    const Color color = focused ? ColorWhite : ColorBlack;
    triangle(canvas, x + w / 2, y + 1 + button_h / 2, UniKeyUp, color);
    triangle(canvas, x + w / 2, bottom_y + button_h / 2, UniKeyDown, color);
    text_center3(canvas, element->label, x + w / 2, y + h / 2 - 2, ColorBlack);
}

static void draw_dpad(
    Canvas* canvas,
    const UniElement* element,
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h,
    bool focused,
    bool captured) {
    const int16_t cw = w / 3;
    const int16_t ch = h / 3;
    const int16_t cx = x + cw;
    const int16_t cy = y + ch;
    const bool active = focused && captured;
    const Color color = active ? ColorWhite : ColorBlack;

    frame(canvas, cx + 1, y + 1, cw - 2, ch - 2, active);
    frame(canvas, x + 1, cy + 1, cw - 2, ch - 2, active);
    frame(canvas, cx + 1, cy + 1, cw - 2, ch - 2, active);
    frame(canvas, x + 2 * cw + 1, cy + 1, w - 2 * cw - 2, ch - 2, active);
    frame(canvas, cx + 1, y + 2 * ch + 1, cw - 2, h - 2 * ch - 2, active);

    triangle(canvas, cx + cw / 2, y + ch / 2, UniKeyUp, color);
    triangle(canvas, x + cw / 2, cy + ch / 2, UniKeyLeft, color);
    triangle(
        canvas,
        x + 2 * cw + (w - 2 * cw) / 2,
        cy + ch / 2,
        UniKeyRight,
        color);
    triangle(
        canvas,
        cx + cw / 2,
        y + 2 * ch + (h - 2 * ch) / 2,
        UniKeyDown,
        color);
    circle(canvas, cx + cw / 2, cy + ch / 2, 4, color);

    if(element->label[0]) {
        text_center3(canvas, element->label, x + w / 2, y + h - 6, ColorBlack);
    }
}

static void draw_element(Canvas* canvas, const UniUiState* state, size_t index) {
    const UniElement* element = &state->remote->elements[index];
    const bool focused = uni_element_focusable(element) && index == state->focus_index;
    int16_t x, y, w, h;
    grid_rect(element, &x, &y, &w, &h);

    switch(element->type) {
    case UniElementStatus:
        draw_status(canvas, state, element);
        break;
    case UniElementScreen:
        draw_screen(canvas, state, element);
        break;
    case UniElementButton:
        draw_button(canvas, element, x, y, w, h, focused);
        break;
    case UniElementHStep:
        draw_hstep(canvas, element, x, y, w, h, focused);
        break;
    case UniElementVStep:
        draw_vstep(canvas, element, x, y, w, h, focused);
        break;
    case UniElementDpad:
        draw_dpad(canvas, element, x, y, w, h, focused, state->dpad_captured);
        break;
    }
}

static void draw_home(Canvas* canvas, const UniUiState* state) {
    frame(canvas, 1, 1, 62, 19, false);
    text3(canvas, "UNI", 4, 8, ColorBlack);
    text_center3(canvas, "REMOTE", 32, 8, ColorBlack);

    const size_t count = uni_remote_store_count(state->store);
    const size_t start = state->selected_remote > 3 ? state->selected_remote - 3 : 0;
    for(size_t row = 0; row < 5 && start + row < count; row++) {
        const size_t index = start + row;
        const UniRemote* remote = uni_remote_store_get(state->store, index);
        const int16_t y = 24 + (int16_t)row * 18;
        const bool active = index == state->selected_remote;
        frame(canvas, 3, y, 58, 15, active);
        const Color color = active ? ColorWhite : ColorBlack;
        text3(canvas, remote->short_name, 7, y + 5, color);
        char name[10] = {0};
        snprintf(name, sizeof(name), "%.9s", remote->name);
        text3(canvas, name, 21, y + 5, color);
        text3(canvas, uni_transport_label(remote->transport), 51, y + 5, color);
    }

    text_center3(canvas, "BACK MENU", 32, 116, ColorBlack);
}

static void draw_remote(Canvas* canvas, const UniUiState* state) {
    for(size_t i = 0; i < state->remote->element_count; i++) draw_element(canvas, state, i);
}

static const UniRemote* menu_remote(const UniUiState* state) {
    if(state->remote) return state->remote;
    return uni_remote_store_get(state->store, state->selected_remote);
}

static void draw_menu_header(Canvas* canvas, const char* title) {
    frame(canvas, 1, 1, 62, 19, false);
    text_center3(canvas, title, 32, 8, ColorBlack);
}

static void draw_menu_row(
    Canvas* canvas,
    int16_t y,
    const char* label,
    const char* value,
    bool active) {
    frame(canvas, 3, y, 58, 17, active);
    const Color color = active ? ColorWhite : ColorBlack;
    text3(canvas, label, 7, y + 6, color);
    if(value && value[0]) {
        const int16_t width = text_width3(value);
        text3(canvas, value, 57 - width, y + 6, color);
    }
}

static void draw_main_menu(Canvas* canvas, const UniUiState* state) {
    static const char* labels[] = {"GLOBAL", "REMOTE", "LAYOUT", "RELOAD", "BACK"};
    draw_menu_header(canvas, "MENU");
    for(size_t i = 0; i < 5; i++) {
        draw_menu_row(canvas, 24 + (int16_t)i * 19, labels[i], NULL, i == state->menu_index);
    }
}

static void draw_global_settings(Canvas* canvas, const UniUiState* state) {
    draw_menu_header(canvas, "GLOBAL");
    const char* repeat = state->settings->repeat_enabled ? "ON" : "OFF";
    const char* auto_open = state->settings->open_default ? "ON" : "OFF";
    const size_t default_index =
        uni_remote_store_find_id(state->store, state->settings->default_remote);
    const UniRemote* default_remote = uni_remote_store_get(state->store, default_index);
    const char* short_name = default_remote ? default_remote->short_name : "---";

    draw_menu_row(canvas, 26, "REPEAT", repeat, state->menu_index == 0);
    draw_menu_row(canvas, 47, "AUTO", auto_open, state->menu_index == 1);
    draw_menu_row(canvas, 68, "DEFAULT", short_name, state->menu_index == 2);
    draw_menu_row(canvas, 89, "BACK", NULL, state->menu_index == 3);
    text_center3(canvas, "LR CHANGE OK SET", 32, 116, ColorBlack);
}

static void draw_remote_settings(Canvas* canvas, const UniUiState* state) {
    const UniRemote* remote = menu_remote(state);
    draw_menu_header(canvas, "REMOTE");
    if(!remote) {
        text_center3(canvas, "NO REMOTE", 32, 55, ColorBlack);
        return;
    }

    char bt[9] = {0};
    snprintf(bt, sizeof(bt), "%.8s", remote->bluetooth_profile);
    draw_menu_row(
        canvas,
        24,
        "REPEAT",
        remote->repeat_enabled ? "ON" : "OFF",
        state->menu_index == 0);
    draw_menu_row(canvas, 43, "DEFAULT", "SET", state->menu_index == 1);
    draw_menu_row(canvas, 62, "LAYOUT", "EDIT", state->menu_index == 2);
    draw_menu_row(canvas, 81, "BT ID", bt[0] ? bt : "---", state->menu_index == 3);
    draw_menu_row(canvas, 100, "BACK", NULL, state->menu_index == 4);
}

static void draw_layout_editor(Canvas* canvas, const UniUiState* state) {
    if(!state->remote) return;
    draw_remote(canvas, state);
    const UniElement* element =
        state->layout_element < state->remote->element_count ?
            &state->remote->elements[state->layout_element] :
            NULL;
    if(element) {
        int16_t x, y, w, h;
        grid_rect(element, &x, &y, &w, &h);
        pset(canvas, x + 1, y + 1, ColorBlack);
        pset(canvas, x + w - 2, y + 1, ColorBlack);
        pset(canvas, x + 1, y + h - 2, ColorBlack);
        pset(canvas, x + w - 2, y + h - 2, ColorBlack);
    }
}

void uni_ui_draw(Canvas* canvas, const UniUiState* state) {
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    switch(state->page) {
    case UniUiHome:
        draw_home(canvas, state);
        break;
    case UniUiRemote:
        if(state->remote) draw_remote(canvas, state);
        else draw_home(canvas, state);
        break;
    case UniUiMenu:
        draw_main_menu(canvas, state);
        break;
    case UniUiGlobalSettings:
        draw_global_settings(canvas, state);
        break;
    case UniUiRemoteSettings:
        draw_remote_settings(canvas, state);
        break;
    case UniUiLayoutEditor:
        draw_layout_editor(canvas, state);
        break;
    }

    canvas_set_color(canvas, ColorBlack);
}
