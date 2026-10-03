#include "action_engine.h"
#include "bt_transport.h"
#include "controller.h"
#include "editor_model.h"
#include "icon_library.h"
#include "ir_transport.h"
#include "layout_library.h"
#include "remote_store.h"
#include "settings.h"
#include "state_engine.h"
#include "ui.h"

#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <storage/storage.h>
#include <string.h>

#define UNI_INPUT_QUEUE_SIZE 8
#define UNI_POLL_MS 20U

typedef struct {
    FuriMessageQueue* input_queue;
    ViewPort* view_port;
    Gui* gui;
    Storage* storage;
    UniIrTransport* ir;
    UniBtTransport* bt;
    UniActionEngine actions;
    UniStateEngine state_engine;
    UniRemoteStore store;
    UniSettings settings;
    UniController controller;
    UniUiState ui;
    UniUiPage menu_return_page;
    bool running;
    bool repeat_enabled;
} UniApp;

static void uni_draw_callback(Canvas* canvas, void* context) {
    UniApp* app = context;
    if(app->ui.page == UniUiLayoutEditor) {
        app->ui.remote_page = app->ui.layout_page;
        app->ui.focus_index = app->ui.layout_element;
        app->ui.dpad_captured = false;
        app->ui.dpad_alt = false;
        app->ui.dpad_hold_key = UniKeyUnknown;
    } else {
        app->ui.remote_page = app->controller.page_index;
        app->ui.focus_index = app->controller.focus_index;
        app->ui.dpad_captured = app->controller.dpad_captured;
        app->ui.dpad_alt = app->controller.dpad_alt;
        app->ui.dpad_hold_key = app->controller.dpad_hold_key;
    }
    uni_ui_draw(canvas, &app->ui);
}

static void uni_input_callback(InputEvent* event, void* context) {
    FuriMessageQueue* queue = context;
    furi_message_queue_put(queue, event, 0);
}

static const UniRemote* selected_remote(const UniApp* app) {
    return uni_remote_store_get(&app->store, app->ui.selected_remote);
}

static UniRemote* selected_remote_mut(UniApp* app) {
    return uni_remote_store_get_mut(&app->store, app->ui.selected_remote);
}

static bool remote_id_matches(const UniRemoteStore* store, size_t index, const char* id) {
    const UniRemote* remote = uni_remote_store_get(store, index);
    return remote && id && id[0] && strcmp(remote->id, id) == 0;
}

static void remember_runtime(UniApp* app) {
    if(!app || !app->storage || !app->ui.remote) return;
    snprintf(
        app->settings.last_remote,
        sizeof(app->settings.last_remote),
        "%s",
        app->ui.remote->id);
    app->settings.last_page = app->controller.page_index;
    app->settings.last_focus = app->controller.focus_index;
    uni_settings_save(app->storage, &app->settings);
}

static void sync_home_category(UniApp* app) {
    const size_t categories = uni_remote_store_category_count(&app->store);
    if(categories == 0) {
        app->ui.home_category = 0;
        return;
    }
    app->ui.home_category =
        uni_remote_store_category_for_remote(&app->store, app->ui.selected_remote);
    if(app->ui.home_category >= categories) app->ui.home_category = 0;
}

static void load_actions_for_selected(UniApp* app) {
    const UniRemote* remote = selected_remote(app);
    if(remote) uni_action_engine_load(&app->actions, remote);
}

static bool prepare_remote_transport(UniApp* app) {
    if(!app || !app->ui.remote) return false;

    app->ui.tx_ok = true;
    app->ui.last_signal[0] = '\0';

    if(app->ui.remote->transport == UniTransportInfrared) {
        return true;
    }

    if(app->ui.remote->transport == UniTransportStatefulIr) {
        app->ui.tx_ok = uni_state_engine_load(&app->state_engine, app->ui.remote);
        if(app->ui.tx_ok) {
            uni_state_engine_summary(
                &app->state_engine,
                app->ui.last_signal,
                sizeof(app->ui.last_signal));
        } else {
            snprintf(
                app->ui.last_signal,
                sizeof(app->ui.last_signal),
                "STATE ERR");
        }
        return app->ui.tx_ok;
    }

    if(app->ui.remote->transport == UniTransportBluetoothHid) {
        app->ui.tx_ok = app->bt && uni_bt_transport_activate(app->bt, app->ui.remote);
        snprintf(
            app->ui.last_signal,
            sizeof(app->ui.last_signal),
            "%s",
            app->ui.tx_ok ? "PAIRING" : "BT ERROR");
        return app->ui.tx_ok;
    }

    app->ui.tx_ok = false;
    snprintf(
        app->ui.last_signal,
        sizeof(app->ui.last_signal),
        "NO TRANS");
    return false;
}

static void refresh_remote_pointer(UniApp* app) {
    if(app->ui.remote) app->ui.remote = selected_remote(app);
    load_actions_for_selected(app);
}

static size_t first_element_on_page(const UniRemote* remote, uint8_t page) {
    if(!remote || !remote->elements) return 0;
    for(size_t i = 0; i < remote->element_count; i++) {
        if(remote->elements[i].page == page) return i;
    }
    return remote->element_count;
}

static void open_remote(UniApp* app) {
    if(!uni_remote_store_load_details(&app->store, app->ui.selected_remote)) return;
    app->ui.remote = selected_remote(app);
    if(!app->ui.remote || !app->ui.remote->elements_loaded) return;
    load_actions_for_selected(app);
    app->ui.page = UniUiRemote;
    prepare_remote_transport(app);
    uni_controller_reset(&app->controller, app->ui.remote);
    if(strcmp(app->settings.last_remote, app->ui.remote->id) == 0 &&
       app->settings.last_page < app->ui.remote->page_count &&
       app->settings.last_focus < app->ui.remote->element_count) {
        const size_t focus = app->settings.last_focus;
        const UniElement* element = &app->ui.remote->elements[focus];
        if(element->page == app->settings.last_page && uni_element_focusable(element)) {
            app->controller.page_index = (uint8_t)app->settings.last_page;
            app->controller.focus_index = focus;
            app->controller.dpad_captured = false;
            app->controller.dpad_alt = false;
        }
    }
}

