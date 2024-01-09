#include "acurite_609txc.h"

#define TAG "subghz_protocolAcurite_609TXC"

// PORTING E PICCOLE MODIFICHE A CURA DI HAISENTECK
/*
 * Help
 * https://github.com/merbanan/rtl_433/blob/5bef4e43133ac4c0e2d18d36f87c52b4f9458453/src/devices/acurite.c#L216
 *
 *     0000 1111 | 0011 0000 | 0101 1100 | 0000 0000 | 1110 0111
 *     iiii iiii | buuu tttt | tttt tttt | hhhh hhhh | cccc cccc
 * - i: identification; changes on battery switch
 * - c: checksum (sum of previous by bytes)
 * - u: unknown
 * - b: battery low; flag to indicate low battery voltage
 * - t: temperature; in °C * 10, 12 bit with complement
 * - h: humidity
 *
 */

static const SubGhzBlockConst subghz_protocol_acurite_609txc_const = {
    .te_short = 500,
    .te_long = 1000,
    .te_delta = 150,
    .min_count_bit_for_found = 40,
};

struct subghz_protocolDecoderAcurite_609TXC {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
};

struct subghz_protocolEncoderAcurite_609TXC {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

typedef enum {
    Acurite_609TXCDecoderStepReset = 0,
    Acurite_609TXCDecoderStepSaveDuration,
    Acurite_609TXCDecoderStepCheckDuration,
} Acurite_609TXCDecoderStep;

const SubGhzProtocolDecoder subghz_protocol_acurite_609txc_decoder = {
    .alloc = subghz_protocol_decoder_acurite_609txc_alloc,
    .free = subghz_protocol_decoder_acurite_609txc_free,

    .feed = subghz_protocol_decoder_acurite_609txc_feed,
    .reset = subghz_protocol_decoder_acurite_609txc_reset,

    .get_hash_data = subghz_protocol_decoder_acurite_609txc_get_hash_data,
    .serialize = subghz_protocol_decoder_acurite_609txc_serialize,
    .deserialize = subghz_protocol_decoder_acurite_609txc_deserialize,
    .get_string = subghz_protocol_decoder_acurite_609txc_get_string,
};

const SubGhzProtocolEncoder subghz_protocol_acurite_609txc_encoder = {
    .alloc = subghz_protocol_encoder_acurite_609txc_alloc,
    .free = subghz_protocol_encoder_acurite_609txc_free,

    .deserialize = subghz_protocol_encoder_acurite_609txc_deserialize,
    .stop = subghz_protocol_encoder_acurite_609txc_stop,
    .yield = subghz_protocol_encoder_acurite_609txc_yield,
};

const SubGhzProtocol subghz_protocol_acurite_609txc = {
    .name = subghz_protocol_ACURITE_609TXC_NAME,
    //.type = SubGhzProtocolWeatherStation,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_315 | SubGhzProtocolFlag_868 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,

    .decoder = &subghz_protocol_acurite_609txc_decoder,
    .encoder = &subghz_protocol_acurite_609txc_encoder,
};

void* subghz_protocol_encoder_acurite_609txc_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocolEncoderAcurite_609TXC* instance = malloc(sizeof(subghz_protocolEncoderAcurite_609TXC));

    instance->base.protocol = &subghz_protocol_acurite_609txc;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 3;
    instance->encoder.size_upload = 40;
    instance->encoder.upload = malloc(instance->encoder.size_upload * sizeof(LevelDuration));
    instance->encoder.is_running = false;
    return instance;
}

void* subghz_protocol_decoder_acurite_609txc_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocolDecoderAcurite_609TXC* instance = malloc(sizeof(subghz_protocolDecoderAcurite_609TXC));
    instance->base.protocol = &subghz_protocol_acurite_609txc;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_acurite_609txc_free(void* context) {
    furi_assert(context);
    subghz_protocolDecoderAcurite_609TXC* instance = context;
    free(instance);
}

