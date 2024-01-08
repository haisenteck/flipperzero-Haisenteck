#include "oregon3.h"

#include "../blocks/const.h"
#include "../blocks/decoder.h"
#include "../blocks/encoder.h"
#include "../blocks/math.h"
#include "../blocks/generic.h"

#include <lib/toolbox/manchester_decoder.h>
#include <lib/flipper_format/flipper_format_i.h>

#define TAG "SubGhzProtocolOregon3"

// PORTING E PICCOLE MODIFICHE A CURA DI HAISENTECK

static const SubGhzBlockConst subghz_protocol_oregon3_const = {
    .te_long = 1100,
    .te_short = 500,
    .te_delta = 300,
    .min_count_bit_for_found = 32,
};

#define OREGON3_PREAMBLE_BITS 28
#define OREGON3_PREAMBLE_MASK 0b1111111111111111111111111111
// 24 ones + 0101 (inverted A)
#define OREGON3_PREAMBLE 0b1111111111111111111111110101

// Fixed part contains:
// - Sensor type: 16 bits
// - Channel: 4 bits
// - ID (changes when batteries are changed): 8 bits
// - Battery status: 4 bits
#define OREGON3_FIXED_PART_BITS (16 + 4 + 8 + 4)
#define OREGON3_SENSOR_ID(d) (((d) >> 16) & 0xFFFF)
#define OREGON3_CHECKSUM_BITS 8

// bit indicating the low battery
#define OREGON3_FLAG_BAT_LOW 0x4

/// Documentation for Oregon Scientific protocols can be found here:
/// https://www.osengr.org/Articles/OS-RF-Protocols-IV.pdf
// Sensors ID
#define ID_THGR221 0xf824

struct SubGhzProtocolDecoderOregon3 {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
    ManchesterState manchester_state;
    bool prev_bit;

    uint8_t var_bits;
    uint64_t var_data;
};

typedef struct SubGhzProtocolDecoderOregon3 SubGhzProtocolDecoderOregon3;

typedef enum {
    Oregon3DecoderStepReset = 0,
    Oregon3DecoderStepFoundPreamble,
    Oregon3DecoderStepVarData,
} Oregon3DecoderStep;

void* subghz_protocol_decoder_oregon3_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    SubGhzProtocolDecoderOregon3* instance = malloc(sizeof(SubGhzProtocolDecoderOregon3));
    instance->base.protocol = &subghz_protocol_oregon3;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->generic.humidity = WS_NO_HUMIDITY;
    instance->generic.temp = WS_NO_TEMPERATURE;
    instance->generic.btn = WS_NO_BTN;
    instance->generic.channel = WS_NO_CHANNEL;
    instance->generic.battery_low = WS_NO_BATT;
    instance->generic.id = WS_NO_ID;
    instance->prev_bit = false;
    return instance;
}

void subghz_protocol_decoder_oregon3_free(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon3* instance = context;
    free(instance);
}

void subghz_protocol_decoder_oregon3_reset(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon3* instance = context;
    instance->decoder.parser_step = Oregon3DecoderStepReset;
    instance->decoder.decode_data = 0UL;
    instance->decoder.decode_count_bit = 0;
    manchester_advance(
        instance->manchester_state, ManchesterEventReset, &instance->manchester_state, NULL);
    instance->prev_bit = false;
    instance->var_data = 0;
    instance->var_bits = 0;
}