static void open_menu(UniApp* app, UniUiPage return_page) {
    app->menu_return_page = return_page;
    app->ui.page = UniUiMenu;
    app->ui.menu_index = 0;
}

static void close_menu(UniApp* app) {
    app->ui.page = app->menu_return_page;
    if(app->ui.page == UniUiHome) app->ui.remote = NULL;
}

static void system_escape(UniApp* app) {
    if(app->ui.page == UniUiHome) {
        app->running = false;
    } else {
        remember_runtime(app);
        uni_state_engine_flush(&app->state_engine);
        uni_state_engine_unload(&app->state_engine);
        if(app->bt) uni_bt_transport_deactivate(app->bt);
        uni_remote_store_unload_details(&app->store, app->ui.selected_remote);
        app->ui.page = UniUiHome;
        app->ui.remote = NULL;
        app->ui.last_signal[0] = '\0';
        memset(&app->controller, 0, sizeof(app->controller));
        app->controller.dpad_hold_key = UniKeyUnknown;
        app->controller.pending_nav_key = UniKeyUnknown;
    }
}

static void open_layout_editor(UniApp* app) {
    const uint8_t preferred_page = app->controller.page_index;
    if(!uni_remote_store_load_details(&app->store, app->ui.selected_remote)) return;
    app->ui.remote = selected_remote(app);
    if(!app->ui.remote || !app->ui.remote->elements_loaded) return;
    load_actions_for_selected(app);
    uni_controller_reset(&app->controller, app->ui.remote);
    app->controller.dpad_captured = false;

    /* Layout editing includes display-only elements such as status/screen. */
    app->ui.layout_page = preferred_page < app->ui.remote->page_count ?
                              preferred_page :
                              0;
    app->ui.layout_element = first_element_on_page(app->ui.remote, app->ui.layout_page);
    if(app->ui.layout_element >= app->ui.remote->element_count) {
        app->ui.layout_page = 0;
        app->ui.layout_element = first_element_on_page(app->ui.remote, 0);
    }
    app->ui.layout_moving = false;
    app->ui.layout_replace_mode = false;
    app->ui.page = UniUiLayoutEditor;
}

static void reload_remotes(UniApp* app) {
    if(app->ui.remote) remember_runtime(app);
    uni_state_engine_flush(&app->state_engine);
    uni_state_engine_unload(&app->state_engine);
    if(app->bt) uni_bt_transport_deactivate(app->bt);
    char keep_id[UNI_ID_MAX] = {0};
    const UniRemote* current = selected_remote(app);
    if(current) snprintf(keep_id, sizeof(keep_id), "%s", current->id);

    const bool restore_remote = app->menu_return_page == UniUiRemote;

    /*
     * scan_remotes() frees the currently loaded element array. Clear every
     * UI/controller reference before scanning so the draw callback can never
     * dereference an unloaded layout.
     */
    app->ui.remote = NULL;
    app->ui.layout_element = 0;
    app->ui.layout_page = 0;
    memset(&app->controller, 0, sizeof(app->controller));
    app->controller.dpad_hold_key = UniKeyUnknown;
    app->controller.pending_nav_key = UniKeyUnknown;

    if(!uni_remote_store_reload(&app->store)) {
        app->menu_return_page = UniUiHome;
        return;
    }

    app->ui.selected_remote =
        keep_id[0] ? uni_remote_store_find_id(&app->store, keep_id) : 0;
    if(!remote_id_matches(&app->store, app->ui.selected_remote, keep_id))
        app->ui.selected_remote = 0;
    sync_home_category(app);

    if(restore_remote) {
        if(uni_remote_store_load_details(&app->store, app->ui.selected_remote)) {
            app->ui.remote = selected_remote(app);
            load_actions_for_selected(app);
            if(app->ui.remote) {
                prepare_remote_transport(app);
                uni_controller_reset(&app->controller, app->ui.remote);
            }
        } else {
            app->menu_return_page = UniUiHome;
        }
    }
}

static void menu_move(UniUiState* ui, size_t count, UniKey key) {
    if(count == 0) return;
    if(key == UniKeyUp) ui->menu_index = ui->menu_index == 0 ? count - 1 : ui->menu_index - 1;
    else if(key == UniKeyDown) ui->menu_index = (ui->menu_index + 1) % count;
}

static const char uni_text_chars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.";

static size_t text_char_index(char ch) {
    for(size_t i = 0; uni_text_chars[i]; i++) {
        if(uni_text_chars[i] == ch) return i;
    }
    return 0;
}

static void begin_text_edit(
    UniApp* app,
    UniTextTarget target,
    const char* initial,
    size_t limit,
    UniUiPage return_page) {
    if(!app) return;
    if(limit >= sizeof(app->ui.text_buffer)) limit = sizeof(app->ui.text_buffer) - 1;

    memset(app->ui.text_buffer, ' ', sizeof(app->ui.text_buffer));
    app->ui.text_buffer[limit] = '\0';
    if(initial) {
        for(size_t i = 0; i < limit && initial[i]; i++) {
            char ch = initial[i];
            if(ch >= 'a' && ch <= 'z') ch = (char)(ch - ('a' - 'A'));
            app->ui.text_buffer[i] = strchr(uni_text_chars, ch) ? ch : ' ';
        }
    }

    app->ui.text_target = target;
    app->ui.text_limit = limit;
    app->ui.text_cursor = 0;
    app->ui.text_return_page = return_page;
    app->ui.page = UniUiTextEdit;
}

