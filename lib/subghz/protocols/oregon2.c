#include "oregon2.h"

#include <lib/subghz/blocks/const.h>
#include <lib/subghz/blocks/decoder.h>
#include <lib/subghz/blocks/encoder.h>
#include <lib/subghz/blocks/math.h>
#include <lib/subghz/protocols/base.h>

#include <lib/toolbox/manchester_decoder.h>
#include <lib/flipper_format/flipper_format_i.h>

#define TAG "subghz_protocol_Oregon2"

// PORTING E PICCOLE MODIFICHE A CURA DI HAISENTECK

static const SubGhzBlockConst subghz_oregon2_const = {
    .te_long = 1000,
    .te_short = 500,
    .te_delta = 200,
    .min_count_bit_for_found = 32,
};

#define OREGON2_PREAMBLE_BITS 19
#define OREGON2_PREAMBLE_MASK 0b1111111111111111111
#define OREGON2_SENSOR_ID(d) (((d) >> 16) & 0xFFFF)
#define OREGON2_CHECKSUM_BITS 8

// 15 ones + 0101 (inverted A)
#define OREGON2_PREAMBLE 0b1111111111111110101

// bit indicating the low battery
#define OREGON2_FLAG_BAT_LOW 0x4

/// Documentation for Oregon Scientific protocols can be found here:
/// http://wmrx00.sourceforge.net/Arduino/OregonScientific-RF-Protocols.pdf
// Sensors ID
#define ID_THGR122N 0x1d20
#define ID_THGR968 0x1d30
#define ID_BTHR918 0x5d50
#define ID_BHTR968 0x5d60
#define ID_RGR968 0x2d10
#define ID_THR228N 0xec40
#define ID_THN132N 0xec40 // same as THR228N but different packet size
#define ID_RTGN318 0x0cc3 // warning: id is from 0x0cc3 and 0xfcc3
#define ID_RTGN129 0x0cc3 // same as RTGN318 but different packet size
#define ID_THGR810 0xf824 // This might be ID_THGR81, but what's true is lost in (git) history
#define ID_THGR810a 0xf8b4 // unconfirmed version
#define ID_THN802 0xc844
#define ID_PCR800 0x2914
#define ID_PCR800a 0x2d14 // Different PCR800 ID - AU version I think
#define ID_WGR800 0x1984
#define ID_WGR800a 0x1994 // unconfirmed version
#define ID_WGR968 0x3d00
#define ID_UV800 0xd874
#define ID_THN129 0xcc43 // THN129 Temp only
#define ID_RTHN129 0x0cd3 // RTHN129 Temp, clock sensors
#define ID_RTHN129_1 0x9cd3
#define ID_RTHN129_2 0xacd3
#define ID_RTHN129_3 0xbcd3
#define ID_RTHN129_4 0xccd3
#define ID_RTHN129_5 0xdcd3
#define ID_BTHGN129 0x5d53 // Baro, Temp, Hygro sensor
#define ID_UVR128 0xec70
#define ID_THGR328N 0xcc23 // Temp & Hygro sensor similar to THR228N with 5 channel instead of 3
#define ID_RTGR328N_1 0xdcc3 // RTGR328N_[1-5] RFclock(date &time)&Temp&Hygro sensor
#define ID_RTGR328N_2 0xccc3
#define ID_RTGR328N_3 0xbcc3
#define ID_RTGR328N_4 0xacc3
#define ID_RTGR328N_5 0x9cc3
#define ID_RTGR328N_6 0x8ce3 // RTGR328N_6&7 RFclock(date &time)&Temp&Hygro sensor like THGR328N
#define ID_RTGR328N_7 0x8ae3

struct subghz_protocol_DecoderOregon2 {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
    ManchesterState manchester_state;
    bool prev_bit;
    bool have_bit;

    uint8_t var_bits;
    uint32_t var_data;
};

typedef struct subghz_protocol_DecoderOregon2 subghz_protocol_DecoderOregon2;

typedef enum {
    Oregon2DecoderStepReset = 0,
    Oregon2DecoderStepFoundPreamble,
    Oregon2DecoderStepVarData,
} Oregon2DecoderStep;