void subghz_protocol_decoder_acurite_609txc_reset(void* context) {
    furi_assert(context);
    subghz_protocolDecoderAcurite_609TXC* instance = context;
    instance->decoder.parser_step = Acurite_609TXCDecoderStepReset;
}

static bool subghz_protocol_acurite_609txc_check(subghz_protocolDecoderAcurite_609TXC* instance) {
    if(!instance->decoder.decode_data) return false;
    uint8_t crc = (uint8_t)(instance->decoder.decode_data >> 32) +
                  (uint8_t)(instance->decoder.decode_data >> 24) +
                  (uint8_t)(instance->decoder.decode_data >> 16) +
                  (uint8_t)(instance->decoder.decode_data >> 8);
    return (crc == (instance->decoder.decode_data & 0xFF));
}

/**
 * Analysis of received data
 * @param instance Pointer to a SubGhzBlockGeneric* instance
 */
static void subghz_protocol_acurite_609txc_remote_controller(SubGhzBlockGeneric* instance) {
    instance->id = (instance->data >> 32) & 0xFF;
    instance->battery_low = (instance->data >> 31) & 1;

    instance->channel = WS_NO_CHANNEL;

    // Temperature in Celsius is encoded as a 12 bit integer value
    // multiplied by 10 using the 4th - 6th nybbles (bytes 1 & 2)
    // negative values are recovered by sign extend from int16_t.
    int16_t temp_raw =
        (int16_t)(((instance->data >> 12) & 0xf000) | ((instance->data >> 16) << 4));
    instance->temp = (temp_raw >> 4) * 0.1f;
    instance->humidity = (instance->data >> 8) & 0xff;
    instance->btn = WS_NO_BTN;
}

void subghz_protocol_decoder_acurite_609txc_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    subghz_protocolDecoderAcurite_609TXC* instance = context;

    switch(instance->decoder.parser_step) {
    case Acurite_609TXCDecoderStepReset:
        if((!level) && (DURATION_DIFF(duration, subghz_protocol_acurite_609txc_const.te_short * 17) <
                        subghz_protocol_acurite_609txc_const.te_delta * 8)) {
            //Found syncPrefix
            instance->decoder.parser_step = Acurite_609TXCDecoderStepSaveDuration;
            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 0;
        }
        break;

    case Acurite_609TXCDecoderStepSaveDuration:
        if(level) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = Acurite_609TXCDecoderStepCheckDuration;
        } else {
            instance->decoder.parser_step = Acurite_609TXCDecoderStepReset;
        }
        break;

    case Acurite_609TXCDecoderStepCheckDuration:
        if(!level) {
            if(DURATION_DIFF(instance->decoder.te_last, subghz_protocol_acurite_609txc_const.te_short) <
               subghz_protocol_acurite_609txc_const.te_delta) {
                if((DURATION_DIFF(duration, subghz_protocol_acurite_609txc_const.te_short) <
                    subghz_protocol_acurite_609txc_const.te_delta) ||
                   (duration > subghz_protocol_acurite_609txc_const.te_long * 3)) {
                    //Found syncPostfix
                    instance->decoder.parser_step = Acurite_609TXCDecoderStepReset;
                    if((instance->decoder.decode_count_bit ==
                        subghz_protocol_acurite_609txc_const.min_count_bit_for_found) &&
                       subghz_protocol_acurite_609txc_check(instance)) {
                        instance->generic.data = instance->decoder.decode_data;
                        instance->generic.data_count_bit = instance->decoder.decode_count_bit;
                        subghz_protocol_acurite_609txc_remote_controller(&instance->generic);
                        if(instance->base.callback)
                            instance->base.callback(&instance->base, instance->base.context);
                    }
                    instance->decoder.decode_data = 0;
                    instance->decoder.decode_count_bit = 0;
                } else if(
                    DURATION_DIFF(duration, subghz_protocol_acurite_609txc_const.te_long) <
                    subghz_protocol_acurite_609txc_const.te_delta * 2) {
                    subghz_protocol_blocks_add_bit(&instance->decoder, 0);
                    instance->decoder.parser_step = Acurite_609TXCDecoderStepSaveDuration;
                } else if(
                    DURATION_DIFF(duration, subghz_protocol_acurite_609txc_const.te_long * 2) <
                    subghz_protocol_acurite_609txc_const.te_delta * 4) {
                    subghz_protocol_blocks_add_bit(&instance->decoder, 1);
                    instance->decoder.parser_step = Acurite_609TXCDecoderStepSaveDuration;
                } else {
                    instance->decoder.parser_step = Acurite_609TXCDecoderStepReset;
                }
            } else {
                instance->decoder.parser_step = Acurite_609TXCDecoderStepReset;
            }
        } else {
            instance->decoder.parser_step = Acurite_609TXCDecoderStepReset;
        }
        break;
    }
}

