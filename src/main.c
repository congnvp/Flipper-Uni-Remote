#include <furi.h>
#include <gui/gui.h>
#include <gui/canvas.h>
#include <input/input.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
    uint8_t control_field;
    bool layout_move;

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

static void frame(Canvas* canvas, int16_t x, int16_t y, int16_t w, int16_t h, bool active) {
    if(w < 3 || h < 3) return;
    if(active) fill_rect(canvas, x, y, w, h, ColorBlack);
    hline(canvas, x + 1, y, w - 2, ColorBlack);
    hline(canvas, x + 1, y + h - 1, w - 2, ColorBlack);
    vline(canvas, x, y + 1, h - 2, ColorBlack);
    vline(canvas, x + w - 1, y + 1, h - 2, ColorBlack);
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
            int16_t n = direction == KeyLeft ? col : 4 - col;
            int16_t span = 1 + 2 * n;
            vline(canvas, cx - 2 + col, cy - span / 2, span, color);
        }
    }
}

static void draw_header(Canvas* canvas, const char* title) {
    frame(canvas, 1, 1, 62, 18, false);
    text_center3(canvas, title, 32, 7, ColorBlack);
}

static void draw_row(Canvas* canvas, int16_t y, const char* label, const char* value, bool active) {
    frame(canvas, 3, y, 58, 17, active);
    Color c = active ? ColorWhite : ColorBlack;
    text3(canvas, label, 7, y + 6, c);
    if(value && value[0]) {
        int16_t tw = text_width3(value);
        text3(canvas, value, 57 - tw, y + 6, c);
    }
}

static size_t scroll_start(size_t selected, size_t count) {
    if(count <= 5) return 0;
    if(selected < 2) return 0;
    if(selected + 3 >= count) return count - 5;
    return selected - 2;
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
    frame(canvas, 1, 1, 62, 18, false);
    triangle(canvas, 6, 10, KeyLeft, ColorBlack);
    triangle(canvas, 58, 10, KeyRight, ColorBlack);
    text_center3(canvas, page_names[app->home_page], 32, 7, ColorBlack);

    uint8_t count = page_remote_count(app, app->home_page);
    for(uint8_t row = 0; row < count && row < 4; row++) {
        uint8_t remote = page_remote_at(app, app->home_page, row);
        draw_row(canvas, 24 + row * 20, app->remotes[remote].name, "", app->home_row == row);
    }
    if(count == 0) text_center3(canvas, "EMPTY", 32, 55, ColorBlack);

    draw_row(canvas, 107, "SETTINGS", "", app->home_row == count);
}

static void draw_menu(Canvas* canvas, const char* title, const char* const* labels, const char* const* values, size_t count, size_t selected) {
    draw_header(canvas, title);
    size_t start = scroll_start(selected, count);
    for(size_t row = 0; row < 5 && start + row < count; row++) {
        size_t i = start + row;
        draw_row(canvas, 24 + (int16_t)row * 19, labels[i], values ? values[i] : "", i == selected);
    }
}

static void draw_settings(Canvas* canvas, const App* app) {
    static const char* labels[] = {"REMOTE MANAGER","FOLDER MANAGER","APP SETTINGS","DATA","ABOUT"};
    draw_menu(canvas, "SETTINGS", labels, NULL, 5, app->menu_index);
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
    text_center3(canvas, "6X3 GRID", 32, 84, ColorBlack);
    text_center3(canvas, "BACK TO RETURN", 32, 111, ColorBlack);
}

static void draw_grid(Canvas* canvas, const App* app, bool editor) {
    static const uint8_t gx[] = {0,21,42,64};
    static const uint8_t gy[] = {0,21,42,64,85,106,128};
    const char** labels = grid_for_remote(app);

    for(uint8_t row = 0; row < 6; row++) {
        for(uint8_t col = 0; col < 3; col++) {
            uint8_t i = row * 3 + col;
            int16_t x = gx[col];
            int16_t y = gy[row];
            int16_t w = gx[col + 1] - x;
            int16_t h = gy[row + 1] - y;
            bool active = i == app->grid_focus;
            frame(canvas, x + 1, y + 1, w - 2, h - 2, active);
            Color c = active ? ColorWhite : ColorBlack;
            text_center3(canvas, labels[i], x + w / 2, y + h / 2 - 2, c);
        }
    }

    if(editor) {
        text3(canvas, app->layout_move ? "MV" : "ED", 2, 2, ColorBlack);
    }
}

static void draw_controls(Canvas* canvas, const App* app) {
    draw_grid(canvas, app, false);
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
        if(key == KeyBack) app->screen = ScreenHome;
        else if(key == KeyUp || key == KeyDown || key == KeyLeft || key == KeyRight) grid_move(app, key);
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
        if(key == KeyUp || key == KeyDown) { menu_move(app, 5, key); return; }
        if(key != KeyOk) return;
        switch(app->menu_index) {
        case 0: app->screen = ScreenRemoteManager; app->menu_index = 0; break;
        case 1: app->screen = ScreenFolderManager; app->menu_index = 0; break;
        case 2: app->screen = ScreenAppSettings; app->menu_index = 0; break;
        case 3: app->screen = ScreenData; app->menu_index = 0; break;
        case 4: app->screen = ScreenAbout; break;
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
