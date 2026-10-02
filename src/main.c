#include <furi.h>
#include <gui/gui.h>
#include <gui/canvas.h>
#include <input/input.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "ui_icons.h"

#define INPUT_QUEUE_SIZE 8
#define HOME_PAGE_COUNT 4
#define REMOTE_COUNT 5

typedef enum {
    KeyUp,
    KeyDown,
    KeyLeft,
    KeyRight,
    KeyOk,
    KeyBack,
    KeyUnknown,
} UiKey;

typedef enum {
    ScreenHome,
    ScreenSettings,
    ScreenRemoteManager,
    ScreenRemoteDetail,
    ScreenRemoteGeneral,
    ScreenFolderManager,
    ScreenAppSettings,
    ScreenData,
    ScreenAbout,
    ScreenRuntime,
    ScreenLayout,
    ScreenControls,
    ScreenControlField,
    ScreenMapAction,
    ScreenSignalList,
    ScreenIrFiles,
    ScreenIrImport,
    ScreenConfirmDuplicate,
    ScreenConfirmDelete,
    ScreenInfo,
} Screen;

typedef enum {
    RemoteTv,
    RemoteAc,
    RemoteProjector,
    RemoteGeneric,
} RemoteKind;

typedef struct {
    char name[16];
    bool favourite;
    uint8_t folder;
    RemoteKind kind;
} RemoteDef;

typedef struct {
    FuriMessageQueue* queue;
    ViewPort* viewport;
    Gui* gui;
    bool running;

    Screen screen;
    Screen info_return;
    const char* info_title;
    const char* info_text;

    uint8_t home_page;
    uint8_t home_row;
    uint8_t menu_index;
    uint8_t selected_remote;
    uint8_t grid_focus;
    uint8_t runtime_page;
    uint8_t control_field;
    bool layout_move;
    bool runtime_pressed;

    RemoteDef remotes[REMOTE_COUNT];
} App;

static const char* page_names[HOME_PAGE_COUNT] = {
    "FAVOURITE",
    "LIVING ROOM",
    "BEDROOM",
    "UNCATEGORIZED",
};

static const char* tv_grid[18] = {
    "PWR", "MUTE", "INPUT",
    "VOL-", "HOME", "VOL+",
    "MENU", "GUIDE", "BACK",
    "RED", "UP", "INFO",
    "LEFT", "OK", "RIGHT",
    "BLUE", "DOWN", "EXIT",
};

static const char* ac_grid[18] = {
    "PWR", "MODE", "FAN",
    "TEMP+", "23C", "SWING",
    "TEMP-", "COOL", "TIMER",
    "ECO", "SLEEP", "LIGHT",
    "UP", "OK", "DOWN",
    "BACK", "AUTO", "OFF",
};

static const char* projector_grid[18] = {
    "PWR", "INPUT", "BLANK",
    "MENU", "AUTO", "FREEZE",
    "ZOOM+", "UP", "FOC+",
    "LEFT", "OK", "RIGHT",
    "ZOOM-", "DOWN", "FOC-",
    "BACK", "ASPECT", "MUTE",
};

static const char* generic_grid[18] = {
    "BTN1", "BTN2", "BTN3",
    "BTN4", "BTN5", "BTN6",
    "BTN7", "BTN8", "BTN9",
    "BTN10", "BTN11", "BTN12",
    "BTN13", "BTN14", "BTN15",
    "BTN16", "BTN17", "BTN18",
};

/* Portrait logical canvas: 64x128 mapped onto native 128x64 LCD. */
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

static const uint8_t font3x5[64][5] = {
    [' ' - 32] = {0,0,0,0,0},
    ['+' - 32] = {0,2,7,2,0},
    ['-' - 32] = {0,0,7,0,0},
    ['.' - 32] = {0,0,0,0,2},
    ['/' - 32] = {1,1,2,4,4},
    ['0' - 32] = {7,5,5,5,7},
    ['1' - 32] = {2,6,2,2,7},
    ['2' - 32] = {6,1,2,4,7},
    ['3' - 32] = {6,1,2,1,6},
    ['4' - 32] = {5,5,7,1,1},
    ['5' - 32] = {7,4,6,1,6},
    ['6' - 32] = {3,4,6,5,2},
    ['7' - 32] = {7,1,2,2,2},
    ['8' - 32] = {2,5,2,5,2},
    ['9' - 32] = {2,5,3,1,6},
    ['A' - 32] = {2,5,7,5,5},
    ['B' - 32] = {6,5,6,5,6},
    ['C' - 32] = {3,4,4,4,3},
    ['D' - 32] = {6,5,5,5,6},
    ['E' - 32] = {7,4,6,4,7},
    ['F' - 32] = {7,4,6,4,4},
    ['G' - 32] = {3,4,5,5,3},
    ['H' - 32] = {5,5,7,5,5},
    ['I' - 32] = {7,2,2,2,7},
    ['J' - 32] = {1,1,1,5,2},
    ['K' - 32] = {5,5,6,5,5},
    ['L' - 32] = {4,4,4,4,7},
    ['M' - 32] = {5,7,7,5,5},
    ['N' - 32] = {5,7,7,7,5},
    ['O' - 32] = {2,5,5,5,2},
    ['P' - 32] = {6,5,6,4,4},
    ['Q' - 32] = {2,5,5,7,3},
    ['R' - 32] = {6,5,6,5,5},
    ['S' - 32] = {3,4,2,1,6},
    ['T' - 32] = {7,2,2,2,2},
    ['U' - 32] = {5,5,5,5,7},
    ['V' - 32] = {5,5,5,5,2},
    ['W' - 32] = {5,5,7,7,5},
    ['X' - 32] = {5,5,2,5,5},
    ['Y' - 32] = {5,5,2,2,2},
    ['Z' - 32] = {7,1,2,4,7},
    ['_' - 32] = {0,0,0,0,7},
};

