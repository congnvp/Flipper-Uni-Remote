#include "stateful_ir.h"

#include <furi.h>
#include <furi_hal_rtc.h>
#include <infrared/worker/infrared_transmit.h>
#include <toolbox/saved_struct.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "UniStateIr"

#define UNI_STATE_MAGIC 0x55U
#define UNI_STATE_VERSION 1U

typedef enum {
    DriverNone = 0,
    DriverLg,
    DriverDaikin,
    DriverPanasonic,
    DriverCarrier,
} DriverKind;

/* ---------- LG AKB75215401 / LG2 ---------- */

typedef enum {
    LgModeCool = 0,
    LgModeDry,
    LgModeFan,
    LgModeAuto,
    LgModeHeat,
} LgMode;

typedef enum {
    LgFanAuto = 0,
    LgFan1,
    LgFan2,
    LgFan3,
    LgFan4,
    LgFan5,
} LgFan;

typedef struct {
    bool power;
    uint8_t mode;
    uint8_t temp_c;
    int8_t auto_bias;
    uint8_t fan;
    uint8_t swing_v;
    uint8_t swing_h;
    bool jet;
    bool eco;
    bool comfort;
    bool auto_clean;
    bool purify;
    bool jet_dry;
    bool fahrenheit;
} LgState;

/* ---------- Daikin ARC433A73 ---------- */

typedef enum {
    DaikinModeCool = 0,
    DaikinModeDry,
    DaikinModeFan,
} DaikinMode;

typedef struct {
    bool power;
    uint8_t mode;
    uint8_t temp_c;
    uint8_t fan;
    bool swing;
    bool powerful;
    bool timer_on_enabled;
    uint16_t timer_on_target_min;
    bool timer_off_enabled;
    uint16_t timer_off_target_min;
} DaikinState;

/* ---------- Panasonic ---------- */

typedef enum {
    PanaModeAuto = 0,
    PanaModeCool,
    PanaModeDry,
} PanaMode;

typedef enum {
    PanaFanQuiet = 0,
    PanaFan1,
    PanaFan2,
    PanaFan3,
    PanaFan4,
    PanaFan5,
    PanaFanAuto,
} PanaFan;

typedef enum {
    PanaExtraOff = 0,
    PanaExtraPowerful,
    PanaExtraEco,
} PanaExtra;

typedef struct {
    bool power;
    uint8_t mode;
    uint8_t temp_half;
    uint8_t fan;
    uint8_t swing;
    bool nanoe;
    uint8_t extra;
    uint8_t sleep;
} PanasonicState;

/* ---------- Carrier WC-UA04NE / Toshiba family ---------- */

typedef enum {
    CarrierModeAuto = 0,
    CarrierModeCool,
    CarrierModeDry,
    CarrierModeFan,
} CarrierMode;

typedef struct {
    bool power;
    uint8_t mode;
    uint8_t temp_c;
    uint8_t fan;
    bool hi_power;
    bool eco;
} CarrierState;

typedef union {
    LgState lg;
    DaikinState daikin;
    PanasonicState panasonic;
    CarrierState carrier;
} DriverState;

typedef struct {
    uint8_t kind;
    DriverState state;
} PersistedState;

struct UniStatefulIr {
    Storage* storage;
    DriverKind kind;
    DriverState state;
    char state_path[UNI_PATH_MAX];
    bool loaded;
};

/* -------------------------------------------------------------------------- */
/* Common persistence                                                         */
/* -------------------------------------------------------------------------- */

static DriverKind driver_from_name(const char* name) {
    if(!name) return DriverNone;
    if(strcmp(name, "lg_ac") == 0 || strcmp(name, "lg_akb75215401") == 0) return DriverLg;
    if(strcmp(name, "daikin_ac") == 0 || strcmp(name, "daikin_arc433a73") == 0)
        return DriverDaikin;
    if(strcmp(name, "panasonic_ac") == 0) return DriverPanasonic;
    if(strcmp(name, "carrier_ac") == 0 || strcmp(name, "carrier_wc_ua4ne") == 0)
        return DriverCarrier;
    return DriverNone;
}

static void state_defaults(UniStatefulIr* s) {
    memset(&s->state, 0, sizeof(s->state));

    switch(s->kind) {
    case DriverLg:
        s->state.lg.power = false;
        s->state.lg.mode = LgModeCool;
        s->state.lg.temp_c = 23U;
        s->state.lg.auto_bias = 0;
        s->state.lg.fan = LgFanAuto;
        s->state.lg.swing_v = 0U;
        s->state.lg.swing_h = 0U;
        break;

    case DriverDaikin:
        s->state.daikin.power = false;
        s->state.daikin.mode = DaikinModeCool;
        s->state.daikin.temp_c = 23U;
        s->state.daikin.fan = 0U;
        s->state.daikin.swing = false;
        s->state.daikin.powerful = false;
        s->state.daikin.timer_on_target_min = 7U * 60U + 30U;
        s->state.daikin.timer_off_target_min = 23U * 60U;
        break;

    case DriverPanasonic:
        s->state.panasonic.power = false;
        s->state.panasonic.mode = PanaModeAuto;
        s->state.panasonic.temp_half = 50U;
        s->state.panasonic.fan = PanaFanAuto;
        s->state.panasonic.swing = 1U; /* Swing2 from original app */
        s->state.panasonic.nanoe = false;
        s->state.panasonic.extra = PanaExtraOff;
        s->state.panasonic.sleep = 0U;
        break;

    case DriverCarrier:
        s->state.carrier.power = false;
        s->state.carrier.mode = CarrierModeCool;
        s->state.carrier.temp_c = 24U;
        s->state.carrier.fan = 0U;
        s->state.carrier.hi_power = false;
        s->state.carrier.eco = false;
        break;

    case DriverNone:
    default:
        break;
    }
}

