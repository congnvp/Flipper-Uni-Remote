#include "controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

UniKey uni_map_physical_key(InputKey key) {
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

static size_t first_focusable(const UniRemote* remote) {
    if(!remote) return 0;
    for(size_t i = 0; i < remote->element_count; i++) {
        if(uni_element_focusable(&remote->elements[i])) return i;
    }
    return 0;
}

static void set_focus(UniController* controller, const UniRemote* remote, size_t index) {
    controller->focus_index = index;
    controller->dpad_captured =
        remote && index < remote->element_count && remote->elements[index].type == UniElementDpad;
}

static void move_focus(UniController* controller, const UniRemote* remote, int direction) {
    if(!remote || remote->element_count == 0) return;

    size_t index = controller->focus_index;
    for(size_t step = 0; step < remote->element_count; step++) {
        if(direction > 0) {
            index = (index + 1) % remote->element_count;
        } else {
            index = index == 0 ? remote->element_count - 1 : index - 1;
        }
        if(uni_element_focusable(&remote->elements[index])) {
            set_focus(controller, remote, index);
            return;
        }
    }
}

static void emit(UniController* controller, const char* signal, bool repeat) {
    if(!signal || signal[0] == '\0') return;
    snprintf(controller->signal, sizeof(controller->signal), "%s", signal);
    controller->repeat = repeat;
    controller->action_ready = true;
}

void uni_controller_reset(UniController* controller, const UniRemote* remote) {
    memset(controller, 0, sizeof(UniController));
    set_focus(controller, remote, first_focusable(remote));
}

static bool is_press(InputType type) {
    return type == InputTypePress || type == InputTypeShort;
}

static bool is_repeat(InputType type) {
    return type == InputTypeRepeat;
}

void uni_controller_handle(
    UniController* controller,
    const UniRemote* remote,
    UniKey key,
    InputType input_type,
    bool repeat_enabled) {
    if(!controller || !remote) return;
    controller->request_home = false;
    controller->request_exit = false;
    controller->action_ready = false;

    /* Long Back is system-reserved and can never be remapped. */
    if(key == UniKeyBack && input_type == InputTypeLong) {
        if(controller->dpad_captured) {
            controller->dpad_captured = false;
        } else {
            controller->request_home = true;
        }
        return;
    }

    if(controller->focus_index >= remote->element_count) {
        set_focus(controller, remote, first_focusable(remote));
    }

    const UniElement* element = &remote->elements[controller->focus_index];
    const bool repeat = is_repeat(input_type);
    if(repeat && (!repeat_enabled || !remote->repeat_enabled)) return;

    if(element->type == UniElementButton) {
        if(key == UniKeyOk && input_type == InputTypeShort) emit(controller, element->tap, false);
        else if(key == UniKeyOk && input_type == InputTypeLong) emit(controller, element->hold, false);
        else if((is_press(input_type) || repeat) && key != UniKeyBack) {
            move_focus(controller, remote, key == UniKeyUp || key == UniKeyLeft ? -1 : 1);
        }
        return;
    }

    if(element->type == UniElementHStep) {
        if((is_press(input_type) || repeat) && key == UniKeyLeft) emit(controller, element->left, repeat);
        else if((is_press(input_type) || repeat) && key == UniKeyRight) emit(controller, element->right, repeat);
        else if(is_press(input_type) && (key == UniKeyUp || key == UniKeyDown)) {
            move_focus(controller, remote, key == UniKeyUp ? -1 : 1);
        }
        return;
    }

    if(element->type == UniElementVStep) {
        if((is_press(input_type) || repeat) && key == UniKeyUp) emit(controller, element->up, repeat);
        else if((is_press(input_type) || repeat) && key == UniKeyDown) emit(controller, element->down, repeat);
        else if(is_press(input_type) && (key == UniKeyLeft || key == UniKeyRight)) {
            move_focus(controller, remote, key == UniKeyLeft ? -1 : 1);
        }
        return;
    }

    if(element->type == UniElementDpad) {
        if(!controller->dpad_captured) {
            if(is_press(input_type) && key != UniKeyBack) {
                move_focus(controller, remote, key == UniKeyUp || key == UniKeyLeft ? -1 : 1);
            }
            return;
        }

        if((is_press(input_type) || repeat) && key == UniKeyUp) emit(controller, element->up, repeat);
        else if((is_press(input_type) || repeat) && key == UniKeyDown) emit(controller, element->down, repeat);
        else if((is_press(input_type) || repeat) && key == UniKeyLeft) emit(controller, element->left, repeat);
        else if((is_press(input_type) || repeat) && key == UniKeyRight) emit(controller, element->right, repeat);
        else if(key == UniKeyOk && input_type == InputTypeShort) emit(controller, element->ok, false);
        else if(key == UniKeyOk && input_type == InputTypeLong) emit(controller, element->ok_hold, false);
    }
}
