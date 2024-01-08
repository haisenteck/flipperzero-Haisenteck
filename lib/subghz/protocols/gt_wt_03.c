#include "gt_wt_03.h"

#define TAG "subghz_protocolGT_WT03"

// PORTING E PICCOLE MODIFICHE A CURA DI HAISENTECK
/*
 * Help
 * https://github.com/merbanan/rtl_433/blob/master/src/devices/gt_wt_03.c
 * 
 * 
 * Globaltronics GT-WT-03 sensor on 433.92MHz.
 * The 01-set sensor has 60 ms packet gap with 10 repeats.
 * The 02-set sensor has no packet gap with 23 repeats.
 * Example:
 *     {41} 17 cf be fa 6a 80 [ S1 C1 26,1 C 78.9 F 48% Bat-Good Manual-Yes ]
 *     {41} 17 cf be fa 6a 80 [ S1 C1 26,1 C 78.9 F 48% Bat-Good Manual-Yes Batt-Changed ]
 *     {41} 17 cf fe fa ea 80 [ S1 C1 26,1 C 78.9 F 48% Bat-Good Manual-No  Batt-Changed ]
 *     {41} 01 cf 6f 11 b2 80 [ S2 C2 23,8 C 74.8 F 48% Bat-LOW  Manual-No ]
 *     {41} 01 c8 d0 2b 76 80 [ S2 C3 -4,4 C 24.1 F 55% Bat-Good Manual-No  Batt-Changed ]
 * Format string:
 *     ID:8h HUM:8d B:b M:b C:2d TEMP:12d CHK:8h 1x
 * Data layout:
 *    TYP IIIIIIII HHHHHHHH BMCCTTTT TTTTTTTT XXXXXXXX
 * - I: Random Device Code: changes with battery reset
 * - H: Humidity: 8 Bit 00-99, Display LL=10%, Display HH=110% (Range 20-95%)
 * - B: Battery: 0=OK 1=LOW
 * - M: Manual Send Button Pressed: 0=not pressed, 1=pressed
 * - C: Channel: 00=CH1, 01=CH2, 10=CH3
 * - T: Temperature: 12 Bit 2's complement, scaled by 10, range-50.0 C (-50.1 shown as Lo) to +70.0 C (+70.1 C is shown as Hi)
 * - X: Checksum, xor shifting key per byte
 * Humidity:
 * - the working range is 20-95 %
 * - if "LL" in display view it sends 10 %
 * - if "HH" in display view it sends 110%
 * Checksum:
 * Per byte xor the key for each 1-bit, shift per bit. Key list per bit, starting at MSB:
 * - 0x00 [07]
 * - 0x80 [06]
 * - 0x40 [05]
 * - 0x20 [04]
 * - 0x10 [03]
 * - 0x88 [02]
 * - 0xc4 [01]
 * - 0x62 [00]
 * Note: this can also be seen as lower byte of a Galois/Fibonacci LFSR-16, gen 0x00, init 0x3100 (or 0x62 if reversed) resetting at every byte.
 * Battery voltages:
 * - U=<2,65V +- ~5% Battery indicator
 * - U=>2.10C +- 5% plausible readings
 * - U=2,00V +- ~5% Temperature offset -5°C Humidity offset unknown
 * - U=<1,95V +- ~5% does not initialize anymore
 * - U=1,90V +- 5% temperature offset -15°C
 * - U=1,80V +- 5% Display is showing refresh pattern
 * - U=1.75V +- ~5% TX causes cut out
 * 
 */

static const SubGhzBlockConst subghz_protocol_gt_wt_03_const = {
    .te_short = 285,
    .te_long = 570,
    .te_delta = 120,
    .min_count_bit_for_found = 41,
};

struct subghz_protocol_decoder_gt_wt_03 {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;

    uint16_t header_count;
};