static ManchesterEvent level_and_duration_to_event(bool level, uint32_t duration) {
    bool is_long = false;

    if(DURATION_DIFF(duration, subghz_protocol_oregon3_const.te_long) < subghz_protocol_oregon3_const.te_delta) {
        is_long = true;
    } else if(DURATION_DIFF(duration, subghz_protocol_oregon3_const.te_short) < subghz_protocol_oregon3_const.te_delta) {
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
static uint8_t oregon3_sensor_id_var_bits(uint16_t sensor_id) {
    switch(sensor_id) {
    case ID_THGR221:
        // nibbles: temp + hum + '0'
        return (4 + 2 + 1) * 4;
    default:
        FURI_LOG_D(TAG, "Unsupported sensor id 0x%x", sensor_id);
        return 0;
    }
}

static void ws_oregon3_decode_const_data(SubGhzBlockGeneric* ws_block) {
    ws_block->id = OREGON3_SENSOR_ID(ws_block->data);
    ws_block->channel = (ws_block->data >> 12) & 0xF;
    ws_block->battery_low = (ws_block->data & OREGON3_FLAG_BAT_LOW) ? 1 : 0;
}

static uint16_t ws_oregon3_bcd_decode_short(uint32_t data) {
    return (data & 0xF) * 10 + ((data >> 4) & 0xF);
}

static float ws_oregon3_decode_temp(uint32_t data) {
    int32_t temp_val;
    temp_val = ws_oregon3_bcd_decode_short(data >> 4);
    temp_val *= 10;
    temp_val += (data >> 12) & 0xF;
    if(data & 0xF) temp_val = -temp_val;
    return (float)temp_val / 10.0;
}

static void ws_oregon3_decode_var_data(SubGhzBlockGeneric* ws_b, uint16_t sensor_id, uint32_t data) {
    switch(sensor_id) {
    case ID_THGR221:
    default:
        ws_b->humidity = ws_oregon3_bcd_decode_short(data >> 4);
        ws_b->temp = ws_oregon3_decode_temp(data >> 12);
        break;
    }
}

void subghz_protocol_decoder_oregon3_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon3* instance = context;
    // Oregon v3.0 protocol is inverted
    ManchesterEvent event = level_and_duration_to_event(!level, duration);

    // low-level bit sequence decoding
    if(event == ManchesterEventReset) {
        instance->decoder.parser_step = Oregon3DecoderStepReset;
        instance->prev_bit = false;
        instance->decoder.decode_data = 0UL;
        instance->decoder.decode_count_bit = 0;
    }
    if(manchester_advance(
           instance->manchester_state, event, &instance->manchester_state, &instance->prev_bit)) {
        subghz_protocol_blocks_add_bit(&instance->decoder, instance->prev_bit);
    }

    switch(instance->decoder.parser_step) {
    case Oregon3DecoderStepReset:
        // waiting for fixed oregon3 preamble
        if(instance->decoder.decode_count_bit >= OREGON3_PREAMBLE_BITS &&
           ((instance->decoder.decode_data & OREGON3_PREAMBLE_MASK) == OREGON3_PREAMBLE)) {
            instance->decoder.parser_step = Oregon3DecoderStepFoundPreamble;
            instance->decoder.decode_count_bit = 0;
            instance->decoder.decode_data = 0UL;
        }
        break;
    case Oregon3DecoderStepFoundPreamble:
        // waiting for fixed oregon3 data
        if(instance->decoder.decode_count_bit == OREGON3_FIXED_PART_BITS) {
            instance->generic.data = instance->decoder.decode_data;
            instance->generic.data_count_bit = instance->decoder.decode_count_bit;
            instance->decoder.decode_data = 0UL;
            instance->decoder.decode_count_bit = 0;

            // reverse nibbles in decoded data as oregon v3.0 is LSB first
            instance->generic.data = (instance->generic.data & 0x55555555) << 1 |
                                     (instance->generic.data & 0xAAAAAAAA) >> 1;
            instance->generic.data = (instance->generic.data & 0x33333333) << 2 |
                                     (instance->generic.data & 0xCCCCCCCC) >> 2;

            ws_oregon3_decode_const_data(&instance->generic);
            instance->var_bits =
                oregon3_sensor_id_var_bits(OREGON3_SENSOR_ID(instance->generic.data));

            if(!instance->var_bits) {
                // sensor is not supported, stop decoding
                instance->decoder.parser_step = Oregon3DecoderStepReset;
            } else {
                instance->decoder.parser_step = Oregon3DecoderStepVarData;
            }
        }
        break;
    case Oregon3DecoderStepVarData:
        // waiting for variable (sensor-specific data)
        if(instance->decoder.decode_count_bit == instance->var_bits + OREGON3_CHECKSUM_BITS) {
            instance->var_data = instance->decoder.decode_data & 0xFFFFFFFFFFFFFFFF;

            // reverse nibbles in var data
            instance->var_data = (instance->var_data & 0x5555555555555555) << 1 |
                                 (instance->var_data & 0xAAAAAAAAAAAAAAAA) >> 1;
            instance->var_data = (instance->var_data & 0x3333333333333333) << 2 |
                                 (instance->var_data & 0xCCCCCCCCCCCCCCCC) >> 2;

            ws_oregon3_decode_var_data(
                &instance->generic,
                OREGON3_SENSOR_ID(instance->generic.data),
                instance->var_data >> OREGON3_CHECKSUM_BITS);

            instance->decoder.parser_step = Oregon3DecoderStepReset;
            if(instance->base.callback)
                instance->base.callback(&instance->base, instance->base.context);
        }
        break;
    }
}

uint8_t subghz_protocol_decoder_oregon3_get_hash_data(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon3* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_oregon3_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon3* instance = context;
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

SubGhzProtocolStatus subghz_protocol_decoder_oregon3_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon3* instance = context;
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
        if(instance->generic.data_count_bit != subghz_protocol_oregon3_const.min_count_bit_for_found) {
            FURI_LOG_E(TAG, "Wrong number of bits in key: %d", instance->generic.data_count_bit);
            ret = SubGhzProtocolStatusErrorValueBitCount;
            break;
        }
    } while(false);
    return ret;
}