static void text_edit_cycle(UniUiState* ui, int direction) {
    if(!ui || ui->text_cursor >= ui->text_limit) return;
    const size_t count = strlen(uni_text_chars);
    size_t index = text_char_index(ui->text_buffer[ui->text_cursor]);
    if(direction < 0) index = index == 0 ? count - 1 : index - 1;
    else index = (index + 1) % count;
    ui->text_buffer[ui->text_cursor] = uni_text_chars[index];
}

static void text_edit_value(const UniUiState* ui, char* out, size_t out_size) {
    if(!ui || !out || out_size == 0) return;
    size_t begin = 0;
    size_t end = ui->text_limit;
    while(begin < end && ui->text_buffer[begin] == ' ') begin++;
    while(end > begin && ui->text_buffer[end - 1] == ' ') end--;

    const size_t length = end - begin < out_size - 1 ? end - begin : out_size - 1;
    memcpy(out, ui->text_buffer + begin, length);
    out[length] = '\0';
}

static size_t sequence_count(const UniActionCatalog* catalog) {
    size_t count = 0;
    for(size_t i = 0; i < catalog->count; i++) {
        if(catalog->actions[i].type == UniActionSequence) count++;
    }
    return count;
}

static const UniNamedAction* sequence_at(const UniActionCatalog* catalog, size_t index) {
    size_t current = 0;
    for(size_t i = 0; i < catalog->count; i++) {
        if(catalog->actions[i].type != UniActionSequence) continue;
        if(current++ == index) return &catalog->actions[i];
    }
    return NULL;
}

static void dispatch_remote_action(UniApp* app) {
    if(!app->controller.action_ready || !app->ui.remote) return;

    app->ui.press_flash_index = app->controller.focus_index;
    app->ui.press_flash_until = furi_get_tick() + furi_ms_to_ticks(120);

    if(app->ui.remote->transport == UniTransportInfrared) {
        snprintf(
            app->ui.last_signal,
            sizeof(app->ui.last_signal),
            "%s",
            app->controller.binding);
        app->ui.tx_ok = uni_action_engine_execute(
            &app->actions,
            app->ui.remote,
            app->controller.binding,
            app->controller.repeat);
    } else if(app->ui.remote->transport == UniTransportStatefulIr) {
        app->ui.tx_ok = uni_state_engine_execute(
            &app->state_engine,
            app->ui.remote,
            app->controller.binding,
            app->controller.repeat);
        if(app->ui.tx_ok)
            uni_state_engine_summary(
                &app->state_engine,
                app->ui.last_signal,
                sizeof(app->ui.last_signal));
    } else if(app->ui.remote->transport == UniTransportBluetoothHid) {
        app->ui.tx_ok = app->bt &&
                        uni_bt_transport_send(
                            app->bt,
                            app->ui.remote,
                            app->controller.binding,
                            app->controller.repeat);
        snprintf(
            app->ui.last_signal,
            sizeof(app->ui.last_signal),
            "%s",
            app->bt && uni_bt_transport_connected(app->bt) ? "CONNECTED" : "PAIRING");
    } else {
        app->ui.tx_ok = false;
    }
    if(app->ui.tx_ok) {
        app->ui.tx_flash_until = furi_get_tick() + furi_ms_to_ticks(250);
    }
    app->controller.action_ready = false;
}

static void handle_home(UniApp* app, const InputEvent* event, UniKey key) {
    const size_t categories = uni_remote_store_category_count(&app->store);
    if(key == UniKeyBack && event->type == InputTypeShort) {
        open_menu(app, UniUiHome);
        return;
    }
    if(event->type != InputTypeShort || categories == 0) return;

    if(key == UniKeyLeft || key == UniKeyRight) {
        if(key == UniKeyLeft) {
            app->ui.home_category =
                app->ui.home_category == 0 ? categories - 1 : app->ui.home_category - 1;
        } else {
            app->ui.home_category = (app->ui.home_category + 1) % categories;
        }
        const size_t first =
            uni_remote_store_category_remote_at(&app->store, app->ui.home_category, 0);
        if(first < uni_remote_store_count(&app->store)) app->ui.selected_remote = first;
        return;
    }

    const size_t count =
        uni_remote_store_category_remote_count(&app->store, app->ui.home_category);
    if(count == 0) return;
    size_t position =
        uni_remote_store_category_position(
            &app->store, app->ui.home_category, app->ui.selected_remote);

    if(key == UniKeyUp) {
        position = position == 0 ? count - 1 : position - 1;
        app->ui.selected_remote =
            uni_remote_store_category_remote_at(&app->store, app->ui.home_category, position);
    } else if(key == UniKeyDown) {
        position = (position + 1) % count;
        app->ui.selected_remote =
            uni_remote_store_category_remote_at(&app->store, app->ui.home_category, position);
    } else if(key == UniKeyOk) {
        open_remote(app);
    }
}

static void handle_remote(UniApp* app, const InputEvent* event, UniKey key) {
    if(key == UniKeyBack && event->type == InputTypeShort && !app->controller.dpad_captured) {
        open_menu(app, UniUiRemote);
        return;
    }

    uni_controller_handle(
        &app->controller,
        app->ui.remote,
        key,
        event->type,
        app->repeat_enabled);
    dispatch_remote_action(app);
}

static void handle_menu(UniApp* app, const InputEvent* event, UniKey key) {
    if(key == UniKeyBack && event->type == InputTypeShort) {
        close_menu(app);
        return;
    }
    if(event->type != InputTypeShort) return;

    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, 5, key);
        return;
    }
    if(key != UniKeyOk) return;

    switch(app->ui.menu_index) {
    case 0:
        app->ui.page = UniUiGlobalSettings;
        app->ui.menu_index = 0;
        break;
    case 1:
        app->ui.page = UniUiRemoteSettings;
        app->ui.menu_index = 0;
        break;
    case 2:
        open_layout_editor(app);
        break;
    case 3:
        reload_remotes(app);
        break;
    case 4:
        close_menu(app);
        break;
    }
}

