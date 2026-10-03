#include "state_engine.h"
#include "state_adapter.h"

#include <furi.h>

#include <flipper_format/flipper_format.h>
#include <stdio.h>
#include <string.h>

#define UNI_STATE_FILETYPE "Flipper Uni Remote State"
#define UNI_STATE_VERSION 1U

static const UniStateAdapter* adapter_for_id(const char* id) {
    if(!id || !id[0]) return NULL;
    const UniStateAdapter* adapters[] = {
        uni_state_adapter_lg(),
        uni_state_adapter_daikin(),
    };
    for(size_t i = 0; i < sizeof(adapters) / sizeof(adapters[0]); i++) {
        if(adapters[i] && strcmp(adapters[i]->id, id) == 0) return adapters[i];
    }
    return NULL;
}

static bool read_u32(FlipperFormat* ff, const char* key, uint32_t* value) {
    flipper_format_rewind(ff);
    return flipper_format_read_uint32(ff, key, value, 1);
}

static bool read_bool(FlipperFormat* ff, const char* key, bool* value) {
    flipper_format_rewind(ff);
    return flipper_format_read_bool(ff, key, value, 1);
}

static bool load_state_file(UniStateEngine* engine) {
    if(!engine || !engine->storage || !engine->adapter || !engine->state_path[0]) return false;
    if(!storage_file_exists(engine->storage, engine->state_path)) return false;

    FlipperFormat* ff = flipper_format_file_alloc(engine->storage);
    if(!ff) return false;

    FuriString* filetype = furi_string_alloc();
    uint32_t version = 0;
    uint32_t value = 0;
    bool ok = false;
    UniDeviceState loaded = engine->state;

    do {
        if(!flipper_format_file_open_existing(ff, engine->state_path)) break;
        if(!flipper_format_read_header(ff, filetype, &version)) break;
        if(strcmp(furi_string_get_cstr(filetype), UNI_STATE_FILETYPE) != 0) break;
        if(version != UNI_STATE_VERSION) break;

        if(!read_bool(ff, "Power", &loaded.power)) break;
        if(!read_u32(ff, "Mode", &value)) break;
        loaded.mode = (UniAcMode)value;
        if(!read_u32(ff, "TempC", &value)) break;
        loaded.temp_c = (uint8_t)value;
        if(!read_u32(ff, "AutoBias", &value)) break;
        loaded.auto_bias = (int8_t)((int32_t)value - 2);
        if(!read_u32(ff, "Fan", &value)) break;
        loaded.fan = (uint8_t)value;
        if(!read_u32(ff, "SwingV", &value)) break;
        loaded.swing_v = (uint8_t)value;
        if(!read_u32(ff, "SwingH", &value)) break;
        loaded.swing_h = (uint8_t)value;

        if(!read_bool(ff, "Powerful", &loaded.powerful)) break;
        if(!read_bool(ff, "Eco", &loaded.eco)) break;
        if(!read_bool(ff, "Comfort", &loaded.comfort)) break;
        if(!read_bool(ff, "AutoClean", &loaded.auto_clean)) break;
        if(!read_bool(ff, "Purify", &loaded.purify)) break;
        if(!read_bool(ff, "JetDry", &loaded.jet_dry)) break;
        if(!read_bool(ff, "DisplayKnown", &loaded.display_known)) break;
        if(!read_bool(ff, "DisplayOn", &loaded.display_assumed_on)) break;
        if(!read_bool(ff, "Fahrenheit", &loaded.fahrenheit)) break;

        if(!read_bool(ff, "TimerOnEnabled", &loaded.timer_on_enabled)) break;
        if(!read_u32(ff, "TimerOnMin", &value)) break;
        loaded.timer_on_target_min = (uint16_t)value;
        if(!read_bool(ff, "TimerOffEnabled", &loaded.timer_off_enabled)) break;
        if(!read_u32(ff, "TimerOffMin", &value)) break;
        loaded.timer_off_target_min = (uint16_t)value;
        if(!read_u32(ff, "SleepMin", &value)) break;
        loaded.sleep_minutes = (uint16_t)value;

        engine->adapter->normalize(&loaded);
        if(!engine->adapter->validate(&loaded)) break;
        engine->state = loaded;
        ok = true;
    } while(false);

    flipper_format_file_close(ff);
    furi_string_free(filetype);
    flipper_format_free(ff);
    return ok;
}

void uni_state_engine_init(UniStateEngine* engine, Storage* storage) {
    if(!engine) return;
    memset(engine, 0, sizeof(UniStateEngine));
    engine->storage = storage;
}