struct subghz_protocol_encoder_gt_wt_03 {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

typedef enum {
    GT_WT03DecoderStepReset = 0,
    GT_WT03DecoderStepCheckPreambule,
    GT_WT03DecoderStepSaveDuration,
    GT_WT03DecoderStepCheckDuration,
} GT_WT03DecoderStep;

const SubGhzProtocolDecoder subghz_protocol_gt_wt_03_decoder = {
    .alloc = subghz_protocol_decoder_gt_wt_03_alloc,
    .free = subghz_protocol_decoder_gt_wt_03_free,

    .feed = subghz_protocol_decoder_gt_wt_03_feed,
    .reset = subghz_protocol_decoder_gt_wt_03_reset,

    .get_hash_data = subghz_protocol_decoder_gt_wt_03_get_hash_data,
    .serialize = subghz_protocol_decoder_gt_wt_03_serialize,
    .deserialize = subghz_protocol_decoder_gt_wt_03_deserialize,
    .get_string = subghz_protocol_decoder_gt_wt_03_get_string,
};

const SubGhzProtocolEncoder subghz_protocol_gt_wt_03_encoder = {
    .alloc = subghz_protocol_encoder_gt_wt_03_alloc,
    .free = subghz_protocol_encoder_gt_wt_03_free,

    .deserialize = subghz_protocol_encoder_gt_wt_03_deserialize,
    .stop = subghz_protocol_encoder_gt_wt_03_stop,
    .yield = subghz_protocol_encoder_gt_wt_03_yield,
};

const SubGhzProtocol subghz_protocol_gt_wt_03 = {
    .name = subghz_protocol_GT_WT_03_NAME,
    //.type = SubGhzProtocolWeatherStation,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_315 | SubGhzProtocolFlag_868 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save,

    .decoder = &subghz_protocol_gt_wt_03_decoder,
    .encoder = &subghz_protocol_gt_wt_03_encoder,
};

void* subghz_protocol_encoder_gt_wt_03_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_encoder_gt_wt_03* instance = malloc(sizeof(subghz_protocol_encoder_gt_wt_03));

    instance->base.protocol = &subghz_protocol_gt_wt_03;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 23;
    instance->encoder.size_upload = 41;
    instance->encoder.upload = malloc(instance->encoder.size_upload * sizeof(LevelDuration));
    instance->encoder.is_running = false;
    return instance;
}

void* subghz_protocol_decoder_gt_wt_03_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_decoder_gt_wt_03* instance = malloc(sizeof(subghz_protocol_decoder_gt_wt_03));
    instance->base.protocol = &subghz_protocol_gt_wt_03;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_gt_wt_03_free(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_gt_wt_03* instance = context;
    free(instance);
}

void subghz_protocol_decoder_gt_wt_03_reset(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_gt_wt_03* instance = context;
    instance->decoder.parser_step = GT_WT03DecoderStepReset;
}

static bool subghz_protocol_gt_wt_03_check_crc(subghz_protocol_decoder_gt_wt_03* instance) {
    uint8_t msg[] = {
        instance->decoder.decode_data >> 33,
        instance->decoder.decode_data >> 25,
        instance->decoder.decode_data >> 17,
        instance->decoder.decode_data >> 9};

    uint8_t sum = 0;
    for(unsigned k = 0; k < sizeof(msg); ++k) {
        uint8_t data = msg[k];
        uint16_t key = 0x3100;
        for(int i = 7; i >= 0; --i) {
            // XOR key into sum if data bit is set
            if((data >> i) & 1) sum ^= key & 0xff;
            // roll the key right
            key = (key >> 1);
        }
    }
    return ((sum ^ (uint8_t)((instance->decoder.decode_data >> 1) & 0xFF)) == 0x2D);
}

/**
 * Analysis of received data
 * @param instance Pointer to a SubGhzBlockGeneric* instance
 */