static void select_default_remote(UniApp* app, int direction) {
    const size_t count = uni_remote_store_count(&app->store);
    if(count == 0) return;
    size_t index = uni_remote_store_find_id(&app->store, app->settings.default_remote);
    if(direction < 0) index = index == 0 ? count - 1 : index - 1;
    else if(direction > 0) index = (index + 1) % count;
    else index = app->ui.selected_remote;

    const UniRemote* remote = uni_remote_store_get(&app->store, index);
    if(remote) {
        snprintf(
            app->settings.default_remote,
            sizeof(app->settings.default_remote),
            "%s",
            remote->id);
    }
}

static void save_global(UniApp* app) {
    app->repeat_enabled = app->settings.repeat_enabled;
    uni_settings_save(app->storage, &app->settings);
}

static void handle_global(UniApp* app, const InputEvent* event, UniKey key) {
    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = UniUiMenu;
        app->ui.menu_index = 0;
        return;
    }
    if(event->type != InputTypeShort) return;
    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, 4, key);
        return;
    }

    if(app->ui.menu_index == 3 && key == UniKeyOk) {
        app->ui.page = UniUiMenu;
        app->ui.menu_index = 0;
    } else if(app->ui.menu_index == 0 &&
              (key == UniKeyLeft || key == UniKeyRight || key == UniKeyOk)) {
        app->settings.repeat_enabled = !app->settings.repeat_enabled;
        save_global(app);
    } else if(app->ui.menu_index == 1 &&
              (key == UniKeyLeft || key == UniKeyRight || key == UniKeyOk)) {
        app->settings.open_default = !app->settings.open_default;
        save_global(app);
    } else if(app->ui.menu_index == 2) {
        if(key == UniKeyLeft) select_default_remote(app, -1);
        else if(key == UniKeyRight) select_default_remote(app, 1);
        else if(key == UniKeyOk) select_default_remote(app, 0);
        else return;
        save_global(app);
    }
}

static void cycle_remote_folder(UniApp* app, int direction) {
    UniRemote* remote = selected_remote_mut(app);
    if(!remote || direction == 0) return;

    const size_t folder_count = uni_remote_store_folder_count(&app->store);
    size_t current = 0;
    bool found = !remote->folder[0];
    if(remote->folder[0]) {
        for(size_t i = 0; i < folder_count; i++) {
            char name[UNI_FOLDER_MAX] = {0};
            if(uni_remote_store_folder_name(&app->store, i, name, sizeof(name)) &&
               strcmp(name, remote->folder) == 0) {
                current = i + 1;
                found = true;
                break;
            }
        }
    }
    if(!found) current = 0;

    const size_t option_count = folder_count + 1;
    if(direction < 0) current = current == 0 ? option_count - 1 : current - 1;
    else current = (current + 1) % option_count;

    char folder[UNI_FOLDER_MAX] = {0};
    if(current > 0)
        uni_remote_store_folder_name(&app->store, current - 1, folder, sizeof(folder));
    uni_remote_store_set_folder(&app->store, app->ui.selected_remote, folder);
    refresh_remote_pointer(app);
    sync_home_category(app);
}

static void handle_remote_settings(UniApp* app, const InputEvent* event, UniKey key) {
    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = UniUiMenu;
        app->ui.menu_index = 0;
        return;
    }
    if(event->type != InputTypeShort) return;

    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, 8, key);
        return;
    }

    UniRemote* remote = selected_remote_mut(app);
    if(!remote) return;
    if(key != UniKeyOk && key != UniKeyLeft && key != UniKeyRight) return;

    switch(app->ui.menu_index) {
    case 0:
        uni_remote_store_set_repeat(
            &app->store,
            app->ui.selected_remote,
            !remote->repeat_enabled);
        refresh_remote_pointer(app);
        break;
    case 1:
        uni_remote_store_set_favorite(
            &app->store,
            app->ui.selected_remote,
            !remote->favorite);
        refresh_remote_pointer(app);
        sync_home_category(app);
        break;
    case 2:
        if(key == UniKeyLeft) cycle_remote_folder(app, -1);
        else if(key == UniKeyRight) cycle_remote_folder(app, 1);
        else if(key == UniKeyOk) {
            begin_text_edit(
                app,
                UniTextFolder,
                remote->folder,
                UNI_FOLDER_MAX - 1,
                UniUiRemoteSettings);
        }
        break;
    case 3:
        if(key == UniKeyOk) {
            snprintf(
                app->settings.default_remote,
                sizeof(app->settings.default_remote),
                "%s",
                remote->id);
            save_global(app);
        }
        break;
    case 4:
        if(key == UniKeyOk) open_layout_editor(app);
        break;
    case 5:
        if(key == UniKeyOk) {
            if(!uni_remote_store_load_details(&app->store, app->ui.selected_remote)) break;
            app->ui.remote = selected_remote(app);
            load_actions_for_selected(app);
            app->ui.page = UniUiKeymap;
            app->ui.menu_index = 0;
        }
        break;
    case 6:
        break;
    case 7:
        if(key == UniKeyOk) {
            sync_home_category(app);
            app->ui.page = UniUiMenu;
            app->ui.menu_index = 0;
        }
        break;
    }
}

