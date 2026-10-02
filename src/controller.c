#include "controller.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Portrait use places the LCD above the D-pad. The physical device is rotated
 * clockwise from its normal landscape orientation. Map physical key positions
 * to the logical directions seen by the user in portrait:
 *
 * physical Left  -> logical Up
 * physical Right -> logical Down
 * physical Down  -> logical Left
 * physical Up    -> logical Right
 */
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

bool uni_controller_move_focus(
    UniController* controller,
    const UniRemote* remote,
    UniKey direction) {
    if(!controller || !remote || controller->focus_index >= remote->element_count) return false;
    if(direction != UniKeyUp && direction != UniKeyDown && direction != UniKeyLeft &&
       direction != UniKeyRight) {
        return false;
    }

    const UniElement* current = &remote->elements[controller->focus_index];
    const int current_cx2 = 2 * current->x + current->w;
    const int current_cy2 = 2 * current->y + current->h;
    int best_score = INT_MAX;
    size_t best_index = controller->focus_index;

    for(size_t i = 0; i < remote->element_count; i++) {
        if(i == controller->focus_index) continue;
        const UniElement* candidate = &remote->elements[i];
        if(!uni_element_focusable(candidate)) continue;

        bool valid = false;
        int primary = 0;
        int secondary = 0;
        const int candidate_cx2 = 2 * candidate->x + candidate->w;
        const int candidate_cy2 = 2 * candidate->y + candidate->h;

        if(direction == UniKeyRight && candidate->x >= current->x + current->w) {
            valid = true;
            primary = candidate->x - (current->x + current->w);
            secondary = abs(candidate_cy2 - current_cy2);
        } else if(direction == UniKeyLeft &&
                  candidate->x + candidate->w <= current->x) {
            valid = true;
            primary = current->x - (candidate->x + candidate->w);
            secondary = abs(candidate_cy2 - current_cy2);
        } else if(direction == UniKeyDown && candidate->y >= current->y + current->h) {
            valid = true;
            primary = candidate->y - (current->y + current->h);
            secondary = abs(candidate_cx2 - current_cx2);
        } else if(direction == UniKeyUp &&
                  candidate->y + candidate->h <= current->y) {
            valid = true;
            primary = current->y - (candidate->y + candidate->h);
            secondary = abs(candidate_cx2 - current_cx2);
        }

        if(!valid) continue;
        const int score = primary * 100 + secondary;
        if(score < best_score) {
            best_score = score;
            best_index = i;
        }
    }

    if(best_index == controller->focus_index) return false;
    set_focus(controller, remote, best_index);
    return true;
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

static bool is_direction_start(InputType type) {
    return type == InputTypePress;
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
        else if(is_direction_start(input_type)) uni_controller_move_focus(controller, remote, key);
        return;
    }

    if(element->type == UniElementHStep) {
        if((is_direction_start(input_type) || repeat) && key == UniKeyLeft)
            emit(controller, element->left, repeat);
        else if((is_direction_start(input_type) || repeat) && key == UniKeyRight)
            emit(controller, element->right, repeat);
        else if(is_direction_start(input_type) && (key == UniKeyUp || key == UniKeyDown))
            uni_controller_move_focus(controller, remote, key);
        return;
    }

    if(element->type == UniElementVStep) {
        if((is_direction_start(input_type) || repeat) && key == UniKeyUp)
            emit(controller, element->up, repeat);
        else if((is_direction_start(input_type) || repeat) && key == UniKeyDown)
            emit(controller, element->down, repeat);
        else if(is_direction_start(input_type) && (key == UniKeyLeft || key == UniKeyRight))
            uni_controller_move_focus(controller, remote, key);
        return;
    }

    if(element->type == UniElementDpad) {
        if(!controller->dpad_captured) {
            if(is_direction_start(input_type)) uni_controller_move_focus(controller, remote, key);
            return;
        }

        if((is_direction_start(input_type) || repeat) && key == UniKeyUp)
            emit(controller, element->up, repeat);
        else if((is_direction_start(input_type) || repeat) && key == UniKeyDown)
            emit(controller, element->down, repeat);
        else if((is_direction_start(input_type) || repeat) && key == UniKeyLeft)
            emit(controller, element->left, repeat);
        else if((is_direction_start(input_type) || repeat) && key == UniKeyRight)
            emit(controller, element->right, repeat);
        else if(key == UniKeyOk && input_type == InputTypeShort)
            emit(controller, element->ok, false);
        else if(key == UniKeyOk && input_type == InputTypeLong)
            emit(controller, element->ok_hold, false);
    }
}
