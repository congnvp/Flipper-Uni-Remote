#include "action_engine.h"
#include "controller.h"
#include "editor_model.h"
#include "icon_library.h"
#include "ir_transport.h"
#include "layout_library.h"
#include "remote_store.h"
#include "settings.h"
#include "stateful_ir.h"
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
    UniActionEngine actions;
    UniRemoteStore store;
    UniSettings settings;
    UniController controller;
    UniStatefulEngine stateful;
    UniUiState ui;
    UniUiPage menu_return_page;
    bool running;
    bool repeat_enabled;
} UniApp;

static void uni_draw_callback(Canvas* canvas, void* context) {
    UniApp* app = context;
    if(app->ui.page == UniUiLayoutEditor) {
        app->ui.focus_index = app->ui.layout_element;
        if(app->ui.remote && app->ui.layout_element < app->ui.remote->element_count)
            app->ui.active_page = app->ui.remote->elements[app->ui.layout_element].page;
        app->ui.dpad_captured = false;
        app->ui.dpad_alt = false;
        app->ui.dpad_hold_key = UniKeyUnknown;
    } else {
        app->ui.focus_index = app->controller.focus_index;
        app->ui.active_page = app->controller.active_page;
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

static void load_actions_for_selected(UniApp* app) {
    const UniRemote* remote = selected_remote(app);
    if(!remote) return;
    if(remote->transport == UniTransportStatefulIr) {
        uni_stateful_open(&app->stateful, remote);
    } else {
        uni_action_engine_load(&app->actions, remote);
    }
}

static void refresh_remote_pointer(UniApp* app) {
    if(app->ui.remote) app->ui.remote = selected_remote(app);
    load_actions_for_selected(app);
}

static void open_remote(UniApp* app) {
    if(!uni_remote_store_load_details(&app->store, app->ui.selected_remote)) return;
    app->ui.remote = selected_remote(app);
    if(!app->ui.remote || !app->ui.remote->elements_loaded) return;
    load_actions_for_selected(app);
    app->ui.page = UniUiRemote;
    app->ui.last_signal[0] = '\0';
    app->ui.tx_ok = true;
    uni_controller_reset(&app->controller, app->ui.remote);
    if(app->ui.remote->transport == UniTransportStatefulIr) {
        uni_stateful_format_status(
            &app->stateful,
            app->ui.last_signal,
            sizeof(app->ui.last_signal));
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
    if(!uni_remote_store_load_details(&app->store, app->ui.selected_remote)) return;
    app->ui.remote = selected_remote(app);
    if(!app->ui.remote || !app->ui.remote->elements_loaded) return;
    load_actions_for_selected(app);
    uni_controller_reset(&app->controller, app->ui.remote);
    app->controller.dpad_captured = false;

    /* Layout editing includes display-only elements such as status/screen. */
    app->ui.layout_element = app->ui.remote->element_count ? 0 : 0;
    app->ui.layout_moving = false;
    app->ui.layout_replace_mode = false;
    app->ui.page = UniUiLayoutEditor;
}

static void reload_remotes(UniApp* app) {
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
    memset(&app->controller, 0, sizeof(app->controller));
    app->controller.dpad_hold_key = UniKeyUnknown;
    app->controller.pending_nav_key = UniKeyUnknown;

    if(!uni_remote_store_reload(&app->store)) {
        app->menu_return_page = UniUiHome;
        return;
    }

    app->ui.selected_remote =
        keep_id[0] ? uni_remote_store_find_id(&app->store, keep_id) : 0;

    if(restore_remote) {
        if(uni_remote_store_load_details(&app->store, app->ui.selected_remote)) {
            app->ui.remote = selected_remote(app);
            load_actions_for_selected(app);
            if(app->ui.remote) uni_controller_reset(&app->controller, app->ui.remote);
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

    snprintf(
        app->ui.last_signal,
        sizeof(app->ui.last_signal),
        "%s",
        app->controller.binding);
    app->ui.tx_flash = true;
    if(app->ui.remote->transport == UniTransportStatefulIr) {
        app->ui.tx_ok = uni_stateful_execute(
            &app->stateful,
            app->ui.remote,
            app->controller.binding,
            app->controller.repeat);
        if(app->ui.tx_ok) {
            uni_stateful_format_status(
                &app->stateful,
                app->ui.last_signal,
                sizeof(app->ui.last_signal));
        }
    } else {
        app->ui.tx_ok = uni_action_engine_execute(
            &app->actions,
            app->ui.remote,
            app->controller.binding,
            app->controller.repeat);
    }
    app->ui.tx_flash = false;
    app->controller.action_ready = false;
}

static void handle_home(UniApp* app, const InputEvent* event, UniKey key) {
    const size_t count = uni_remote_store_count(&app->store);
    if(key == UniKeyBack && event->type == InputTypeShort) {
        open_menu(app, UniUiHome);
        return;
    }
    if(event->type != InputTypeShort || count == 0) return;

    if(key == UniKeyUp) {
        app->ui.selected_remote =
            app->ui.selected_remote == 0 ? count - 1 : app->ui.selected_remote - 1;
    } else if(key == UniKeyDown) {
        app->ui.selected_remote = (app->ui.selected_remote + 1) % count;
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

static void handle_remote_settings(UniApp* app, const InputEvent* event, UniKey key) {
    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = UniUiMenu;
        app->ui.menu_index = 0;
        return;
    }
    if(event->type != InputTypeShort) return;

    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, 6, key);
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
        if(key == UniKeyOk) {
            snprintf(
                app->settings.default_remote,
                sizeof(app->settings.default_remote),
                "%s",
                remote->id);
            save_global(app);
        }
        break;
    case 2:
        if(key == UniKeyOk) open_layout_editor(app);
        break;
    case 3:
        if(key == UniKeyOk) {
            if(!uni_remote_store_load_details(&app->store, app->ui.selected_remote)) break;
            app->ui.remote = selected_remote(app);
            load_actions_for_selected(app);
            app->ui.page = UniUiKeymap;
            app->ui.menu_index = 0;
        }
        break;
    case 4:
        break;
    case 5:
        if(key == UniKeyOk) {
            app->ui.page = UniUiMenu;
            app->ui.menu_index = 0;
        }
        break;
    }
}

static size_t first_layout_element_on_page(const UniRemote* remote, uint8_t page) {
    if(!remote) return 0;
    for(size_t i = 0; i < remote->element_count; i++) {
        if(remote->elements[i].page == page) return i;
    }
    return remote->element_count;
}

static void layout_select(UniApp* app, UniKey key) {
    if(!app->ui.remote || !app->ui.remote->elements ||
       app->ui.remote->element_count == 0 ||
       app->ui.layout_element >= app->ui.remote->element_count) {
        return;
    }

    int8_t dx = 0;
    int8_t dy = 0;
    if(key == UniKeyLeft) dx = -1;
    else if(key == UniKeyRight) dx = 1;
    else if(key == UniKeyUp) dy = -1;
    else if(key == UniKeyDown) dy = 1;
    else return;

    const UniElement* current = &app->ui.remote->elements[app->ui.layout_element];
    const uint8_t page = current->page;
    int best_score = 10000;
    size_t best_index = app->ui.layout_element;

    for(size_t i = 0; i < app->ui.remote->element_count; i++) {
        if(i == app->ui.layout_element) continue;
        const UniElement* candidate = &app->ui.remote->elements[i];
        if(candidate->page != page) continue;
        const int score = uni_element_direction_score(current, candidate, dx, dy);
        if(score >= 0 && score < best_score) {
            best_score = score;
            best_index = i;
        }
    }

    if(best_index == app->ui.layout_element &&
       (key == UniKeyUp || key == UniKeyDown) &&
       app->ui.remote->page_count > 1) {
        int16_t next = (int16_t)page + (key == UniKeyDown ? 1 : -1);
        while(next < 0) next += app->ui.remote->page_count;
        while(next >= app->ui.remote->page_count) next -= app->ui.remote->page_count;
        for(uint8_t tries = 0; tries < app->ui.remote->page_count; tries++) {
            size_t candidate = first_layout_element_on_page(app->ui.remote, (uint8_t)next);
            if(candidate < app->ui.remote->element_count) {
                best_index = candidate;
                break;
            }
            next += key == UniKeyDown ? 1 : -1;
            while(next < 0) next += app->ui.remote->page_count;
            while(next >= app->ui.remote->page_count) next -= app->ui.remote->page_count;
        }
    }

    app->ui.layout_element = best_index;
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
    if(app->ui.layout_moving) layout_move(app, key);
    else layout_select(app, key);
}

static void handle_layout_tools(UniApp* app, const InputEvent* event, UniKey key) {
    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = UniUiLayoutEditor;
        return;
    }
    if(event->type != InputTypeShort) return;
    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, 7, key);
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
    case 5: /* TEMPLATE */
        app->ui.page = UniUiLayoutPreset;
        app->ui.menu_index = 0;
        break;
    case 6: /* DONE */
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
        uint8_t page = 0;
        const UniRemote* current = selected_remote(app);
        if(current && current->element_count &&
           app->ui.layout_element < current->element_count) {
            page = current->elements[app->ui.layout_element].page;
        }
        ok = uni_remote_store_add_element(
            &app->store,
            app->ui.selected_remote,
            app->ui.menu_index,
            page,
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
        app->ui.layout_element = app->ui.remote && app->ui.remote->element_count ? 0 : 0;
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
    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = app->ui.map_target == UniMapHardKey ? UniUiKeymap : UniUiMapField;
        app->ui.menu_index = 0;
        return;
    }
    if(event->type != InputTypeShort) return;
    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, 4, key);
        return;
    }
    if(key != UniKeyOk) return;

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
}

