#pragma once

#include "action_engine.h"
#include "controller.h"
#include "remote_store.h"
#include "settings.h"
#include <gui/canvas.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UNI_LAST_SIGNAL_MAX UNI_BINDING_MAX

typedef enum {
    UniUiHome,
    UniUiRemote,
    UniUiMenu,
    UniUiGlobalSettings,
    UniUiRemoteSettings,
    UniUiLayoutEditor,
    UniUiLayoutTools,
    UniUiAddElement,
    UniUiLayoutPreset,
    UniUiMapField,
    UniUiMapKind,
    UniUiMapPick,
    UniUiIconField,
    UniUiIconPick,
    UniUiKeymap,
    UniUiTextEdit,
    UniUiPagePick,
    UniUiActionList,
    UniUiActionEdit,
    UniUiActionStepKind,
    UniUiActionStepPick,
} UniUiPage;

typedef enum {
    UniMapElement,
    UniMapHardKey,
} UniMapTarget;

typedef enum {
    UniPickSignal,
    UniPickSequence,
} UniPickerKind;

typedef enum {
    UniTextLabel,
    UniTextFolder,
    UniTextActionId,
} UniTextTarget;

typedef struct {
    UniUiPage page;
    const UniRemoteStore* store;
    const UniSettings* settings;
    const UniActionEngine* action_engine;
    size_t selected_remote;
    size_t home_category;
    const UniRemote* remote;
    uint8_t remote_page;

    size_t focus_index;
    bool dpad_captured;
    bool dpad_alt;
    UniKey dpad_hold_key;

    uint32_t tx_flash_until;
    uint32_t press_flash_until;
    size_t press_flash_index;
    bool tx_ok;
    char last_signal[UNI_LAST_SIGNAL_MAX];

    size_t menu_index;
    size_t layout_element;
    uint8_t layout_page;
    bool layout_moving;
    bool layout_replace_mode;

    UniMapTarget map_target;
    UniPickerKind picker_kind;
    size_t edit_field;
    UniHardKeySlot hard_slot;

    UniTextTarget text_target;
    UniUiPage text_return_page;
    size_t text_cursor;
    size_t text_limit;
    char text_buffer[UNI_FOLDER_MAX];

    size_t action_index;
    size_t action_step;
    bool action_step_append;
} UniUiState;

void uni_ui_draw(Canvas* canvas, const UniUiState* state);