static bool state_valid(const UniStatefulIr* s, const DriverState* state) {
    if(!s || !state) return false;

    switch(s->kind) {
    case DriverLg:
        return state->lg.mode <= LgModeHeat &&
               state->lg.temp_c >= 18U && state->lg.temp_c <= 30U &&
               state->lg.auto_bias >= -2 && state->lg.auto_bias <= 2 &&
               state->lg.fan <= LgFan5 &&
               state->lg.swing_v <= 7U && state->lg.swing_h <= 8U;

    case DriverDaikin:
        return state->daikin.mode <= DaikinModeFan &&
               state->daikin.temp_c >= 18U && state->daikin.temp_c <= 32U &&
               state->daikin.fan <= 5U &&
               state->daikin.timer_on_target_min < 1440U &&
               state->daikin.timer_off_target_min < 1440U;

    case DriverPanasonic:
        return state->panasonic.mode <= PanaModeDry &&
               state->panasonic.temp_half >= 32U && state->panasonic.temp_half <= 60U &&
               state->panasonic.fan <= PanaFanAuto &&
               state->panasonic.swing <= 5U &&
               state->panasonic.extra <= PanaExtraEco &&
               state->panasonic.sleep <= 10U;

    case DriverCarrier:
        return state->carrier.mode <= CarrierModeFan &&
               state->carrier.temp_c >= 17U && state->carrier.temp_c <= 30U &&
               state->carrier.fan <= 5U &&
               !(state->carrier.hi_power && state->carrier.eco);

    case DriverNone:
    default:
        return false;
    }
}

static bool save_state(const UniStatefulIr* s) {
    if(!s || !s->loaded || !s->state_path[0]) return false;
    PersistedState p = {
        .kind = (uint8_t)s->kind,
        .state = s->state,
    };
    return saved_struct_save(
        s->state_path,
        &p,
        sizeof(p),
        UNI_STATE_MAGIC,
        UNI_STATE_VERSION);
}

static void load_saved_state(UniStatefulIr* s) {
    PersistedState p = {0};
    if(saved_struct_load(
           s->state_path,
           &p,
           sizeof(p),
           UNI_STATE_MAGIC,
           UNI_STATE_VERSION) &&
       p.kind == (uint8_t)s->kind &&
       state_valid(s, &p.state)) {
        s->state = p.state;
    }
}

/* -------------------------------------------------------------------------- */
/* Helpers                                                                    */
/* -------------------------------------------------------------------------- */

static int8_t wrap_delta(int8_t value, int8_t delta, int8_t count) {
    int8_t out = (int8_t)(value + delta);
    while(out < 0) out = (int8_t)(out + count);
    while(out >= count) out = (int8_t)(out - count);
    return out;
}

static uint16_t time_add(uint16_t value, int16_t delta) {
    int32_t v = (int32_t)value + delta;
    while(v < 0) v += 1440;
    while(v >= 1440) v -= 1440;
    return (uint16_t)v;
}

static bool repeat_action_allowed(const char* action) {
    return strcmp(action, "temp_up") == 0 ||
           strcmp(action, "temp_down") == 0 ||
           strcmp(action, "fan_next") == 0 ||
           strcmp(action, "fan_prev") == 0 ||
           strcmp(action, "mode_next") == 0 ||
           strcmp(action, "mode_prev") == 0 ||
           strcmp(action, "swing_v_next") == 0 ||
           strcmp(action, "swing_v_prev") == 0 ||
           strcmp(action, "swing_h_next") == 0 ||
           strcmp(action, "swing_h_prev") == 0 ||
           strcmp(action, "swing_next") == 0 ||
           strcmp(action, "swing_prev") == 0 ||
           strcmp(action, "timer_on_plus") == 0 ||
           strcmp(action, "timer_on_minus") == 0 ||
           strcmp(action, "timer_off_plus") == 0 ||
           strcmp(action, "timer_off_minus") == 0;
}

/* -------------------------------------------------------------------------- */
/* LG                                                                         */
/* -------------------------------------------------------------------------- */

#define LG_ADDRESS           0x88U
#define LG_FREQ              38000U
#define LG_DUTY              0.33f
#define LG_TIMING_COUNT      59U
#define LG_COMMAND_GAP_MS    120U
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

static const uint8_t lg_fan_native[6] = {0x5U, 0x0U, 0x9U, 0x2U, 0xAU, 0x4U};

static const uint32_t lg_swing_v_code[8] = {
    0x881315AUL, 0x8813048UL, 0x8813059UL, 0x881306AUL,
    0x881307BUL, 0x881308CUL, 0x881309DUL, 0x8813149UL,
};

static const uint32_t lg_swing_h_code[9] = {
    0x881317CUL, 0x88130BFUL, 0x88130C0UL, 0x88130D1UL, 0x88130E2UL,
    0x88130F3UL, 0x8813105UL, 0x8813116UL, 0x881316BUL,
};

static uint8_t lg_checksum16(uint16_t command) {
    uint8_t sum = 0;
    sum += (command >> 0) & 0xFU;
    sum += (command >> 4) & 0xFU;
    sum += (command >> 8) & 0xFU;
    sum += (command >> 12) & 0xFU;
    return sum & 0xFU;
}

static uint32_t lg_raw_from_command(uint16_t command) {
    return ((uint32_t)LG_ADDRESS << 20) |
           ((uint32_t)command << 4) |
           lg_checksum16(command);
}

static uint16_t lg_build_state_command(const LgState* s, bool power_on_frame) {
    uint16_t command = 0;
    switch(s->mode) {
    case LgModeDry:
        command = 0x0990U;
        break;
    case LgModeFan:
        command = 0x0A00U;
        command |= (uint16_t)(3U << 4);
        command |= lg_fan_native[s->fan];
        break;
    case LgModeAuto:
        command = 0x0B00U;
        command |= (uint16_t)((uint8_t)(s->auto_bias + 2) << 4);
        command |= 0x5U;
        break;
    case LgModeHeat:
        command = 0x0C00U;
        command |= (uint16_t)((s->temp_c - 15U) << 4);
        command |= lg_fan_native[s->fan];
        break;
    case LgModeCool:
    default:
        command = 0x0800U;
        command |= (uint16_t)((s->temp_c - 15U) << 4);
        command |= lg_fan_native[s->fan];
        break;
    }
    if(power_on_frame) command &= (uint16_t)~0x0800U;
    return command;
}

static void lg_encode(uint32_t code, uint32_t timings[LG_TIMING_COUNT]) {
    uint8_t i = 0;
    timings[i++] = 3200U;
    timings[i++] = 9900U;
    for(int8_t bit = 27; bit >= 0; bit--) {
        timings[i++] = 480U;
        timings[i++] = (code & (1UL << bit)) ? 1600U : 550U;
    }
    timings[i++] = 480U;
    furi_check(i == LG_TIMING_COUNT);
}

static void lg_send_code(uint32_t code) {
    uint32_t timings[LG_TIMING_COUNT];
    lg_encode(code, timings);
    infrared_send_raw_ext(timings, LG_TIMING_COUNT, true, LG_FREQ, LG_DUTY);
}

