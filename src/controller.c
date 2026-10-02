#include "controller.h"

#include <furi.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNI_DOUBLE_OK_MS 240U

/*
 * Portrait mapping when LCD is above the controls:
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

static size_t first_focusable(const UniRemote* remote, uint8_t page) {
    if(!remote) return 0;
    for(size_t i = 0; i < remote->element_count; i++) {
        if(remote->elements[i].page == page && uni_element_focusable(&remote->elements[i])) return i;
    }
    return remote->element_count;
}

static void set_focus(
    UniController* controller,
    const UniRemote* remote,
    size_t index,
    bool remember_previous) {
    if(!remote || index >= remote->element_count) return;

    if(remember_previous && remote->elements[index].type == UniElementDpad &&
       index != controller->focus_index) {
        controller->focus_before_capture = controller->focus_index;
        controller->focus_before_capture_valid = true;
    }

    controller->focus_index = index;
    controller->page = remote->elements[index].page;
    controller->dpad_captured = remote->elements[index].type == UniElementDpad;
    if(!controller->dpad_captured) {
        controller->dpad_alt = false;
        controller->dpad_hold_key = UniKeyUnknown;
        controller->ok_pending = false;
    }
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

    int8_t dx = 0;
    int8_t dy = 0;
    if(direction == UniKeyLeft) dx = -1;
    else if(direction == UniKeyRight) dx = 1;
    else if(direction == UniKeyUp) dy = -1;
    else if(direction == UniKeyDown) dy = 1;

    const UniElement* current = &remote->elements[controller->focus_index];
    int best_score = INT_MAX;
    size_t best_index = controller->focus_index;

    for(size_t i = 0; i < remote->element_count; i++) {
        if(i == controller->focus_index) continue;
        const UniElement* candidate = &remote->elements[i];
        if(candidate->page != controller->page || !uni_element_focusable(candidate)) continue;

        const int score = uni_element_direction_score(current, candidate, dx, dy);
        if(score >= 0 && score < best_score) {
            best_score = score;
            best_index = i;
        }
    }

    if(best_index != controller->focus_index) {
        set_focus(controller, remote, best_index, true);
        return true;
    }

    if((direction == UniKeyUp || direction == UniKeyDown) &&
       remote->page_count > 1) {
        uint8_t target = controller->page;
        if(direction == UniKeyDown) target = (uint8_t)((target + 1) % remote->page_count);
        else target = target == 0 ? (uint8_t)(remote->page_count - 1) : (uint8_t)(target - 1);

        const uint8_t current_x = current->x;
        int target_score = INT_MAX;
        size_t target_index = remote->element_count;
        for(size_t i = 0; i < remote->element_count; i++) {
            const UniElement* candidate = &remote->elements[i];
            if(candidate->page != target || !uni_element_focusable(candidate)) continue;
            const int dx_abs = candidate->x > current_x ? candidate->x - current_x : current_x - candidate->x;
            const int edge = direction == UniKeyDown ? candidate->y : (5 - candidate->y);
            const int score = dx_abs * 16 + edge;
            if(score < target_score) {
                target_score = score;
                target_index = i;
            }
        }
        if(target_index < remote->element_count) {
            set_focus(controller, remote, target_index, true);
            return true;
        }
    }
    return false;
}

static void emit(UniController* controller, const char* binding, bool repeat) {
    if(!binding || binding[0] == '\0') return;
    snprintf(controller->binding, sizeof(controller->binding), "%s", binding);
    controller->repeat = repeat;
    controller->action_ready = true;
}

static const char* dpad_direction_binding(const UniElement* e, UniKey key, bool hold_layer) {
    if(!e) return "";
    if(key == UniKeyUp) return hold_layer && e->up_hold[0] ? e->up_hold : e->up;
    if(key == UniKeyDown) return hold_layer && e->down_hold[0] ? e->down_hold : e->down;
    if(key == UniKeyLeft) return hold_layer && e->left_hold[0] ? e->left_hold : e->left;
    if(key == UniKeyRight) return hold_layer && e->right_hold[0] ? e->right_hold : e->right;
    return "";
}

static const char* dpad_hold_binding(const UniElement* e, UniKey key) {
    if(!e) return "";
    if(key == UniKeyUp) return e->up_hold;
    if(key == UniKeyDown) return e->down_hold;
    if(key == UniKeyLeft) return e->left_hold;
    if(key == UniKeyRight) return e->right_hold;
    return "";
}

static UniHardKeySlot hard_slot_for_key(UniKey key) {
    switch(key) {
    case UniKeyUp:
        return UniHardUpHold;
    case UniKeyDown:
        return UniHardDownHold;
    case UniKeyLeft:
        return UniHardLeftHold;
    case UniKeyRight:
        return UniHardRightHold;
    case UniKeyOk:
        return UniHardOkHold;
    default:
        return UniHardCount;
    }
}

static bool element_captures_direction(const UniElement* e, UniKey key, bool dpad_captured) {
    if(!e) return false;
    if(e->type == UniElementDpad && dpad_captured) return true;
    if(e->type == UniElementHStep && (key == UniKeyLeft || key == UniKeyRight)) return true;
    if(e->type == UniElementVStep && (key == UniKeyUp || key == UniKeyDown)) return true;
    return false;
}

static void release_dpad_capture(UniController* controller, const UniRemote* remote) {
    if(!controller->dpad_captured) return;

    const UniElement* dpad = controller->focus_index < remote->element_count ?
                                 &remote->elements[controller->focus_index] :
                                 NULL;
    const bool keep_alt = dpad && dpad->alt_sticky;

    controller->dpad_captured = false;
    controller->ok_pending = false;
    controller->dpad_hold_key = UniKeyUnknown;
    if(!keep_alt) controller->dpad_alt = false;

    if(controller->focus_before_capture_valid &&
       controller->focus_before_capture < remote->element_count &&
       uni_element_focusable(&remote->elements[controller->focus_before_capture])) {
        controller->focus_index = controller->focus_before_capture;
    }
    controller->focus_before_capture_valid = false;
}

void uni_controller_reset(UniController* controller, const UniRemote* remote) {
    memset(controller, 0, sizeof(UniController));
    controller->dpad_hold_key = UniKeyUnknown;
    controller->pending_nav_key = UniKeyUnknown;

    controller->page = 0;
    size_t first = first_focusable(remote, 0);
    if(remote && first >= remote->element_count) {
        for(uint8_t page = 1; page < remote->page_count; page++) {
            first = first_focusable(remote, page);
            if(first < remote->element_count) { controller->page = page; break; }
        }
    }
    if(remote && first < remote->element_count) {
        controller->focus_index = first;
        controller->dpad_captured = remote->elements[first].type == UniElementDpad;
    }
}

static void handle_hard_hold_candidate(
    UniController* controller,
    const UniRemote* remote,
    const UniElement* element,
    UniKey key,
    InputType input_type) {
    const UniHardKeySlot slot = hard_slot_for_key(key);
    if(slot >= UniHardCount || !remote->hard_bindings[slot][0]) return;
    if(element_captures_direction(element, key, controller->dpad_captured)) return;

    if(input_type == InputTypePress) {
        controller->nav_pending = true;
        controller->pending_nav_key = key;
    } else if(input_type == InputTypeShort && controller->nav_pending &&
              controller->pending_nav_key == key) {
        controller->nav_pending = false;
        controller->pending_nav_key = UniKeyUnknown;
        uni_controller_move_focus(controller, remote, key);
    } else if(input_type == InputTypeLong) {
        controller->nav_pending = false;
        controller->pending_nav_key = UniKeyUnknown;
        emit(controller, remote->hard_bindings[slot], false);
    }
}

static bool hard_hold_candidate_active(
    const UniController* controller,
    const UniRemote* remote,
    const UniElement* element,
    UniKey key) {
    const UniHardKeySlot slot = hard_slot_for_key(key);
    return !controller->dpad_captured && slot < UniHardCount &&
           remote->hard_bindings[slot][0] &&
           !element_captures_direction(element, key, controller->dpad_captured);
}

static void handle_dpad(
    UniController* controller,
    const UniRemote* remote,
    const UniElement* element,
    UniKey key,
    InputType input_type,
    bool repeat_enabled) {
    UNUSED(remote);

    if(key == UniKeyBack && input_type == InputTypeShort) {
        release_dpad_capture(controller, remote);
        return;
    }

    if(key == UniKeyOk) {
        if(input_type == InputTypeShort) {
            const uint32_t now = furi_get_tick();
            const uint32_t window = furi_ms_to_ticks(UNI_DOUBLE_OK_MS);
            if(controller->ok_pending && (now - controller->ok_pending_tick) <= window) {
                controller->ok_pending = false;
                controller->dpad_alt = !controller->dpad_alt;
                controller->dpad_hold_key = UniKeyUnknown;
            } else {
                if(controller->ok_pending) emit(controller, element->ok, false);
                controller->ok_pending = true;
                controller->ok_pending_tick = now;
            }
        } else if(input_type == InputTypeLong) {
            controller->ok_pending = false;
            emit(controller, element->ok_hold, false);
        }
        return;
    }

    if(key != UniKeyUp && key != UniKeyDown && key != UniKeyLeft && key != UniKeyRight) return;

    if(input_type == InputTypeRelease) {
        if(controller->dpad_hold_key == key) controller->dpad_hold_key = UniKeyUnknown;
        return;
    }

    if(controller->dpad_alt) {
        if(input_type == InputTypePress) {
            emit(controller, dpad_direction_binding(element, key, true), false);
        } else if(input_type == InputTypeRepeat && repeat_enabled) {
            emit(controller, dpad_direction_binding(element, key, true), true);
        }
        return;
    }

    const char* hold = dpad_hold_binding(element, key);
    const bool has_hold = hold && hold[0];

    /*
     * If a separate hold action exists, defer the normal action until Short.
     * This prevents a long press from firing the normal action first.
     */
    if(input_type == InputTypePress) {
        if(!has_hold) emit(controller, dpad_direction_binding(element, key, false), false);
    } else if(input_type == InputTypeShort) {
        if(has_hold) emit(controller, dpad_direction_binding(element, key, false), false);
    } else if(input_type == InputTypeLong) {
        if(has_hold) {
            controller->dpad_hold_key = key;
            emit(controller, hold, false);
        }
    } else if(input_type == InputTypeRepeat && repeat_enabled) {
        const bool using_hold = controller->dpad_hold_key == key && has_hold;
        emit(
            controller,
            using_hold ? hold : dpad_direction_binding(element, key, false),
            true);
    }
}

