#include "ui.h"

#include <gui/canvas.h>
#include <string.h>

/*
 * Logical canvas is 64x128 and is rotated into the Flipper's native 128x64 LCD.
 * Hold the device clockwise: LCD at the top, controls at the bottom.
 * logical (x,y) -> native (y, 63-x)
 */

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
    if(active) fill_rect(canvas, x, y, w, h, ColorBlack);
    hline(canvas, x + 1, y, w - 2, ColorBlack);
    hline(canvas, x + 1, y + h - 1, w - 2, ColorBlack);
    vline(canvas, x, y + 1, h - 2, ColorBlack);
    vline(canvas, x + w - 1, y + 1, h - 2, ColorBlack);
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
    size_t len = strlen(text);
    return len ? (int16_t)(len * 4 - 1) : 0;
}

static void text_center3(Canvas* canvas, const char* text, int16_t cx, int16_t y, Color color) {
    text3(canvas, text, cx - text_width3(text) / 2, y, color);
}

static void triangle(Canvas* canvas, int16_t cx, int16_t cy, UniAction direction, Color color) {
    if(direction == UniActionUp || direction == UniActionDown) {
        for(int16_t r = 0; r < 5; r++) {
            int16_t n = direction == UniActionUp ? r : 4 - r;
            int16_t span = 1 + 2 * n;
            hline(canvas, cx - span / 2, cy - 2 + r, span, color);
        }
    } else {
        for(int16_t c = 0; c < 5; c++) {
            int16_t n = direction == UniActionLeft ? c : 4 - c;
            int16_t span = 1 + 2 * n;
            vline(canvas, cx - 2 + c, cy - span / 2, span, color);
        }
    }
}

static const char* action_label(UniAction action) {
    switch(action) {
    case UniActionPower:
        return "PWR";
    case UniActionMute:
        return "MUT";
    case UniActionUp:
        return "UP";
    case UniActionDown:
        return "DWN";
    case UniActionLeft:
        return "LFT";
    case UniActionRight:
        return "RGT";
    case UniActionOk:
        return "OK";
    default:
        return "---";
    }
}

static void draw_status(Canvas* canvas, const UniUiState* state) {
    frame(canvas, 1, 1, 62, 19, false);
    text3(canvas, state->profile ? "IR" : "UNI", 4, 8, ColorBlack);
    if(state->profile) text_center3(canvas, state->profile->short_name, 32, 8, ColorBlack);
    text3(canvas, state->tx_flash ? "TX" : "--", 52, 8, ColorBlack);
}

static void draw_home(Canvas* canvas, const UniUiState* state) {
    draw_status(canvas, state);
    text_center3(canvas, "REMOTE", 32, 27, ColorBlack);

    const size_t count = uni_profiles_count();
    for(size_t i = 0; i < count && i < 4; i++) {
        int16_t y = 41 + (int16_t)i * 18;
        bool active = i == state->selected_profile;
        frame(canvas, 3, y, 58, 15, active);
        const UniRemoteProfile* profile = uni_profiles_get(i);
        text_center3(canvas, profile->short_name, 32, y + 5, active ? ColorWhite : ColorBlack);
    }

    text_center3(canvas, "OK OPEN", 32, 116, ColorBlack);
}

static void draw_remote(Canvas* canvas, const UniUiState* state) {
    draw_status(canvas, state);

    frame(canvas, 1, 22, 62, 41, false);
    text_center3(canvas, "NEC DEMO", 32, 28, ColorBlack);
    text_center3(
        canvas,
        state->last_action_valid ? action_label(state->last_action) : "READY",
        32,
        43,
        ColorBlack);
    text_center3(canvas, "HOLD OK MUT", 32, 54, ColorBlack);

    frame(canvas, 1, 65, 19, 19, false);
    triangle(canvas, 10, 74, UniActionLeft, ColorBlack);
    text_center3(canvas, "CH", 32, 71, ColorBlack);
    frame(canvas, 43, 65, 20, 19, false);
    triangle(canvas, 53, 74, UniActionRight, ColorBlack);

    frame(canvas, 1, 86, 19, 15, false);
    triangle(canvas, 10, 93, UniActionUp, ColorBlack);
    text_center3(canvas, "VOL", 10, 103, ColorBlack);
    frame(canvas, 1, 112, 19, 15, false);
    triangle(canvas, 10, 119, UniActionDown, ColorBlack);

    frame(canvas, 22, 86, 19, 41, false);
    text_center3(canvas, "PWR", 31, 100, ColorBlack);
    for(int16_t x = -4; x <= 4; x++) {
        for(int16_t y = -4; y <= 4; y++) {
            int16_t d = x * x + y * y;
            if(d >= 12 && d <= 20) pset(canvas, 31 + x, 113 + y, ColorBlack);
        }
    }

    frame(canvas, 43, 86, 20, 41, false);
    text_center3(canvas, "MOD", 53, 103, ColorBlack);
}

void uni_ui_draw(Canvas* canvas, const UniUiState* state) {
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    if(state->page == UniUiHome) {
        draw_home(canvas, state);
    } else {
        draw_remote(canvas, state);
    }

    canvas_set_color(canvas, ColorBlack);
}