static void lg_send_full(const LgState* s, bool power_on_frame) {
    lg_send_code(lg_raw_from_command(lg_build_state_command(s, power_on_frame)));
}

static void lg_send_aux_after_power_on(const LgState* s) {
    furi_delay_ms(LG_COMMAND_GAP_MS);
    lg_send_code(lg_swing_v_code[s->swing_v]);
    furi_delay_ms(LG_COMMAND_GAP_MS);
    lg_send_code(lg_swing_h_code[s->swing_h]);
    furi_delay_ms(LG_COMMAND_GAP_MS);
    lg_send_code(s->eco ? LG_CMD_ENERGY_ON : LG_CMD_ENERGY_OFF);

    if(s->auto_clean) {
        furi_delay_ms(LG_COMMAND_GAP_MS);
        lg_send_code(LG_CMD_AUTOCLEAN_ON);
    }
    if(s->purify) {
        furi_delay_ms(LG_COMMAND_GAP_MS);
        lg_send_code(LG_CMD_PURIFY_ON);
    }
    if(s->jet_dry) {
        furi_delay_ms(LG_COMMAND_GAP_MS);
        lg_send_code(LG_CMD_JETDRY_ON);
    }
    if(s->comfort) {
        furi_delay_ms(LG_COMMAND_GAP_MS);
        lg_send_code(LG_CMD_COMFORT_ON);
    }
    if(s->jet) {
        furi_delay_ms(LG_COMMAND_GAP_MS);
        lg_send_code(LG_CMD_JET_ON);
    }
}

static bool lg_mode_uses_temp(uint8_t mode) {
    return mode == LgModeCool || mode == LgModeHeat;
}

static bool lg_mode_uses_fan(uint8_t mode) {
    return mode == LgModeCool || mode == LgModeHeat || mode == LgModeFan;
}

static bool lg_execute(LgState* s, const char* a) {
    if(strcmp(a, "power") == 0) {
        if(s->power) {
            lg_send_code(LG_CMD_POWER_OFF);
            s->power = false;
        } else {
            lg_send_full(s, true);
            s->power = true;
            lg_send_aux_after_power_on(s);
        }
        return true;
    }

    if(strcmp(a, "mode_next") == 0 || strcmp(a, "mode_prev") == 0) {
        const int8_t d = strcmp(a, "mode_next") == 0 ? 1 : -1;
        s->mode = (uint8_t)wrap_delta((int8_t)s->mode, d, 5);
        s->jet = false;
        if(s->power) lg_send_full(s, false);
        return true;
    }

    if(strcmp(a, "temp_up") == 0 || strcmp(a, "temp_down") == 0) {
        const int8_t d = strcmp(a, "temp_up") == 0 ? 1 : -1;
        if(s->mode == LgModeAuto) {
            int8_t v = (int8_t)(s->auto_bias + d);
            if(v < -2) v = -2;
            if(v > 2) v = 2;
            s->auto_bias = v;
        } else if(lg_mode_uses_temp(s->mode)) {
            int16_t v = (int16_t)s->temp_c + d;
            if(v < 18) v = 18;
            if(v > 30) v = 30;
            s->temp_c = (uint8_t)v;
        }
        s->jet = false;
        if(s->power) lg_send_full(s, false);
        return true;
    }

    if(strcmp(a, "fan_next") == 0 || strcmp(a, "fan_prev") == 0) {
        if(lg_mode_uses_fan(s->mode)) {
            const int8_t d = strcmp(a, "fan_next") == 0 ? 1 : -1;
            s->fan = (uint8_t)wrap_delta((int8_t)s->fan, d, 6);
            s->jet = false;
            if(s->power) lg_send_full(s, false);
        }
        return true;
    }

    if(strcmp(a, "swing_v_next") == 0 || strcmp(a, "swing_v_prev") == 0) {
        const int8_t d = strcmp(a, "swing_v_next") == 0 ? 1 : -1;
        s->swing_v = (uint8_t)wrap_delta((int8_t)s->swing_v, d, 8);
        if(s->power) lg_send_code(lg_swing_v_code[s->swing_v]);
        return true;
    }

    if(strcmp(a, "swing_h_next") == 0 || strcmp(a, "swing_h_prev") == 0) {
        const int8_t d = strcmp(a, "swing_h_next") == 0 ? 1 : -1;
        s->swing_h = (uint8_t)wrap_delta((int8_t)s->swing_h, d, 9);
        if(s->power) lg_send_code(lg_swing_h_code[s->swing_h]);
        return true;
    }

    if(strcmp(a, "jet") == 0) {
        if(!s->jet) {
            if(!s->power) {
                lg_send_full(s, true);
                s->power = true;
                furi_delay_ms(LG_COMMAND_GAP_MS);
            }
            lg_send_code(LG_CMD_JET_ON);
            s->jet = true;
        } else {
            s->jet = false;
            if(s->power) lg_send_full(s, false);
        }
        return true;
    }

    if(strcmp(a, "eco") == 0) {
        s->eco = !s->eco;
        if(s->power) lg_send_code(s->eco ? LG_CMD_ENERGY_ON : LG_CMD_ENERGY_OFF);
        return true;
    }

    if(strcmp(a, "comfort") == 0) {
        s->comfort = !s->comfort;
        if(s->power) lg_send_code(s->comfort ? LG_CMD_COMFORT_ON : LG_CMD_COMFORT_OFF);
        return true;
    }

    if(strcmp(a, "light") == 0) {
        lg_send_code(LG_CMD_LIGHT_TOGGLE);
        return true;
    }

    if(strcmp(a, "auto_clean") == 0) {
        s->auto_clean = !s->auto_clean;
        if(s->power) lg_send_code(s->auto_clean ? LG_CMD_AUTOCLEAN_ON : LG_CMD_AUTOCLEAN_OFF);
        return true;
    }

    if(strcmp(a, "purify") == 0) {
        s->purify = !s->purify;
        if(s->power) lg_send_code(s->purify ? LG_CMD_PURIFY_ON : LG_CMD_PURIFY_OFF);
        return true;
    }

    if(strcmp(a, "jet_dry") == 0) {
        s->jet_dry = !s->jet_dry;
        if(s->power) lg_send_code(s->jet_dry ? LG_CMD_JETDRY_ON : LG_CMD_JETDRY_OFF);
        return true;
    }

    if(strcmp(a, "unit") == 0) {
        lg_send_code(s->fahrenheit ? LG_CMD_F_TO_C : LG_CMD_C_TO_F);
        s->fahrenheit = !s->fahrenheit;
        return true;
    }

    if(strcmp(a, "diagnosis") == 0) {
        lg_send_code(LG_CMD_DIAGNOSIS);
        return true;
    }

    return false;
}