void* subghz_protocol_decoder_oregon2_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_DecoderOregon2* instance = malloc(sizeof(subghz_protocol_DecoderOregon2));
    instance->base.protocol = &subghz_protocol_oregon2;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->generic.humidity = WS_NO_HUMIDITY;
    instance->generic.temp = WS_NO_TEMPERATURE;
    instance->generic.btn = WS_NO_BTN;
    instance->generic.channel = WS_NO_CHANNEL;
    instance->generic.battery_low = WS_NO_BATT;
    instance->generic.id = WS_NO_ID;
    return instance;
}

void subghz_protocol_decoder_oregon2_free(void* context) {
    furi_assert(context);
    subghz_protocol_DecoderOregon2* instance = context;
    free(instance);
}

void subghz_protocol_decoder_oregon2_reset(void* context) {
    furi_assert(context);
    subghz_protocol_DecoderOregon2* instance = context;
    instance->decoder.parser_step = Oregon2DecoderStepReset;
    instance->decoder.decode_data = 0UL;
    instance->decoder.decode_count_bit = 0;
    manchester_advance(
        instance->manchester_state, ManchesterEventReset, &instance->manchester_state, NULL);
    instance->have_bit = false;
    instance->var_data = 0;
    instance->var_bits = 0;
}

static ManchesterEvent level_and_duration_to_event(bool level, uint32_t duration) {
    bool is_long = false;

    if(DURATION_DIFF(duration, subghz_oregon2_const.te_long) < subghz_oregon2_const.te_delta) {
        is_long = true;
    } else if(DURATION_DIFF(duration, subghz_oregon2_const.te_short) < subghz_oregon2_const.te_delta) {
        is_long = false;
    } else {
        return ManchesterEventReset;
    }

    if(level)
        return is_long ? ManchesterEventLongHigh : ManchesterEventShortHigh;
    else
        return is_long ? ManchesterEventLongLow : ManchesterEventShortLow;
}

// From sensor id code return amount of bits in variable section
// https://temofeev.ru/info/articles/o-dekodirovanii-protokola-pogodnykh-datchikov-oregon-scientific
static uint8_t oregon2_sensor_id_var_bits(uint16_t sensor_id) {
    switch(sensor_id) {
    case ID_THR228N:
    case ID_RTHN129_1:
    case ID_RTHN129_2:
    case ID_RTHN129_3:
    case ID_RTHN129_4:
    case ID_RTHN129_5:
        return 16;
    case ID_THGR122N:
        return 24;
    default:
        return 0;
    }
}

static void subghz_oregon2_decode_const_data(SubGhzBlockGeneric* ws_block) {
    ws_block->id = OREGON2_SENSOR_ID(ws_block->data);

    uint8_t ch_bits = (ws_block->data >> 12) & 0xF;
    ws_block->channel = 1;
    while(ch_bits > 1) {
        ws_block->channel++;
        ch_bits >>= 1;
    }

    ws_block->battery_low = (ws_block->data & OREGON2_FLAG_BAT_LOW) ? 1 : 0;
}

uint16_t bcd_decode_short(uint32_t data) {
    return (data & 0xF) * 10 + ((data >> 4) & 0xF);
}

static float subghz_oregon2_decode_temp(uint32_t data) {
    int32_t temp_val;
    temp_val = bcd_decode_short(data >> 4);
    temp_val *= 10;
    temp_val += (data >> 12) & 0xF;
    if(data & 0xF) temp_val = -temp_val;
    return (float)temp_val / 10.0;
}

static void subghz_oregon2_decode_var_data(SubGhzBlockGeneric* ws_b, uint16_t sensor_id, uint32_t data) {
    switch(sensor_id) {
    case ID_THR228N:
    case ID_RTHN129_1:
    case ID_RTHN129_2:
    case ID_RTHN129_3:
    case ID_RTHN129_4:
    case ID_RTHN129_5:
        ws_b->temp = subghz_oregon2_decode_temp(data);
        ws_b->humidity = WS_NO_HUMIDITY;
        return;
    case ID_THGR122N:
        ws_b->humidity = bcd_decode_short(data);
        ws_b->temp = subghz_oregon2_decode_temp(data >> 8);
        return;
    default:
        break;
    }
}

