#include "thermopro_tx4.h"

#define TAG "subghz_protocolThermoPRO_TX4"

// PORTING E PICCOLE MODIFICHE A CURA DI HAISENTECK
/*
 * Help
 * https://github.com/merbanan/rtl_433/blob/master/src/devices/thermopro_tx2.c
 *
  * The sensor sends 37 bits 6 times, before the first packet there is a sync pulse.
 * The packets are ppm modulated (distance coding) with a pulse of ~500 us
 * followed by a short gap of ~2000 us for a 0 bit or a long ~4000 us gap for a
 * 1 bit, the sync gap is ~9000 us.
 * The data is grouped in 9 nibbles
 *     [type] [id0] [id1] [flags] [temp0] [temp1] [temp2] [humi0] [humi1]
 * - type: 4 bit fixed 1001 (9) or 0110 (5)
 * - id: 8 bit a random id that is generated when the sensor starts, could include battery status
 *   the same batteries often generate the same id
 * - flags(3): is 1 when the battery is low, otherwise 0 (ok)
 * - flags(2): is 1 when the sensor sends a reading when pressing the button on the sensor
 * - flags(1,0): the channel number that can be set by the sensor (1, 2, 3, X)
 * - temp: 12 bit signed scaled by 10
 * - humi: 8 bit always 11001100 (0xCC) if no humidity sensor is available
 * 
 */

#define THERMO_PRO_TX4_TYPE_1 0b1001
#define THERMO_PRO_TX4_TYPE_2 0b0110

static const SubGhzBlockConst subghz_protocol_thermopro_tx4_const = {
    .te_short = 500,
    .te_long = 2000,
    .te_delta = 150,
    .min_count_bit_for_found = 37,
};

struct subghz_protocol_decoder_thermopro_tx4 {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
};

struct subghz_protocol_encoder_thermopro_tx4 {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

typedef enum {
    ThermoPRO_TX4DecoderStepReset = 0,
    ThermoPRO_TX4DecoderStepSaveDuration,
    ThermoPRO_TX4DecoderStepCheckDuration,
} ThermoPRO_TX4DecoderStep;

const SubGhzProtocolDecoder subghz_protocol_thermopro_tx4_decoder = {
    .alloc = subghz_protocol_decoder_thermopro_tx4_alloc,
    .free = subghz_protocol_decoder_thermopro_tx4_free,

    .feed = subghz_protocol_decoder_thermopro_tx4_feed,
    .reset = subghz_protocol_decoder_thermopro_tx4_reset,

    .get_hash_data = subghz_protocol_decoder_thermopro_tx4_get_hash_data,
    .serialize = subghz_protocol_decoder_thermopro_tx4_serialize,
    .deserialize = subghz_protocol_decoder_thermopro_tx4_deserialize,
    .get_string = subghz_protocol_decoder_thermopro_tx4_get_string,
};

const SubGhzProtocolEncoder subghz_protocol_thermopro_tx4_encoder = {
    .alloc = subghz_protocol_encoder_thermopro_tx4_alloc,
    .free = subghz_protocol_encoder_thermopro_tx4_free,

    .deserialize = subghz_protocol_encoder_thermopro_tx4_deserialize,
    .stop = subghz_protocol_encoder_thermopro_tx4_stop,
    .yield = subghz_protocol_encoder_thermopro_tx4_yield,
};

const SubGhzProtocol subghz_protocol_thermopro_tx4 = {
    .name = subghz_protocol_THERMOPRO_TX4_NAME,
    //.type = SubGhzProtocolWeatherStation,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_315 | SubGhzProtocolFlag_868 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,

    .decoder = &subghz_protocol_thermopro_tx4_decoder,
    .encoder = &subghz_protocol_thermopro_tx4_encoder,
};

void* subghz_protocol_encoder_thermopro_tx4_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_encoder_thermopro_tx4* instance = malloc(sizeof(subghz_protocol_encoder_thermopro_tx4));

