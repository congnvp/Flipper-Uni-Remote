#include "state_adapter.h"

#include <furi.h>
#include <furi_hal_rtc.h>
#include <infrared/worker/infrared_transmit.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LG_ADDRESS 0x88U
#define LG_IR_FREQUENCY 38000U
#define LG_IR_DUTY 0.33f
#define LG_FRAME_TIMINGS 59U
#define LG_COMMAND_GAP_MS 120U

#define LG_CMD_POWER_OFF     0x88C0051UL
#define LG_CMD_ENERGY_ON     0x8810045UL
#define LG_CMD_ENERGY_OFF    0x8810056UL
#define LG_CMD_JET_ON        0x8810089UL
#define LG_CMD_LIGHT_TOGGLE  0x88C00A6UL
#define LG_CMD_COMFORT_ON    0x88C08F3UL
#define LG_CMD_COMFORT_OFF   0x88A000AUL
#define LG_CMD_DIAGNOSIS     0x88C0CE6UL
#define LG_CMD_C_TO_F        0x88C0174UL
#define LG_CMD_F_TO_C        0x88C0163UL
#define LG_CMD_AUTOCLEAN_ON  0x88C00B7UL
#define LG_CMD_AUTOCLEAN_OFF 0x88C00C8UL
#define LG_CMD_PURIFY_ON     0x88C000CUL
#define LG_CMD_PURIFY_OFF    0x88C0084UL
#define LG_CMD_JETDRY_ON     0x88C096BUL
#define LG_CMD_JETDRY_OFF    0x88C097CUL

#define LG_CMD_TIMER_ON_BASE  0x8000U
#define LG_CMD_TIMER_OFF_BASE 0x9000U
#define LG_CMD_SLEEP_BASE     0xA000U
#define LG_CMD_CLEAR_TIMERS   0xB000U

static const uint8_t fan_native[6] = {
    0x5, 0x0, 0x9, 0x2, 0xA, 0x4,
};

static const uint32_t swing_v_code[8] = {
    0x881315AUL,
    0x8813048UL,
    0x8813059UL,
    0x881306AUL,
    0x881307BUL,
    0x881308CUL,
    0x881309DUL,
    0x8813149UL,
};

static const uint32_t swing_h_code[9] = {
    0x881317CUL,
    0x88130BFUL,
    0x88130C0UL,
    0x88130D1UL,
    0x88130E2UL,
    0x88130F3UL,
    0x8813105UL,
    0x8813116UL,
    0x881316BUL,
};

static uint8_t lg_checksum16(uint16_t command) {
    uint8_t sum = 0;
    sum += (command >> 0) & 0xF;
    sum += (command >> 4) & 0xF;
    sum += (command >> 8) & 0xF;
    sum += (command >> 12) & 0xF;
    return sum & 0xF;
}

static uint32_t lg_raw_from_command(uint16_t command) {
    return ((uint32_t)LG_ADDRESS << 20) | ((uint32_t)command << 4) | lg_checksum16(command);
}

static uint16_t lg_build_state_command(const UniDeviceState* state, bool power_on_frame) {
    uint16_t command = 0;

    switch(state->mode) {
    case UniAcModeDry:
        command = 0x0990U;
        break;
    case UniAcModeFan:
        command = 0x0A00U;
        command |= (uint16_t)(3U << 4);
        command |= fan_native[state->fan];
        break;
    case UniAcModeAuto: {
        const uint8_t auto_field = (uint8_t)(state->auto_bias + 2);
        command = 0x0B00U;
        command |= (uint16_t)(auto_field << 4);
        command |= 0x5U;
        break;
    }
    case UniAcModeHeat:
        command = 0x0C00U;
        command |= (uint16_t)((state->temp_c - 15U) << 4);
        command |= fan_native[state->fan];
        break;
    case UniAcModeCool:
    default:
        command = 0x0800U;
        command |= (uint16_t)((state->temp_c - 15U) << 4);
        command |= fan_native[state->fan];
        break;
    }

    if(power_on_frame) command &= (uint16_t)~0x0800U;
    return command;
}

static uint32_t lg_build_state_raw(const UniDeviceState* state, bool power_on_frame) {
    return lg_raw_from_command(lg_build_state_command(state, power_on_frame));
}