void subghz_protocol_decoder_oregon2_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    subghz_protocol_DecoderOregon2* instance = context;
    // oregon v2.1 signal is inverted
    ManchesterEvent event = level_and_duration_to_event(!level, duration);
    bool data;

    // low-level bit sequence decoding
    if(event == ManchesterEventReset) {
        instance->decoder.parser_step = Oregon2DecoderStepReset;
        instance->have_bit = false;
        instance->decoder.decode_data = 0UL;
        instance->decoder.decode_count_bit = 0;
    }
    if(manchester_advance(instance->manchester_state, event, &instance->manchester_state, &data)) {
        if(instance->have_bit) {
            if(!instance->prev_bit && data) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 1);
            } else if(instance->prev_bit && !data) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 0);
            } else {
                subghz_protocol_decoder_oregon2_reset(context);
            }
            instance->have_bit = false;
        } else {
            instance->prev_bit = data;
            instance->have_bit = true;
        }
    }

    switch(instance->decoder.parser_step) {
    case Oregon2DecoderStepReset:
        // waiting for fixed oregon2 preamble
        if(instance->decoder.decode_count_bit >= OREGON2_PREAMBLE_BITS &&
           ((instance->decoder.decode_data & OREGON2_PREAMBLE_MASK) == OREGON2_PREAMBLE)) {
            instance->decoder.parser_step = Oregon2DecoderStepFoundPreamble;
            instance->decoder.decode_count_bit = 0;
            instance->decoder.decode_data = 0UL;
        }
        break;
    case Oregon2DecoderStepFoundPreamble:
        // waiting for fixed oregon2 data
        if(instance->decoder.decode_count_bit == 32) {
            instance->generic.data = instance->decoder.decode_data;
            instance->generic.data_count_bit = instance->decoder.decode_count_bit;
            instance->decoder.decode_data = 0UL;
            instance->decoder.decode_count_bit = 0;

            // reverse nibbles in decoded data
            instance->generic.data = (instance->generic.data & 0x55555555) << 1 |
                                     (instance->generic.data & 0xAAAAAAAA) >> 1;
            instance->generic.data = (instance->generic.data & 0x33333333) << 2 |
                                     (instance->generic.data & 0xCCCCCCCC) >> 2;

            subghz_oregon2_decode_const_data(&instance->generic);
            instance->var_bits =
                oregon2_sensor_id_var_bits(OREGON2_SENSOR_ID(instance->generic.data));

            if(!instance->var_bits) {
                // sensor is not supported, stop decoding, but showing the decoded fixed part
                instance->decoder.parser_step = Oregon2DecoderStepReset;
                if(instance->base.callback)
                    instance->base.callback(&instance->base, instance->base.context);
            } else {
                instance->decoder.parser_step = Oregon2DecoderStepVarData;
            }
        }
        break;
    case Oregon2DecoderStepVarData:
        // waiting for variable (sensor-specific data)
        if(instance->decoder.decode_count_bit == instance->var_bits + OREGON2_CHECKSUM_BITS) {
            instance->var_data = instance->decoder.decode_data & 0xFFFFFFFF;

            // reverse nibbles in var data
            instance->var_data = (instance->var_data & 0x55555555) << 1 |
                                 (instance->var_data & 0xAAAAAAAA) >> 1;
            instance->var_data = (instance->var_data & 0x33333333) << 2 |
                                 (instance->var_data & 0xCCCCCCCC) >> 2;

            subghz_oregon2_decode_var_data(
                &instance->generic,
                OREGON2_SENSOR_ID(instance->generic.data),
                instance->var_data >> OREGON2_CHECKSUM_BITS);

            instance->decoder.parser_step = Oregon2DecoderStepReset;
            if(instance->base.callback)
                instance->base.callback(&instance->base, instance->base.context);
        }
        break;
    }
}