static bool layout_select(UniApp* app, UniKey key) {
    if(!app->ui.remote || !app->ui.remote->elements ||
       app->ui.remote->element_count == 0 ||
       app->ui.layout_element >= app->ui.remote->element_count) {
        return false;
    }

    int8_t dx = 0;
    int8_t dy = 0;
    if(key == UniKeyLeft) dx = -1;
    else if(key == UniKeyRight) dx = 1;
    else if(key == UniKeyUp) dy = -1;
    else if(key == UniKeyDown) dy = 1;
    else return false;

    const UniElement* current = &app->ui.remote->elements[app->ui.layout_element];
    int best_score = 10000;
    size_t best_index = app->ui.layout_element;

    /*
     * Layout Editor navigates all elements by occupied cells, not bounding
     * rectangles. This makes the four D-pad corner slots independently selectable.
     */
    for(size_t i = 0; i < app->ui.remote->element_count; i++) {
        if(i == app->ui.layout_element) continue;
        const UniElement* candidate = &app->ui.remote->elements[i];
        if(candidate->page != app->ui.layout_page) continue;
        const int score = uni_element_direction_score(current, candidate, dx, dy);
        if(score >= 0 && score < best_score) {
            best_score = score;
            best_index = i;
        }
    }

    if(best_index == app->ui.layout_element) return false;
    app->ui.layout_element = best_index;
    return true;
}

static bool layout_change_page(UniApp* app, int8_t delta) {
    const UniRemote* remote = app->ui.remote;
    if(!remote || remote->page_count <= 1 || delta == 0) return false;

    const uint8_t count = remote->page_count;
    for(uint8_t attempt = 0; attempt < count; attempt++) {
        int next = (int)app->ui.layout_page + delta;
        if(next < 0) next += count;
        if(next >= count) next -= count;
        app->ui.layout_page = (uint8_t)next;

        const size_t first = first_element_on_page(remote, app->ui.layout_page);
        if(first < remote->element_count) {
            app->ui.layout_element = first;
            app->ui.layout_moving = false;
            return true;
        }
    }
    return false;
}

static void layout_move(UniApp* app, UniKey key) {
    int8_t dx = 0, dy = 0;
    if(key == UniKeyLeft) dx = -1;
    else if(key == UniKeyRight) dx = 1;
    else if(key == UniKeyUp) dy = -1;
    else if(key == UniKeyDown) dy = 1;
    else return;

    if(uni_remote_store_move_element(
           &app->store,
           app->ui.selected_remote,
           app->ui.layout_element,
           dx,
           dy)) {
        refresh_remote_pointer(app);
    }
}

static void handle_layout_editor(UniApp* app, const InputEvent* event, UniKey key) {
    if(key == UniKeyBack && event->type == InputTypeShort) {
        if(app->ui.layout_moving) {
            app->ui.layout_moving = false;
        } else {
            app->ui.page = UniUiLayoutTools;
            app->ui.menu_index = 0;
        }
        return;
    }
    if(key == UniKeyOk && event->type == InputTypeShort) {
        app->ui.layout_moving = !app->ui.layout_moving;
        return;
    }
    if(event->type != InputTypePress) return;
    if(app->ui.layout_moving) {
        layout_move(app, key);
    } else {
        const bool moved = layout_select(app, key);
        if(!moved && key == UniKeyLeft) layout_change_page(app, -1);
        else if(!moved && key == UniKeyRight) layout_change_page(app, 1);
    }
}

static void handle_layout_tools(UniApp* app, const InputEvent* event, UniKey key) {
    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = UniUiLayoutEditor;
        return;
    }
    if(event->type != InputTypeShort) return;
    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, 9, key);
        return;
    }
    if(key != UniKeyOk) return;

    UniRemote* remote = selected_remote_mut(app);
    if(!remote) return;

    switch(app->ui.menu_index) {
    case 0: /* ADD */
        app->ui.layout_replace_mode = false;
        app->ui.page = UniUiAddElement;
        app->ui.menu_index = 0;
        break;
    case 1: /* REPLACE selected element */
        if(remote->element_count && app->ui.layout_element < remote->element_count) {
            app->ui.layout_replace_mode = true;
            app->ui.page = UniUiAddElement;
            app->ui.menu_index = 0;
        }
        break;
    case 2: /* REMOVE */
        if(remote->element_count && app->ui.layout_element < remote->element_count) {
            if(uni_remote_store_remove_element(
                   &app->store,
                   app->ui.selected_remote,
                   app->ui.layout_element)) {
                refresh_remote_pointer(app);
                remote = selected_remote_mut(app);
                if(remote && remote->element_count) {
                    if(app->ui.layout_element >= remote->element_count)
                        app->ui.layout_element = remote->element_count - 1;
                } else {
                    app->ui.layout_element = 0;
                }
            }
        }
        app->ui.page = UniUiLayoutEditor;
        break;
    case 3: /* MAP */
        if(remote->element_count && app->ui.layout_element < remote->element_count &&
           uni_editor_binding_count(&remote->elements[app->ui.layout_element]) > 0) {
            load_actions_for_selected(app);
            app->ui.map_target = UniMapElement;
            app->ui.page = UniUiMapField;
            app->ui.menu_index = 0;
        }
        break;
    case 4: /* ICON */
        if(remote->element_count && app->ui.layout_element < remote->element_count &&
           uni_editor_icon_count(&remote->elements[app->ui.layout_element]) > 0) {
            app->ui.page = UniUiIconField;
            app->ui.menu_index = 0;
        }
        break;
    case 5: /* LABEL */
        if(remote->element_count && app->ui.layout_element < remote->element_count) {
            begin_text_edit(
                app,
                UniTextLabel,
                remote->elements[app->ui.layout_element].label,
                UNI_LABEL_MAX - 1,
                UniUiLayoutTools);
        }
        break;
    case 6: /* PAGE */
        if(remote->element_count && app->ui.layout_element < remote->element_count) {
            app->ui.page = UniUiPagePick;
            app->ui.menu_index = remote->elements[app->ui.layout_element].page;
        }
        break;
    case 7: /* TEMPLATE */
        app->ui.page = UniUiLayoutPreset;
        app->ui.menu_index = 0;
        break;
    case 8: /* DONE */
        app->ui.page = UniUiMenu;
        app->ui.menu_index = 0;
        break;
    }
}

