#include "state_adapter.h"

#include <furi.h>
#include <furi_hal_rtc.h>
#include <infrared/worker/infrared_transmit.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DAIKIN_IR_FREQUENCY 38000U
#define DAIKIN_IR_DUTY 0.50f
#define DAIKIN_STATE_LEN 35U
#define DAIKIN_TIMING_COUNT 584U

#define DAIKIN_HDR_MARK 3650U
#define DAIKIN_HDR_SPACE 1623U
#define DAIKIN_BIT_MARK 428U
#define DAIKIN_ZERO_SPACE 428U
#define DAIKIN_ONE_SPACE 1280U
#define DAIKIN_GAP 29000U
#define DAIKIN_UNUSED_TIME 0x600U

static const uint8_t fan_native[6] = {
    0xAU, 0x3U, 0x4U, 0x5U, 0x6U, 0x7U,
};

static uint8_t checksum_bytes(const uint8_t* data, uint8_t count) {
    uint8_t sum = 0;
    for(uint8_t i = 0; i < count; i++) sum = (uint8_t)(sum + data[i]);
    return sum;
}

static void daikin_set_time12(
    uint8_t state[DAIKIN_STATE_LEN],
    uint16_t on_time,
    uint16_t off_time) {
    state[26] = (uint8_t)(on_time & 0xFFU);
    state[27] = (uint8_t)(((on_time >> 8) & 0x0FU) | ((off_time & 0x0FU) << 4));
    state[28] = (uint8_t)((off_time >> 4) & 0xFFU);
}

static uint16_t current_minutes(void) {
    DateTime dt;
    furi_hal_rtc_get_datetime(&dt);
    return (uint16_t)(dt.hour * 60U + dt.minute);
}

static uint8_t daikin_mode_native(UniAcMode mode) {
    switch(mode) {
    case UniAcModeDry:
        return 0x2U;
    case UniAcModeFan:
        return 0x6U;
    case UniAcModeCool:
    default:
        return 0x3U;
    }
}

static void daikin_build_state(const UniDeviceState* ac, uint8_t out[DAIKIN_STATE_LEN]) {
    memset(out, 0, DAIKIN_STATE_LEN);

    out[0] = 0x11;
    out[1] = 0xDA;
    out[2] = 0x27;
    out[4] = 0xC5;

    out[8] = 0x11;
    out[9] = 0xDA;
    out[10] = 0x27;
    out[12] = 0x42;

    out[16] = 0x11;
    out[17] = 0xDA;
    out[18] = 0x27;
    out[21] = 0x08;
    out[31] = 0xC0;

    const uint16_t now = current_minutes();
    out[13] = (uint8_t)(now & 0xFFU);
    out[14] = (uint8_t)((now >> 8) & 0x07U);

    if(ac->power) out[21] |= 0x01U;
    if(ac->timer_on_enabled) out[21] |= 0x02U;
    if(ac->timer_off_enabled) out[21] |= 0x04U;
    out[21] |= (uint8_t)(daikin_mode_native(ac->mode) << 4);

    if(ac->mode == UniAcModeCool) {
        out[22] = (uint8_t)(ac->temp_c * 2U);
    } else if(ac->mode == UniAcModeDry) {
        out[22] = 0xC0U;
    } else {
        out[22] = 50U;
    }

    const uint8_t fan = (ac->mode == UniAcModeDry) ? 0xAU : fan_native[ac->fan];
    out[24] = (uint8_t)((fan << 4) | (ac->swing_v ? 0x0FU : 0x00U));

    const uint16_t on_time =
        ac->timer_on_enabled ? ac->timer_on_target_min : DAIKIN_UNUSED_TIME;
    const uint16_t off_time =
        ac->timer_off_enabled ? ac->timer_off_target_min : DAIKIN_UNUSED_TIME;
    daikin_set_time12(out, on_time, off_time);

    if(ac->powerful && ac->power) out[29] |= 0x01U;

    out[7] = checksum_bytes(out, 7);
    out[15] = checksum_bytes(out + 8, 7);
    out[34] = checksum_bytes(out + 16, 18);
}