static void lg_encode_raw(uint32_t code, uint32_t timings[LG_FRAME_TIMINGS]) {
    uint8_t i = 0;
    timings[i++] = 3200;
    timings[i++] = 9900;
    for(int8_t bit = 27; bit >= 0; bit--) {
        timings[i++] = 480;
        timings[i++] = (code & (1UL << bit)) ? 1600 : 550;
    }
    timings[i++] = 480;
    furi_check(i == LG_FRAME_TIMINGS);
}

static void lg_send_code(uint32_t code) {
    uint32_t timings[LG_FRAME_TIMINGS];
    lg_encode_raw(code, timings);
    infrared_send_raw_ext(timings, LG_FRAME_TIMINGS, true, LG_IR_FREQUENCY, LG_IR_DUTY);
}

static void lg_send_command16(uint16_t command) {
    lg_send_code(lg_raw_from_command(command));
}

static void lg_send_full_state(const UniDeviceState* state, bool power_on_frame) {
    lg_send_code(lg_build_state_raw(state, power_on_frame));
}

static uint16_t current_minutes(void) {
    DateTime dt;
    furi_hal_rtc_get_datetime(&dt);
    return (uint16_t)(dt.hour * 60U + dt.minute);
}

static uint16_t relative_minutes(uint16_t target) {
    const uint16_t now = current_minutes();
    return (uint16_t)((target + 1440U - now) % 1440U);
}

static uint16_t time_add(uint16_t value, int16_t delta) {
    int32_t v = (int32_t)value + delta;
    while(v < 0) v += 1440;
    while(v >= 1440) v -= 1440;
    return (uint16_t)v;
}

static uint8_t wrap_u8(uint8_t value, int8_t delta, uint8_t count) {
    int16_t out = (int16_t)value + delta;
    while(out < 0) out += count;
    while(out >= count) out -= count;
    return (uint8_t)out;
}

static bool mode_uses_temperature(UniAcMode mode) {
    return mode == UniAcModeCool || mode == UniAcModeHeat;
}

static bool mode_uses_fan(UniAcMode mode) {
    return mode == UniAcModeCool || mode == UniAcModeHeat || mode == UniAcModeFan;
}

static int8_t operation_delta(const char* operation, const char* field) {
    char plus[32];
    char minus[32];
    snprintf(plus, sizeof(plus), "%s:+", field);
    snprintf(minus, sizeof(minus), "%s:-", field);
    if(strcmp(operation, plus) == 0) return 1;
    if(strcmp(operation, minus) == 0) return -1;
    return 0;
}

static void timer_send_on(const UniDeviceState* state) {
    lg_send_command16((uint16_t)(LG_CMD_TIMER_ON_BASE + relative_minutes(state->timer_on_target_min)));
}

static void timer_send_off(const UniDeviceState* state) {
    lg_send_command16((uint16_t)(LG_CMD_TIMER_OFF_BASE + relative_minutes(state->timer_off_target_min)));
}

static void timer_send_sleep(const UniDeviceState* state) {
    lg_send_command16((uint16_t)(LG_CMD_SLEEP_BASE + state->sleep_minutes));
}

static void timer_resync(const UniDeviceState* state) {
    lg_send_command16(LG_CMD_CLEAR_TIMERS);
    if(state->timer_on_enabled) {
        furi_delay_ms(LG_COMMAND_GAP_MS);
        timer_send_on(state);
    }
    if(state->timer_off_enabled) {
        furi_delay_ms(LG_COMMAND_GAP_MS);
        timer_send_off(state);
    }
    if(state->sleep_minutes) {
        furi_delay_ms(LG_COMMAND_GAP_MS);
        timer_send_sleep(state);
    }
}

static void send_aux_state_after_power_on(const UniDeviceState* state) {
    furi_delay_ms(LG_COMMAND_GAP_MS);
    lg_send_code(swing_v_code[state->swing_v]);
    furi_delay_ms(LG_COMMAND_GAP_MS);
    lg_send_code(swing_h_code[state->swing_h]);
    furi_delay_ms(LG_COMMAND_GAP_MS);
    lg_send_code(state->eco ? LG_CMD_ENERGY_ON : LG_CMD_ENERGY_OFF);

    if(state->auto_clean) {
        furi_delay_ms(LG_COMMAND_GAP_MS);
        lg_send_code(LG_CMD_AUTOCLEAN_ON);
    }
    if(state->purify) {
        furi_delay_ms(LG_COMMAND_GAP_MS);
        lg_send_code(LG_CMD_PURIFY_ON);
    }
    if(state->jet_dry) {
        furi_delay_ms(LG_COMMAND_GAP_MS);
        lg_send_code(LG_CMD_JETDRY_ON);
    }
    if(state->comfort) {
        furi_delay_ms(LG_COMMAND_GAP_MS);
        lg_send_code(LG_CMD_COMFORT_ON);
    }
    if(state->powerful) {
        furi_delay_ms(LG_COMMAND_GAP_MS);
        lg_send_code(LG_CMD_JET_ON);
    }
}