    instance->base.protocol = &subghz_protocol_thermopro_tx4;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 6;
    instance->encoder.size_upload = 37;
    instance->encoder.upload = malloc(instance->encoder.size_upload * sizeof(LevelDuration));
    instance->encoder.is_running = false;
    return instance;
}

void* subghz_protocol_decoder_thermopro_tx4_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_decoder_thermopro_tx4* instance = malloc(sizeof(subghz_protocol_decoder_thermopro_tx4));
    instance->base.protocol = &subghz_protocol_thermopro_tx4;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_thermopro_tx4_free(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_thermopro_tx4* instance = context;
    free(instance);
}

void subghz_protocol_decoder_thermopro_tx4_reset(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_thermopro_tx4* instance = context;
    instance->decoder.parser_step = ThermoPRO_TX4DecoderStepReset;
}

static bool subghz_protocol_thermopro_tx4_check(subghz_protocol_decoder_thermopro_tx4* instance) {
    uint8_t type = instance->decoder.decode_data >> 33;

    if((type == THERMO_PRO_TX4_TYPE_1) || (type == THERMO_PRO_TX4_TYPE_2)) {
        return true;
    } else {
        return false;
    }
}

/**
 * Analysis of received data
 * @param instance Pointer to a SubGhzBlockGeneric* instance
 */
static void subghz_protocol_thermopro_tx4_remote_controller(SubGhzBlockGeneric* instance) {
    instance->id = (instance->data >> 25) & 0xFF;
    instance->battery_low = (instance->data >> 24) & 1;
    instance->btn = (instance->data >> 23) & 1;
    instance->channel = ((instance->data >> 21) & 0x03) + 1;

    if(!((instance->data >> 20) & 1)) {
        instance->temp = (float)((instance->data >> 9) & 0x07FF) / 10.0f;
    } else {
        instance->temp = (float)((~(instance->data >> 9) & 0x07FF) + 1) / -10.0f;
    }

    instance->humidity = (instance->data >> 1) & 0xFF;
}

void subghz_protocol_decoder_thermopro_tx4_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    subghz_protocol_decoder_thermopro_tx4* instance = context;

    switch(instance->decoder.parser_step) {
    case ThermoPRO_TX4DecoderStepReset:
        if((!level) && (DURATION_DIFF(duration, subghz_protocol_thermopro_tx4_const.te_short * 18) <
                        subghz_protocol_thermopro_tx4_const.te_delta * 10)) {
            //Found sync
            instance->decoder.parser_step = ThermoPRO_TX4DecoderStepSaveDuration;
            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 0;
        }
        break;

    case ThermoPRO_TX4DecoderStepSaveDuration:
        if(level) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = ThermoPRO_TX4DecoderStepCheckDuration;
        } else {
            instance->decoder.parser_step = ThermoPRO_TX4DecoderStepReset;
        }
        break;

    case ThermoPRO_TX4DecoderStepCheckDuration:
        if(!level) {
            if(DURATION_DIFF(duration, subghz_protocol_thermopro_tx4_const.te_short * 18) <
               subghz_protocol_thermopro_tx4_const.te_delta * 10) {
                //Found sync
                instance->decoder.parser_step = ThermoPRO_TX4DecoderStepReset;
                if((instance->decoder.decode_count_bit ==
                    subghz_protocol_thermopro_tx4_const.min_count_bit_for_found) &&
                   subghz_protocol_thermopro_tx4_check(instance)) {
                    instance->generic.data = instance->decoder.decode_data;
                    instance->generic.data_count_bit = instance->decoder.decode_count_bit;
                    subghz_protocol_thermopro_tx4_remote_controller(&instance->generic);
                    if(instance->base.callback)
                        instance->base.callback(&instance->base, instance->base.context);
                    instance->decoder.parser_step = ThermoPRO_TX4DecoderStepCheckDuration;
                }
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;

                break;
            } else if(
                (DURATION_DIFF(
                     instance->decoder.te_last, subghz_protocol_thermopro_tx4_const.te_short) <
                 subghz_protocol_thermopro_tx4_const.te_delta) &&
                (DURATION_DIFF(duration, subghz_protocol_thermopro_tx4_const.te_long) <
                 subghz_protocol_thermopro_tx4_const.te_delta * 2)) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 0);
                instance->decoder.parser_step = ThermoPRO_TX4DecoderStepSaveDuration;
            } else if(
                (DURATION_DIFF(
                     instance->decoder.te_last, subghz_protocol_thermopro_tx4_const.te_short) <
                 subghz_protocol_thermopro_tx4_const.te_delta) &&
                (DURATION_DIFF(duration, subghz_protocol_thermopro_tx4_const.te_long * 2) <
                 subghz_protocol_thermopro_tx4_const.te_delta * 4)) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 1);
                instance->decoder.parser_step = ThermoPRO_TX4DecoderStepSaveDuration;
            } else {
                instance->decoder.parser_step = ThermoPRO_TX4DecoderStepReset;
            }
        } else {
            instance->decoder.parser_step = ThermoPRO_TX4DecoderStepReset;
        }
        break;
    }
}