/* -------------------------------------------------------------------------- */
/* Daikin                                                                     */
/* -------------------------------------------------------------------------- */

#define DAIKIN_FREQ 38000U
#define DAIKIN_DUTY 0.50f
#define DAIKIN_STATE_LEN 35U
#define DAIKIN_TIMING_COUNT 584U
#define DAIKIN_HDR_MARK 3650U
#define DAIKIN_HDR_SPACE 1623U
#define DAIKIN_BIT_MARK 428U
#define DAIKIN_ZERO_SPACE 428U
#define DAIKIN_ONE_SPACE 1280U
#define DAIKIN_GAP 29000U
#define DAIKIN_UNUSED_TIME 0x600U

static const uint8_t daikin_fan_native[6] = {0xAU, 0x3U, 0x4U, 0x5U, 0x6U, 0x7U};

static uint8_t daikin_checksum(const uint8_t* data, uint8_t count) {
    uint8_t sum = 0;
    for(uint8_t i = 0; i < count; i++) sum = (uint8_t)(sum + data[i]);
    return sum;
}

static uint8_t daikin_mode_native(uint8_t mode) {
    if(mode == DaikinModeDry) return 0x2U;
    if(mode == DaikinModeFan) return 0x6U;
    return 0x3U;
}

static void daikin_set_time12(uint8_t state[DAIKIN_STATE_LEN], uint16_t on_time, uint16_t off_time) {
    state[26] = (uint8_t)(on_time & 0xFFU);
    state[27] = (uint8_t)(((on_time >> 8) & 0x0FU) | ((off_time & 0x0FU) << 4));
    state[28] = (uint8_t)((off_time >> 4) & 0xFFU);
}

static uint16_t current_minutes(void) {
    DateTime dt;
    furi_hal_rtc_get_datetime(&dt);
    return (uint16_t)(dt.hour * 60U + dt.minute);
}

static void daikin_build_state(const DaikinState* s, uint8_t out[DAIKIN_STATE_LEN]) {
    memset(out, 0, DAIKIN_STATE_LEN);

    out[0] = 0x11U; out[1] = 0xDAU; out[2] = 0x27U; out[4] = 0xC5U;
    out[8] = 0x11U; out[9] = 0xDAU; out[10] = 0x27U; out[12] = 0x42U;
    out[16] = 0x11U; out[17] = 0xDAU; out[18] = 0x27U; out[21] = 0x08U;
    out[31] = 0xC0U;

    const uint16_t now = current_minutes();
    out[13] = (uint8_t)(now & 0xFFU);
    out[14] = (uint8_t)((now >> 8) & 0x07U);

    if(s->power) out[21] |= 0x01U;
    if(s->timer_on_enabled) out[21] |= 0x02U;
    if(s->timer_off_enabled) out[21] |= 0x04U;
    out[21] |= (uint8_t)(daikin_mode_native(s->mode) << 4);

    if(s->mode == DaikinModeCool) out[22] = (uint8_t)(s->temp_c * 2U);
    else if(s->mode == DaikinModeDry) out[22] = 0xC0U;
    else out[22] = 50U;

    const uint8_t fan = s->mode == DaikinModeDry ? 0xAU : daikin_fan_native[s->fan];
    out[24] = (uint8_t)((fan << 4) | (s->swing ? 0x0FU : 0x00U));

    daikin_set_time12(
        out,
        s->timer_on_enabled ? s->timer_on_target_min : DAIKIN_UNUSED_TIME,
        s->timer_off_enabled ? s->timer_off_target_min : DAIKIN_UNUSED_TIME);

    if(s->powerful && s->power) out[29] |= 0x01U;

    out[7] = daikin_checksum(out, 7U);
    out[15] = daikin_checksum(out + 8, 7U);
    out[34] = daikin_checksum(out + 16, 18U);
}

static void daikin_append_bit(uint32_t* timings, uint16_t* pos, bool one) {
    timings[(*pos)++] = DAIKIN_BIT_MARK;
    timings[(*pos)++] = one ? DAIKIN_ONE_SPACE : DAIKIN_ZERO_SPACE;
}

static void daikin_append_section(
    uint32_t* timings,
    uint16_t* pos,
    const uint8_t* data,
    uint8_t count) {
    timings[(*pos)++] = DAIKIN_HDR_MARK;
    timings[(*pos)++] = DAIKIN_HDR_SPACE;
    for(uint8_t i = 0; i < count; i++) {
        for(uint8_t bit = 0; bit < 8; bit++) {
            daikin_append_bit(timings, pos, (data[i] & (1U << bit)) != 0);
        }
    }
    timings[(*pos)++] = DAIKIN_BIT_MARK;
    timings[(*pos)++] = DAIKIN_ZERO_SPACE + DAIKIN_GAP;
}

static void daikin_encode(const uint8_t state[DAIKIN_STATE_LEN], uint32_t* timings) {
    uint16_t pos = 0;
    for(uint8_t i = 0; i < 5; i++) daikin_append_bit(timings, &pos, false);
    timings[pos++] = DAIKIN_BIT_MARK;
    timings[pos++] = DAIKIN_ZERO_SPACE + DAIKIN_GAP;
    daikin_append_section(timings, &pos, state, 8U);
    daikin_append_section(timings, &pos, state + 8, 8U);
    daikin_append_section(timings, &pos, state + 16, 19U);
    furi_check(pos == DAIKIN_TIMING_COUNT);
}

static void daikin_send(const DaikinState* s) {
    uint8_t state[DAIKIN_STATE_LEN];
    uint32_t timings[DAIKIN_TIMING_COUNT];
    daikin_build_state(s, state);
    daikin_encode(state, timings);
    infrared_send_raw_ext(timings, DAIKIN_TIMING_COUNT, true, DAIKIN_FREQ, DAIKIN_DUTY);
}

static void daikin_normalize(DaikinState* s) {
    if(s->mode == DaikinModeDry) s->fan = 0U;
}