static void subghz_protocol_gt_wt_03_remote_controller(SubGhzBlockGeneric* instance) {
    instance->id = instance->data >> 33;
    instance->humidity = (instance->data >> 25) & 0xFF;

    if(instance->humidity <= 10) { // actually the sensors sends 10 below working range of 20%
        instance->humidity = 0;
    } else if(instance->humidity > 95) { // actually the sensors sends 110 above working range of 90%
        instance->humidity = 100;
    }

    instance->battery_low = (instance->data >> 24) & 1;
    instance->btn = (instance->data >> 23) & 1;
    instance->channel = ((instance->data >> 21) & 0x03) + 1;

    if(!((instance->data >> 20) & 1)) {
        instance->temp = (float)((instance->data >> 9) & 0x07FF) / 10.0f;
    } else {
        instance->temp = (float)((~(instance->data >> 9) & 0x07FF) + 1) / -10.0f;
    }
}

void subghz_protocol_decoder_gt_wt_03_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    subghz_protocol_decoder_gt_wt_03* instance = context;

    switch(instance->decoder.parser_step) {
    case GT_WT03DecoderStepReset:
        if((level) && (DURATION_DIFF(duration, subghz_protocol_gt_wt_03_const.te_short * 3) <
                       subghz_protocol_gt_wt_03_const.te_delta * 2)) {
            instance->decoder.parser_step = GT_WT03DecoderStepCheckPreambule;
            instance->decoder.te_last = duration;
            instance->header_count = 0;
        }
        break;

    case GT_WT03DecoderStepCheckPreambule:
        if(level) {
            instance->decoder.te_last = duration;
        } else {
            if((DURATION_DIFF(instance->decoder.te_last, subghz_protocol_gt_wt_03_const.te_short * 3) <
                subghz_protocol_gt_wt_03_const.te_delta * 2) &&
               (DURATION_DIFF(duration, subghz_protocol_gt_wt_03_const.te_short * 3) <
                subghz_protocol_gt_wt_03_const.te_delta * 2)) {
                //Found preambule
                instance->header_count++;
            } else if(instance->header_count == 4) {
                if((DURATION_DIFF(instance->decoder.te_last, subghz_protocol_gt_wt_03_const.te_short) <
                    subghz_protocol_gt_wt_03_const.te_delta) &&
                   (DURATION_DIFF(duration, subghz_protocol_gt_wt_03_const.te_long) <
                    subghz_protocol_gt_wt_03_const.te_delta)) {
                    instance->decoder.decode_data = 0;
                    instance->decoder.decode_count_bit = 0;
                    subghz_protocol_blocks_add_bit(&instance->decoder, 0);
                    instance->decoder.parser_step = GT_WT03DecoderStepSaveDuration;
                } else if(
                    (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_gt_wt_03_const.te_long) <
                     subghz_protocol_gt_wt_03_const.te_delta) &&
                    (DURATION_DIFF(duration, subghz_protocol_gt_wt_03_const.te_short) <
                     subghz_protocol_gt_wt_03_const.te_delta)) {
                    instance->decoder.decode_data = 0;
                    instance->decoder.decode_count_bit = 0;
                    subghz_protocol_blocks_add_bit(&instance->decoder, 1);
                    instance->decoder.parser_step = GT_WT03DecoderStepSaveDuration;
                } else {
                    instance->decoder.parser_step = GT_WT03DecoderStepReset;
                }
            } else {
                instance->decoder.parser_step = GT_WT03DecoderStepReset;
            }
        }
        break;

    case GT_WT03DecoderStepSaveDuration:
        if(level) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = GT_WT03DecoderStepCheckDuration;
        } else {
            instance->decoder.parser_step = GT_WT03DecoderStepReset;
        }
        break;

    case GT_WT03DecoderStepCheckDuration:
        if(!level) {
            if(((DURATION_DIFF(instance->decoder.te_last, subghz_protocol_gt_wt_03_const.te_short * 3) <
                 subghz_protocol_gt_wt_03_const.te_delta * 2) &&
                (DURATION_DIFF(duration, subghz_protocol_gt_wt_03_const.te_short * 3) <
                 subghz_protocol_gt_wt_03_const.te_delta * 2))) {
                if((instance->decoder.decode_count_bit ==
                    subghz_protocol_gt_wt_03_const.min_count_bit_for_found) &&
                   subghz_protocol_gt_wt_03_check_crc(instance)) {
                    instance->generic.data = instance->decoder.decode_data;
                    instance->generic.data_count_bit = instance->decoder.decode_count_bit;
                    subghz_protocol_gt_wt_03_remote_controller(&instance->generic);
                    if(instance->base.callback)
                        instance->base.callback(&instance->base, instance->base.context);
                }
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                instance->header_count = 1;
                instance->decoder.parser_step = GT_WT03DecoderStepCheckPreambule;
                break;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_gt_wt_03_const.te_short) <
                 subghz_protocol_gt_wt_03_const.te_delta) &&
                (DURATION_DIFF(duration, subghz_protocol_gt_wt_03_const.te_long) <
                 subghz_protocol_gt_wt_03_const.te_delta)) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 0);
                instance->decoder.parser_step = GT_WT03DecoderStepSaveDuration;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_gt_wt_03_const.te_long) <
                 subghz_protocol_gt_wt_03_const.te_delta) &&
                (DURATION_DIFF(duration, subghz_protocol_gt_wt_03_const.te_short) <
                 subghz_protocol_gt_wt_03_const.te_delta)) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 1);
                instance->decoder.parser_step = GT_WT03DecoderStepSaveDuration;
            } else {
                instance->decoder.parser_step = GT_WT03DecoderStepReset;
            }
        } else {
            instance->decoder.parser_step = GT_WT03DecoderStepReset;
        }
        break;
    }
}

