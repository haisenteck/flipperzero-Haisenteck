#include "lacrosse_tx141thbv2.h"

#define TAG "subghz_protocolLaCrosse_TX141THBv2"

#define LACROSSE_TX141TH_BV2_BIT_COUNT 41

// PORTING E PICCOLE MODIFICHE A CURA DI HAISENTECK
/*
 * Help
 * https://github.com/merbanan/rtl_433/blob/master/src/devices/lacrosse_tx141x.c
 *  
 *     iiii iiii | bkcc tttt | tttt tttt | hhhh hhhh | cccc cccc | u - 41 bit
 *        or
 *     iiii iiii | bkcc tttt | tttt tttt | hhhh hhhh | cccc cccc | -40 bit
 * - i: identification; changes on battery switch
 * - c: lfsr_digest8_reflect;
 * - u: unknown; 
 * - b: battery low; flag to indicate low battery voltage
 * - h: Humidity; 
 * - t: Temperature; in °F as binary number with one decimal place + 50 °F offset
 * - n: Channel; Channel number 1 - 3
 */

static const SubGhzBlockConst subghz_protocol_lacrosse_tx141thbv2_const = {
    .te_short = 208,
    .te_long = 417,
    .te_delta = 120,
    .min_count_bit_for_found = 40,
};

struct subghz_protocol_decoder_lacrosse_tx141thbv2 {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;

    uint16_t header_count;
};

struct subghz_protocol_encoder_lacrosse_tx141thbv2 {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

typedef enum {
    LaCrosse_TX141THBv2DecoderStepReset = 0,
    LaCrosse_TX141THBv2DecoderStepCheckPreambule,
    LaCrosse_TX141THBv2DecoderStepSaveDuration,
    LaCrosse_TX141THBv2DecoderStepCheckDuration,
} LaCrosse_TX141THBv2DecoderStep;

const SubGhzProtocolDecoder subghz_protocol_lacrosse_tx141thbv2_decoder = {
    .alloc = subghz_protocol_decoder_lacrosse_tx141thbv2_alloc,
    .free = subghz_protocol_decoder_lacrosse_tx141thbv2_free,

    .feed = subghz_protocol_decoder_lacrosse_tx141thbv2_feed,
    .reset = subghz_protocol_decoder_lacrosse_tx141thbv2_reset,

    .get_hash_data = subghz_protocol_decoder_lacrosse_tx141thbv2_get_hash_data,
    .serialize = subghz_protocol_decoder_lacrosse_tx141thbv2_serialize,
    .deserialize = subghz_protocol_decoder_lacrosse_tx141thbv2_deserialize,
    .get_string = subghz_protocol_decoder_lacrosse_tx141thbv2_get_string,
};

const SubGhzProtocolEncoder subghz_protocol_lacrosse_tx141thbv2_encoder = {
    .alloc = subghz_protocol_encoder_lacrosse_tx141thbv2_alloc,
    .free = subghz_protocol_encoder_lacrosse_tx141thbv2_free,

    .deserialize = subghz_protocol_encoder_lacrosse_tx141thbv2_deserialize,
    .stop = subghz_protocol_encoder_lacrosse_tx141thbv2_stop,
    .yield = subghz_protocol_encoder_lacrosse_tx141thbv2_yield,
};

const SubGhzProtocol subghz_protocol_lacrosse_tx141thbv2 = {
    .name = subghz_protocol_LACROSSE_TX141THBV2_NAME,
    .type = SubGhzProtocolWeatherStation,
    //.type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_315 | SubGhzProtocolFlag_868 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,

    .decoder = &subghz_protocol_lacrosse_tx141thbv2_decoder,
    .encoder = &subghz_protocol_lacrosse_tx141thbv2_encoder,
};

void* subghz_protocol_encoder_lacrosse_tx141thbv2_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_encoder_lacrosse_tx141thbv2* instance = malloc(sizeof(subghz_protocol_encoder_lacrosse_tx141thbv2));

    instance->base.protocol = &subghz_protocol_lacrosse_tx141thbv2;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 3;
    instance->encoder.size_upload = 40;
    instance->encoder.upload = malloc(instance->encoder.size_upload * sizeof(LevelDuration));
    instance->encoder.is_running = false;
    return instance;
}

void* subghz_protocol_decoder_lacrosse_tx141thbv2_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_decoder_lacrosse_tx141thbv2* instance =
        malloc(sizeof(subghz_protocol_decoder_lacrosse_tx141thbv2));
    instance->base.protocol = &subghz_protocol_lacrosse_tx141thbv2;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_lacrosse_tx141thbv2_free(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_lacrosse_tx141thbv2* instance = context;
    free(instance);
}

void subghz_protocol_decoder_lacrosse_tx141thbv2_reset(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_lacrosse_tx141thbv2* instance = context;
    instance->decoder.parser_step = LaCrosse_TX141THBv2DecoderStepReset;
}