uint8_t subghz_protocol_decoder_oregon2_get_hash_data(void* context) {
    furi_assert(context);
    subghz_protocol_DecoderOregon2* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_oregon2_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    subghz_protocol_DecoderOregon2* instance = context;
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;
    ret = subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
    if(ret != SubGhzProtocolStatusOk) return ret;
    uint32_t temp = instance->var_bits;
    if(!flipper_format_write_uint32(flipper_format, "VarBits", &temp, 1)) {
        FURI_LOG_E(TAG, "Error adding VarBits");
        return SubGhzProtocolStatusErrorParserOthers;
    }
    if(!flipper_format_write_hex(
           flipper_format,
           "VarData",
           (const uint8_t*)&instance->var_data,
           sizeof(instance->var_data))) {
        FURI_LOG_E(TAG, "Error adding VarData");
        return SubGhzProtocolStatusErrorParserOthers;
    }
    return ret;
}

SubGhzProtocolStatus subghz_protocol_decoder_oregon2_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_DecoderOregon2* instance = context;
    uint32_t temp_data;
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;
    do {
        ret = subghz_block_generic_deserialize(&instance->generic, flipper_format);
        if(ret != SubGhzProtocolStatusOk) {
            break;
        }
        if(!flipper_format_read_uint32(flipper_format, "VarBits", &temp_data, 1)) {
            FURI_LOG_E(TAG, "Missing VarLen");
            ret = SubGhzProtocolStatusErrorParserOthers;
            break;
        }
        instance->var_bits = (uint8_t)temp_data;
        if(!flipper_format_read_hex(
               flipper_format,
               "VarData",
               (uint8_t*)&instance->var_data,
               sizeof(instance->var_data))) { //-V1051
            FURI_LOG_E(TAG, "Missing VarData");
            ret = SubGhzProtocolStatusErrorParserOthers;
            break;
        }
        if(instance->generic.data_count_bit != subghz_oregon2_const.min_count_bit_for_found) {
            FURI_LOG_E(TAG, "Wrong number of bits in key: %d", instance->generic.data_count_bit);
            ret = SubGhzProtocolStatusErrorValueBitCount;
            break;
        }
    } while(false);
    return ret;
}

static void oregon2_append_check_sum(uint32_t fix_data, uint32_t var_data, FuriString* output) {
    uint8_t sum = fix_data & 0xF;
    uint8_t ref_sum = var_data & 0xFF;
    var_data >>= 8;

    for(uint8_t i = 1; i < 8; i++) {
        fix_data >>= 4;
        var_data >>= 4;
        sum += (fix_data & 0xF) + (var_data & 0xF);
    }

    // swap calculated sum nibbles
    sum = (((sum >> 4) & 0xF) | (sum << 4)) & 0xFF;
    if(sum == ref_sum)
        furi_string_cat_printf(output, "Sum ok: 0x%hhX", ref_sum);
    else
        furi_string_cat_printf(output, "Sum err: 0x%hhX vs 0x%hhX", ref_sum, sum);
}

void subghz_protocol_decoder_oregon2_get_string(void* context, FuriString* output) {
    furi_assert(context);
    subghz_protocol_DecoderOregon2* instance = context;
    furi_string_cat_printf(
        output,
        "%s\r\n"
        "ID: 0x%04lX, ch: %d, bat: %d, rc: 0x%02lX\r\n",
        instance->generic.protocol_name,
        instance->generic.id,
        instance->generic.channel,
        instance->generic.battery_low,
        (uint32_t)(instance->generic.data >> 4) & 0xFF);

    if(instance->var_bits > 0) {
        furi_string_cat_printf(
            output,
            "Temp:%d.%d C Hum:%d%%",
            (int16_t)instance->generic.temp,
            abs(
                ((int16_t)(instance->generic.temp * 10) -
                 (((int16_t)instance->generic.temp) * 10))),
            instance->generic.humidity);
        oregon2_append_check_sum((uint32_t)instance->generic.data, instance->var_data, output);
    }
}

const SubGhzProtocolDecoder subghz_protocol_oregon2_decoder = {
    .alloc = subghz_protocol_decoder_oregon2_alloc,
    .free = subghz_protocol_decoder_oregon2_free,

    .feed = subghz_protocol_decoder_oregon2_feed,
    .reset = subghz_protocol_decoder_oregon2_reset,

    .get_hash_data = subghz_protocol_decoder_oregon2_get_hash_data,
    .serialize = subghz_protocol_decoder_oregon2_serialize,
    .deserialize = subghz_protocol_decoder_oregon2_deserialize,
    .get_string = subghz_protocol_decoder_oregon2_get_string,
};