void subghz_protocol_encoder_gt_wt_03_free(void* context) {
    furi_assert(context);
    subghz_protocol_encoder_gt_wt_03* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

uint8_t subghz_protocol_decoder_gt_wt_03_get_hash_data(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_gt_wt_03* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_gt_wt_03_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    subghz_protocol_decoder_gt_wt_03* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

static bool subghz_protocol_encoder_gt_wt_03_get_upload(subghz_protocol_encoder_gt_wt_03* instance) {
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
                level_duration_make(true, (uint32_t)subghz_protocol_gt_wt_03_const.te_long);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_gt_wt_03_const.te_short);
        } else {
            //send bit 0
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_protocol_gt_wt_03_const.te_short);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_gt_wt_03_const.te_long);
        }
    }
    if(bit_read(instance->generic.data, 0)) {
        //send bit 1
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_gt_wt_03_const.te_long);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_gt_wt_03_const.te_short +
                subghz_protocol_gt_wt_03_const.te_long * 7);
    } else {
        //send bit 0
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_gt_wt_03_const.te_short);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_gt_wt_03_const.te_long +
                subghz_protocol_gt_wt_03_const.te_long * 7);
    }
    return true;
}

LevelDuration subghz_protocol_encoder_gt_wt_03_yield(void* context) {
    subghz_protocol_encoder_gt_wt_03* instance = context;

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

void subghz_protocol_encoder_gt_wt_03_stop(void* context) {
    subghz_protocol_encoder_gt_wt_03* instance = context;
    instance->encoder.is_running = false;
}

SubGhzProtocolStatus subghz_protocol_decoder_gt_wt_03_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_decoder_gt_wt_03* instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, subghz_protocol_gt_wt_03_const.min_count_bit_for_found);
}

SubGhzProtocolStatus subghz_protocol_encoder_gt_wt_03_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_encoder_gt_wt_03* instance = context;
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;
    do {
        ret = subghz_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            subghz_protocol_gt_wt_03_const.min_count_bit_for_found);
        if(ret != SubGhzProtocolStatusOk) {
            break;
        }
        //optional parameter parameter
        flipper_format_read_uint32(
            flipper_format, "Repeat", (uint32_t*)&instance->encoder.repeat, 1);

        if(!subghz_protocol_encoder_gt_wt_03_get_upload(instance)) {
            ret = SubGhzProtocolStatusErrorEncoderGetUpload;
            break;
        }
        instance->encoder.is_running = true;

    } while(false);

    return ret;
}

void subghz_protocol_decoder_gt_wt_03_get_string(void* context, FuriString* output) {
    furi_assert(context);
    subghz_protocol_decoder_gt_wt_03* instance = context;
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