static bool subghz_protocol_lacrosse_tx141thbv2_check_crc(subghz_protocol_decoder_lacrosse_tx141thbv2* instance) {
    if(!instance->decoder.decode_data) return false;
    uint64_t data = instance->decoder.decode_data;
    if(instance->decoder.decode_count_bit == LACROSSE_TX141TH_BV2_BIT_COUNT) {
        data >>= 1;
    }
    uint8_t msg[] = {data >> 32, data >> 24, data >> 16, data >> 8};

    uint8_t crc = subghz_protocol_blocks_lfsr_digest8_reflect(msg, 4, 0x31, 0xF4);
    return (crc == (data & 0xFF));
}

/**
 * Analysis of received data
 * @param instance Pointer to a SubGhzBlockGeneric* instance
 */
static void subghz_protocol_lacrosse_tx141thbv2_remote_controller(SubGhzBlockGeneric* instance) {
    uint64_t data = instance->data;
    if(instance->data_count_bit == LACROSSE_TX141TH_BV2_BIT_COUNT) {
        data >>= 1;
    }
    instance->id = data >> 32;
    instance->battery_low = (data >> 31) & 1;
    instance->btn = (data >> 30) & 1;
    instance->channel = ((data >> 28) & 0x03) + 1;
    instance->temp = ((float)((data >> 16) & 0x0FFF) - 500.0f) / 10.0f;
    instance->humidity = (data >> 8) & 0xFF;
}

/**
 * Analysis of received data
 * @param instance Pointer to a SubGhzBlockGeneric* instance
 */
static bool subghz_protocol_decoder_lacrosse_tx141thbv2_add_bit(
    subghz_protocol_decoder_lacrosse_tx141thbv2* instance,
    uint32_t te_last,
    uint32_t te_current) {
    furi_assert(instance);
    bool ret = false;
    if(DURATION_DIFF(
           te_last + te_current,
           subghz_protocol_lacrosse_tx141thbv2_const.te_short +
               subghz_protocol_lacrosse_tx141thbv2_const.te_long) <
       subghz_protocol_lacrosse_tx141thbv2_const.te_delta * 2) {
        if(te_last > te_current) {
            subghz_protocol_blocks_add_bit(&instance->decoder, 1);
        } else {
            subghz_protocol_blocks_add_bit(&instance->decoder, 0);
        }
        ret = true;
    }

    return ret;
}

void subghz_protocol_decoder_lacrosse_tx141thbv2_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    subghz_protocol_decoder_lacrosse_tx141thbv2* instance = context;

    switch(instance->decoder.parser_step) {
    case LaCrosse_TX141THBv2DecoderStepReset:
        if((level) &&
           (DURATION_DIFF(duration, subghz_protocol_lacrosse_tx141thbv2_const.te_short * 4) <
            subghz_protocol_lacrosse_tx141thbv2_const.te_delta * 2)) {
            instance->decoder.parser_step = LaCrosse_TX141THBv2DecoderStepCheckPreambule;
            instance->decoder.te_last = duration;
            instance->header_count = 0;
        }
        break;

    case LaCrosse_TX141THBv2DecoderStepCheckPreambule:
        if(level) {
            instance->decoder.te_last = duration;
        } else {
            if((DURATION_DIFF(
                    instance->decoder.te_last,
                    subghz_protocol_lacrosse_tx141thbv2_const.te_short * 4) <
                subghz_protocol_lacrosse_tx141thbv2_const.te_delta * 2) &&
               (DURATION_DIFF(duration, subghz_protocol_lacrosse_tx141thbv2_const.te_short * 4) <
                subghz_protocol_lacrosse_tx141thbv2_const.te_delta * 2)) {
                //Found preambule
                instance->header_count++;
            } else if(instance->header_count == 4) {
                if(subghz_protocol_decoder_lacrosse_tx141thbv2_add_bit(
                       instance, instance->decoder.te_last, duration)) {
                    instance->decoder.decode_data = instance->decoder.decode_data & 1;
                    instance->decoder.decode_count_bit = 1;
                    instance->decoder.parser_step = LaCrosse_TX141THBv2DecoderStepSaveDuration;
                } else {
                    instance->decoder.parser_step = LaCrosse_TX141THBv2DecoderStepReset;
                }
            } else {
                instance->decoder.parser_step = LaCrosse_TX141THBv2DecoderStepReset;
            }
        }
        break;

    case LaCrosse_TX141THBv2DecoderStepSaveDuration:
        if(level) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = LaCrosse_TX141THBv2DecoderStepCheckDuration;
        } else {
            instance->decoder.parser_step = LaCrosse_TX141THBv2DecoderStepReset;
        }
        break;

    case LaCrosse_TX141THBv2DecoderStepCheckDuration:
        if(!level) {
            if(((DURATION_DIFF(
                     instance->decoder.te_last,
                     subghz_protocol_lacrosse_tx141thbv2_const.te_short * 3) <
                 subghz_protocol_lacrosse_tx141thbv2_const.te_delta * 2) &&
                (DURATION_DIFF(duration, subghz_protocol_lacrosse_tx141thbv2_const.te_short * 4) <
                 subghz_protocol_lacrosse_tx141thbv2_const.te_delta * 2))) {
                if((instance->decoder.decode_count_bit ==
                    subghz_protocol_lacrosse_tx141thbv2_const.min_count_bit_for_found) ||
                   (instance->decoder.decode_count_bit == LACROSSE_TX141TH_BV2_BIT_COUNT)) {
                    if(subghz_protocol_lacrosse_tx141thbv2_check_crc(instance)) {
                        instance->generic.data = instance->decoder.decode_data;
                        instance->generic.data_count_bit = instance->decoder.decode_count_bit;
                        subghz_protocol_lacrosse_tx141thbv2_remote_controller(&instance->generic);
                        if(instance->base.callback)
                            instance->base.callback(&instance->base, instance->base.context);
                    }
                    instance->decoder.decode_data = 0;
                    instance->decoder.decode_count_bit = 0;
                    instance->header_count = 1;
                    instance->decoder.parser_step = LaCrosse_TX141THBv2DecoderStepCheckPreambule;
                    break;
                }
            } else if(subghz_protocol_decoder_lacrosse_tx141thbv2_add_bit(
                          instance, instance->decoder.te_last, duration)) {
                instance->decoder.parser_step = LaCrosse_TX141THBv2DecoderStepSaveDuration;
            } else {
                instance->decoder.parser_step = LaCrosse_TX141THBv2DecoderStepReset;
            }
        } else {
            instance->decoder.parser_step = LaCrosse_TX141THBv2DecoderStepReset;
        }
        break;
    }
}