static bool daikin_execute(DaikinState* s, const char* a) {
    if(strcmp(a, "power") == 0) {
        s->power = !s->power;
        if(!s->power) s->powerful = false;
        daikin_send(s);
        return true;
    }

    if(strcmp(a, "mode_next") == 0 || strcmp(a, "mode_prev") == 0) {
        const int8_t d = strcmp(a, "mode_next") == 0 ? 1 : -1;
        s->mode = (uint8_t)wrap_delta((int8_t)s->mode, d, 3);
        daikin_normalize(s);
        if(s->power) daikin_send(s);
        return true;
    }

    if(strcmp(a, "temp_up") == 0 || strcmp(a, "temp_down") == 0) {
        if(s->mode == DaikinModeCool) {
            const int8_t d = strcmp(a, "temp_up") == 0 ? 1 : -1;
            int16_t v = (int16_t)s->temp_c + d;
            if(v < 18) v = 18;
            if(v > 32) v = 32;
            s->temp_c = (uint8_t)v;
            if(s->power) daikin_send(s);
        }
        return true;
    }

    if(strcmp(a, "fan_next") == 0 || strcmp(a, "fan_prev") == 0) {
        if(s->mode != DaikinModeDry) {
            const int8_t d = strcmp(a, "fan_next") == 0 ? 1 : -1;
            s->fan = (uint8_t)wrap_delta((int8_t)s->fan, d, 6);
            if(s->power) daikin_send(s);
        }
        return true;
    }

    if(strcmp(a, "swing") == 0) {
        s->swing = !s->swing;
        if(s->power) daikin_send(s);
        return true;
    }

    if(strcmp(a, "powerful") == 0) {
        if(s->power) {
            s->powerful = !s->powerful;
            daikin_send(s);
        }
        return true;
    }

    if(strcmp(a, "timer_on") == 0) {
        s->timer_on_enabled = !s->timer_on_enabled;
        daikin_send(s);
        return true;
    }

    if(strcmp(a, "timer_off") == 0) {
        s->timer_off_enabled = !s->timer_off_enabled;
        daikin_send(s);
        return true;
    }

    if(strcmp(a, "timer_on_plus") == 0 || strcmp(a, "timer_on_minus") == 0) {
        const int16_t d = strcmp(a, "timer_on_plus") == 0 ? 10 : -10;
        s->timer_on_target_min = time_add(s->timer_on_target_min, d);
        if(s->timer_on_enabled) daikin_send(s);
        return true;
    }

    if(strcmp(a, "timer_off_plus") == 0 || strcmp(a, "timer_off_minus") == 0) {
        const int16_t d = strcmp(a, "timer_off_plus") == 0 ? 10 : -10;
        s->timer_off_target_min = time_add(s->timer_off_target_min, d);
        if(s->timer_off_enabled) daikin_send(s);
        return true;
    }

    if(strcmp(a, "timer_clear") == 0) {
        s->timer_on_enabled = false;
        s->timer_off_enabled = false;
        daikin_send(s);
        return true;
    }

    return false;
}

/* -------------------------------------------------------------------------- */
/* Panasonic                                                                  */
/* -------------------------------------------------------------------------- */

#define PANA_FREQ 38000U
#define PANA_DUTY 0.33f
#define PANA_HDR_MARK 3456U
#define PANA_HDR_SPACE 1728U
#define PANA_BIT_MARK 432U
#define PANA_ONE_SPACE 1296U
#define PANA_ZERO_SPACE 432U
#define PANA_SECTION_GAP 10000U
#define PANA_MAX_TIMINGS 460U

static void pana_append(uint32_t* timings, size_t* count, uint32_t value) {
    if(*count < PANA_MAX_TIMINGS) timings[(*count)++] = value;
}

static void pana_append_byte_lsb(uint32_t* timings, size_t* count, uint8_t value) {
    for(uint8_t bit = 0; bit < 8; bit++) {
        pana_append(timings, count, PANA_BIT_MARK);
        pana_append(
            timings,
            count,
            (value & (1U << bit)) ? PANA_ONE_SPACE : PANA_ZERO_SPACE);
    }
}

static size_t pana_encode_sections(
    const uint8_t* section1,
    size_t section1_len,
    const uint8_t* section2,
    size_t section2_len,
    uint32_t* timings) {
    size_t count = 0;
    pana_append(timings, &count, PANA_HDR_MARK);
    pana_append(timings, &count, PANA_HDR_SPACE);
    for(size_t i = 0; i < section1_len; i++) pana_append_byte_lsb(timings, &count, section1[i]);
    pana_append(timings, &count, PANA_BIT_MARK);
    pana_append(timings, &count, PANA_SECTION_GAP);
    pana_append(timings, &count, PANA_HDR_MARK);
    pana_append(timings, &count, PANA_HDR_SPACE);
    for(size_t i = 0; i < section2_len; i++) pana_append_byte_lsb(timings, &count, section2[i]);
    pana_append(timings, &count, PANA_BIT_MARK);
    return count;
}

static uint8_t pana_checksum(const uint8_t* frame, size_t start, size_t end_exclusive) {
    uint8_t sum = 0;
    for(size_t i = start; i < end_exclusive; i++) sum = (uint8_t)(sum + frame[i]);
    return sum;
}

static uint8_t pana_mode_byte(const PanasonicState* s) {
    uint8_t value = 0x08U;
    if(s->power) value |= 0x01U;
    if(s->mode == PanaModeCool) value |= 0x30U;
    else if(s->mode == PanaModeDry) value |= 0x20U;
    return value;
}

static uint8_t pana_fan_nibble(uint8_t fan) {
    switch(fan) {
    case PanaFan1: return 0x3U;
    case PanaFan2: return 0x4U;
    case PanaFan3: return 0x5U;
    case PanaFan4: return 0x6U;
    case PanaFan5: return 0x7U;
    case PanaFanQuiet:
    case PanaFanAuto:
    default: return 0xAU;
    }
}

static uint8_t pana_swing_nibble(uint8_t swing) {
    if(swing <= 4U) return (uint8_t)(swing + 1U);
    return 0xFU;
}