void subghz_protocol_encoder_thermopro_tx4_free(void* context) {
    furi_assert(context);
    subghz_protocol_encoder_thermopro_tx4* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

uint8_t subghz_protocol_decoder_thermopro_tx4_get_hash_data(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_thermopro_tx4* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_thermopro_tx4_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    subghz_protocol_decoder_thermopro_tx4* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

static bool subghz_protocol_encoder_thermopro_tx4_get_upload(subghz_protocol_encoder_thermopro_tx4* instance) {
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
                level_duration_make(true, (uint32_t)subghz_protocol_thermopro_tx4_const.te_long);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_thermopro_tx4_const.te_short);
        } else {
            //send bit 0
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_protocol_thermopro_tx4_const.te_short);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_thermopro_tx4_const.te_long);
        }
    }
    if(bit_read(instance->generic.data, 0)) {
        //send bit 1
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_thermopro_tx4_const.te_long);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_thermopro_tx4_const.te_short +
                subghz_protocol_thermopro_tx4_const.te_long * 7);
    } else {
        //send bit 0
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_thermopro_tx4_const.te_short);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_thermopro_tx4_const.te_long +
                subghz_protocol_thermopro_tx4_const.te_long * 7);
    }
    return true;
}

LevelDuration subghz_protocol_encoder_thermopro_tx4_yield(void* context) {
    subghz_protocol_encoder_thermopro_tx4* instance = context;

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

void subghz_protocol_encoder_thermopro_tx4_stop(void* context) {
    subghz_protocol_encoder_thermopro_tx4* instance = context;
    instance->encoder.is_running = false;
}

SubGhzProtocolStatus subghz_protocol_decoder_thermopro_tx4_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_decoder_thermopro_tx4* instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic,
        flipper_format,
        subghz_protocol_thermopro_tx4_const.min_count_bit_for_found);
}

SubGhzProtocolStatus subghz_protocol_encoder_thermopro_tx4_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_encoder_thermopro_tx4* instance = context;
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;
    do {
        ret = subghz_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            subghz_protocol_thermopro_tx4_const.min_count_bit_for_found);
        if(ret != SubGhzProtocolStatusOk) {
            break;
        }
        //optional parameter parameter
        flipper_format_read_uint32(
            flipper_format, "Repeat", (uint32_t*)&instance->encoder.repeat, 1);

        if(!subghz_protocol_encoder_thermopro_tx4_get_upload(instance)) {
            ret = SubGhzProtocolStatusErrorEncoderGetUpload;
            break;
        }
        instance->encoder.is_running = true;

    } while(false);

    return ret;
}

void subghz_protocol_decoder_thermopro_tx4_get_string(void* context, FuriString* output) {
    furi_assert(context);
    subghz_protocol_decoder_thermopro_tx4* instance = context;
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