static void oregon3_append_check_sum(uint32_t fix_data, uint64_t var_data, FuriString* output) {
    uint8_t sum = fix_data & 0xF;
    uint8_t ref_sum = var_data & 0xFF;
    var_data >>= 4;

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

void subghz_protocol_decoder_oregon3_get_string(void* context, FuriString* output) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon3* instance = context;
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
        oregon3_append_check_sum((uint32_t)instance->generic.data, instance->var_data, output);
    }
}

const SubGhzProtocolDecoder subghz_protocol_oregon3_decoder = {
    .alloc = subghz_protocol_decoder_oregon3_alloc,
    .free = subghz_protocol_decoder_oregon3_free,

    .feed = subghz_protocol_decoder_oregon3_feed,
    .reset = subghz_protocol_decoder_oregon3_reset,

    .get_hash_data = subghz_protocol_decoder_oregon3_get_hash_data,
    .serialize = subghz_protocol_decoder_oregon3_serialize,
    .deserialize = subghz_protocol_decoder_oregon3_deserialize,
    .get_string = subghz_protocol_decoder_oregon3_get_string,
};

struct subghz_protocol_encoder_oregon3 {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

const SubGhzProtocol subghz_protocol_oregon3 = {
    .name = SUBGHZ_PROTOCOL_OREGON3_NAME,
    //.type = SubGhzProtocolWeatherStation,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,

    .decoder = &subghz_protocol_oregon3_decoder,
    .encoder = &subghz_protocol_oregon3_encoder,
};


const SubGhzProtocolEncoder subghz_protocol_oregon3_encoder = {
    .alloc = subghz_protocol_encoder_oregon3_alloc,
    .free = subghz_protocol_encoder_oregon3_free,

    .deserialize = subghz_protocol_encoder_oregon3_deserialize,
    .stop = subghz_protocol_encoder_oregon3_stop,
    .yield = subghz_protocol_encoder_oregon3_yield,
};

void* subghz_protocol_encoder_oregon3_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_encoder_oregon3* instance = malloc(sizeof(subghz_protocol_encoder_oregon3));

    instance->base.protocol = &subghz_protocol_oregon3;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 2;
    instance->encoder.size_upload = 32;
    instance->encoder.upload = malloc(instance->encoder.size_upload * sizeof(LevelDuration));
    instance->encoder.is_running = false;
    return instance;
}

void subghz_protocol_encoder_oregon3_free(void* context) {
    furi_assert(context);
    subghz_protocol_encoder_oregon3* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

static bool subghz_protocol_encoder_oregon3_get_upload(subghz_protocol_encoder_oregon3* instance) {
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
                level_duration_make(true, (uint32_t)subghz_protocol_oregon3_const.te_long);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_oregon3_const.te_short);
        } else {
            //send bit 0
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_protocol_oregon3_const.te_short);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_oregon3_const.te_long);
        }
    }
    if(bit_read(instance->generic.data, 0)) {
        //send bit 1
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_oregon3_const.te_long);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_oregon3_const.te_short +
                subghz_protocol_oregon3_const.te_long * 7);
    } else {
        //send bit 0
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_oregon3_const.te_short);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_oregon3_const.te_long +
                subghz_protocol_oregon3_const.te_long * 7);
    }
    return true;
}

LevelDuration subghz_protocol_encoder_oregon3_yield(void* context) {
    subghz_protocol_encoder_oregon3* instance = context;

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

void subghz_protocol_encoder_oregon3_stop(void* context) {
    subghz_protocol_encoder_oregon3* instance = context;
    instance->encoder.is_running = false;
}

SubGhzProtocolStatus subghz_protocol_encoder_oregon3_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_encoder_oregon3* instance = context;
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;
    do {
        ret = subghz_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            subghz_protocol_oregon3_const.min_count_bit_for_found);
        if(ret != SubGhzProtocolStatusOk) {
            break;
        }
        //optional parameter parameter
        flipper_format_read_uint32(
            flipper_format, "Repeat", (uint32_t*)&instance->encoder.repeat, 1);

        if(!subghz_protocol_encoder_oregon3_get_upload(instance)) {
            ret = SubGhzProtocolStatusErrorEncoderGetUpload;
            break;
        }
        instance->encoder.is_running = true;

    } while(false);

    return ret;
}