static void pana_build_state(const PanasonicState* s, uint8_t out[27]) {
    static const uint8_t fixed[27] = {
        0x02U,0x20U,0xE0U,0x04U,0x00U,0x00U,0x00U,0x06U,
        0x02U,0x20U,0xE0U,0x04U,0x00U,0x09U,0x32U,0x80U,
        0xA2U,0x00U,0x00U,0x0EU,0xE0U,0x00U,0x00U,0x89U,
        0x00U,0x00U,0x00U,
    };
    memcpy(out, fixed, sizeof(fixed));
    out[13] = pana_mode_byte(s);
    out[14] = s->temp_half;
    out[16] = (uint8_t)((pana_fan_nibble(s->fan) << 4) | pana_swing_nibble(s->swing));
    out[21] = 0x00U;
    if(s->fan == PanaFanQuiet) out[21] |= 0x20U;
    else if(s->extra == PanaExtraPowerful) out[21] |= 0x01U;
    out[25] = 0x00U;
    if(s->extra == PanaExtraEco) out[25] |= 0x10U;
    if(s->nanoe) out[25] |= 0x06U;
    out[26] = pana_checksum(out, 8U, 26U);
}

static uint8_t pana_sleep_code(uint8_t sleep) {
    static const uint8_t codes[11] = {
        0x00U,0x1FU,0x1EU,0x1CU,0x1AU,0x18U,0x16U,0x14U,0x12U,0x10U,0x0EU,
    };
    return sleep <= 10U ? codes[sleep] : 0x00U;
}

static void pana_build_sleep(uint8_t sleep, uint8_t out[16]) {
    static const uint8_t base[16] = {
        0x02U,0x20U,0xE0U,0x04U,0x00U,0x00U,0x00U,0x06U,
        0x02U,0x20U,0xE0U,0x04U,0x80U,0x00U,0x10U,0x00U,
    };
    memcpy(out, base, sizeof(base));
    out[13] = pana_sleep_code(sleep);
    out[15] = pana_checksum(out, 8U, 15U);
}

static void pana_send_state(const PanasonicState* s) {
    uint8_t frame[27];
    uint32_t timings[PANA_MAX_TIMINGS];
    pana_build_state(s, frame);
    const size_t count = pana_encode_sections(frame, 8U, frame + 8, 19U, timings);
    infrared_send_raw_ext(timings, count, true, PANA_FREQ, PANA_DUTY);
}

static void pana_send_sleep(const PanasonicState* s) {
    uint8_t frame[16];
    uint32_t timings[PANA_MAX_TIMINGS];
    pana_build_sleep(s->sleep, frame);
    const size_t count = pana_encode_sections(frame, 8U, frame + 8, 8U, timings);
    infrared_send_raw_ext(timings, count, true, PANA_FREQ, PANA_DUTY);
}

static void pana_normalize(PanasonicState* s) {
    if(s->fan == PanaFanQuiet && s->extra == PanaExtraPowerful) s->extra = PanaExtraOff;
}

static bool pana_execute(PanasonicState* s, const char* a) {
    if(strcmp(a, "power") == 0) {
        s->power = !s->power;
        pana_send_state(s);
        return true;
    }

    if(strcmp(a, "mode_next") == 0 || strcmp(a, "mode_prev") == 0) {
        const int8_t d = strcmp(a, "mode_next") == 0 ? 1 : -1;
        s->mode = (uint8_t)wrap_delta((int8_t)s->mode, d, 3);
        if(s->power) pana_send_state(s);
        return true;
    }

    if(strcmp(a, "temp_up") == 0 || strcmp(a, "temp_down") == 0) {
        const int8_t d = strcmp(a, "temp_up") == 0 ? 1 : -1;
        int16_t v = (int16_t)s->temp_half + d;
        if(v < 32) v = 32;
        if(v > 60) v = 60;
        s->temp_half = (uint8_t)v;
        if(s->power) pana_send_state(s);
        return true;
    }

    if(strcmp(a, "fan_next") == 0 || strcmp(a, "fan_prev") == 0) {
        const int8_t d = strcmp(a, "fan_next") == 0 ? 1 : -1;
        s->fan = (uint8_t)wrap_delta((int8_t)s->fan, d, 7);
        pana_normalize(s);
        if(s->power) pana_send_state(s);
        return true;
    }

    if(strcmp(a, "swing_next") == 0 || strcmp(a, "swing_prev") == 0) {
        const int8_t d = strcmp(a, "swing_next") == 0 ? 1 : -1;
        s->swing = (uint8_t)wrap_delta((int8_t)s->swing, d, 6);
        if(s->power) pana_send_state(s);
        return true;
    }

    if(strcmp(a, "nanoe") == 0) {
        s->nanoe = !s->nanoe;
        pana_send_state(s);
        return true;
    }

    if(strcmp(a, "extra_next") == 0 || strcmp(a, "extra_prev") == 0) {
        const int8_t d = strcmp(a, "extra_next") == 0 ? 1 : -1;
        s->extra = (uint8_t)wrap_delta((int8_t)s->extra, d, 3);
        if(s->extra == PanaExtraPowerful && s->fan == PanaFanQuiet) s->fan = PanaFanAuto;
        if(s->power) pana_send_state(s);
        return true;
    }

    if(strcmp(a, "sleep_next") == 0 || strcmp(a, "sleep_prev") == 0) {
        const int8_t d = strcmp(a, "sleep_next") == 0 ? 1 : -1;
        s->sleep = (uint8_t)wrap_delta((int8_t)s->sleep, d, 11);
        if(s->power) pana_send_sleep(s);
        return true;
    }

    return false;
}

/* -------------------------------------------------------------------------- */
/* Carrier / Toshiba                                                          */
/* -------------------------------------------------------------------------- */

#define CARRIER_FREQ 38000U
#define CARRIER_DUTY 0.33f
#define CARRIER_HDR_MARK 4400U
#define CARRIER_HDR_SPACE 4395U
#define CARRIER_BIT_MARK 540U
#define CARRIER_ZERO_SPACE 543U
#define CARRIER_ONE_SPACE 1627U
#define CARRIER_FRAME_GAP 6712U
#define CARRIER_MAX_TIMINGS 327U

static uint8_t carrier_xor(const uint8_t* data, uint8_t count) {
    uint8_t value = 0;
    for(uint8_t i = 0; i < count; i++) value ^= data[i];
    return value;
}

static uint8_t carrier_mode_native(uint8_t mode) {
    if(mode == CarrierModeCool) return 0x01U;
    if(mode == CarrierModeDry) return 0x02U;
    if(mode == CarrierModeFan) return 0x04U;
    return 0x00U;
}

static uint8_t carrier_fan_native(uint8_t fan) {
    return fan == 0U ? 0x00U : (uint8_t)(fan + 1U);
}

