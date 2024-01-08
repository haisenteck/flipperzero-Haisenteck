#include "gt_wt_02.h"

#define TAG "subghz_protocol_GT_WT02"

// PORTING E PICCOLE MODIFICHE A CURA DI HAISENTECK
/*
 * Help
 * https://github.com/merbanan/rtl_433/blob/master/src/devices/gt_wt_02.c
 * 
 *  GT-WT-02 sensor on 433.92MHz.
 * Example and frame description provided by https://github.com/ludwich66
 *    [01] {37} 34 00 ed 47 60 : 00110100 00000000 11101101 01000111 01100000
 *    code, BatOK,not-man-send, Channel1, +23,7°C, 35%
 *    [01] {37} 34 8f 87 15 90 : 00110100 10001111 10000111 00010101 10010000
 *    code, BatOK,not-man-send, Channel1,-12,1°C, 10%
 * Humidity:
 * - the working range is 20-90 %
 * - if "LL" in display view it sends 10 %
 * - if "HH" in display view it sends 110%
 * SENSOR: GT-WT-02 (ALDI Globaltronics..)
 *    TYP IIIIIIII BMCCTTTT TTTTTTTT HHHHHHHX XXXXX
 * TYPE Description:
 * - I = Random Device Code, changes with battery reset
 * - B = Battery 0=OK 1=LOW
 * - M = Manual Send Button Pressed 0=not pressed 1=pressed
 * - C = Channel 00=CH1, 01=CH2, 10=CH3
 * - T = Temperature, 12 Bit 2's complement, scaled by 10
 * - H = Humidity = 7 Bit bin2dez 00-99, Display LL=10%, Display HH=110% (Range 20-90%)
 * - X = Checksum, sum modulo 64
 * A Lidl AURIO (from 12/2018) with PCB marking YJ-T12 V02 has two extra bits in front.
 * 
*/

static const SubGhzBlockConst subghz_protocol_gt_wt_02_const = {
    .te_short = 500,
    .te_long = 2000,
    .te_delta = 150,
    .min_count_bit_for_found = 37,
};

struct subghz_protocol_decoder_gt_wt_02 {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
};

struct subghz_protocol_encoder_gt_wt_02 {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

typedef enum {
    GT_WT02DecoderStepReset = 0,
    GT_WT02DecoderStepSaveDuration,
    GT_WT02DecoderStepCheckDuration,
} GT_WT02DecoderStep;

const SubGhzProtocolDecoder subghz_protocol_gt_wt_02_decoder = {
    .alloc = subghz_protocol_decoder_gt_wt_02_alloc,
    .free = subghz_protocol_decoder_gt_wt_02_free,

    .feed = subghz_protocol_decoder_gt_wt_02_feed,
    .reset = subghz_protocol_decoder_gt_wt_02_reset,

    .get_hash_data = subghz_protocol_decoder_gt_wt_02_get_hash_data,
    .serialize = subghz_protocol_decoder_gt_wt_02_serialize,
    .deserialize = subghz_protocol_decoder_gt_wt_02_deserialize,
    .get_string = subghz_protocol_decoder_gt_wt_02_get_string,
};

const SubGhzProtocolEncoder subghz_protocol_gt_wt_02_encoder = {
    .alloc = subghz_protocol_encoder_gt_wt_02_alloc,
    .free = subghz_protocol_encoder_gt_wt_02_free,

    .deserialize = subghz_protocol_encoder_gt_wt_02_deserialize,
    .stop = subghz_protocol_encoder_gt_wt_02_stop,
    .yield = subghz_protocol_encoder_gt_wt_02_yield,
};

const SubGhzProtocol subghz_protocol_gt_wt_02 = {
    .name = SUBGHZ_PROTOCOL_GT_WT_02_NAME,
    //.type = SubGhzProtocolWeatherStation,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_315 | SubGhzProtocolFlag_868 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save,

    .decoder = &subghz_protocol_gt_wt_02_decoder,
    .encoder = &subghz_protocol_gt_wt_02_encoder,
};

void* subghz_protocol_encoder_gt_wt_02_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_encoder_gt_wt_02* instance = malloc(sizeof(subghz_protocol_encoder_gt_wt_02));