static void text3(Canvas* canvas, const char* text, int16_t x, int16_t y, Color color) {
    while(*text) {
        char ch = *text++;
        if(ch >= 'a' && ch <= 'z') ch -= 32;
        if(ch < 32 || ch > '_') ch = ' ';
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

static void triangle(Canvas* canvas, int16_t cx, int16_t cy, UiKey direction, Color color) {
    if(direction == KeyLeft || direction == KeyRight) {
        for(int16_t col = 0; col < 5; col++) {
            int16_t n = direction == KeyLeft ? 4 - col : col;
            int16_t span = 1 + 2 * n;
            vline(canvas, cx - 2 + col, cy - span / 2, span, color);
        }
    } else if(direction == KeyUp || direction == KeyDown) {
        for(int16_t row = 0; row < 5; row++) {
            int16_t n = direction == KeyUp ? 4 - row : row;
            int16_t span = 1 + 2 * n;
            hline(canvas, cx - span / 2, cy - 2 + row, span, color);
        }
    }
}

static void outline_rect(Canvas* canvas, int16_t x, int16_t y, int16_t w, int16_t h, Color color) {
    if(w < 2 || h < 2) return;
    hline(canvas, x, y, w, color);
    hline(canvas, x, y + h - 1, w, color);
    vline(canvas, x, y, h, color);
    vline(canvas, x + w - 1, y, h, color);
}

static void ring(Canvas* canvas, int16_t cx, int16_t cy, Color color) {
    static const int8_t pts[][2] = {
        {-3,-5},{-2,-6},{-1,-6},{0,-6},{1,-6},{2,-6},{3,-5},
        {5,-3},{6,-2},{6,-1},{6,0},{6,1},{6,2},{5,3},
        {3,5},{2,6},{1,6},{0,6},{-1,6},{-2,6},{-3,5},
        {-5,3},{-6,2},{-6,1},{-6,0},{-6,-1},{-6,-2},{-5,-3},
    };
    for(size_t i = 0; i < sizeof(pts) / sizeof(pts[0]); i++) {
        pset(canvas, cx + pts[i][0], cy + pts[i][1], color);
    }
}

static void draw_icon(Canvas* canvas, const char* label, int16_t x, int16_t y, Color color) {
    const int16_t cx = x + 9;
    const int16_t cy = y + 9;

    if(strcmp(label, "PWR") == 0) {
        ring(canvas, cx, cy + 1, color);
        vline(canvas, cx, cy - 6, 7, color);
    } else if(strcmp(label, "MUTE") == 0) {
        fill_rect(canvas, cx - 6, cy - 2, 3, 5, color);
        hline(canvas, cx - 3, cy - 3, 2, color);
        hline(canvas, cx - 3, cy + 3, 2, color);
        hline(canvas, cx + 2, cy - 4, 5, color);
        hline(canvas, cx + 2, cy + 4, 5, color);
    } else if(strcmp(label, "INPUT") == 0) {
        outline_rect(canvas, x + 3, y + 4, 10, 11, color);
        hline(canvas, x + 8, y + 9, 7, color);
        triangle(canvas, x + 13, y + 9, KeyRight, color);
    } else if(strcmp(label, "HOME") == 0) {
        hline(canvas, cx - 5, cy, 11, color);
        vline(canvas, cx - 4, cy, 6, color);
        vline(canvas, cx + 4, cy, 6, color);
        triangle(canvas, cx, cy - 3, KeyUp, color);
    } else if(strcmp(label, "MENU") == 0) {
        hline(canvas, cx - 6, cy - 5, 13, color);
        hline(canvas, cx - 6, cy, 13, color);
        hline(canvas, cx - 6, cy + 5, 13, color);
    } else if(strcmp(label, "BACK") == 0) {
        triangle(canvas, cx - 3, cy, KeyLeft, color);
        hline(canvas, cx - 1, cy, 8, color);
    } else if(strcmp(label, "UP") == 0) {
        triangle(canvas, cx, cy, KeyUp, color);
    } else if(strcmp(label, "DOWN") == 0) {
        triangle(canvas, cx, cy, KeyDown, color);
    } else if(strcmp(label, "LEFT") == 0) {
        triangle(canvas, cx, cy, KeyLeft, color);
    } else if(strcmp(label, "RIGHT") == 0) {
        triangle(canvas, cx, cy, KeyRight, color);
    } else if(strcmp(label, "OK") == 0) {
        ring(canvas, cx, cy, color);
        text_center3(canvas, "OK", cx, cy - 2, color);
    } else if(strcmp(label, "INFO") == 0) {
        ring(canvas, cx, cy, color);
        pset(canvas, cx, cy - 4, color);
        vline(canvas, cx, cy - 1, 6, color);
    } else if(strcmp(label, "VOL+") == 0 || strcmp(label, "TEMP+") == 0 ||
              strcmp(label, "ZOOM+") == 0 || strcmp(label, "FOC+") == 0) {
        hline(canvas, cx - 5, cy, 11, color);
        vline(canvas, cx, cy - 5, 11, color);
    } else if(strcmp(label, "VOL-") == 0 || strcmp(label, "TEMP-") == 0 ||
              strcmp(label, "ZOOM-") == 0 || strcmp(label, "FOC-") == 0) {
        hline(canvas, cx - 5, cy, 11, color);
    } else if(strcmp(label, "FAN") == 0) {
        pset(canvas, cx, cy, color);
        hline(canvas, cx - 1, cy - 5, 3, color);
        vline(canvas, cx + 5, cy - 1, 3, color);
        hline(canvas, cx - 1, cy + 5, 3, color);
        vline(canvas, cx - 5, cy - 1, 3, color);
        hline(canvas, cx + 1, cy - 4, 3, color);
        vline(canvas, cx + 4, cy + 1, 3, color);
        hline(canvas, cx - 3, cy + 4, 3, color);
        vline(canvas, cx - 4, cy - 3, 3, color);
    } else if(strcmp(label, "COOL") == 0 || strcmp(label, "FREEZE") == 0) {
        hline(canvas, cx - 6, cy, 13, color);
        vline(canvas, cx, cy - 6, 13, color);
        for(int8_t d = -4; d <= 4; d++) {
            if((d & 1) == 0) {
                pset(canvas, cx + d, cy + d, color);
                pset(canvas, cx + d, cy - d, color);
            }
        }
    } else if(strcmp(label, "LIGHT") == 0) {
        ring(canvas, cx, cy, color);
        hline(canvas, cx - 7, cy, 3, color);
        hline(canvas, cx + 5, cy, 3, color);
        vline(canvas, cx, cy - 7, 3, color);
        vline(canvas, cx, cy + 5, 3, color);
    } else if(strcmp(label, "SLEEP") == 0) {
        vline(canvas, cx - 4, cy - 4, 9, color);
        pset(canvas, cx - 3, cy - 5, color);
        pset(canvas, cx - 2, cy - 6, color);
        pset(canvas, cx - 3, cy + 5, color);
        pset(canvas, cx - 2, cy + 6, color);
        vline(canvas, cx + 2, cy - 3, 7, color);
        pset(canvas, cx + 3, cy - 2, color);
        pset(canvas, cx + 3, cy + 2, color);
    } else if(strcmp(label, "TIMER") == 0) {
        ring(canvas, cx, cy, color);
        vline(canvas, cx, cy - 4, 5, color);
        hline(canvas, cx, cy, 4, color);
    } else if(strcmp(label, "SWING") == 0) {
        triangle(canvas, cx - 4, cy, KeyLeft, color);
        triangle(canvas, cx + 4, cy, KeyRight, color);
    } else if(strcmp(label, "GUIDE") == 0) {
        for(int8_t dx = -5; dx <= 5; dx += 5) {
            for(int8_t dy = -5; dy <= 5; dy += 5) {
                outline_rect(canvas, cx + dx - 1, cy + dy - 1, 3, 3, color);
            }
        }
    } else if(strcmp(label, "BLANK") == 0) {
        outline_rect(canvas, cx - 5, cy - 5, 11, 11, color);
    } else {
        char short_label[5] = {0};
        strncpy(short_label, label, 4);
        text_center3(canvas, short_label, cx, cy - 2, color);
    }
}

static void draw_header(Canvas* canvas, const char* title) {
    text_center3(canvas, title, 32, 4, ColorBlack);
    hline(canvas, 2, 13, 60, ColorBlack);
}

/* Menu/list rows are intentionally borderless. Selection is a compact inverted strip. */
static void draw_row(Canvas* canvas, int16_t y, const char* label, const char* value, bool active) {
    if(active) fill_rect(canvas, 2, y, 60, 12, ColorBlack);
    Color c = active ? ColorWhite : ColorBlack;
    text3(canvas, label, 5, y + 4, c);
    if(value && value[0]) {
        int16_t tw = text_width3(value);
        text3(canvas, value, 59 - tw, y + 4, c);
    }
}

static size_t scroll_start(size_t selected, size_t count) {
    if(count <= 7) return 0;
    if(selected < 3) return 0;
    if(selected + 4 >= count) return count - 7;
    return selected - 3;
}

static uint8_t page_remote_count(const App* app, uint8_t page) {
    uint8_t count = 0;
    for(uint8_t i = 0; i < REMOTE_COUNT; i++) {
        if((page == 0 && app->remotes[i].favourite) ||
           (page > 0 && app->remotes[i].folder == page)) {
            count++;
        }
    }
    return count;
}

static uint8_t page_remote_at(const App* app, uint8_t page, uint8_t row) {
    uint8_t current = 0;
    for(uint8_t i = 0; i < REMOTE_COUNT; i++) {
        if((page == 0 && app->remotes[i].favourite) ||
           (page > 0 && app->remotes[i].folder == page)) {
            if(current == row) return i;
            current++;
        }
    }
    return 0;
}

static const char* folder_name(uint8_t folder) {
    if(folder >= HOME_PAGE_COUNT) return "UNCATEGORIZED";
    return page_names[folder];
}

static const char** grid_for_remote(const App* app) {
    switch(app->remotes[app->selected_remote].kind) {
    case RemoteAc: return ac_grid;
    case RemoteProjector: return projector_grid;
    case RemoteGeneric: return generic_grid;
    case RemoteTv:
    default:
        return tv_grid;
    }
}

static void draw_home(Canvas* canvas, const App* app) {
    text_center3(canvas, page_names[app->home_page], 32, 4, ColorBlack);
    hline(canvas, 2, 13, 60, ColorBlack);

    uint8_t count = page_remote_count(app, app->home_page);
    for(uint8_t row = 0; row < count && row < 6; row++) {
        uint8_t remote = page_remote_at(app, app->home_page, row);
        draw_row(canvas, 18 + row * 14, app->remotes[remote].name, "", app->home_row == row);
    }
    if(count == 0) text_center3(canvas, "EMPTY", 32, 45, ColorBlack);

    hline(canvas, 2, 108, 60, ColorBlack);
    draw_row(canvas, 113, "SETTINGS", "", app->home_row == count);
}

static void draw_menu(Canvas* canvas, const char* title, const char* const* labels, const char* const* values, size_t count, size_t selected) {
    draw_header(canvas, title);
    size_t start = scroll_start(selected, count);
    for(size_t row = 0; row < 7 && start + row < count; row++) {
        size_t i = start + row;
        draw_row(canvas, 18 + (int16_t)row * 15, labels[i], values ? values[i] : "", i == selected);
    }
}

static void draw_settings(Canvas* canvas, const App* app) {
    static const char* labels[] = {"REMOTE MANAGER","FOLDER MANAGER","APP SETTINGS","DATA","ABOUT","ICON DEMO"};
    draw_menu(canvas, "SETTINGS", labels, NULL, 6, app->menu_index);
}

static void draw_remote_manager(Canvas* canvas, const App* app) {
    const char* labels[REMOTE_COUNT];
    for(uint8_t i = 0; i < REMOTE_COUNT; i++) labels[i] = app->remotes[i].name;
    draw_menu(canvas, "REMOTE MANAGER", labels, NULL, REMOTE_COUNT, app->menu_index);
}

static void draw_remote_detail(Canvas* canvas, const App* app) {
    static const char* labels[] = {"GENERAL","LAYOUT","CONTROLS","IR FILES","DUPLICATE","DELETE"};
    draw_menu(canvas, app->remotes[app->selected_remote].name, labels, NULL, 6, app->menu_index);
}

static void draw_remote_general(Canvas* canvas, const App* app) {
    static const char* labels[] = {"RENAME","FAVOURITE","FOLDER"};
    const char* values[3] = {
        "",
        app->remotes[app->selected_remote].favourite ? "ON" : "OFF",
        folder_name(app->remotes[app->selected_remote].folder),
    };
    draw_menu(canvas, "GENERAL", labels, values, 3, app->menu_index);
}

static void draw_folder_manager(Canvas* canvas, const App* app) {
    UNUSED(app);
    static const char* labels[] = {"CREATE FOLDER","LIVING ROOM","BEDROOM","UNCATEGORIZED"};
    static const char* values[] = {"","EDIT","EDIT","SYSTEM"};
    draw_menu(canvas, "FOLDER MANAGER", labels, values, 4, app->menu_index);
}

static void draw_app_settings(Canvas* canvas, const App* app) {
    UNUSED(app);
    static const char* labels[] = {"START PAGE","REMEMBER FOCUS","DEV BUILD"};
    static const char* values[] = {"FAV","ON","ON"};
    draw_menu(canvas, "APP SETTINGS", labels, values, 3, app->menu_index);
}

static void draw_data(Canvas* canvas, const App* app) {
    static const char* labels[] = {"IMPORT REMOTE","EXPORT REMOTE","BACKUP","RESTORE"};
    draw_menu(canvas, "DATA", labels, NULL, 4, app->menu_index);
}

static void draw_about(Canvas* canvas) {
    draw_header(canvas, "ABOUT");
    text_center3(canvas, "UNI REMOTE", 32, 39, ColorBlack);
    text_center3(canvas, "V0.1.0 DEV", 32, 54, ColorBlack);
    text_center3(canvas, "UI PROTOTYPE", 32, 69, ColorBlack);
    text_center3(canvas, "8PX STATUS", 32, 84, ColorBlack);
    text_center3(canvas, "BACK TO RETURN", 32, 111, ColorBlack);
}

static uint8_t runtime_page_count(const App* app) {
    switch(app->remotes[app->selected_remote].kind) {
    case RemoteAc: return 4;
    case RemoteProjector: return 3;
    case RemoteGeneric: return 2;
    case RemoteTv:
    default:
        return 5;
    }
}

static void draw_status_ir(Canvas* canvas, int16_t x, int16_t y) {
    /* 7x7 IR/emit icon. */
    pset(canvas, x, y + 3, ColorBlack);
    vline(canvas, x + 2, y + 2, 3, ColorBlack);
    pset(canvas, x + 3, y + 1, ColorBlack);
    pset(canvas, x + 3, y + 5, ColorBlack);
    pset(canvas, x + 4, y, ColorBlack);
    pset(canvas, x + 4, y + 6, ColorBlack);
    pset(canvas, x + 5, y + 1, ColorBlack);
    pset(canvas, x + 5, y + 5, ColorBlack);
    pset(canvas, x + 6, y + 3, ColorBlack);
}

static void draw_status_tv(Canvas* canvas, int16_t x, int16_t y) {
    outline_rect(canvas, x, y, 7, 5, ColorBlack);
    pset(canvas, x + 2, y + 5, ColorBlack);
    pset(canvas, x + 4, y + 5, ColorBlack);
    hline(canvas, x + 1, y + 6, 5, ColorBlack);
}

static void draw_status_snow(Canvas* canvas, int16_t x, int16_t y) {
    const int16_t cx = x + 3;
    const int16_t cy = y + 3;
    hline(canvas, x, cy, 7, ColorBlack);
    vline(canvas, cx, y, 7, ColorBlack);
    pset(canvas, x + 1, y + 1, ColorBlack);
    pset(canvas, x + 5, y + 5, ColorBlack);
    pset(canvas, x + 5, y + 1, ColorBlack);
    pset(canvas, x + 1, y + 5, ColorBlack);
}

static void draw_status_fan(Canvas* canvas, int16_t x, int16_t y) {
    const int16_t cx = x + 3;
    const int16_t cy = y + 3;
    pset(canvas, cx, cy, ColorBlack);
    hline(canvas, cx - 1, y, 3, ColorBlack);
    vline(canvas, x + 6, cy - 1, 3, ColorBlack);
    hline(canvas, cx - 1, y + 6, 3, ColorBlack);
    vline(canvas, x, cy - 1, 3, ColorBlack);
}

static void draw_status_projector(Canvas* canvas, int16_t x, int16_t y) {
    outline_rect(canvas, x, y + 1, 7, 5, ColorBlack);
    outline_rect(canvas, x + 4, y + 2, 2, 2, ColorBlack);
    pset(canvas, x + 1, y + 6, ColorBlack);
    pset(canvas, x + 5, y + 6, ColorBlack);
}

static void draw_status_generic(Canvas* canvas, int16_t x, int16_t y) {
    pset(canvas, x + 1, y + 1, ColorBlack);
    pset(canvas, x + 5, y + 1, ColorBlack);
    pset(canvas, x + 1, y + 5, ColorBlack);
    pset(canvas, x + 5, y + 5, ColorBlack);
    outline_rect(canvas, x, y, 7, 7, ColorBlack);
}

static void draw_status_bar(Canvas* canvas, const App* app) {
    /* Full 64x8 usable area; intentionally no border. */
    draw_status_ir(canvas, 1, 0);

    switch(app->remotes[app->selected_remote].kind) {
    case RemoteAc:
        draw_status_snow(canvas, 11, 0);
        text3(canvas, "23", 20, 1, ColorBlack);
        draw_status_fan(canvas, 30, 0);
        text3(canvas, "3", 39, 1, ColorBlack);
        break;
    case RemoteProjector:
        draw_status_projector(canvas, 11, 0);
        text3(canvas, "PJ", 20, 1, ColorBlack);
        break;
    case RemoteGeneric:
        draw_status_generic(canvas, 11, 0);
        text3(canvas, "GEN", 20, 1, ColorBlack);
        break;
    case RemoteTv:
    default:
        draw_status_tv(canvas, 11, 0);
        text3(canvas, "TV", 20, 1, ColorBlack);
        break;
    }

    /* Right side shows live TX while OK is held; otherwise current page. */
    if(app->runtime_pressed && app->screen == ScreenRuntime) {
        const char* tx = "TX";
        int16_t tw = text_width3(tx);
        text3(canvas, tx, 63 - tw, 1, ColorBlack);
    } else {
        char page_text[4] = {'P', (char)('1' + app->runtime_page), '\0', '\0'};
        int16_t tw = text_width3(page_text);
        text3(canvas, page_text, 63 - tw, 1, ColorBlack);
    }
}

static void draw_page_indicator(Canvas* canvas, const App* app) {
    const uint8_t count = runtime_page_count(app);
    if(count == 0) return;

    /* All four rightmost columns belong to page navigation. */
    const uint8_t gap = 2;
    const uint8_t total = (uint8_t)(count + (count - 1) * gap + 1);
    int16_t y = 8 + (120 - total) / 2;
    const int16_t x = 61;

    for(uint8_t i = 0; i < count; i++) {
        if(i == app->runtime_page) {
            vline(canvas, x, y, 2, ColorBlack);
            y += 2;
        } else {
            pset(canvas, x, y, ColorBlack);
            y += 1;
        }
        if(i + 1 < count) y += gap;
    }
}

static void draw_button_boundary(
    Canvas* canvas,
    int16_t x,
    int16_t y,
    bool filled,
    Color color) {
    /* 19x19 near-square: each corner is clipped by one pixel. */
    if(filled) {
        hline(canvas, x + 1, y, 17, color);
        fill_rect(canvas, x, y + 1, 19, 17, color);
        hline(canvas, x + 1, y + 18, 17, color);
    } else {
        hline(canvas, x + 1, y, 17, color);
        hline(canvas, x + 1, y + 18, 17, color);
        vline(canvas, x, y + 1, 17, color);
        vline(canvas, x + 18, y + 1, 17, color);
    }
}

static void draw_grid(Canvas* canvas, const App* app, bool editor) {
    const char** labels = grid_for_remote(app);

    /*
     * Runtime geometry:
     * Y: 8 status + 6*(19 button + 1 gap) = 128.
     * X: 3*(19 button + 1 gap) + 4 page-navigation columns = 64.
     * There are no shared grid lines; every control owns its 19x19 boundary.
     */
    draw_status_bar(canvas, app);

    for(uint8_t row = 0; row < 6; row++) {
        for(uint8_t col = 0; col < 3; col++) {
            uint8_t i = row * 3 + col;
            int16_t x = col * 20;
            int16_t y = 8 + row * 20;
            bool focused = i == app->grid_focus;
            bool pressed = focused && app->runtime_pressed && app->screen == ScreenRuntime;
            bool filled = focused && !pressed;
            Color icon_color = filled ? ColorWhite : ColorBlack;

            draw_button_boundary(canvas, x, y, filled, ColorBlack);
            draw_icon(canvas, labels[i], x, y, icon_color);
        }
    }

    if(!editor) draw_page_indicator(canvas, app);

    if(editor) {
        text3(canvas, app->layout_move ? "MV" : "ED", 48, 1, ColorBlack);
    }
}

static void draw_controls(Canvas* canvas, const App* app) {
    draw_grid(canvas, app, false);
}

static uint8_t icon_demo_pages(void) {
    return (uint8_t)((ui_icon_count() + 17) / 18);
}

static void draw_icon_demo(Canvas* canvas, const App* app) {
    uint8_t pages = icon_demo_pages();
    uint8_t base = (uint8_t)(app->runtime_page * 18);
    uint8_t selected = (uint8_t)(base + app->grid_focus);
    const char* name = selected < ui_icon_count() ? ui_icon_name((UiIcon)selected) : "EMPTY";
    char short_name[11] = {0};
    strncpy(short_name, name, 10);
    text3(canvas, short_name, 1, 1, ColorBlack);

    char pg[6] = {'P',(char)('1'+app->runtime_page),'/',(char)('0'+pages),'\0','\0'};
    int16_t pw = text_width3(pg);
    text3(canvas, pg, 63 - pw, 1, ColorBlack);

    for(uint8_t row=0; row<6; row++) {
        for(uint8_t col=0; col<3; col++) {
            uint8_t slot=(uint8_t)(row*3+col);
            uint8_t idx=(uint8_t)(base+slot);
            int16_t x=col*20, y=8+row*20;
            bool focused=slot==app->grid_focus;
            bool pressed=focused && app->runtime_pressed;
            bool filled=focused && !pressed;
            Color icon_color=filled?ColorWhite:ColorBlack;
            draw_button_boundary(canvas,x,y,filled,ColorBlack);
            if(idx<ui_icon_count()) ui_icon_draw(canvas,(UiIcon)idx,x,y,icon_color);
        }
    }

    const uint8_t gap=2;
    const uint8_t total=(uint8_t)(pages+(pages-1)*gap+1);
    int16_t yy=8+(120-total)/2;
    for(uint8_t i=0;i<pages;i++){
        if(i==app->runtime_page){vline(canvas,61,yy,2,ColorBlack);yy+=2;}
        else {pset(canvas,61,yy,ColorBlack);yy+=1;}
        if(i+1<pages) yy+=gap;
    }
}

static void draw_control_field(Canvas* canvas, const App* app) {
    static const char* labels[] = {"TAP","HOLD","CLEAR"};
    draw_menu(canvas, "CONTROL", labels, NULL, 3, app->menu_index);
}

static void draw_map_action(Canvas* canvas, const App* app) {
    static const char* labels[] = {"IR SIGNAL","NONE"};
    draw_menu(canvas, app->control_field == 0 ? "MAP TAP" : "MAP HOLD", labels, NULL, 2, app->menu_index);
}

static void draw_signal_list(Canvas* canvas, const App* app) {
    static const char* labels[] = {"POWER","MUTE","INPUT","VOL UP","VOL DOWN","HOME","MENU"};
    draw_menu(canvas, "IR SIGNAL", labels, NULL, 7, app->menu_index);
}

static void draw_ir_files(Canvas* canvas, const App* app) {
    static const char* labels[] = {"ADD IR FILE","TV_ORIG.IR","AUX.IR"};
    draw_menu(canvas, "IR FILES", labels, NULL, 3, app->menu_index);
}

static void draw_ir_import(Canvas* canvas, const App* app) {
    static const char* labels[] = {"FLIPPER IR","BROWSE STORAGE","BACK"};
    draw_menu(canvas, "IMPORT IR FILE", labels, NULL, 3, app->menu_index);
}

static void draw_confirm(Canvas* canvas, const App* app, const char* title) {
    static const char* labels[] = {"YES","NO"};
    draw_menu(canvas, title, labels, NULL, 2, app->menu_index);
}

static void draw_info(Canvas* canvas, const App* app) {
    draw_header(canvas, app->info_title ? app->info_title : "INFO");
    text_center3(canvas, app->info_text ? app->info_text : "UI ONLY", 32, 54, ColorBlack);
    text_center3(canvas, "BACK TO RETURN", 32, 106, ColorBlack);
}

static void draw_callback(Canvas* canvas, void* context) {
    App* app = context;
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    switch(app->screen) {
    case ScreenHome: draw_home(canvas, app); break;
    case ScreenSettings: draw_settings(canvas, app); break;
    case ScreenRemoteManager: draw_remote_manager(canvas, app); break;
    case ScreenRemoteDetail: draw_remote_detail(canvas, app); break;
    case ScreenRemoteGeneral: draw_remote_general(canvas, app); break;
    case ScreenFolderManager: draw_folder_manager(canvas, app); break;
    case ScreenAppSettings: draw_app_settings(canvas, app); break;
    case ScreenData: draw_data(canvas, app); break;
    case ScreenAbout: draw_about(canvas); break;
    case ScreenRuntime: draw_grid(canvas, app, false); break;
    case ScreenLayout: draw_grid(canvas, app, true); break;
    case ScreenControls: draw_controls(canvas, app); break;
    case ScreenControlField: draw_control_field(canvas, app); break;
    case ScreenMapAction: draw_map_action(canvas, app); break;
    case ScreenSignalList: draw_signal_list(canvas, app); break;
    case ScreenIrFiles: draw_ir_files(canvas, app); break;
    case ScreenIrImport: draw_ir_import(canvas, app); break;
    case ScreenConfirmDuplicate: draw_confirm(canvas, app, "DUPLICATE?"); break;
    case ScreenConfirmDelete: draw_confirm(canvas, app, "DELETE?"); break;
    case ScreenInfo: draw_info(canvas, app); break;
    case ScreenIconDemo: draw_icon_demo(canvas, app); break;
    }
}

static void input_callback(InputEvent* event, void* context) {
    FuriMessageQueue* queue = context;
    furi_message_queue_put(queue, event, 0);
}

/* Physical portrait mapping, verified in the earlier hardware prototype. */
static UiKey map_key(InputKey key) {
    switch(key) {
    case InputKeyLeft: return KeyUp;
    case InputKeyRight: return KeyDown;
    case InputKeyDown: return KeyLeft;
    case InputKeyUp: return KeyRight;
    case InputKeyOk: return KeyOk;
    case InputKeyBack: return KeyBack;
    default: return KeyUnknown;
    }
}

static void menu_move(App* app, uint8_t count, UiKey key) {
    if(count == 0) return;
    if(key == KeyUp) app->menu_index = app->menu_index == 0 ? count - 1 : app->menu_index - 1;
    else if(key == KeyDown) app->menu_index = (app->menu_index + 1) % count;
}

static void grid_move(App* app, UiKey key) {
    uint8_t row = app->grid_focus / 3;
    uint8_t col = app->grid_focus % 3;
    if(key == KeyUp) row = row == 0 ? 5 : row - 1;
    else if(key == KeyDown) row = (row + 1) % 6;
    else if(key == KeyLeft) col = col == 0 ? 2 : col - 1;
    else if(key == KeyRight) col = (col + 1) % 3;
    app->grid_focus = row * 3 + col;
}

static void runtime_grid_move(App* app, UiKey key) {
    uint8_t row = app->grid_focus / 3;
    uint8_t col = app->grid_focus % 3;
    const uint8_t pages = runtime_page_count(app);

    if(key == KeyDown && row == 5) {
        app->runtime_page = (app->runtime_page + 1) % pages;
        app->grid_focus = col;
        return;
    }
    if(key == KeyUp && row == 0) {
        app->runtime_page = app->runtime_page == 0 ? pages - 1 : app->runtime_page - 1;
        app->grid_focus = 15 + col;
        return;
    }

    grid_move(app, key);
}

static void show_info(App* app, Screen return_to, const char* title, const char* text) {
    app->info_return = return_to;
    app->info_title = title;
    app->info_text = text;
    app->screen = ScreenInfo;
}

static void handle_home(App* app, UiKey key) {
    uint8_t count = page_remote_count(app, app->home_page);

    if(key == KeyLeft) {
        app->home_page = app->home_page == 0 ? HOME_PAGE_COUNT - 1 : app->home_page - 1;
        count = page_remote_count(app, app->home_page);
        if(app->home_row > count) app->home_row = count;
    } else if(key == KeyRight) {
        app->home_page = (app->home_page + 1) % HOME_PAGE_COUNT;
        count = page_remote_count(app, app->home_page);
        if(app->home_row > count) app->home_row = count;
    } else if(key == KeyUp) {
        app->home_row = app->home_row == 0 ? count : app->home_row - 1;
    } else if(key == KeyDown) {
        app->home_row = app->home_row >= count ? 0 : app->home_row + 1;
    } else if(key == KeyOk) {
        if(app->home_row == count) {
            app->screen = ScreenSettings;
            app->menu_index = 0;
        } else {
            app->selected_remote = page_remote_at(app, app->home_page, app->home_row);
            app->grid_focus = 0;
            app->runtime_page = 0;
            app->runtime_pressed = false;
            app->screen = ScreenRuntime;
        }
    } else if(key == KeyBack) {
        app->running = false;
    }
}

static void handle_short(App* app, UiKey key) {
    if(app->screen == ScreenHome) {
        handle_home(app, key);
        return;
    }

    if(app->screen == ScreenRuntime) {
        if(key == KeyBack) {
            app->runtime_pressed = false;
            app->screen = ScreenHome;
        } else if(key == KeyUp || key == KeyDown || key == KeyLeft || key == KeyRight) {
            runtime_grid_move(app, key);
        }
        return;
    }

    if(app->screen == ScreenLayout) {
        if(key == KeyBack) {
            app->layout_move = false;
            app->screen = ScreenRemoteDetail;
            app->menu_index = 1;
        } else if(key == KeyOk) {
            app->layout_move = !app->layout_move;
        } else if(key == KeyUp || key == KeyDown || key == KeyLeft || key == KeyRight) {
            grid_move(app, key);
        }
        return;
    }

    if(app->screen == ScreenControls) {
        if(key == KeyBack) {
            app->screen = ScreenRemoteDetail;
            app->menu_index = 2;
        } else if(key == KeyOk) {
            app->screen = ScreenControlField;
            app->menu_index = 0;
        } else if(key == KeyUp || key == KeyDown || key == KeyLeft || key == KeyRight) {
            grid_move(app, key);
        }
        return;
    }

    if(app->screen == ScreenIconDemo) {
        uint8_t row=app->grid_focus/3;
        uint8_t col=app->grid_focus%3;
        uint8_t pages=icon_demo_pages();
        if(key==KeyBack){app->runtime_pressed=false;app->screen=ScreenSettings;app->menu_index=5;return;}
        if(key==KeyDown && row==5){app->runtime_page=(app->runtime_page+1)%pages;app->grid_focus=col;return;}
        if(key==KeyUp && row==0){app->runtime_page=app->runtime_page==0?pages-1:app->runtime_page-1;app->grid_focus=(uint8_t)(15+col);return;}
        if(key==KeyUp||key==KeyDown||key==KeyLeft||key==KeyRight) grid_move(app,key);
        return;
    }

    if(app->screen == ScreenInfo) {
        if(key == KeyBack || key == KeyOk) app->screen = app->info_return;
        return;
    }

    if(app->screen == ScreenAbout) {
        if(key == KeyBack || key == KeyOk) {
            app->screen = ScreenSettings;
            app->menu_index = 4;
        }
        return;
    }

    if(app->screen == ScreenSettings) {
        if(key == KeyBack) { app->screen = ScreenHome; return; }
        if(key == KeyUp || key == KeyDown) { menu_move(app, 6, key); return; }
        if(key != KeyOk) return;
        switch(app->menu_index) {
        case 0: app->screen = ScreenRemoteManager; app->menu_index = 0; break;
        case 1: app->screen = ScreenFolderManager; app->menu_index = 0; break;
        case 2: app->screen = ScreenAppSettings; app->menu_index = 0; break;
        case 3: app->screen = ScreenData; app->menu_index = 0; break;
        case 4: app->screen = ScreenAbout; break;
        case 5: app->screen = ScreenIconDemo; app->runtime_page = 0; app->grid_focus = 0; app->runtime_pressed = false; break;
        }
        return;
    }

    if(app->screen == ScreenRemoteManager) {
        if(key == KeyBack) { app->screen = ScreenSettings; app->menu_index = 0; return; }
        if(key == KeyUp || key == KeyDown) { menu_move(app, REMOTE_COUNT, key); return; }
        if(key == KeyOk) {
            app->selected_remote = app->menu_index;
            app->screen = ScreenRemoteDetail;
            app->menu_index = 0;
        }
        return;
    }

    if(app->screen == ScreenRemoteDetail) {
        if(key == KeyBack) { app->screen = ScreenRemoteManager; app->menu_index = app->selected_remote; return; }
        if(key == KeyUp || key == KeyDown) { menu_move(app, 6, key); return; }
        if(key != KeyOk) return;
        switch(app->menu_index) {
        case 0: app->screen = ScreenRemoteGeneral; app->menu_index = 0; break;
        case 1: app->screen = ScreenLayout; app->grid_focus = 0; break;
        case 2: app->screen = ScreenControls; app->grid_focus = 0; break;
        case 3: app->screen = ScreenIrFiles; app->menu_index = 0; break;
        case 4: app->screen = ScreenConfirmDuplicate; app->menu_index = 1; break;
        case 5: app->screen = ScreenConfirmDelete; app->menu_index = 1; break;
        }
        return;
    }

    if(app->screen == ScreenRemoteGeneral) {
        if(key == KeyBack) { app->screen = ScreenRemoteDetail; app->menu_index = 0; return; }
        if(key == KeyUp || key == KeyDown) { menu_move(app, 3, key); return; }
        if(app->menu_index == 0 && key == KeyOk) {
            show_info(app, ScreenRemoteGeneral, "RENAME", "TEXT INPUT LATER");
        } else if(app->menu_index == 1 && key == KeyOk) {
            app->remotes[app->selected_remote].favourite = !app->remotes[app->selected_remote].favourite;
        } else if(app->menu_index == 2 && (key == KeyOk || key == KeyLeft || key == KeyRight)) {
            uint8_t* folder = &app->remotes[app->selected_remote].folder;
            if(key == KeyLeft) *folder = *folder <= 1 ? 3 : *folder - 1;
            else *folder = *folder >= 3 ? 1 : *folder + 1;
        }
        return;
    }

    if(app->screen == ScreenFolderManager) {
        if(key == KeyBack) { app->screen = ScreenSettings; app->menu_index = 1; return; }
        if(key == KeyUp || key == KeyDown) { menu_move(app, 4, key); return; }
        if(key == KeyOk) show_info(app, ScreenFolderManager, "FOLDERS", "EDITOR LATER");
        return;
    }

    if(app->screen == ScreenAppSettings) {
        if(key == KeyBack) { app->screen = ScreenSettings; app->menu_index = 2; return; }
        if(key == KeyUp || key == KeyDown) menu_move(app, 3, key);
        return;
    }

    if(app->screen == ScreenData) {
        if(key == KeyBack) { app->screen = ScreenSettings; app->menu_index = 3; return; }
        if(key == KeyUp || key == KeyDown) { menu_move(app, 4, key); return; }
        if(key == KeyOk) show_info(app, ScreenData, "DATA", "MODULE LATER");
        return;
    }

    if(app->screen == ScreenControlField) {
        if(key == KeyBack) { app->screen = ScreenControls; return; }
        if(key == KeyUp || key == KeyDown) { menu_move(app, 3, key); return; }
        if(key == KeyOk) {
            if(app->menu_index == 2) {
                show_info(app, ScreenControlField, "CONTROL", "MAPPING CLEARED");
            } else {
                app->control_field = app->menu_index;
                app->screen = ScreenMapAction;
                app->menu_index = 0;
            }
        }
        return;
    }

    if(app->screen == ScreenMapAction) {
        if(key == KeyBack) { app->screen = ScreenControlField; app->menu_index = app->control_field; return; }
        if(key == KeyUp || key == KeyDown) { menu_move(app, 2, key); return; }
        if(key == KeyOk) {
            if(app->menu_index == 0) { app->screen = ScreenSignalList; app->menu_index = 0; }
            else show_info(app, ScreenMapAction, "MAP CONTROL", "SET TO NONE");
        }
        return;
    }

    if(app->screen == ScreenSignalList) {
        if(key == KeyBack) { app->screen = ScreenMapAction; app->menu_index = 0; return; }
        if(key == KeyUp || key == KeyDown) { menu_move(app, 7, key); return; }
        if(key == KeyOk) show_info(app, ScreenSignalList, "IR SIGNAL", "MAPPING PREVIEW");
        return;
    }

    if(app->screen == ScreenIrFiles) {
        if(key == KeyBack) { app->screen = ScreenRemoteDetail; app->menu_index = 3; return; }
        if(key == KeyUp || key == KeyDown) { menu_move(app, 3, key); return; }
        if(key == KeyOk) {
            if(app->menu_index == 0) { app->screen = ScreenIrImport; app->menu_index = 0; }
            else show_info(app, ScreenIrFiles, "IR FILE", "COPIED IN REMOTE");
        }
        return;
    }

    if(app->screen == ScreenIrImport) {
        if(key == KeyBack) { app->screen = ScreenIrFiles; app->menu_index = 0; return; }
        if(key == KeyUp || key == KeyDown) { menu_move(app, 3, key); return; }
        if(key == KeyOk) {
            if(app->menu_index == 2) { app->screen = ScreenIrFiles; app->menu_index = 0; }
            else show_info(app, ScreenIrImport, "IMPORT IR", "COPY TO REMOTE");
        }
        return;
    }

    if(app->screen == ScreenConfirmDuplicate || app->screen == ScreenConfirmDelete) {
        Screen confirm_screen = app->screen;
        if(key == KeyBack) { app->screen = ScreenRemoteDetail; return; }
        if(key == KeyUp || key == KeyDown) { menu_move(app, 2, key); return; }
        if(key == KeyOk) {
            if(app->menu_index == 0) {
                show_info(app, ScreenRemoteDetail,
                    confirm_screen == ScreenConfirmDuplicate ? "DUPLICATE" : "DELETE",
                    confirm_screen == ScreenConfirmDuplicate ? "PREVIEW ONLY" : "NOT DELETED");
            } else {
                app->screen = ScreenRemoteDetail;
            }
        }
    }
}

static void init_remotes(App* app) {
    strcpy(app->remotes[0].name, "LIVING TV");
    app->remotes[0].favourite = true;
    app->remotes[0].folder = 1;
    app->remotes[0].kind = RemoteTv;

    strcpy(app->remotes[1].name, "DAIKIN AC");
    app->remotes[1].favourite = true;
    app->remotes[1].folder = 1;
    app->remotes[1].kind = RemoteAc;

    strcpy(app->remotes[2].name, "PROJECTOR");
    app->remotes[2].favourite = true;
    app->remotes[2].folder = 2;
    app->remotes[2].kind = RemoteProjector;

    strcpy(app->remotes[3].name, "BEDROOM TV");
    app->remotes[3].favourite = false;
    app->remotes[3].folder = 2;
    app->remotes[3].kind = RemoteTv;

    strcpy(app->remotes[4].name, "UNTITLED");
    app->remotes[4].favourite = false;
    app->remotes[4].folder = 3;
    app->remotes[4].kind = RemoteGeneric;
}

int32_t uni_remote_app(void* p) {
    UNUSED(p);

    App* app = calloc(1, sizeof(App));
    if(!app) return -1;

    init_remotes(app);
    app->queue = furi_message_queue_alloc(INPUT_QUEUE_SIZE, sizeof(InputEvent));
    app->viewport = view_port_alloc();
    app->gui = furi_record_open(RECORD_GUI);
    app->running = app->queue && app->viewport && app->gui;
    app->screen = ScreenHome;

    if(app->running) {
        view_port_draw_callback_set(app->viewport, draw_callback, app);
        view_port_input_callback_set(app->viewport, input_callback, app->queue);
        gui_add_view_port(app->gui, app->viewport, GuiLayerFullscreen);

        InputEvent event;
        while(app->running) {
            if(furi_message_queue_get(app->queue, &event, FuriWaitForever) != FuriStatusOk) continue;

            UiKey key = map_key(event.key);

            /*
             * Remote buttons act on press, not release. For this UI-only build,
             * holding OK keeps the focused button visually depressed and shows TX.
             * A future IR core can transmit once on Press and repeat only for
             * commands explicitly marked repeatable.
             */
            if((app->screen == ScreenRuntime || app->screen == ScreenIconDemo) && key == KeyOk) {
                if(event.type == InputTypePress) {
                    app->runtime_pressed = true;
                } else if(event.type == InputTypeRelease) {
                    app->runtime_pressed = false;
                }
            }

            if(key == KeyBack && event.type == InputTypeLong) {
                app->running = false;
            } else if(event.type == InputTypeShort) {
                handle_short(app, key);
            }
            view_port_update(app->viewport);
        }

        gui_remove_view_port(app->gui, app->viewport);
    }

    if(app->gui) furi_record_close(RECORD_GUI);
    if(app->viewport) view_port_free(app->viewport);
    if(app->queue) furi_message_queue_free(app->queue);
    free(app);
    return 0;
}
