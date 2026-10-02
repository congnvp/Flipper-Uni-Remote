#include "controller.h"
#include "ir_transport.h"
#include "remote_store.h"
#include "settings.h"
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

typedef struct {
    FuriMessageQueue* input_queue;
    ViewPort* view_port;
    Gui* gui;
    Storage* storage;
    UniIrTransport* ir;
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
        app->ui.focus_index = app->ui.layout_element;
        app->ui.dpad_captured = false;
    } else {
        app->ui.focus_index = app->controller.focus_index;
        app->ui.dpad_captured = app->controller.dpad_captured;
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

static void open_remote(UniApp* app) {
    app->ui.remote = selected_remote(app);
    if(!app->ui.remote) return;
    app->ui.page = UniUiRemote;
    app->ui.last_signal[0] = '\0';
    app->ui.tx_ok = true;
    uni_controller_reset(&app->controller, app->ui.remote);
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

static void open_layout_editor(UniApp* app) {
    app->ui.remote = selected_remote(app);
    if(!app->ui.remote) return;
    uni_controller_reset(&app->controller, app->ui.remote);
    app->controller.dpad_captured = false;
    app->ui.layout_element = app->controller.focus_index;
    app->ui.layout_moving = false;
    app->ui.page = UniUiLayoutEditor;
}

static void reload_remotes(UniApp* app) {
    char keep_id[UNI_ID_MAX] = {0};
    const UniRemote* current = selected_remote(app);
    if(current) snprintf(keep_id, sizeof(keep_id), "%s", current->id);

    if(uni_remote_store_reload(&app->store)) {
        app->ui.selected_remote =
            keep_id[0] ? uni_remote_store_find_id(&app->store, keep_id) : 0;
        if(app->menu_return_page == UniUiRemote) {
            app->ui.remote = selected_remote(app);
            if(app->ui.remote) uni_controller_reset(&app->controller, app->ui.remote);
        }
    }
}

static void handle_home(UniApp* app, const InputEvent* event, UniKey key) {
    const size_t count = uni_remote_store_count(&app->store);

    if(key == UniKeyBack && event->type == InputTypeLong) {
        app->running = false;
        return;
    }
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

static void dispatch_remote_action(UniApp* app) {
    if(!app->controller.action_ready || !app->ui.remote) return;

    snprintf(app->ui.last_signal, sizeof(app->ui.last_signal), "%s", app->controller.signal);
    app->ui.tx_flash = true;

    if(app->ui.remote->transport == UniTransportInfrared) {
        app->ui.tx_ok = uni_ir_transport_send(
            app->ir,
            app->ui.remote->signal_path,
            app->controller.signal,
            app->controller.repeat);
    } else {
        app->ui.tx_ok = false;
    }

    app->ui.tx_flash = false;
    app->controller.action_ready = false;
}

static void handle_remote(UniApp* app, const InputEvent* event, UniKey key) {
    if(key == UniKeyBack && event->type == InputTypeShort) {
        if(!app->controller.dpad_captured) open_menu(app, UniUiRemote);
        return;
    }

    uni_controller_handle(
        &app->controller,
        app->ui.remote,
        key,
        event->type,
        app->repeat_enabled);

    if(app->controller.request_home) {
        app->ui.page = UniUiHome;
        app->ui.remote = NULL;
        app->ui.last_signal[0] = '\0';
        return;
    }

    dispatch_remote_action(app);
}

static void menu_move(UniUiState* ui, size_t count, UniKey key) {
    if(count == 0) return;
    if(key == UniKeyUp) {
        ui->menu_index = ui->menu_index == 0 ? count - 1 : ui->menu_index - 1;
    } else if(key == UniKeyDown) {
        ui->menu_index = (ui->menu_index + 1) % count;
    }
}

static void handle_menu(UniApp* app, const InputEvent* event, UniKey key) {
    if(key == UniKeyBack &&
       (event->type == InputTypeShort || event->type == InputTypeLong)) {
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
    default:
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

static void handle_global_settings(UniApp* app, const InputEvent* event, UniKey key) {
    if(key == UniKeyBack &&
       (event->type == InputTypeShort || event->type == InputTypeLong)) {
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
        return;
    }

    if(app->ui.menu_index == 0 &&
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
    if(key == UniKeyBack &&
       (event->type == InputTypeShort || event->type == InputTypeLong)) {
        app->ui.page = UniUiMenu;
        app->ui.menu_index = 0;
        return;
    }
    if(event->type != InputTypeShort) return;

    if(key == UniKeyUp || key == UniKeyDown) {
        menu_move(&app->ui, 5, key);
        return;
    }

    UniRemote* remote = uni_remote_store_get_mut(&app->store, app->ui.selected_remote);
    if(!remote) return;

    if(app->ui.menu_index == 0 &&
       (key == UniKeyLeft || key == UniKeyRight || key == UniKeyOk)) {
        uni_remote_store_set_repeat(&app->store, app->ui.selected_remote, !remote->repeat_enabled);
        app->ui.remote = selected_remote(app);
    } else if(app->ui.menu_index == 1 && key == UniKeyOk) {
        snprintf(
            app->settings.default_remote,
            sizeof(app->settings.default_remote),
            "%s",
            remote->id);
        save_global(app);
    } else if(app->ui.menu_index == 2 && key == UniKeyOk) {
        open_layout_editor(app);
    } else if(app->ui.menu_index == 4 && key == UniKeyOk) {
        app->ui.page = UniUiMenu;
        app->ui.menu_index = 0;
    }
}

static void layout_select(UniApp* app, UniKey key) {
    app->controller.focus_index = app->ui.layout_element;
    app->controller.dpad_captured = false;
    if(uni_controller_move_focus(&app->controller, app->ui.remote, key)) {
        app->controller.dpad_captured = false;
        app->ui.layout_element = app->controller.focus_index;
    }
}

static void layout_move(UniApp* app, UniKey key) {
    int8_t dx = 0;
    int8_t dy = 0;
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
        app->ui.remote = selected_remote(app);
    }
}

static void handle_layout_editor(UniApp* app, const InputEvent* event, UniKey key) {
    if(key == UniKeyBack && event->type == InputTypeLong) {
        app->ui.layout_moving = false;
        app->ui.page = UniUiMenu;
        app->ui.menu_index = 0;
        return;
    }
    if(key == UniKeyBack && event->type == InputTypeShort) {
        if(app->ui.layout_moving) app->ui.layout_moving = false;
        else {
            app->ui.page = UniUiMenu;
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

int32_t uni_remote_app(void* p) {
    UNUSED(p);

    UniApp* app = calloc(1, sizeof(UniApp));
    if(!app) return -1;

    app->input_queue = furi_message_queue_alloc(UNI_INPUT_QUEUE_SIZE, sizeof(InputEvent));
    app->view_port = view_port_alloc();
    app->storage = furi_record_open(RECORD_STORAGE);
    app->gui = furi_record_open(RECORD_GUI);
    app->repeat_enabled = true;

    const bool settings_ok =
        app->storage && uni_settings_load_or_create(app->storage, &app->settings);
    const bool store_ok = app->storage && uni_remote_store_init(&app->store, app->storage);
    app->ir = app->storage ? uni_ir_transport_alloc(app->storage) : NULL;
    if(settings_ok) app->repeat_enabled = app->settings.repeat_enabled;
    app->running = app->input_queue && app->view_port && app->gui && app->storage && app->ir &&
                   store_ok && settings_ok;

    app->ui.page = UniUiHome;
    app->ui.store = &app->store;
    app->ui.settings = &app->settings;
    app->ui.selected_remote =
        settings_ok ? uni_remote_store_find_id(&app->store, app->settings.default_remote) : 0;
    app->ui.remote = NULL;
    app->ui.tx_ok = true;
    app->menu_return_page = UniUiHome;

    if(app->running && app->settings.open_default) open_remote(app);

    if(app->running) {
        view_port_draw_callback_set(app->view_port, uni_draw_callback, app);
        view_port_input_callback_set(app->view_port, uni_input_callback, app->input_queue);
        gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

        InputEvent event;
        while(app->running) {
            if(furi_message_queue_get(app->input_queue, &event, FuriWaitForever) ==
               FuriStatusOk) {
                const UniKey key = uni_map_physical_key(event.key);
                switch(app->ui.page) {
                case UniUiHome:
                    handle_home(app, &event, key);
                    break;
                case UniUiRemote:
                    handle_remote(app, &event, key);
                    break;
                case UniUiMenu:
                    handle_menu(app, &event, key);
                    break;
                case UniUiGlobalSettings:
                    handle_global_settings(app, &event, key);
                    break;
                case UniUiRemoteSettings:
                    handle_remote_settings(app, &event, key);
                    break;
                case UniUiLayoutEditor:
                    handle_layout_editor(app, &event, key);
                    break;
                }
                view_port_update(app->view_port);
            }
        }

        gui_remove_view_port(app->gui, app->view_port);
    }

    if(app->gui) furi_record_close(RECORD_GUI);
    if(app->ir) uni_ir_transport_free(app->ir);
    if(app->storage) furi_record_close(RECORD_STORAGE);
    if(app->view_port) view_port_free(app->view_port);
    if(app->input_queue) furi_message_queue_free(app->input_queue);
    free(app);

    return 0;
}