void subghz_protocol_encoder_lacrosse_tx141thbv2_free(void* context) {
    furi_assert(context);
    subghz_protocol_encoder_lacrosse_tx141thbv2* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

uint8_t subghz_protocol_decoder_lacrosse_tx141thbv2_get_hash_data(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_lacrosse_tx141thbv2* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_lacrosse_tx141thbv2_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    subghz_protocol_decoder_lacrosse_tx141thbv2* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

static bool subghz_protocol_encoder_lacrosse_tx141thbv2_get_upload(subghz_protocol_encoder_lacrosse_tx141thbv2* instance) {
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
                level_duration_make(true, (uint32_t)subghz_protocol_lacrosse_tx141thbv2_const.te_long);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_lacrosse_tx141thbv2_const.te_short);
        } else {
            //send bit 0
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_protocol_lacrosse_tx141thbv2_const.te_short);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_lacrosse_tx141thbv2_const.te_long);
        }
    }
    if(bit_read(instance->generic.data, 0)) {
        //send bit 1
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_lacrosse_tx141thbv2_const.te_long);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_lacrosse_tx141thbv2_const.te_short +
                subghz_protocol_lacrosse_tx141thbv2_const.te_long * 7);
    } else {
        //send bit 0
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_lacrosse_tx141thbv2_const.te_short);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_lacrosse_tx141thbv2_const.te_long +
                subghz_protocol_lacrosse_tx141thbv2_const.te_long * 7);
    }
    return true;
}

LevelDuration subghz_protocol_encoder_lacrosse_tx141thbv2_yield(void* context) {
    subghz_protocol_encoder_lacrosse_tx141thbv2* instance = context;

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

void subghz_protocol_encoder_lacrosse_tx141thbv2_stop(void* context) {
    subghz_protocol_encoder_lacrosse_tx141thbv2* instance = context;
    instance->encoder.is_running = false;
}

SubGhzProtocolStatus subghz_protocol_decoder_lacrosse_tx141thbv2_deserialize(
    void* context,
    FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_decoder_lacrosse_tx141thbv2* instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic,
        flipper_format,
        subghz_protocol_lacrosse_tx141thbv2_const.min_count_bit_for_found);
}

SubGhzProtocolStatus subghz_protocol_encoder_lacrosse_tx141thbv2_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_encoder_lacrosse_tx141thbv2* instance = context;
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;
    do {
        ret = subghz_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            subghz_protocol_lacrosse_tx141thbv2_const.min_count_bit_for_found);
        if(ret != SubGhzProtocolStatusOk) {
            break;
        }
        //optional parameter parameter
        flipper_format_read_uint32(
            flipper_format, "Repeat", (uint32_t*)&instance->encoder.repeat, 1);

        if(!subghz_protocol_encoder_lacrosse_tx141thbv2_get_upload(instance)) {
            ret = SubGhzProtocolStatusErrorEncoderGetUpload;
            break;
        }
        instance->encoder.is_running = true;

    } while(false);

    return ret;
}

void subghz_protocol_decoder_lacrosse_tx141thbv2_get_string(void* context, FuriString* output) {
    furi_assert(context);
    subghz_protocol_decoder_lacrosse_tx141thbv2* instance = context;
    furi_string_printf(
        output,
        "%s %dbit\r\n"
        "Key:0x%lX%08lX\r\n"
        "Sn:0x%lX Ch:%d  Bat:%d\r\n"
        "Temp:%3.1f C Hum:%d%%",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        (uint32_t)(instance->generic.data >> 32),
        (uint32_t)(instance->generic.data),
        instance->generic.id,
        instance->generic.channel,
        instance->generic.battery_low,
        (double)instance->generic.temp,
        instance->generic.humidity);
}