static void lg_defaults(UniDeviceState* state) {
    memset(state, 0, sizeof(UniDeviceState));
    state->power = false;
    state->mode = UniAcModeCool;
    state->temp_c = 23;
    state->auto_bias = 0;
    state->fan = 0;
    state->swing_v = 0;
    state->swing_h = 0;
    state->timer_on_target_min = 7U * 60U + 30U;
    state->timer_off_target_min = 23U * 60U;
}

static bool lg_validate(const UniDeviceState* state) {
    if(!state) return false;
    if(state->mode > UniAcModeHeat) return false;
    if(state->temp_c < 18U || state->temp_c > 30U) return false;
    if(state->auto_bias < -2 || state->auto_bias > 2) return false;
    if(state->fan > 5U) return false;
    if(state->swing_v > 7U) return false;
    if(state->swing_h > 8U) return false;
    if(state->timer_on_target_min >= 1440U) return false;
    if(state->timer_off_target_min >= 1440U) return false;
    if(state->sleep_minutes > 420U) return false;
    return true;
}

static void lg_normalize(UniDeviceState* state) {
    if(!state) return;
    if(state->temp_c < 18U) state->temp_c = 18U;
    if(state->temp_c > 30U) state->temp_c = 30U;
    if(state->auto_bias < -2) state->auto_bias = -2;
    if(state->auto_bias > 2) state->auto_bias = 2;
    if(state->fan > 5U) state->fan = 0;
    if(state->swing_v > 7U) state->swing_v = 0;
    if(state->swing_h > 8U) state->swing_h = 0;
}