static void timing_append_bit(
    uint32_t timings[DAIKIN_TIMING_COUNT],
    uint16_t* pos,
    bool one) {
    timings[(*pos)++] = DAIKIN_BIT_MARK;
    timings[(*pos)++] = one ? DAIKIN_ONE_SPACE : DAIKIN_ZERO_SPACE;
}

static void timing_append_section(
    uint32_t timings[DAIKIN_TIMING_COUNT],
    uint16_t* pos,
    const uint8_t* data,
    uint8_t count) {
    timings[(*pos)++] = DAIKIN_HDR_MARK;
    timings[(*pos)++] = DAIKIN_HDR_SPACE;

    for(uint8_t i = 0; i < count; i++) {
        for(uint8_t bit = 0; bit < 8; bit++) {
            timing_append_bit(timings, pos, (data[i] & (1U << bit)) != 0);
        }
    }

    timings[(*pos)++] = DAIKIN_BIT_MARK;
    timings[(*pos)++] = DAIKIN_ZERO_SPACE + DAIKIN_GAP;
}

static void daikin_encode_raw(
    const uint8_t state[DAIKIN_STATE_LEN],
    uint32_t timings[DAIKIN_TIMING_COUNT]) {
    uint16_t pos = 0;

    for(uint8_t i = 0; i < 5; i++) timing_append_bit(timings, &pos, false);
    timings[pos++] = DAIKIN_BIT_MARK;
    timings[pos++] = DAIKIN_ZERO_SPACE + DAIKIN_GAP;

    timing_append_section(timings, &pos, state, 8);
    timing_append_section(timings, &pos, state + 8, 8);
    timing_append_section(timings, &pos, state + 16, 19);

    furi_check(pos == DAIKIN_TIMING_COUNT);
}