static void handle_add_element(UniApp* app, const InputEvent* event, UniKey key) {
    const size_t count = uni_element_preset_count();
    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = UniUiLayoutTools;
        app->ui.menu_index = app->ui.layout_replace_mode ? 1 : 0;
        return;
    }
    if(event->type != InputTypeShort) return;
    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, count, key);
        return;
    }
    if(key != UniKeyOk) return;

    size_t index = app->ui.layout_element;
    bool ok = false;

    if(app->ui.layout_replace_mode) {
        ok = uni_remote_store_replace_element(
            &app->store,
            app->ui.selected_remote,
            app->ui.layout_element,
            app->ui.menu_index,
            &index);
    } else {
        ok = uni_remote_store_add_element(
            &app->store,
            app->ui.selected_remote,
            app->ui.menu_index,
            app->ui.layout_page,
            &index);
    }

    if(ok) {
        refresh_remote_pointer(app);
        app->ui.layout_element = index;
        app->ui.page = UniUiLayoutEditor;
        app->ui.layout_moving = false;
        app->ui.layout_replace_mode = false;
    }
}

static void handle_layout_preset(UniApp* app, const InputEvent* event, UniKey key) {
    const size_t count = uni_layout_preset_count();
    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = UniUiLayoutTools;
        app->ui.menu_index = 0;
        return;
    }
    if(event->type != InputTypeShort) return;
    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, count, key);
        return;
    }
    if(key == UniKeyOk &&
       uni_remote_store_apply_layout(
           &app->store,
           app->ui.selected_remote,
           app->ui.menu_index)) {
        refresh_remote_pointer(app);
        uni_controller_reset(&app->controller, app->ui.remote);
        app->controller.dpad_captured = false;
        app->ui.layout_page = 0;
        app->ui.layout_element = first_element_on_page(app->ui.remote, 0);
        app->ui.page = UniUiLayoutEditor;
    }
}

static void handle_map_field(UniApp* app, const InputEvent* event, UniKey key) {
    const UniRemote* remote = selected_remote(app);
    const UniElement* element =
        remote && app->ui.layout_element < remote->element_count ?
            &remote->elements[app->ui.layout_element] :
            NULL;
    const size_t count = uni_editor_binding_count(element);

    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = UniUiLayoutTools;
        app->ui.menu_index = 0;
        return;
    }
    if(event->type != InputTypeShort || count == 0) return;
    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, count, key);
    } else if(key == UniKeyOk) {
        app->ui.edit_field = app->ui.menu_index;
        app->ui.map_target = UniMapElement;
        app->ui.page = UniUiMapKind;
        app->ui.menu_index = 0;
    }
}

static bool set_current_binding(UniApp* app, const char* binding) {
    if(app->ui.map_target == UniMapHardKey) {
        return uni_remote_store_set_hard_binding(
            &app->store,
            app->ui.selected_remote,
            app->ui.hard_slot,
            binding);
    }

    const UniRemote* remote = selected_remote(app);
    if(!remote || app->ui.layout_element >= remote->element_count) return false;
    const UniElement* e = &remote->elements[app->ui.layout_element];
    const char* field = uni_editor_binding_key(e, app->ui.edit_field);
    return uni_remote_store_set_binding(
        &app->store,
        app->ui.selected_remote,
        app->ui.layout_element,
        field,
        binding);
}

static void return_after_mapping(UniApp* app) {
    refresh_remote_pointer(app);
    app->ui.page = app->ui.map_target == UniMapHardKey ? UniUiKeymap : UniUiMapField;
    app->ui.menu_index = 0;
}

static void handle_map_kind(UniApp* app, const InputEvent* event, UniKey key) {
    const UniRemote* remote = selected_remote(app);
    const bool ir = !remote || remote->transport == UniTransportInfrared;
    const size_t option_count = ir ? 4 : 3;

    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = app->ui.map_target == UniMapHardKey ? UniUiKeymap : UniUiMapField;
        app->ui.menu_index = 0;
        return;
    }
    if(event->type != InputTypeShort) return;
    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, option_count, key);
        return;
    }
    if(key != UniKeyOk) return;

    if(ir) {
        if(app->ui.menu_index == 0) {
            app->ui.picker_kind = UniPickSignal;
            app->ui.page = UniUiMapPick;
            app->ui.menu_index = 0;
        } else if(app->ui.menu_index == 1) {
            app->ui.picker_kind = UniPickSequence;
            app->ui.page = UniUiMapPick;
            app->ui.menu_index = 0;
        } else if(app->ui.menu_index == 2) {
            if(set_current_binding(app, "")) return_after_mapping(app);
        } else {
            app->ui.page = app->ui.map_target == UniMapHardKey ? UniUiKeymap : UniUiMapField;
            app->ui.menu_index = 0;
        }
    } else {
        if(app->ui.menu_index == 0) {
            app->ui.picker_kind = UniPickSignal;
            app->ui.page = UniUiMapPick;
            app->ui.menu_index = 0;
        } else if(app->ui.menu_index == 1) {
            if(set_current_binding(app, "")) return_after_mapping(app);
        } else {
            app->ui.page = app->ui.map_target == UniMapHardKey ? UniUiKeymap : UniUiMapField;
            app->ui.menu_index = 0;
        }
    }
}