void subghz_protocol_encoder_acurite_609txc_free(void* context) {
    furi_assert(context);
    subghz_protocolEncoderAcurite_609TXC* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

uint8_t subghz_protocol_decoder_acurite_609txc_get_hash_data(void* context) {
    furi_assert(context);
    subghz_protocolDecoderAcurite_609TXC* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_acurite_609txc_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    subghz_protocolDecoderAcurite_609TXC* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

void subghz_protocol_encoder_acurite_609txc_stop(void* context) {
    subghz_protocolEncoderAcurite_609TXC* instance = context;
    instance->encoder.is_running = false;
}

static bool subghz_protocol_encoder_acurite_609txc_get_upload(subghz_protocolEncoderAcurite_609TXC* instance) {
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
                level_duration_make(true, (uint32_t)subghz_protocol_acurite_609txc_const.te_long);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_acurite_609txc_const.te_short);
        } else {
            //send bit 0
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_protocol_acurite_609txc_const.te_short);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_acurite_609txc_const.te_long);
        }
    }
    if(bit_read(instance->generic.data, 0)) {
        //send bit 1
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_acurite_609txc_const.te_long);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_acurite_609txc_const.te_short +
                subghz_protocol_acurite_609txc_const.te_long * 7);
    } else {
        //send bit 0
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_acurite_609txc_const.te_short);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_acurite_609txc_const.te_long +
                subghz_protocol_acurite_609txc_const.te_long * 7);
    }
    return true;
}

SubGhzProtocolStatus subghz_protocol_encoder_acurite_609txc_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocolEncoderAcurite_609TXC* instance = context;
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;
    do {
        ret = subghz_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            subghz_protocol_acurite_609txc_const.min_count_bit_for_found);
        if(ret != SubGhzProtocolStatusOk) {
            break;
        }
        //optional parameter parameter
        flipper_format_read_uint32(
            flipper_format, "Repeat", (uint32_t*)&instance->encoder.repeat, 1);

        if(!subghz_protocol_encoder_acurite_609txc_get_upload(instance)) {
            ret = SubGhzProtocolStatusErrorEncoderGetUpload;
            break;
        }
        instance->encoder.is_running = true;

    } while(false);

    return ret;
}

LevelDuration subghz_protocol_encoder_acurite_609txc_yield(void* context) {
    subghz_protocolEncoderAcurite_609TXC* instance = context;

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

SubGhzProtocolStatus subghz_protocol_decoder_acurite_609txc_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocolDecoderAcurite_609TXC* instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic,
        flipper_format,
        subghz_protocol_acurite_609txc_const.min_count_bit_for_found);
}

void subghz_protocol_decoder_acurite_609txc_get_string(void* context, FuriString* output) {
    furi_assert(context);
    subghz_protocolDecoderAcurite_609TXC* instance = context;
    furi_string_printf(
        output,
        "%s %dbit\r\n"
        "Key:0x%lX%08lX\r\n"
        "Sn:0x%lX Ch:%d  Bat:%d\r\n"
        "Temp:%3.1f C Hum:%d%%",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        (uint32_t)(instance->generic.data >> 40),
        (uint32_t)(instance->generic.data),
        instance->generic.id,
        instance->generic.channel,
        instance->generic.battery_low,
        (double)instance->generic.temp,
        instance->generic.humidity);
}