    instance->base.protocol = &subghz_protocol_gt_wt_02;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 10;
    instance->encoder.size_upload = 52;
    instance->encoder.upload = malloc(instance->encoder.size_upload * sizeof(LevelDuration));
    instance->encoder.is_running = false;
    return instance;
}

void* subghz_protocol_decoder_gt_wt_02_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_decoder_gt_wt_02* instance = malloc(sizeof(subghz_protocol_decoder_gt_wt_02));
    instance->base.protocol = &subghz_protocol_gt_wt_02;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_gt_wt_02_free(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_gt_wt_02* instance = context;
    free(instance);
}

void subghz_protocol_decoder_gt_wt_02_reset(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_gt_wt_02* instance = context;
    instance->decoder.parser_step = GT_WT02DecoderStepReset;
}

static bool subghz_protocol_gt_wt_02_check(subghz_protocol_decoder_gt_wt_02* instance) {
    if(!instance->decoder.decode_data) return false;
    uint8_t sum = (instance->decoder.decode_data >> 5) & 0xe;
    uint64_t temp_data = instance->decoder.decode_data >> 9;
    for(uint8_t i = 0; i < 7; i++) {
        sum += (temp_data >> (i * 4)) & 0xF;
    }
    return ((uint8_t)(instance->decoder.decode_data & 0x3F) == (sum & 0x3F));
}

/**
 * Analysis of received data
 * @param instance Pointer to a SubGhzBlockGeneric* instance
 */
static void subghz_protocol_gt_wt_02_remote_controller(SubGhzBlockGeneric* instance) {
    instance->id = (instance->data >> 29) & 0xFF;
    instance->battery_low = (instance->data >> 28) & 1;
    instance->btn = (instance->data >> 27) & 1;
    instance->channel = ((instance->data >> 25) & 0x3) + 1;

    if(!((instance->data >> 24) & 1)) {
        instance->temp = (float)((instance->data >> 13) & 0x07FF) / 10.0f;
    } else {
        instance->temp = (float)((~(instance->data >> 13) & 0x07FF) + 1) / -10.0f;
    }

    instance->humidity = (instance->data >> 6) & 0x7F;
    if(instance->humidity <= 10) // actually the sensors sends 10 below working range of 20%
        instance->humidity = 0;
    else if(instance->humidity > 90) // actually the sensors sends 110 above working range of 90%
        instance->humidity = 100;
}

void subghz_protocol_decoder_gt_wt_02_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    subghz_protocol_decoder_gt_wt_02* instance = context;

    switch(instance->decoder.parser_step) {
    case GT_WT02DecoderStepReset:
        if((!level) && (DURATION_DIFF(duration, subghz_protocol_gt_wt_02_const.te_short * 18) <
                        subghz_protocol_gt_wt_02_const.te_delta * 8)) {
            //Found syncPrefix
            instance->decoder.parser_step = GT_WT02DecoderStepSaveDuration;
            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 0;
        }
        break;

    case GT_WT02DecoderStepSaveDuration:
        if(level) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = GT_WT02DecoderStepCheckDuration;
        } else {
            instance->decoder.parser_step = GT_WT02DecoderStepReset;
        }
        break;

    case GT_WT02DecoderStepCheckDuration:
        if(!level) {
            if(DURATION_DIFF(instance->decoder.te_last, subghz_protocol_gt_wt_02_const.te_short) <
               subghz_protocol_gt_wt_02_const.te_delta) {
                if(DURATION_DIFF(duration, subghz_protocol_gt_wt_02_const.te_short * 18) <
                   subghz_protocol_gt_wt_02_const.te_delta * 8) {
                    //Found syncPostfix
                    instance->decoder.parser_step = GT_WT02DecoderStepReset;
                    if((instance->decoder.decode_count_bit ==
                        subghz_protocol_gt_wt_02_const.min_count_bit_for_found) &&
                       subghz_protocol_gt_wt_02_check(instance)) {
                        instance->generic.data = instance->decoder.decode_data;
                        instance->generic.data_count_bit = instance->decoder.decode_count_bit;
                        subghz_protocol_gt_wt_02_remote_controller(&instance->generic);
                        if(instance->base.callback)
                            instance->base.callback(&instance->base, instance->base.context);
                    } else if(instance->decoder.decode_count_bit == 1) {
                        instance->decoder.parser_step = GT_WT02DecoderStepSaveDuration;
                    }
                    instance->decoder.decode_data = 0;
                    instance->decoder.decode_count_bit = 0;
                } else if(
                    DURATION_DIFF(duration, subghz_protocol_gt_wt_02_const.te_long) <
                    subghz_protocol_gt_wt_02_const.te_delta * 2) {
                    subghz_protocol_blocks_add_bit(&instance->decoder, 0);
                    instance->decoder.parser_step = GT_WT02DecoderStepSaveDuration;
                } else if(
                    DURATION_DIFF(duration, subghz_protocol_gt_wt_02_const.te_long * 2) <
                    subghz_protocol_gt_wt_02_const.te_delta * 4) {
                    subghz_protocol_blocks_add_bit(&instance->decoder, 1);
                    instance->decoder.parser_step = GT_WT02DecoderStepSaveDuration;
                } else {
                    instance->decoder.parser_step = GT_WT02DecoderStepReset;
                }
            } else {
                instance->decoder.parser_step = GT_WT02DecoderStepReset;
            }
        } else {
            instance->decoder.parser_step = GT_WT02DecoderStepReset;
        }
        break;
    }
}