static bool lg_execute(UniDeviceState* state, const char* operation, bool repeat) {
    if(!state || !operation) return false;

    if(strcmp(operation, "power:toggle") == 0) {
        if(repeat) return true;
        if(state->power) {
            lg_send_code(LG_CMD_POWER_OFF);
            state->power = false;
            state->powerful = false;
        } else {
            lg_send_full_state(state, true);
            state->power = true;
            send_aux_state_after_power_on(state);
        }
        return true;
    }

    int8_t delta = operation_delta(operation, "mode");
    if(delta) {
        state->powerful = false;
        state->mode = (UniAcMode)wrap_u8((uint8_t)state->mode, delta, 5);
        if(state->power) lg_send_full_state(state, false);
        return true;
    }

    delta = operation_delta(operation, "temp");
    if(delta) {
        state->powerful = false;
        if(state->mode == UniAcModeAuto) {
            int8_t bias = state->auto_bias + delta;
            if(bias < -2) bias = -2;
            if(bias > 2) bias = 2;
            state->auto_bias = bias;
            if(state->power) lg_send_full_state(state, false);
        } else if(mode_uses_temperature(state->mode)) {
            int16_t temp = (int16_t)state->temp_c + delta;
            if(temp < 18) temp = 18;
            if(temp > 30) temp = 30;
            state->temp_c = (uint8_t)temp;
            if(state->power) lg_send_full_state(state, false);
        }
        return true;
    }

    delta = operation_delta(operation, "fan");
    if(delta) {
        if(mode_uses_fan(state->mode)) {
            state->powerful = false;
            state->fan = wrap_u8(state->fan, delta, 6);
            if(state->power) lg_send_full_state(state, false);
        }
        return true;
    }

    delta = operation_delta(operation, "swing_v");
    if(delta) {
        state->swing_v = wrap_u8(state->swing_v, delta, 8);
        if(state->power) lg_send_code(swing_v_code[state->swing_v]);
        return true;
    }

    delta = operation_delta(operation, "swing_h");
    if(delta) {
        state->swing_h = wrap_u8(state->swing_h, delta, 9);
        if(state->power) lg_send_code(swing_h_code[state->swing_h]);
        return true;
    }

    if(strcmp(operation, "jet:toggle") == 0) {
        if(repeat) return true;
        if(!state->powerful) {
            if(!state->power) {
                lg_send_full_state(state, true);
                state->power = true;
                furi_delay_ms(LG_COMMAND_GAP_MS);
            }
            lg_send_code(LG_CMD_JET_ON);
            state->powerful = true;
        } else {
            state->powerful = false;
            if(state->power) lg_send_full_state(state, false);
        }
        return true;
    }

    if(strcmp(operation, "eco:toggle") == 0) {
        if(repeat) return true;
        state->eco = !state->eco;
        if(state->power) lg_send_code(state->eco ? LG_CMD_ENERGY_ON : LG_CMD_ENERGY_OFF);
        return true;
    }

    if(strcmp(operation, "comfort:toggle") == 0) {
        if(repeat) return true;
        state->comfort = !state->comfort;
        if(state->power) lg_send_code(state->comfort ? LG_CMD_COMFORT_ON : LG_CMD_COMFORT_OFF);
        return true;
    }

    if(strcmp(operation, "display:toggle") == 0) {
        if(repeat) return true;
        lg_send_code(LG_CMD_LIGHT_TOGGLE);
        if(!state->display_known) {
            state->display_known = true;
            state->display_assumed_on = false;
        } else {
            state->display_assumed_on = !state->display_assumed_on;
        }
        return true;
    }

    if(strcmp(operation, "auto_clean:toggle") == 0) {
        if(repeat) return true;
        state->auto_clean = !state->auto_clean;
        if(state->power)
            lg_send_code(state->auto_clean ? LG_CMD_AUTOCLEAN_ON : LG_CMD_AUTOCLEAN_OFF);
        return true;
    }

    if(strcmp(operation, "purify:toggle") == 0) {
        if(repeat) return true;
        state->purify = !state->purify;
        if(state->power) lg_send_code(state->purify ? LG_CMD_PURIFY_ON : LG_CMD_PURIFY_OFF);
        return true;
    }

    if(strcmp(operation, "jet_dry:toggle") == 0) {
        if(repeat) return true;
        state->jet_dry = !state->jet_dry;
        if(state->power) lg_send_code(state->jet_dry ? LG_CMD_JETDRY_ON : LG_CMD_JETDRY_OFF);
        return true;
    }

    delta = operation_delta(operation, "timer_on");
    if(delta) {
        state->timer_on_target_min = time_add(state->timer_on_target_min, (int16_t)delta * 10);
        if(state->timer_on_enabled) timer_send_on(state);
        return true;
    }
    if(strcmp(operation, "timer_on:toggle") == 0) {
        if(repeat) return true;
        state->timer_on_enabled = !state->timer_on_enabled;
        timer_resync(state);
        return true;
    }

    delta = operation_delta(operation, "timer_off");
    if(delta) {
        state->timer_off_target_min = time_add(state->timer_off_target_min, (int16_t)delta * 10);
        if(state->timer_off_enabled) timer_send_off(state);
        return true;
    }
    if(strcmp(operation, "timer_off:toggle") == 0) {
        if(repeat) return true;
        state->timer_off_enabled = !state->timer_off_enabled;
        timer_resync(state);
        return true;
    }

    delta = operation_delta(operation, "sleep");
    if(delta) {
        int16_t hours = (int16_t)(state->sleep_minutes / 60U) + delta;
        if(hours < 0) hours = 0;
        if(hours > 7) hours = 7;
        state->sleep_minutes = (uint16_t)hours * 60U;
        timer_send_sleep(state);
        return true;
    }

    if(strcmp(operation, "timers:clear") == 0) {
        if(repeat) return true;
        state->timer_on_enabled = false;
        state->timer_off_enabled = false;
        state->sleep_minutes = 0;
        lg_send_command16(LG_CMD_CLEAR_TIMERS);
        return true;
    }

    if(strcmp(operation, "unit:toggle") == 0) {
        if(repeat) return true;
        lg_send_code(state->fahrenheit ? LG_CMD_F_TO_C : LG_CMD_C_TO_F);
        state->fahrenheit = !state->fahrenheit;
        return true;
    }

    if(strcmp(operation, "diagnosis:send") == 0) {
        if(repeat) return true;
        lg_send_code(LG_CMD_DIAGNOSIS);
        return true;
    }

    return false;
}

const UniStateAdapter* uni_state_adapter_lg(void) {
    static const UniStateAdapter adapter = {
        .id = "LG_AC",
        .defaults = lg_defaults,
        .validate = lg_validate,
        .normalize = lg_normalize,
        .execute = lg_execute,
    };
    return &adapter;
}