static uint8_t carrier_build_state(const CarrierState* s, uint8_t out[10]) {
    const bool extended = s->power && (s->hi_power || s->eco);
    const uint8_t length = extended ? 10U : 9U;
    memset(out, 0, 10U);
    out[0] = 0xF2U;
    out[1] = 0x0DU;
    out[2] = (uint8_t)(length - 6U);
    out[3] = (uint8_t)~out[2];
    out[4] = extended ? 0x09U : 0x01U;
    out[5] = (uint8_t)((s->temp_c - 17U) << 4);
    const uint8_t mode = s->power ? carrier_mode_native(s->mode) : 0x07U;
    out[6] = (uint8_t)((carrier_fan_native(s->fan) << 5) | mode);
    out[7] = 0x00U;
    if(extended) out[8] = s->hi_power ? 0x01U : 0x03U;
    out[length - 1U] = carrier_xor(out, (uint8_t)(length - 1U));
    return length;
}

static void carrier_build_swing(uint8_t value, uint8_t out[7]) {
    out[0] = 0xF2U; out[1] = 0x0DU; out[2] = 0x01U; out[3] = 0xFEU;
    out[4] = 0x21U; out[5] = value; out[6] = carrier_xor(out, 6U);
}

static void carrier_append_frame(
    const uint8_t* data,
    uint8_t bytes,
    uint32_t* timings,
    uint16_t* pos) {
    timings[(*pos)++] = CARRIER_HDR_MARK;
    timings[(*pos)++] = CARRIER_HDR_SPACE;
    for(uint8_t i = 0; i < bytes; i++) {
        for(uint8_t bit = 0; bit < 8; bit++) {
            const uint8_t mask = (uint8_t)(0x80U >> bit);
            timings[(*pos)++] = CARRIER_BIT_MARK;
            timings[(*pos)++] = (data[i] & mask) ? CARRIER_ONE_SPACE : CARRIER_ZERO_SPACE;
        }
    }
    timings[(*pos)++] = CARRIER_BIT_MARK;
}

static uint16_t carrier_encode(
    const uint8_t* data,
    uint8_t bytes,
    uint8_t frames,
    uint32_t* timings) {
    uint16_t pos = 0;
    for(uint8_t frame = 0; frame < frames; frame++) {
        if(frame > 0U) timings[pos++] = CARRIER_FRAME_GAP;
        carrier_append_frame(data, bytes, timings, &pos);
    }
    furi_check(pos <= CARRIER_MAX_TIMINGS);
    return pos;
}

static void carrier_send_bytes(const uint8_t* data, uint8_t bytes, uint8_t frames) {
    uint32_t timings[CARRIER_MAX_TIMINGS];
    const uint16_t count = carrier_encode(data, bytes, frames, timings);
    infrared_send_raw_ext(timings, count, true, CARRIER_FREQ, CARRIER_DUTY);
}

static void carrier_send_state(const CarrierState* s) {
    uint8_t data[10];
    const uint8_t bytes = carrier_build_state(s, data);
    carrier_send_bytes(data, bytes, 2U);
}

static void carrier_send_swing(uint8_t value, uint8_t frames) {
    uint8_t data[7];
    carrier_build_swing(value, data);
    carrier_send_bytes(data, 7U, frames);
}

static void carrier_normalize(CarrierState* s) {
    if(s->mode == CarrierModeDry) s->fan = 0U;
}

static bool carrier_execute(CarrierState* s, const char* a) {
    if(strcmp(a, "power") == 0) {
        s->power = !s->power;
        if(!s->power) {
            s->hi_power = false;
            s->eco = false;
        }
        carrier_send_state(s);
        return true;
    }

    if(strcmp(a, "mode_next") == 0 || strcmp(a, "mode_prev") == 0) {
        const int8_t d = strcmp(a, "mode_next") == 0 ? 1 : -1;
        s->mode = (uint8_t)wrap_delta((int8_t)s->mode, d, 4);
        s->hi_power = false;
        s->eco = false;
        carrier_normalize(s);
        if(s->power) carrier_send_state(s);
        return true;
    }

    if(strcmp(a, "temp_up") == 0 || strcmp(a, "temp_down") == 0) {
        const int8_t d = strcmp(a, "temp_up") == 0 ? 1 : -1;
        int16_t v = (int16_t)s->temp_c + d;
        if(v < 17) v = 17;
        if(v > 30) v = 30;
        s->temp_c = (uint8_t)v;
        if(s->power) carrier_send_state(s);
        return true;
    }

    if(strcmp(a, "fan_next") == 0 || strcmp(a, "fan_prev") == 0) {
        if(s->mode != CarrierModeDry) {
            const int8_t d = strcmp(a, "fan_next") == 0 ? 1 : -1;
            s->fan = (uint8_t)wrap_delta((int8_t)s->fan, d, 6);
            if(s->power) carrier_send_state(s);
        }
        return true;
    }

    if(strcmp(a, "fix") == 0) {
        carrier_send_swing(0x00U, 1U);
        return true;
    }

    if(strcmp(a, "swing") == 0) {
        carrier_send_swing(0x04U, 2U);
        return true;
    }

    if(strcmp(a, "hi_power") == 0) {
        if(s->power) {
            s->hi_power = !s->hi_power;
            if(s->hi_power) s->eco = false;
            carrier_send_state(s);
        }
        return true;
    }

    if(strcmp(a, "eco") == 0) {
        if(s->power) {
            s->eco = !s->eco;
            if(s->eco) s->hi_power = false;
            carrier_send_state(s);
        }
        return true;
    }

    return false;
}

/* -------------------------------------------------------------------------- */
/* Driver catalogs and public API                                             */
/* -------------------------------------------------------------------------- */

static const char* const lg_actions[] = {
    "power","mode_next","mode_prev","temp_up","temp_down","fan_next","fan_prev",
    "swing_v_next","swing_v_prev","swing_h_next","swing_h_prev",
    "jet","eco","comfort","light","auto_clean","purify","jet_dry","unit","diagnosis",
};

static const char* const daikin_actions[] = {
    "power","mode_next","mode_prev","temp_up","temp_down","fan_next","fan_prev",
    "swing","powerful","timer_on","timer_off","timer_on_plus","timer_on_minus",
    "timer_off_plus","timer_off_minus","timer_clear",
};

static const char* const panasonic_actions[] = {
    "power","mode_next","mode_prev","temp_up","temp_down","fan_next","fan_prev",
    "swing_next","swing_prev","nanoe","extra_next","extra_prev","sleep_next","sleep_prev",
};