static void handle_map_pick(UniApp* app, const InputEvent* event, UniKey key) {
    const UniRemote* remote = selected_remote(app);
    const bool ir = !remote || remote->transport == UniTransportInfrared;
    const size_t count =
        ir ?
            (app->ui.picker_kind == UniPickSignal ?
                 app->actions.signals.count :
                 sequence_count(&app->actions.actions)) :
            uni_editor_transport_action_count(remote);

    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = UniUiMapKind;
        app->ui.menu_index = 0;
        return;
    }
    if(event->type != InputTypeShort || count == 0) return;
    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, count, key);
        return;
    }
    if(key != UniKeyOk) return;

    char binding[UNI_BINDING_MAX] = {0};
    if(!ir) {
        snprintf(
            binding,
            sizeof(binding),
            "%s",
            uni_editor_transport_action_binding(remote, app->ui.menu_index));
    } else if(app->ui.picker_kind == UniPickSignal) {
        snprintf(
            binding,
            sizeof(binding),
            "sig:%s",
            app->actions.signals.names[app->ui.menu_index]);
    } else {
        const UniNamedAction* action =
            sequence_at(&app->actions.actions, app->ui.menu_index);
        if(!action) return;
        snprintf(binding, sizeof(binding), "act:%s", action->id);
    }

    if(binding[0] && set_current_binding(app, binding)) return_after_mapping(app);
}

static void handle_icon_field(UniApp* app, const InputEvent* event, UniKey key) {
    const UniRemote* remote = selected_remote(app);
    const UniElement* element =
        remote && app->ui.layout_element < remote->element_count ?
            &remote->elements[app->ui.layout_element] :
            NULL;
    const size_t count = uni_editor_icon_count(element);

    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = UniUiLayoutTools;
        app->ui.menu_index = 0;
        return;
    }
    if(event->type != InputTypeShort || count == 0) return;
    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, count, key);
    } else if(key == UniKeyOk) {
        app->ui.edit_field = app->ui.menu_index;
        app->ui.page = UniUiIconPick;
        app->ui.menu_index = 0;
    }
}

static void handle_icon_pick(UniApp* app, const InputEvent* event, UniKey key) {
    const size_t count = uni_icon_count() + 1;
    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = UniUiIconField;
        app->ui.menu_index = 0;
        return;
    }
    if(event->type != InputTypeShort) return;
    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, count, key);
        return;
    }
    if(key != UniKeyOk) return;

    const UniRemote* remote = selected_remote(app);
    if(!remote || app->ui.layout_element >= remote->element_count) return;
    const UniElement* e = &remote->elements[app->ui.layout_element];
    const char* field = uni_editor_icon_key(e, app->ui.edit_field);
    const char* icon_id = "";
    if(app->ui.menu_index > 0) {
        const UniIconDef* icon = uni_icon_get(app->ui.menu_index - 1);
        if(icon) icon_id = icon->id;
    }

    if(uni_remote_store_set_icon(
           &app->store,
           app->ui.selected_remote,
           app->ui.layout_element,
           field,
           icon_id)) {
        refresh_remote_pointer(app);
        app->ui.page = UniUiIconField;
        app->ui.menu_index = 0;
    }
}

static void handle_text_edit(UniApp* app, const InputEvent* event, UniKey key) {
    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = app->ui.text_return_page;
        app->ui.menu_index = app->ui.text_target == UniTextFolder ? 2 : 5;
        return;
    }

    const bool step_event = event->type == InputTypePress || event->type == InputTypeRepeat;
    if(step_event) {
        if(key == UniKeyLeft && app->ui.text_cursor > 0) {
            app->ui.text_cursor--;
        } else if(key == UniKeyRight && app->ui.text_cursor + 1 < app->ui.text_limit) {
            app->ui.text_cursor++;
        } else if(key == UniKeyUp) {
            text_edit_cycle(&app->ui, 1);
        } else if(key == UniKeyDown) {
            text_edit_cycle(&app->ui, -1);
        }
        return;
    }

    if(key != UniKeyOk || event->type != InputTypeShort) return;

    char value[UNI_FOLDER_MAX] = {0};
    text_edit_value(&app->ui, value, sizeof(value));

    bool ok = false;
    if(app->ui.text_target == UniTextFolder) {
        ok = uni_remote_store_set_folder(
            &app->store,
            app->ui.selected_remote,
            value);
        if(ok) {
            refresh_remote_pointer(app);
            sync_home_category(app);
        }
    } else {
        ok = uni_remote_store_set_label(
            &app->store,
            app->ui.selected_remote,
            app->ui.layout_element,
            value);
        if(ok) refresh_remote_pointer(app);
    }

    if(ok) {
        app->ui.page = app->ui.text_return_page;
        app->ui.menu_index = app->ui.text_target == UniTextFolder ? 2 : 5;
    }
}

static void handle_page_pick(UniApp* app, const InputEvent* event, UniKey key) {
    const UniRemote* remote = selected_remote(app);
    if(!remote || app->ui.layout_element >= remote->element_count) {
        app->ui.page = UniUiLayoutTools;
        app->ui.menu_index = 6;
        return;
    }

    const size_t count =
        remote->page_count + (remote->page_count < UNI_MAX_PAGES ? 1 : 0);

    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = UniUiLayoutTools;
        app->ui.menu_index = 6;
        return;
    }
    if(event->type != InputTypeShort) return;

    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, count, key);
        return;
    }
    if(key != UniKeyOk || app->ui.menu_index >= count) return;

    const uint8_t target = (uint8_t)app->ui.menu_index;
    if(uni_remote_store_set_element_page(
           &app->store,
           app->ui.selected_remote,
           app->ui.layout_element,
           target)) {
        refresh_remote_pointer(app);
        app->ui.layout_page = target;
        app->ui.page = UniUiLayoutEditor;
        app->ui.layout_moving = false;
    }
}

static void handle_keymap(UniApp* app, const InputEvent* event, UniKey key) {
    const size_t count = UniHardCount + 1;
    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = UniUiRemoteSettings;
        app->ui.menu_index = 5;
        return;
    }
    if(event->type != InputTypeShort) return;
    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, count, key);
        return;
    }
    if(key != UniKeyOk) return;

    if(app->ui.menu_index >= UniHardCount) {
        app->ui.page = UniUiRemoteSettings;
        app->ui.menu_index = 5;
        return;
    }

    app->ui.map_target = UniMapHardKey;
    app->ui.hard_slot = (UniHardKeySlot)app->ui.menu_index;
    app->ui.page = UniUiMapKind;
    app->ui.menu_index = 0;
}

