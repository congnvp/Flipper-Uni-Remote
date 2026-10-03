#pragma once

#include "remote.h"
#include <stddef.h>

size_t uni_editor_binding_count(const UniElement* element);
const char* uni_editor_binding_key(const UniElement* element, size_t index);
const char* uni_editor_binding_label(const UniElement* element, size_t index);

size_t uni_editor_icon_count(const UniElement* element);
const char* uni_editor_icon_key(const UniElement* element, size_t index);
const char* uni_editor_icon_label(const UniElement* element, size_t index);

const char* uni_editor_hard_label(UniHardKeySlot slot);

size_t uni_editor_transport_action_count(const UniRemote* remote);
const char* uni_editor_transport_action_binding(const UniRemote* remote, size_t index);
const char* uni_editor_transport_action_label(const UniRemote* remote, size_t index);