static const char* const carrier_actions[] = {
    "power","mode_next","mode_prev","temp_up","temp_down","fan_next","fan_prev",
    "fix","swing","hi_power","eco",
};

static const char* const* action_table(DriverKind kind, size_t* count) {
    if(count) *count = 0;
    switch(kind) {
    case DriverLg:
        if(count) *count = sizeof(lg_actions) / sizeof(lg_actions[0]);
        return lg_actions;
    case DriverDaikin:
        if(count) *count = sizeof(daikin_actions) / sizeof(daikin_actions[0]);
        return daikin_actions;
    case DriverPanasonic:
        if(count) *count = sizeof(panasonic_actions) / sizeof(panasonic_actions[0]);
        return panasonic_actions;
    case DriverCarrier:
        if(count) *count = sizeof(carrier_actions) / sizeof(carrier_actions[0]);
        return carrier_actions;
    case DriverNone:
    default:
        return NULL;
    }
}

UniStatefulIr* uni_stateful_ir_alloc(Storage* storage) {
    if(!storage) return NULL;
    UniStatefulIr* s = calloc(1, sizeof(UniStatefulIr));
    if(!s) return NULL;
    s->storage = storage;
    return s;
}

void uni_stateful_ir_free(UniStatefulIr* stateful) {
    if(!stateful) return;
    if(stateful->loaded) save_state(stateful);
    free(stateful);
}

bool uni_stateful_ir_load(UniStatefulIr* s, const UniRemote* remote) {
    if(!s || !remote) return false;
    if(s->loaded) save_state(s);

    s->kind = driver_from_name(remote->state_driver);
    s->loaded = false;
    s->state_path[0] = '\0';
    if(s->kind == DriverNone || !remote->state_path[0]) return false;

    snprintf(s->state_path, sizeof(s->state_path), "%s", remote->state_path);
    state_defaults(s);
    s->loaded = true;
    load_saved_state(s);
    return true;
}

void uni_stateful_ir_unload(UniStatefulIr* s) {
    if(!s) return;
    if(s->loaded) save_state(s);
    s->loaded = false;
    s->kind = DriverNone;
    s->state_path[0] = '\0';
    memset(&s->state, 0, sizeof(s->state));
}

bool uni_stateful_ir_execute(UniStatefulIr* s, const char* action, bool repeat) {
    if(!s || !s->loaded || !action || !action[0]) return false;
    if(repeat && !repeat_action_allowed(action)) return false;

    bool ok = false;
    switch(s->kind) {
    case DriverLg:
        ok = lg_execute(&s->state.lg, action);
        break;
    case DriverDaikin:
        ok = daikin_execute(&s->state.daikin, action);
        break;
    case DriverPanasonic:
        ok = pana_execute(&s->state.panasonic, action);
        break;
    case DriverCarrier:
        ok = carrier_execute(&s->state.carrier, action);
        break;
    case DriverNone:
    default:
        break;
    }

    if(ok) save_state(s);
    return ok;
}

size_t uni_stateful_ir_action_count(const UniStatefulIr* s) {
    if(!s || !s->loaded) return 0;
    size_t count = 0;
    action_table(s->kind, &count);
    return count;
}

const char* uni_stateful_ir_action_name(const UniStatefulIr* s, size_t index) {
    if(!s || !s->loaded) return NULL;
    size_t count = 0;
    const char* const* table = action_table(s->kind, &count);
    if(!table || index >= count) return NULL;
    return table[index];
}

static const char* lg_mode_name(uint8_t mode) {
    static const char* const n[] = {"CL","DR","FN","AU","HT"};
    return mode < 5U ? n[mode] : "??";
}

static const char* daikin_mode_name(uint8_t mode) {
    static const char* const n[] = {"CL","DR","FN"};
    return mode < 3U ? n[mode] : "??";
}

static const char* pana_mode_name(uint8_t mode) {
    static const char* const n[] = {"AU","CL","DR"};
    return mode < 3U ? n[mode] : "??";
}

static const char* carrier_mode_name(uint8_t mode) {
    static const char* const n[] = {"AU","CL","DR","FN"};
    return mode < 4U ? n[mode] : "??";
}

void uni_stateful_ir_status(const UniStatefulIr* s, char* out, size_t out_size) {
    if(!out || out_size == 0) return;
    out[0] = '\0';
    if(!s || !s->loaded) {
        snprintf(out, out_size, "STATE ?");
        return;
    }

    switch(s->kind) {
    case DriverLg: {
        const LgState* v = &s->state.lg;
        if(v->mode == LgModeAuto) {
            snprintf(
                out,
                out_size,
                "%s %s A%+d F%u",
                v->power ? "ON" : "OF",
                lg_mode_name(v->mode),
                (int)v->auto_bias,
                (unsigned)v->fan);
        } else if(lg_mode_uses_temp(v->mode)) {
            snprintf(
                out,
                out_size,
                "%s %s%u F%u",
                v->power ? "ON" : "OF",
                lg_mode_name(v->mode),
                (unsigned)v->temp_c,
                (unsigned)v->fan);
        } else {
            snprintf(
                out,
                out_size,
                "%s %s F%u",
                v->power ? "ON" : "OF",
                lg_mode_name(v->mode),
                (unsigned)v->fan);
        }
        break;
    }

    case DriverDaikin: {
        const DaikinState* v = &s->state.daikin;
        snprintf(
            out,
            out_size,
            "%s %s%u F%u%s",
            v->power ? "ON" : "OF",
            daikin_mode_name(v->mode),
            (unsigned)v->temp_c,
            (unsigned)v->fan,
            v->swing ? " S" : "");
        break;
    }

    case DriverPanasonic: {
        const PanasonicState* v = &s->state.panasonic;
        snprintf(
            out,
            out_size,
            "%s %s%u.%c F%u",
            v->power ? "ON" : "OF",
            pana_mode_name(v->mode),
            (unsigned)(v->temp_half / 2U),
            (v->temp_half & 1U) ? '5' : '0',
            (unsigned)v->fan);
        break;
    }

    case DriverCarrier: {
        const CarrierState* v = &s->state.carrier;
        snprintf(
            out,
            out_size,
            "%s %s%u F%u",
            v->power ? "ON" : "OF",
            carrier_mode_name(v->mode),
            (unsigned)v->temp_c,
            (unsigned)v->fan);
        break;
    }

    case DriverNone:
    default:
        snprintf(out, out_size, "STATE ?");
        break;
    }
}