static void handle_event(UniApp* app, const InputEvent* event) {
    const UniKey key = uni_map_physical_key(event->key);

    if(key == UniKeyBack && event->type == InputTypeLong) {
        system_escape(app);
        return;
    }

    switch(app->ui.page) {
    case UniUiHome:
        handle_home(app, event, key);
        break;
    case UniUiRemote:
        handle_remote(app, event, key);
        break;
    case UniUiMenu:
        handle_menu(app, event, key);
        break;
    case UniUiGlobalSettings:
        handle_global(app, event, key);
        break;
    case UniUiRemoteSettings:
        handle_remote_settings(app, event, key);
        break;
    case UniUiLayoutEditor:
        handle_layout_editor(app, event, key);
        break;
    case UniUiLayoutTools:
        handle_layout_tools(app, event, key);
        break;
    case UniUiAddElement:
        handle_add_element(app, event, key);
        break;
    case UniUiLayoutPreset:
        handle_layout_preset(app, event, key);
        break;
    case UniUiMapField:
        handle_map_field(app, event, key);
        break;
    case UniUiMapKind:
        handle_map_kind(app, event, key);
        break;
    case UniUiMapPick:
        handle_map_pick(app, event, key);
        break;
    case UniUiIconField:
        handle_icon_field(app, event, key);
        break;
    case UniUiIconPick:
        handle_icon_pick(app, event, key);
        break;
    case UniUiKeymap:
        handle_keymap(app, event, key);
        break;
    case UniUiTextEdit:
        handle_text_edit(app, event, key);
        break;
    case UniUiPagePick:
        handle_page_pick(app, event, key);
        break;
    }
}

int32_t uni_remote_app(void* p) {
    UNUSED(p);

    UniApp* app = calloc(1, sizeof(UniApp));
    if(!app) return -1;

    app->input_queue = furi_message_queue_alloc(UNI_INPUT_QUEUE_SIZE, sizeof(InputEvent));
    app->view_port = view_port_alloc();
    app->storage = furi_record_open(RECORD_STORAGE);
    app->gui = furi_record_open(RECORD_GUI);

    const bool settings_ok =
        app->storage && uni_settings_load_or_create(app->storage, &app->settings);
    const bool store_ok =
        app->storage && uni_remote_store_init(&app->store, app->storage);
    app->ir = app->storage ? uni_ir_transport_alloc(app->storage) : NULL;
    app->bt = app->storage ? uni_bt_transport_alloc(app->storage) : NULL;
    uni_action_engine_init(&app->actions, app->storage, app->ir);
    uni_state_engine_init(&app->state_engine, app->storage);

    app->repeat_enabled = settings_ok ? app->settings.repeat_enabled : true;
    app->running = app->input_queue && app->view_port && app->gui && app->storage &&
                   app->ir && store_ok && settings_ok;

    app->ui.page = UniUiHome;
    app->ui.store = &app->store;
    app->ui.settings = &app->settings;
    app->ui.action_engine = &app->actions;
    app->ui.selected_remote = 0;
    if(settings_ok && app->settings.last_remote[0]) {
        const size_t last = uni_remote_store_find_id(&app->store, app->settings.last_remote);
        if(remote_id_matches(&app->store, last, app->settings.last_remote))
            app->ui.selected_remote = last;
    } else if(settings_ok) {
        const size_t fallback =
            uni_remote_store_find_id(&app->store, app->settings.default_remote);
        if(remote_id_matches(&app->store, fallback, app->settings.default_remote))
            app->ui.selected_remote = fallback;
    }
    sync_home_category(app);
    app->ui.tx_ok = true;
    app->ui.dpad_hold_key = UniKeyUnknown;
    app->menu_return_page = UniUiHome;

    if(app->running && app->settings.open_default) {
        const size_t default_index =
            uni_remote_store_find_id(&app->store, app->settings.default_remote);
        if(remote_id_matches(&app->store, default_index, app->settings.default_remote)) {
            app->ui.selected_remote = default_index;
            sync_home_category(app);
        }
        open_remote(app);
    }

    if(app->running) {
        view_port_draw_callback_set(app->view_port, uni_draw_callback, app);
        view_port_input_callback_set(app->view_port, uni_input_callback, app->input_queue);
        gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

        InputEvent event;
        while(app->running) {
            const FuriStatus status = furi_message_queue_get(
                app->input_queue,
                &event,
                furi_ms_to_ticks(UNI_POLL_MS));

            if(status == FuriStatusOk) {
                handle_event(app, &event);
            } else if(app->ui.page == UniUiRemote && app->ui.remote) {
                uni_controller_poll(&app->controller, app->ui.remote);
                dispatch_remote_action(app);
                if(app->ui.remote->transport == UniTransportBluetoothHid && app->bt) {
                    snprintf(
                        app->ui.last_signal,
                        sizeof(app->ui.last_signal),
                        "%s",
                        uni_bt_transport_connected(app->bt) ? "CONNECTED" : "PAIRING");
                }
            }
            view_port_update(app->view_port);
        }

        gui_remove_view_port(app->gui, app->view_port);
    }

    if(app->ui.remote) remember_runtime(app);
    for(size_t i = 0; i < uni_remote_store_count(&app->store); i++) {
        uni_remote_store_unload_details(&app->store, i);
    }

    uni_state_engine_unload(&app->state_engine);
    if(app->gui) furi_record_close(RECORD_GUI);
    if(app->bt) uni_bt_transport_free(app->bt);
    if(app->ir) uni_ir_transport_free(app->ir);
    if(app->storage) furi_record_close(RECORD_STORAGE);
    if(app->view_port) view_port_free(app->view_port);
    if(app->input_queue) furi_message_queue_free(app->input_queue);
    free(app);
    return 0;
}
