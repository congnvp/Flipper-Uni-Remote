#include "action_engine.h"

#include <flipper_format/flipper_format.h>
#include <furi.h>
#include <stdio.h>
#include <string.h>

#define UNI_ACTION_FILETYPE "Flipper Uni Remote Actions"
#define UNI_ACTION_VERSION 1U

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
    load_signal_names(engine->storage, remote->signal_path, &engine->signals);
    return load_actions(engine->storage, remote->action_path, &engine->actions);
}

const UniNamedAction* uni_action_find(const UniActionCatalog* catalog, const char* id) {
    if(!catalog || !id) return NULL;
    for(size_t i = 0; i < catalog->count; i++) {
        if(strcmp(catalog->actions[i].id, id) == 0) return &catalog->actions[i];
    }
    return NULL;
}

bool uni_action_engine_execute(
    UniActionEngine* engine,
    const UniRemote* remote,
    const char* binding,
    bool repeat) {
    if(!engine || !remote || !binding || !binding[0]) return false;
    if(remote->transport != UniTransportInfrared) return false;

    const char* signal_name = binding;
    if(strncmp(binding, "sig:", 4) == 0) {
        signal_name = binding + 4;
    } else if(strncmp(binding, "act:", 4) == 0) {
        const UniNamedAction* action = uni_action_find(&engine->actions, binding + 4);
        if(!action) return false;

        if(action->type == UniActionSignal) {
            signal_name = action->signal;
        } else {
            if(repeat) return false;
            bool ok = true;
            for(size_t i = 0; i < action->step_count; i++) {
                if(!uni_ir_transport_send(
                       engine->ir,
                       remote->signal_path,
                       action->steps[i],
                       false,
                       remote->ir_burst)) {
                    ok = false;
                    break;
                }
                if(action->delays_ms[i] > 0 && i + 1 < action->step_count) {
                    furi_delay_ms(action->delays_ms[i]);
                }
            }
            return ok;
        }
    }

    return uni_ir_transport_send(
        engine->ir,
        remote->signal_path,
        signal_name,
        repeat,
        remote->ir_burst);
}