void uni_controller_handle(
    UniController* controller,
    const UniRemote* remote,
    UniKey key,
    InputType input_type,
    bool repeat_enabled) {
    if(!controller || !remote) return;
    controller->request_home = false;
    controller->action_ready = false;

    /* Long Back is always system escape, even inside D-pad capture. */
    if(key == UniKeyBack && input_type == InputTypeLong) {
        controller->request_home = true;
        controller->ok_pending = false;
        controller->nav_pending = false;
        return;
    }

    if(controller->focus_index >= remote->element_count) {
        const size_t first = first_focusable(remote, controller->page);
        if(first < remote->element_count) set_focus(controller, remote, first, false);
    }
    if(controller->focus_index >= remote->element_count) return;

    const UniElement* element = &remote->elements[controller->focus_index];

    if(element->type == UniElementDpad && controller->dpad_captured) {
        handle_dpad(controller, remote, element, key, input_type, repeat_enabled);
        return;
    }

    if(key == UniKeyBack) return;

    if((key == UniKeyUp || key == UniKeyDown || key == UniKeyLeft || key == UniKeyRight) &&
       hard_hold_candidate_active(controller, remote, element, key)) {
        handle_hard_hold_candidate(controller, remote, element, key, input_type);
        return;
    }

    const bool repeat = input_type == InputTypeRepeat;
    if(repeat && (!repeat_enabled || !remote->repeat_enabled)) return;

    if(element->type == UniElementButton) {
        if(key == UniKeyOk && input_type == InputTypePress && !element->hold[0]) {
            emit(controller, element->tap, false);
        } else if(key == UniKeyOk && input_type == InputTypeShort && element->hold[0]) {
            emit(controller, element->tap, false);
        } else if(key == UniKeyOk && input_type == InputTypeLong) {
            if(element->hold[0]) emit(controller, element->hold, false);
            else emit(controller, remote->hard_bindings[UniHardOkHold], false);
        } else if(input_type == InputTypePress) {
            uni_controller_move_focus(controller, remote, key);
        }
        return;
    }

    if(element->type == UniElementHStep) {
        if((input_type == InputTypePress || repeat) && key == UniKeyLeft)
            emit(controller, element->left, repeat);
        else if((input_type == InputTypePress || repeat) && key == UniKeyRight)
            emit(controller, element->right, repeat);
        else if(input_type == InputTypePress && (key == UniKeyUp || key == UniKeyDown))
            uni_controller_move_focus(controller, remote, key);
        else if(key == UniKeyOk && input_type == InputTypeLong)
            emit(controller, remote->hard_bindings[UniHardOkHold], false);
        return;
    }

    if(element->type == UniElementVStep) {
        if((input_type == InputTypePress || repeat) && key == UniKeyUp)
            emit(controller, element->up, repeat);
        else if((input_type == InputTypePress || repeat) && key == UniKeyDown)
            emit(controller, element->down, repeat);
        else if(input_type == InputTypePress && (key == UniKeyLeft || key == UniKeyRight))
            uni_controller_move_focus(controller, remote, key);
        else if(key == UniKeyOk && input_type == InputTypeLong)
            emit(controller, remote->hard_bindings[UniHardOkHold], false);
        return;
    }

    if(element->type == UniElementDpad && !controller->dpad_captured) {
        if(input_type == InputTypePress) uni_controller_move_focus(controller, remote, key);
    }
}

void uni_controller_poll(UniController* controller, const UniRemote* remote) {
    if(!controller || !remote || !controller->ok_pending || controller->action_ready) return;
    if(controller->focus_index >= remote->element_count) {
        controller->ok_pending = false;
        return;
    }
    const UniElement* element = &remote->elements[controller->focus_index];
    if(element->type != UniElementDpad || !controller->dpad_captured) {
        controller->ok_pending = false;
        return;
    }

    const uint32_t elapsed = furi_get_tick() - controller->ok_pending_tick;
    if(elapsed >= furi_ms_to_ticks(UNI_DOUBLE_OK_MS)) {
        controller->ok_pending = false;
        emit(controller, element->ok, false);
    }
}
