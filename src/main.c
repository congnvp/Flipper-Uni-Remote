#include "ir_transport.h"
#include "profile.h"
#include "ui.h"

#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdbool.h>
#include <stdlib.h>

#define UNI_INPUT_QUEUE_SIZE 8

typedef struct {
    FuriMessageQueue* input_queue;
    ViewPort* view_port;
    Gui* gui;
    UniIrTransport* ir;
    UniUiState ui;
    bool running;
} UniApp;

typedef enum {
    UniKeyUp,
    UniKeyDown,
    UniKeyLeft,
    UniKeyRight,
    UniKeyOk,
    UniKeyBack,
    UniKeyUnknown,
} UniKey;

/*
 * Device is held clockwise in portrait orientation:
 * physical LEFT becomes logical UP, RIGHT -> DOWN,
 * DOWN -> LEFT, UP -> RIGHT.
 */
static UniKey uni_map_key(InputKey key) {
    switch(key) {
    case InputKeyLeft:
        return UniKeyUp;
    case InputKeyRight:
        return UniKeyDown;
    case InputKeyDown:
        return UniKeyLeft;
    case InputKeyUp:
        return UniKeyRight;
    case InputKeyOk:
        return UniKeyOk;
    case InputKeyBack:
        return UniKeyBack;
    default:
        return UniKeyUnknown;
    }
}

static void uni_draw_callback(Canvas* canvas, void* context) {
    UniApp* app = context;
    uni_ui_draw(canvas, &app->ui);
}

static void uni_input_callback(InputEvent* event, void* context) {
    FuriMessageQueue* queue = context;
    furi_message_queue_put(queue, event, 0);
}

static bool uni_send_action(UniApp* app, UniAction action, bool repeat) {
    const UniRemoteProfile* profile = app->ui.profile;
    if(!profile || action >= UniActionCount || !profile->has_action[action]) return false;
    if(profile->transport != UniTransportInfrared) return false;

    app->ui.last_action = action;
    app->ui.last_action_valid = true;
    app->ui.tx_flash = true;
    bool sent = uni_ir_transport_send(app->ir, &profile->ir[action], repeat);
    app->ui.tx_flash = false;
    return sent;
}

static void uni_handle_home(UniApp* app, const InputEvent* event, UniKey key) {
    if(event->type != InputTypeShort) return;

    size_t count = uni_profiles_count();
    if(count == 0) return;

    if(key == UniKeyUp) {
        app->ui.selected_profile =
            app->ui.selected_profile == 0 ? count - 1 : app->ui.selected_profile - 1;
    } else if(key == UniKeyDown) {
        app->ui.selected_profile = (app->ui.selected_profile + 1) % count;
    } else if(key == UniKeyOk) {
        app->ui.profile = uni_profiles_get(app->ui.selected_profile);
        app->ui.page = UniUiRemote;
        app->ui.last_action_valid = false;
    } else if(key == UniKeyBack) {
        app->running = false;
    }
}

static UniAction uni_direction_action(UniKey key) {
    switch(key) {
    case UniKeyUp:
        return UniActionUp;
    case UniKeyDown:
        return UniActionDown;
    case UniKeyLeft:
        return UniActionLeft;
    case UniKeyRight:
        return UniActionRight;
    default:
        return UniActionCount;
    }
}

static void uni_handle_remote(UniApp* app, const InputEvent* event, UniKey key) {
    if(key == UniKeyBack && event->type == InputTypeShort) {
        app->ui.page = UniUiHome;
        app->ui.profile = NULL;
        app->ui.last_action_valid = false;
        return;
    }

    if(key == UniKeyOk) {
        if(event->type == InputTypeShort) {
            uni_send_action(app, UniActionPower, false);
        } else if(event->type == InputTypeLong) {
            uni_send_action(app, UniActionMute, false);
        }
        return;
    }

    UniAction action = uni_direction_action(key);
    if(action == UniActionCount) return;

    if(event->type == InputTypePress) {
        uni_send_action(app, action, false);
    } else if(event->type == InputTypeRepeat) {
        uni_send_action(app, action, true);
    }
}

int32_t uni_remote_app(void* p) {
    UNUSED(p);

    UniApp* app = calloc(1, sizeof(UniApp));
    if(!app) return -1;

    app->input_queue = furi_message_queue_alloc(UNI_INPUT_QUEUE_SIZE, sizeof(InputEvent));
    app->view_port = view_port_alloc();
    app->ir = uni_ir_transport_alloc();
    app->gui = furi_record_open(RECORD_GUI);
    app->running = app->input_queue && app->view_port && app->ir && app->gui;

    app->ui.page = UniUiHome;
    app->ui.selected_profile = 0;
    app->ui.profile = NULL;
    app->ui.last_action_valid = false;
    app->ui.tx_flash = false;

    if(app->running) {
        view_port_draw_callback_set(app->view_port, uni_draw_callback, app);
        view_port_input_callback_set(app->view_port, uni_input_callback, app->input_queue);
        gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

        InputEvent event;
        while(app->running) {
            if(furi_message_queue_get(app->input_queue, &event, FuriWaitForever) == FuriStatusOk) {
                UniKey key = uni_map_key(event.key);
                if(app->ui.page == UniUiHome) {
                    uni_handle_home(app, &event, key);
                } else {
                    uni_handle_remote(app, &event, key);
                }
                view_port_update(app->view_port);
            }
        }

        gui_remove_view_port(app->gui, app->view_port);
    }

    if(app->gui) furi_record_close(RECORD_GUI);
    if(app->ir) uni_ir_transport_free(app->ir);
    if(app->view_port) view_port_free(app->view_port);
    if(app->input_queue) furi_message_queue_free(app->input_queue);
    free(app);

    return 0;
}