static void handle_map_pick(UniApp* app, const InputEvent* event, UniKey key) {
    const size_t count =
        app->ui.picker_kind == UniPickSignal ?
            app->actions.signals.count :
            sequence_count(&app->actions.actions);

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
    if(app->ui.picker_kind == UniPickSignal) {
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

    if(set_current_binding(app, binding)) return_after_mapping(app);
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

static void handle_keymap(UniApp* app, const InputEvent* event, UniKey key) {
    const size_t count = UniHardCount + 1;
    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = UniUiRemoteSettings;
        app->ui.menu_index = 3;
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
        app->ui.menu_index = 3;
        return;
    }

    app->ui.map_target = UniMapHardKey;
    app->ui.hard_slot = (UniHardKeySlot)app->ui.menu_index;
    app->ui.page = UniUiMapKind;
    app->ui.menu_index = 0;
}

static void handle_event(UniApp* app, const InputEvent* event) {
    const UniKey key = uni_map_physical_key(event->key);
    if(app->ui.page == UniUiRemote && key == UniKeyOk) {
        if(event->type == InputTypePress) app->ui.pressed = true;
        else if(event->type == InputTypeRelease || event->type == InputTypeShort) app->ui.pressed = false;
    } else if(app->ui.page != UniUiRemote) {
        app->ui.pressed = false;
    }

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
    uni_action_engine_init(&app->actions, app->storage, app->ir);
    uni_stateful_init(&app->stateful, app->storage);

    app->repeat_enabled = settings_ok ? app->settings.repeat_enabled : true;
    app->running = app->input_queue && app->view_port && app->gui && app->storage &&
                   app->ir && store_ok && settings_ok;

    app->ui.page = UniUiHome;
    app->ui.store = &app->store;
    app->ui.settings = &app->settings;
    app->ui.action_engine = &app->actions;
    app->ui.selected_remote =
        settings_ok ?
            uni_remote_store_find_id(&app->store, app->settings.default_remote) :
            0;
    app->ui.tx_ok = true;
    app->ui.dpad_hold_key = UniKeyUnknown;
    app->menu_return_page = UniUiHome;

    if(app->running && app->settings.open_default) open_remote(app);

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
            }
            view_port_update(app->view_port);
        }

        gui_remove_view_port(app->gui, app->view_port);
    }

    for(size_t i = 0; i < uni_remote_store_count(&app->store); i++) {
        uni_remote_store_unload_details(&app->store, i);
    }

    if(app->gui) furi_record_close(RECORD_GUI);
    if(app->ir) uni_ir_transport_free(app->ir);
    if(app->storage) furi_record_close(RECORD_STORAGE);
    if(app->view_port) view_port_free(app->view_port);
    if(app->input_queue) furi_message_queue_free(app->input_queue);
    free(app);
    return 0;
}