static void daikin_send(const UniDeviceState* state) {
    uint8_t frame[DAIKIN_STATE_LEN];
    uint32_t timings[DAIKIN_TIMING_COUNT];
    daikin_build_state(state, frame);
    daikin_encode_raw(frame, timings);
    infrared_send_raw_ext(
        timings,
        DAIKIN_TIMING_COUNT,
        true,
        DAIKIN_IR_FREQUENCY,
        DAIKIN_IR_DUTY);
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

static int8_t operation_delta(const char* operation, const char* field) {
    char plus[32];
    char minus[32];
    snprintf(plus, sizeof(plus), "%s:+", field);
    snprintf(minus, sizeof(minus), "%s:-", field);
    if(strcmp(operation, plus) == 0) return 1;
    if(strcmp(operation, minus) == 0) return -1;
    return 0;
}

static bool mode_uses_temperature(UniAcMode mode) {
    return mode == UniAcModeCool;
}

static bool mode_uses_fan(UniAcMode mode) {
    return mode == UniAcModeCool || mode == UniAcModeFan;
}

static void daikin_defaults(UniDeviceState* state) {
    memset(state, 0, sizeof(UniDeviceState));
    state->power = false;
    state->mode = UniAcModeCool;
    state->temp_c = 23;
    state->fan = 0;
    state->swing_v = 0;
    state->timer_on_target_min = 7U * 60U + 30U;
    state->timer_off_target_min = 23U * 60U;
}

static bool daikin_validate(const UniDeviceState* state) {
    if(!state) return false;
    if(state->mode > UniAcModeFan) return false;
    if(state->temp_c < 18U || state->temp_c > 32U) return false;
    if(state->fan > 5U) return false;
    if(state->swing_v > 1U) return false;
    if(state->swing_h != 0U) return false;
    if(state->timer_on_target_min >= 1440U) return false;
    if(state->timer_off_target_min >= 1440U) return false;
    return true;
}

static void daikin_normalize(UniDeviceState* state) {
    if(!state) return;
    if(state->mode > UniAcModeFan) state->mode = UniAcModeCool;
    if(state->temp_c < 18U) state->temp_c = 18U;
    if(state->temp_c > 32U) state->temp_c = 32U;
    if(state->fan > 5U) state->fan = 0;
    if(state->mode == UniAcModeDry) state->fan = 0;
    if(!state->power) state->powerful = false;
    state->swing_v = state->swing_v ? 1U : 0U;
    state->swing_h = 0;
    state->eco = false;
    state->comfort = false;
    state->auto_clean = false;
    state->purify = false;
    state->jet_dry = false;
    state->display_known = false;
    state->display_assumed_on = false;
    state->fahrenheit = false;
    state->sleep_minutes = 0;
}

static bool daikin_execute(UniDeviceState* state, const char* operation, bool repeat) {
    if(!state || !operation) return false;

    if(strcmp(operation, "power:toggle") == 0) {
        if(repeat) return true;
        state->power = !state->power;
        if(!state->power) state->powerful = false;
        daikin_send(state);
        return true;
    }

    int8_t delta = operation_delta(operation, "mode");
    if(delta) {
        state->mode = (UniAcMode)wrap_u8((uint8_t)state->mode, delta, 3);
        daikin_normalize(state);
        if(state->power) daikin_send(state);
        return true;
    }

    delta = operation_delta(operation, "temp");
    if(delta) {
        if(mode_uses_temperature(state->mode)) {
            int16_t temp = (int16_t)state->temp_c + delta;
            if(temp < 18) temp = 18;
            if(temp > 32) temp = 32;
            state->temp_c = (uint8_t)temp;
            if(state->power) daikin_send(state);
        }
        return true;
    }

    delta = operation_delta(operation, "fan");
    if(delta) {
        if(mode_uses_fan(state->mode)) {
            state->fan = wrap_u8(state->fan, delta, 6);
            if(state->power) daikin_send(state);
        }
        return true;
    }

    if(strcmp(operation, "swing_v:toggle") == 0 ||
       strcmp(operation, "swing:toggle") == 0) {
        if(repeat) return true;
        state->swing_v = state->swing_v ? 0U : 1U;
        if(state->power) daikin_send(state);
        return true;
    }

    if(strcmp(operation, "powerful:toggle") == 0) {
        if(repeat) return true;
        if(state->power) {
            state->powerful = !state->powerful;
            daikin_send(state);
        }
        return true;
    }

    delta = operation_delta(operation, "timer_on");
    if(delta) {
        state->timer_on_target_min =
            time_add(state->timer_on_target_min, (int16_t)delta * 10);
        if(state->timer_on_enabled) daikin_send(state);
        return true;
    }
    if(strcmp(operation, "timer_on:toggle") == 0) {
        if(repeat) return true;
        state->timer_on_enabled = !state->timer_on_enabled;
        daikin_send(state);
        return true;
    }

    delta = operation_delta(operation, "timer_off");
    if(delta) {
        state->timer_off_target_min =
            time_add(state->timer_off_target_min, (int16_t)delta * 10);
        if(state->timer_off_enabled) daikin_send(state);
        return true;
    }
    if(strcmp(operation, "timer_off:toggle") == 0) {
        if(repeat) return true;
        state->timer_off_enabled = !state->timer_off_enabled;
        daikin_send(state);
        return true;
    }

    if(strcmp(operation, "timers:clear") == 0) {
        if(repeat) return true;
        state->timer_on_enabled = false;
        state->timer_off_enabled = false;
        daikin_send(state);
        return true;
    }

    return false;
}

const UniStateAdapter* uni_state_adapter_daikin(void) {
    static const UniStateAdapter adapter = {
        .id = "DAIKIN_ARC433A73",
        .defaults = daikin_defaults,
        .validate = daikin_validate,
        .normalize = daikin_normalize,
        .execute = daikin_execute,
    };
    return &adapter;
}