bool uni_state_engine_flush(UniStateEngine* engine) {
    if(!engine || !engine->loaded || !engine->dirty || !engine->storage ||
       !engine->adapter || !engine->state_path[0]) {
        return true;
    }

    FlipperFormat* ff = flipper_format_file_alloc(engine->storage);
    if(!ff) return false;

    bool ok = false;
    do {
        if(!flipper_format_file_open_always(ff, engine->state_path)) break;
        if(!flipper_format_write_header_cstr(ff, UNI_STATE_FILETYPE, UNI_STATE_VERSION)) break;
        if(!flipper_format_write_bool(ff, "Power", &engine->state.power, 1)) break;

        uint32_t value = (uint32_t)engine->state.mode;
        if(!flipper_format_write_uint32(ff, "Mode", &value, 1)) break;
        value = engine->state.temp_c;
        if(!flipper_format_write_uint32(ff, "TempC", &value, 1)) break;
        value = (uint32_t)(engine->state.auto_bias + 2);
        if(!flipper_format_write_uint32(ff, "AutoBias", &value, 1)) break;
        value = engine->state.fan;
        if(!flipper_format_write_uint32(ff, "Fan", &value, 1)) break;
        value = engine->state.swing_v;
        if(!flipper_format_write_uint32(ff, "SwingV", &value, 1)) break;
        value = engine->state.swing_h;
        if(!flipper_format_write_uint32(ff, "SwingH", &value, 1)) break;

        if(!flipper_format_write_bool(ff, "Powerful", &engine->state.powerful, 1)) break;
        if(!flipper_format_write_bool(ff, "Eco", &engine->state.eco, 1)) break;
        if(!flipper_format_write_bool(ff, "Comfort", &engine->state.comfort, 1)) break;
        if(!flipper_format_write_bool(ff, "AutoClean", &engine->state.auto_clean, 1)) break;
        if(!flipper_format_write_bool(ff, "Purify", &engine->state.purify, 1)) break;
        if(!flipper_format_write_bool(ff, "JetDry", &engine->state.jet_dry, 1)) break;
        if(!flipper_format_write_bool(ff, "DisplayKnown", &engine->state.display_known, 1)) break;
        if(!flipper_format_write_bool(ff, "DisplayOn", &engine->state.display_assumed_on, 1)) break;
        if(!flipper_format_write_bool(ff, "Fahrenheit", &engine->state.fahrenheit, 1)) break;

        if(!flipper_format_write_bool(ff, "TimerOnEnabled", &engine->state.timer_on_enabled, 1))
            break;
        value = engine->state.timer_on_target_min;
        if(!flipper_format_write_uint32(ff, "TimerOnMin", &value, 1)) break;
        if(!flipper_format_write_bool(ff, "TimerOffEnabled", &engine->state.timer_off_enabled, 1))
            break;
        value = engine->state.timer_off_target_min;
        if(!flipper_format_write_uint32(ff, "TimerOffMin", &value, 1)) break;
        value = engine->state.sleep_minutes;
        if(!flipper_format_write_uint32(ff, "SleepMin", &value, 1)) break;

        ok = true;
    } while(false);

    flipper_format_file_close(ff);
    flipper_format_free(ff);
    if(ok) engine->dirty = false;
    return ok;
}

void uni_state_engine_unload(UniStateEngine* engine) {
    if(!engine) return;
    uni_state_engine_flush(engine);
    Storage* storage = engine->storage;
    memset(engine, 0, sizeof(UniStateEngine));
    engine->storage = storage;
}

bool uni_state_engine_load(UniStateEngine* engine, const UniRemote* remote) {
    if(!engine || !engine->storage || !remote ||
       remote->transport != UniTransportStatefulIr) {
        return false;
    }

    if(engine->loaded && strcmp(engine->remote_id, remote->id) == 0) return true;
    uni_state_engine_unload(engine);

    engine->adapter = adapter_for_id(remote->state_adapter);
    if(!engine->adapter) return false;

    snprintf(engine->remote_id, sizeof(engine->remote_id), "%s", remote->id);
    snprintf(engine->state_path, sizeof(engine->state_path), "%s", remote->state_path);
    engine->adapter->defaults(&engine->state);
    engine->adapter->normalize(&engine->state);
    engine->loaded = true;
    engine->dirty = false;

    if(!load_state_file(engine)) {
        engine->adapter->defaults(&engine->state);
        engine->adapter->normalize(&engine->state);
    }
    return engine->adapter->validate(&engine->state);
}

bool uni_state_engine_execute(
    UniStateEngine* engine,
    const UniRemote* remote,
    const char* binding,
    bool repeat) {
    if(!engine || !remote || !binding || strncmp(binding, "state:", 6) != 0) return false;
    if(!uni_state_engine_load(engine, remote)) return false;

    const UniDeviceState before = engine->state;
    if(!engine->adapter->execute(&engine->state, binding + 6, repeat)) {
        engine->state = before;
        return false;
    }

    engine->adapter->normalize(&engine->state);
    if(!engine->adapter->validate(&engine->state)) {
        engine->state = before;
        return false;
    }

    if(memcmp(&before, &engine->state, sizeof(UniDeviceState)) != 0) engine->dirty = true;
    return true;
}

void uni_state_engine_summary(const UniStateEngine* engine, char* out, size_t out_size) {
    if(!out || out_size == 0) return;
    out[0] = '\0';
    if(!engine || !engine->loaded || !engine->adapter) return;

    static const char* modes[] = {"COOL", "DRY", "FAN", "AUTO", "HEAT"};
    const uint8_t mode = (uint8_t)engine->state.mode;
    const char* mode_name = mode < (sizeof(modes) / sizeof(modes[0])) ? modes[mode] : "?";

    if(!engine->state.power) {
        snprintf(out, out_size, "OFF %uC", engine->state.temp_c);
    } else if(engine->state.mode == UniAcModeAuto) {
        snprintf(out, out_size, "A%+d %s", engine->state.auto_bias, mode_name);
    } else {
        snprintf(out, out_size, "%uC %s", engine->state.temp_c, mode_name);
    }
}