struct subghz_protocol_encoder_oregon2 {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

const SubGhzProtocol subghz_protocol_oregon2 = {
    .name = SUBGHZ_PROTOCOL_OREGON2_NAME,
    //.type = SubGhzProtocolWeatherStation,
    .type = SubGhzProtocolTypeStatic,
	.flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Save,

    .decoder = &subghz_protocol_oregon2_decoder,
    .encoder = &subghz_protocol_oregon2_encoder,
};


const SubGhzProtocolEncoder subghz_protocol_oregon2_encoder = {
    .alloc = subghz_protocol_encoder_oregon2_alloc,
    .free = subghz_protocol_encoder_oregon2_free,

    .deserialize = subghz_protocol_encoder_oregon2_deserialize,
    .stop = subghz_protocol_encoder_oregon2_stop,
    .yield = subghz_protocol_encoder_oregon2_yield,
};

void* subghz_protocol_encoder_oregon2_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_encoder_oregon2* instance = malloc(sizeof(subghz_protocol_encoder_oregon2));

    instance->base.protocol = &subghz_protocol_oregon2;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 10;
    instance->encoder.size_upload = 52;
    instance->encoder.upload = malloc(instance->encoder.size_upload * sizeof(LevelDuration));
    instance->encoder.is_running = false;
    return instance;
}

void subghz_protocol_encoder_oregon2_free(void* context) {
    furi_assert(context);
    subghz_protocol_encoder_oregon2* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

static bool subghz_protocol_encoder_oregon2_get_upload(subghz_protocol_encoder_oregon2* instance) {
    furi_assert(instance);
    size_t index = 0;
    size_t size_upload = (instance->generic.data_count_bit * 2);
    if(size_upload > instance->encoder.size_upload) {
        FURI_LOG_E(TAG, "Size upload exceeds allocated encoder buffer.");
        return false;
    } else {
        instance->encoder.size_upload = size_upload;
    }

    for(uint8_t i = instance->generic.data_count_bit; i > 1; i--) {
        if(bit_read(instance->generic.data, i - 1)) {
            //send bit 1
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_oregon2_const.te_long);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_oregon2_const.te_short);
        } else {
            //send bit 0
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_oregon2_const.te_short);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_oregon2_const.te_long);
        }
    }
    if(bit_read(instance->generic.data, 0)) {
        //send bit 1
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_oregon2_const.te_long);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_oregon2_const.te_short +
                subghz_oregon2_const.te_long * 7);
    } else {
        //send bit 0
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_oregon2_const.te_short);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_oregon2_const.te_long +
                subghz_oregon2_const.te_long * 7);
    }
    return true;
}

LevelDuration subghz_protocol_encoder_oregon2_yield(void* context) {
    subghz_protocol_encoder_oregon2* instance = context;

    if(instance->encoder.repeat == 0 || !instance->encoder.is_running) {
        instance->encoder.is_running = false;
        return level_duration_reset();
    }

    LevelDuration ret = instance->encoder.upload[instance->encoder.front];

    if(++instance->encoder.front == instance->encoder.size_upload) {
        instance->encoder.repeat--;
        instance->encoder.front = 0;
    }

    return ret;
}

void subghz_protocol_encoder_oregon2_stop(void* context) {
    subghz_protocol_encoder_oregon2* instance = context;
    instance->encoder.is_running = false;
}

SubGhzProtocolStatus subghz_protocol_encoder_oregon2_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_encoder_oregon2* instance = context;
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;
    do {
        ret = subghz_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            subghz_oregon2_const.min_count_bit_for_found);
        if(ret != SubGhzProtocolStatusOk) {
            break;
        }
        //optional parameter parameter
        flipper_format_read_uint32(
            flipper_format, "Repeat", (uint32_t*)&instance->encoder.repeat, 1);

        if(!subghz_protocol_encoder_oregon2_get_upload(instance)) {
            ret = SubGhzProtocolStatusErrorEncoderGetUpload;
            break;
        }
        instance->encoder.is_running = true;

    } while(false);

    return ret;
}