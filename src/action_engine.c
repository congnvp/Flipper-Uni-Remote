#include "action_engine.h"

#include <flipper_format/flipper_format.h>
#include <furi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNI_ACTION_FILETYPE "Flipper Uni Remote Actions"
#define UNI_ACTION_VERSION 1U

static bool action_catalog_valid(const UniActionEngine* engine);

static bool load_signal_names(Storage* storage, const char* path, UniSignalCatalog* catalog) {
    memset(catalog, 0, sizeof(UniSignalCatalog));
    File* file = storage_file_alloc(storage);
    if(!file) return false;
    if(!storage_file_open(file, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(file);
        return false;
    }

    char line[96];
    size_t length = 0;
    uint8_t ch = 0;
    while(storage_file_read(file, &ch, 1) == 1) {
        if(ch == '\n' || ch == '\r') {
            if(length > 0) {
                line[length] = '\0';
                if(strncmp(line, "name:", 5) == 0 && catalog->count < UNI_MAX_SIGNALS) {
                    const char* value = line + 5;
                    while(*value == ' ') value++;
                    snprintf(
                        catalog->names[catalog->count],
                        UNI_SIGNAL_NAME_MAX,
                        "%.31s",
                        value);
                    catalog->count++;
                }
                length = 0;
            }
        } else if(length + 1 < sizeof(line)) {
            line[length++] = (char)ch;
        }
    }
    if(length > 0) {
        line[length] = '\0';
        if(strncmp(line, "name:", 5) == 0 && catalog->count < UNI_MAX_SIGNALS) {
            const char* value = line + 5;
            while(*value == ' ') value++;
            snprintf(catalog->names[catalog->count], UNI_SIGNAL_NAME_MAX, "%.31s", value);
            catalog->count++;
        }
    }

    storage_file_close(file);
    storage_file_free(file);
    return catalog->count > 0;
}

static bool read_string(
    FlipperFormat* ff,
    const char* key,
    char* out,
    size_t out_size,
    bool required) {
    FuriString* value = furi_string_alloc();
    flipper_format_rewind(ff);
    const bool found = flipper_format_read_string(ff, key, value);
    if(found) snprintf(out, out_size, "%s", furi_string_get_cstr(value));
    furi_string_free(value);
    return found || !required;
}

static bool read_u32(FlipperFormat* ff, const char* key, uint32_t* out, bool required) {
    flipper_format_rewind(ff);
    const bool found = flipper_format_read_uint32(ff, key, out, 1);
    return found || !required;
}

static bool load_actions(Storage* storage, const char* path, UniActionCatalog* catalog) {
    memset(catalog, 0, sizeof(UniActionCatalog));
    if(!storage_file_exists(storage, path)) return true;

    FlipperFormat* ff = flipper_format_file_alloc(storage);
    if(!ff) return false;

    FuriString* filetype = furi_string_alloc();
    uint32_t version = 0;
    uint32_t count = 0;
    bool ok = false;

    do {
        if(!flipper_format_file_open_existing(ff, path)) break;
        if(!flipper_format_read_header(ff, filetype, &version)) break;
        if(strcmp(furi_string_get_cstr(filetype), UNI_ACTION_FILETYPE) != 0) break;
        if(version != UNI_ACTION_VERSION) break;
        if(!read_u32(ff, "ActionCount", &count, true)) break;
        if(count > UNI_MAX_ACTIONS) break;

        for(uint32_t i = 0; i < count; i++) {
            UniNamedAction* action = &catalog->actions[i];
            char key[40];
            char type[16] = {0};

            snprintf(key, sizeof(key), "Action%luId", (unsigned long)i);
            if(!read_string(ff, key, action->id, sizeof(action->id), true)) goto done;
            snprintf(key, sizeof(key), "Action%luType", (unsigned long)i);
            if(!read_string(ff, key, type, sizeof(type), true)) goto done;

            if(strcmp(type, "signal") == 0) {
                action->type = UniActionSignal;
                snprintf(key, sizeof(key), "Action%luSignal", (unsigned long)i);
                if(!read_string(ff, key, action->signal, sizeof(action->signal), true)) goto done;
            } else if(strcmp(type, "sequence") == 0) {
                action->type = UniActionSequence;
                uint32_t steps = 0;
                snprintf(key, sizeof(key), "Action%luStepCount", (unsigned long)i);
                if(!read_u32(ff, key, &steps, true) || steps > UNI_MAX_SEQUENCE_STEPS) goto done;
                action->step_count = steps;
                for(uint32_t s = 0; s < steps; s++) {
                    snprintf(
                        key,
                        sizeof(key),
                        "Action%luStep%lu",
                        (unsigned long)i,
                        (unsigned long)s);
                    if(!read_string(
                           ff,
                           key,
                           action->steps[s],
                           sizeof(action->steps[s]),
                           true)) {
                        goto done;
                    }
                    snprintf(
                        key,
                        sizeof(key),
                        "Action%luDelay%lu",
                        (unsigned long)i,
                        (unsigned long)s);
                    read_u32(ff, key, &action->delays_ms[s], false);
                    if(action->delays_ms[s] > UNI_MAX_SEQUENCE_DELAY_MS) goto done;
                }
            } else {
                goto done;
            }
        }
        catalog->count = count;
        ok = true;
    } while(false);

done:
    flipper_format_file_close(ff);
    furi_string_free(filetype);
    flipper_format_free(ff);
    return ok;
}

void uni_action_engine_init(UniActionEngine* engine, Storage* storage, UniIrTransport* ir) {
    memset(engine, 0, sizeof(UniActionEngine));
    engine->storage = storage;
    engine->ir = ir;
}

bool uni_action_engine_load(UniActionEngine* engine, const UniRemote* remote) {
    if(!engine || !engine->storage || !remote) return false;
    const bool signals_ok =
        load_signal_names(engine->storage, remote->signal_path, &engine->signals);
    if(remote->transport == UniTransportInfrared && !signals_ok) return false;
    if(!load_actions(engine->storage, remote->action_path, &engine->actions)) return false;
    return action_catalog_valid(engine);
}

const UniNamedAction* uni_action_find(const UniActionCatalog* catalog, const char* id) {
    if(!catalog || !id) return NULL;
    for(size_t i = 0; i < catalog->count; i++) {
        if(strcmp(catalog->actions[i].id, id) == 0) return &catalog->actions[i];
    }
    return NULL;
}

static bool signal_exists(const UniSignalCatalog* catalog, const char* name) {
    if(!catalog || !name || !name[0]) return false;
    for(size_t i = 0; i < catalog->count; i++) {
        if(strcmp(catalog->names[i], name) == 0) return true;
    }
    return false;
}

static bool valid_action_id(const char* id) {
    if(!id || !id[0] || strlen(id) >= UNI_ACTION_ID_MAX) return false;
    for(size_t i = 0; id[i]; i++) {
        const char ch = id[i];
        const bool valid =
            (ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '_' || ch == '-';
        if(!valid) return false;
    }
    return true;
}

static int catalog_action_index(const UniActionCatalog* catalog, const char* id) {
    if(!catalog || !id) return -1;
    for(size_t i = 0; i < catalog->count; i++) {
        if(strcmp(catalog->actions[i].id, id) == 0) return (int)i;
    }
    return -1;
}

static bool step_reference_valid(
    const UniActionEngine* engine,
    const char* binding) {
    if(!engine || !binding || !binding[0]) return false;
    if(strncmp(binding, "sig:", 4) == 0) {
        return signal_exists(&engine->signals, binding + 4);
    }
    if(strncmp(binding, "act:", 4) == 0) {
        return catalog_action_index(&engine->actions, binding + 4) >= 0;
    }
    if(strchr(binding, ':')) return false;
    return signal_exists(&engine->signals, binding);
}

static bool validate_action_dfs(
    const UniActionEngine* engine,
    size_t index,
    uint32_t* visiting,
    uint32_t* visited) {
    if(!engine || index >= engine->actions.count || !visiting || !visited) return false;
    const uint32_t bit = 1UL << index;
    if(*visited & bit) return true;
    if(*visiting & bit) return false;

    *visiting |= bit;
    const UniNamedAction* action = &engine->actions.actions[index];
    if(action->type == UniActionSequence) {
        for(size_t s = 0; s < action->step_count; s++) {
            const char* step = action->steps[s];
            if(!step_reference_valid(engine, step)) return false;
            if(action->delays_ms[s] > UNI_MAX_SEQUENCE_DELAY_MS) return false;
            if(strncmp(step, "act:", 4) == 0) {
                const int nested = catalog_action_index(&engine->actions, step + 4);
                if(nested < 0 ||
                   !validate_action_dfs(
                       engine,
                       (size_t)nested,
                       visiting,
                       visited)) {
                    return false;
                }
            }
        }
    } else if(action->type == UniActionSignal) {
        if(!signal_exists(&engine->signals, action->signal)) return false;
    } else {
        return false;
    }

    *visiting &= ~bit;
    *visited |= bit;
    return true;
}

static bool action_catalog_valid(const UniActionEngine* engine) {
    if(!engine || engine->actions.count > UNI_MAX_ACTIONS) return false;

    for(size_t i = 0; i < engine->actions.count; i++) {
        const UniNamedAction* action = &engine->actions.actions[i];
        if(!valid_action_id(action->id)) return false;
        for(size_t j = i + 1; j < engine->actions.count; j++) {
            if(strcmp(action->id, engine->actions.actions[j].id) == 0) return false;
        }
        if(action->type == UniActionSequence &&
           action->step_count > UNI_MAX_SEQUENCE_STEPS) {
            return false;
        }
    }

    uint32_t visiting = 0;
    uint32_t visited = 0;
    for(size_t i = 0; i < engine->actions.count; i++) {
        if(!validate_action_dfs(engine, i, &visiting, &visited)) return false;
    }
    return true;
}

static bool write_action_string(FlipperFormat* ff, const char* key, const char* value) {
    return flipper_format_write_string_cstr(ff, key, value ? value : "");
}

bool uni_action_engine_save(UniActionEngine* engine, const UniRemote* remote) {
    if(!engine || !engine->storage || !remote || !remote->action_path[0]) return false;
    if(!action_catalog_valid(engine)) return false;

    FlipperFormat* ff = flipper_format_file_alloc(engine->storage);
    if(!ff) return false;

    bool ok = false;
    do {
        if(!flipper_format_file_open_always(ff, remote->action_path)) break;
        if(!flipper_format_write_header_cstr(ff, UNI_ACTION_FILETYPE, UNI_ACTION_VERSION)) break;

        uint32_t count = engine->actions.count;
        if(!flipper_format_write_uint32(ff, "ActionCount", &count, 1)) break;

        for(size_t i = 0; i < engine->actions.count; i++) {
            const UniNamedAction* action = &engine->actions.actions[i];
            char key[40];

            snprintf(key, sizeof(key), "Action%luId", (unsigned long)i);
            if(!write_action_string(ff, key, action->id)) goto done_save;
            snprintf(key, sizeof(key), "Action%luType", (unsigned long)i);
            if(!write_action_string(
                   ff,
                   key,
                   action->type == UniActionSignal ? "signal" : "sequence")) {
                goto done_save;
            }

            if(action->type == UniActionSignal) {
                snprintf(key, sizeof(key), "Action%luSignal", (unsigned long)i);
                if(!write_action_string(ff, key, action->signal)) goto done_save;
            } else {
                uint32_t steps = action->step_count;
                snprintf(key, sizeof(key), "Action%luStepCount", (unsigned long)i);
                if(!flipper_format_write_uint32(ff, key, &steps, 1)) goto done_save;
                for(size_t s = 0; s < action->step_count; s++) {
                    snprintf(
                        key,
                        sizeof(key),
                        "Action%luStep%lu",
                        (unsigned long)i,
                        (unsigned long)s);
                    if(!write_action_string(ff, key, action->steps[s])) goto done_save;
                    snprintf(
                        key,
                        sizeof(key),
                        "Action%luDelay%lu",
                        (unsigned long)i,
                        (unsigned long)s);
                    if(!flipper_format_write_uint32(ff, key, &action->delays_ms[s], 1)) {
                        goto done_save;
                    }
                }
            }
        }
        ok = true;
    } while(false);

done_save:
    flipper_format_file_close(ff);
    flipper_format_free(ff);
    return ok;
}

size_t uni_action_sequence_count(const UniActionCatalog* catalog) {
    if(!catalog) return 0;
    size_t count = 0;
    for(size_t i = 0; i < catalog->count; i++) {
        if(catalog->actions[i].type == UniActionSequence) count++;
    }
    return count;
}

size_t uni_action_sequence_index(const UniActionCatalog* catalog, size_t position) {
    if(!catalog) return 0;
    size_t current = 0;
    for(size_t i = 0; i < catalog->count; i++) {
        if(catalog->actions[i].type != UniActionSequence) continue;
        if(current++ == position) return i;
    }
    return catalog->count;
}

static bool binding_refs_action(const char* binding, const char* id) {
    return binding && id && strncmp(binding, "act:", 4) == 0 &&
           strcmp(binding + 4, id) == 0;
}

bool uni_action_engine_action_in_use(
    const UniActionEngine* engine,
    const UniRemote* remote,
    const char* id) {
    if(!engine || !id || !id[0]) return false;

    for(size_t i = 0; i < engine->actions.count; i++) {
        const UniNamedAction* action = &engine->actions.actions[i];
        if(strcmp(action->id, id) == 0) continue;
        if(action->type != UniActionSequence) continue;
        for(size_t s = 0; s < action->step_count; s++) {
            if(binding_refs_action(action->steps[s], id)) return true;
        }
    }

    if(!remote || !remote->elements_loaded || !remote->elements) return false;
    for(size_t i = 0; i < remote->element_count; i++) {
        const UniElement* e = &remote->elements[i];
        const char* bindings[] = {
            e->tap,e->hold,e->up,e->down,e->left,e->right,e->ok,
            e->up_hold,e->down_hold,e->left_hold,e->right_hold,e->ok_hold};
        for(size_t b = 0; b < sizeof(bindings) / sizeof(bindings[0]); b++) {
            if(binding_refs_action(bindings[b], id)) return true;
        }
    }
    for(size_t i = 0; i < UniHardCount; i++) {
        if(binding_refs_action(remote->hard_bindings[i], id)) return true;
    }
    return false;
}

static UniActionCatalog* catalog_backup(const UniActionEngine* engine) {
    if(!engine) return NULL;
    UniActionCatalog* backup = malloc(sizeof(UniActionCatalog));
    if(backup) memcpy(backup, &engine->actions, sizeof(UniActionCatalog));
    return backup;
}

static bool finish_action_mutation(
    UniActionEngine* engine,
    const UniRemote* remote,
    UniActionCatalog* backup) {
    if(!backup) return false;
    const bool ok = uni_action_engine_save(engine, remote);
    if(!ok) memcpy(&engine->actions, backup, sizeof(UniActionCatalog));
    free(backup);
    return ok;
}

bool uni_action_engine_add_sequence(
    UniActionEngine* engine,
    const UniRemote* remote,
    size_t* action_index_out) {
    if(!engine || !remote || engine->actions.count >= UNI_MAX_ACTIONS) return false;
    UniActionCatalog* backup = catalog_backup(engine);
    if(!backup) return false;

    size_t index = engine->actions.count;
    UniNamedAction* action = &engine->actions.actions[index];
    memset(action, 0, sizeof(UniNamedAction));
    action->type = UniActionSequence;

    for(size_t n = 1; n <= UNI_MAX_ACTIONS; n++) {
        char candidate[UNI_ACTION_ID_MAX];
        snprintf(candidate, sizeof(candidate), "macro%lu", (unsigned long)n);
        if(catalog_action_index(&engine->actions, candidate) < 0) {
            snprintf(action->id, sizeof(action->id), "%s", candidate);
            break;
        }
    }
    if(!action->id[0]) {
        free(backup);
        return false;
    }

    engine->actions.count++;
    if(!finish_action_mutation(engine, remote, backup)) return false;
    if(action_index_out) *action_index_out = index;
    return true;
}

bool uni_action_engine_remove_sequence(
    UniActionEngine* engine,
    const UniRemote* remote,
    size_t action_index) {
    if(!engine || !remote || action_index >= engine->actions.count) return false;
    UniNamedAction* action = &engine->actions.actions[action_index];
    if(action->type != UniActionSequence ||
       uni_action_engine_action_in_use(engine, remote, action->id)) {
        return false;
    }

    UniActionCatalog* backup = catalog_backup(engine);
    if(!backup) return false;
    for(size_t i = action_index; i + 1 < engine->actions.count; i++) {
        engine->actions.actions[i] = engine->actions.actions[i + 1];
    }
    engine->actions.count--;
    memset(&engine->actions.actions[engine->actions.count], 0, sizeof(UniNamedAction));
    return finish_action_mutation(engine, remote, backup);
}

bool uni_action_engine_rename_sequence(
    UniActionEngine* engine,
    const UniRemote* remote,
    size_t action_index,
    const char* id) {
    if(!engine || !remote || action_index >= engine->actions.count || !valid_action_id(id)) {
        return false;
    }
    UniNamedAction* action = &engine->actions.actions[action_index];
    if(action->type != UniActionSequence) return false;
    const int duplicate = catalog_action_index(&engine->actions, id);
    if(duplicate >= 0 && (size_t)duplicate != action_index) return false;
    if(strcmp(action->id, id) != 0 &&
       uni_action_engine_action_in_use(engine, remote, action->id)) {
        return false;
    }

    UniActionCatalog* backup = catalog_backup(engine);
    if(!backup) return false;
    snprintf(action->id, sizeof(action->id), "%s", id);
    return finish_action_mutation(engine, remote, backup);
}

bool uni_action_engine_set_step(
    UniActionEngine* engine,
    const UniRemote* remote,
    size_t action_index,
    size_t step_index,
    const char* binding) {
    if(!engine || !remote || action_index >= engine->actions.count || !binding) return false;
    UniNamedAction* action = &engine->actions.actions[action_index];
    if(action->type != UniActionSequence || step_index >= action->step_count ||
       strlen(binding) >= UNI_SIGNAL_NAME_MAX) {
        return false;
    }

    UniActionCatalog* backup = catalog_backup(engine);
    if(!backup) return false;
    snprintf(action->steps[step_index], sizeof(action->steps[step_index]), "%s", binding);
    return finish_action_mutation(engine, remote, backup);
}

bool uni_action_engine_append_step(
    UniActionEngine* engine,
    const UniRemote* remote,
    size_t action_index,
    const char* binding) {
    if(!engine || !remote || action_index >= engine->actions.count || !binding) return false;
    UniNamedAction* action = &engine->actions.actions[action_index];
    if(action->type != UniActionSequence || action->step_count >= UNI_MAX_SEQUENCE_STEPS ||
       strlen(binding) >= UNI_SIGNAL_NAME_MAX) {
        return false;
    }

    UniActionCatalog* backup = catalog_backup(engine);
    if(!backup) return false;
    const size_t step = action->step_count++;
    snprintf(action->steps[step], sizeof(action->steps[step]), "%s", binding);
    action->delays_ms[step] = 0;
    return finish_action_mutation(engine, remote, backup);
}

bool uni_action_engine_remove_step(
    UniActionEngine* engine,
    const UniRemote* remote,
    size_t action_index,
    size_t step_index) {
    if(!engine || !remote || action_index >= engine->actions.count) return false;
    UniNamedAction* action = &engine->actions.actions[action_index];
    if(action->type != UniActionSequence || step_index >= action->step_count) return false;

    UniActionCatalog* backup = catalog_backup(engine);
    if(!backup) return false;
    for(size_t i = step_index; i + 1 < action->step_count; i++) {
        snprintf(action->steps[i], sizeof(action->steps[i]), "%s", action->steps[i + 1]);
        action->delays_ms[i] = action->delays_ms[i + 1];
    }
    action->step_count--;
    action->steps[action->step_count][0] = '\0';
    action->delays_ms[action->step_count] = 0;
    return finish_action_mutation(engine, remote, backup);
}

bool uni_action_engine_set_delay(
    UniActionEngine* engine,
    const UniRemote* remote,
    size_t action_index,
    size_t step_index,
    uint32_t delay_ms) {
    if(!engine || !remote || action_index >= engine->actions.count ||
       delay_ms > UNI_MAX_SEQUENCE_DELAY_MS) {
        return false;
    }
    UniNamedAction* action = &engine->actions.actions[action_index];
    if(action->type != UniActionSequence || step_index >= action->step_count) return false;

    UniActionCatalog* backup = catalog_backup(engine);
    if(!backup) return false;
    action->delays_ms[step_index] = delay_ms;
    return finish_action_mutation(engine, remote, backup);
}

static int action_index(const UniActionCatalog* catalog, const UniNamedAction* action) {
    if(!catalog || !action) return -1;
    for(size_t i = 0; i < catalog->count; i++) {
        if(&catalog->actions[i] == action) return (int)i;
    }
    return -1;
}

static bool execute_binding_internal(
    UniActionEngine* engine,
    const UniRemote* remote,
    const char* binding,
    bool repeat,
    uint32_t visiting,
    uint8_t depth) {
    if(!engine || !remote || !binding || !binding[0]) return false;
    if(depth > UNI_MAX_ACTIONS) return false;

    const char* signal_name = binding;
    if(strncmp(binding, "sig:", 4) == 0) {
        signal_name = binding + 4;
    } else if(strncmp(binding, "act:", 4) == 0) {
        const UniNamedAction* action = uni_action_find(&engine->actions, binding + 4);
        if(!action) return false;

        const int index = action_index(&engine->actions, action);
        if(index < 0 || index >= 32) return false;
        const uint32_t bit = 1UL << (uint32_t)index;
        if(visiting & bit) return false;

        if(action->type == UniActionSignal) {
            signal_name = action->signal;
        } else {
            if(repeat) return false;
            const uint32_t next_visiting = visiting | bit;
            for(size_t i = 0; i < action->step_count; i++) {
                if(!execute_binding_internal(
                       engine,
                       remote,
                       action->steps[i],
                       false,
                       next_visiting,
                       (uint8_t)(depth + 1))) {
                    return false;
                }
                if(action->delays_ms[i] > 0 && i + 1 < action->step_count) {
                    furi_delay_ms(action->delays_ms[i]);
                }
            }
            return true;
        }
    }

    return uni_ir_transport_send(
        engine->ir,
        remote->signal_path,
        signal_name,
        repeat,
        remote->ir_burst);
}

bool uni_action_engine_execute(
    UniActionEngine* engine,
    const UniRemote* remote,
    const char* binding,
    bool repeat) {
    if(!engine || !remote || !binding || !binding[0]) return false;
    if(remote->transport != UniTransportInfrared) return false;
    return execute_binding_internal(engine, remote, binding, repeat, 0, 0);
}