void subghz_protocol_encoder_gt_wt_02_free(void* context) {
    furi_assert(context);
    subghz_protocol_encoder_gt_wt_02* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

uint8_t subghz_protocol_decoder_gt_wt_02_get_hash_data(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_gt_wt_02* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_gt_wt_02_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    subghz_protocol_decoder_gt_wt_02* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}


static bool subghz_protocol_encoder_gt_wt_02_get_upload(subghz_protocol_encoder_gt_wt_02* instance) {
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
                level_duration_make(true, (uint32_t)subghz_protocol_gt_wt_02_const.te_long);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_gt_wt_02_const.te_short);
        } else {
            //send bit 0
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_protocol_gt_wt_02_const.te_short);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_gt_wt_02_const.te_long);
        }
    }
    if(bit_read(instance->generic.data, 0)) {
        //send bit 1
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_gt_wt_02_const.te_long);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_gt_wt_02_const.te_short +
                subghz_protocol_gt_wt_02_const.te_long * 7);
    } else {
        //send bit 0
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_gt_wt_02_const.te_short);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_gt_wt_02_const.te_long +
                subghz_protocol_gt_wt_02_const.te_long * 7);
    }
    return true;
}

LevelDuration subghz_protocol_encoder_gt_wt_02_yield(void* context) {
    subghz_protocol_encoder_gt_wt_02* instance = context;

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

void subghz_protocol_encoder_gt_wt_02_stop(void* context) {
    subghz_protocol_encoder_gt_wt_02* instance = context;
    instance->encoder.is_running = false;
}

SubGhzProtocolStatus subghz_protocol_decoder_gt_wt_02_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_decoder_gt_wt_02* instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, subghz_protocol_gt_wt_02_const.min_count_bit_for_found);
}

SubGhzProtocolStatus subghz_protocol_encoder_gt_wt_02_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_encoder_gt_wt_02* instance = context;
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;
    do {
        ret = subghz_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            subghz_protocol_gt_wt_02_const.min_count_bit_for_found);
        if(ret != SubGhzProtocolStatusOk) {
            break;
        }
        //optional parameter parameter
        flipper_format_read_uint32(
            flipper_format, "Repeat", (uint32_t*)&instance->encoder.repeat, 1);

        if(!subghz_protocol_encoder_gt_wt_02_get_upload(instance)) {
            ret = SubGhzProtocolStatusErrorEncoderGetUpload;
            break;
        }
        instance->encoder.is_running = true;

    } while(false);

    return ret;
}

void subghz_protocol_decoder_gt_wt_02_get_string(void* context, FuriString* output) {
    furi_assert(context);
    subghz_protocol_decoder_gt_wt_02* instance = context;
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
