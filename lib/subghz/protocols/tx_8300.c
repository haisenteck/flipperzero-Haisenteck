#include "tx_8300.h"

#define TAG "subghz_protocolTX_8300"

// PORTING E PICCOLE MODIFICHE A CURA DI HAISENTECK
/*
 * Help
 * https://github.com/merbanan/rtl_433/blob/master/src/devices/ambientweather_tx8300.c
 *
 * Ambient Weather TX-8300 (also sold as TFA 30.3211.02).
 * 1970us pulse with variable gap (third pulse 3920 us).
 * Above 79% humidity, gap after third pulse is 5848 us.
 * - Bit 1 : 1970us pulse with 3888 us gap
 * - Bit 0 : 1970us pulse with 1936 us gap
 * 74 bit (2 bit preamble and 72 bit data => 9 bytes => 18 nibbles)
 * The preamble seems to be a repeat counter (00, and 01 seen),
 * the first 4 bytes are data,
 * the second 4 bytes the same data inverted,
 * the last byte is a checksum.
 * Preamble format (2 bits):
 *     [1 bit (0)] [1 bit rolling count]
 * Payload format (32 bits):
 *     HHHHhhhh ??CCNIII IIIITTTT ttttuuuu
 * - H = First BCD digit humidity (the MSB might be distorted by the demod)
 * - h = Second BCD digit humidity, invalid humidity seems to be 0x0e
 * - ? = Likely battery flag, 2 bits
 * - C = Channel, 2 bits
 * - N = Negative temperature sign bit
 * - I = ID, 7-bit
 * - T = First BCD digit temperature
 * - t = Second BCD digit temperature
 * - u = Third BCD digit temperature
 * The Checksum seems to covers the 4 data bytes and is something like Fletcher-8.
 **/

#define TX_8300_PACKAGE_SIZE 32

static const SubGhzBlockConst subghz_protocol_tx_8300_const = {
    .te_short = 1940,
    .te_long = 3880,
    .te_delta = 250,
    .min_count_bit_for_found = 72,
};

struct subghz_protocol_decoder_TX_8300 {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
    uint32_t package_1;
    uint32_t package_2;
};

struct subghz_protocol_encoder_tx_8300 {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

typedef enum {
    TX_8300DecoderStepReset = 0,
    TX_8300DecoderStepCheckPreambule,
    TX_8300DecoderStepSaveDuration,
    TX_8300DecoderStepCheckDuration,
} TX_8300DecoderStep;

const SubGhzProtocolDecoder subghz_protocol_tx_8300_decoder = {
    .alloc = subghz_protocol_decoder_tx_8300_alloc,
    .free = subghz_protocol_decoder_tx_8300_free,

    .feed = subghz_protocol_decoder_tx_8300_feed,
    .reset = subghz_protocol_decoder_tx_8300_reset,

    .get_hash_data = subghz_protocol_decoder_tx_8300_get_hash_data,
    .serialize = subghz_protocol_decoder_tx_8300_serialize,
    .deserialize = subghz_protocol_decoder_tx_8300_deserialize,
    .get_string = subghz_protocol_decoder_tx_8300_get_string,
};

const SubGhzProtocolEncoder subghz_protocol_tx_8300_encoder = {
    .alloc = subghz_protocol_encoder_tx_8300_alloc,
    .free = subghz_protocol_encoder_tx_8300_free,

    .deserialize = subghz_protocol_encoder_tx_8300_deserialize,
    .stop = subghz_protocol_encoder_tx_8300_stop,
    .yield = subghz_protocol_encoder_tx_8300_yield,
};

const SubGhzProtocol subghz_protocol_tx_8300 = {
    .name = subghz_protocol_TX_8300_NAME,
    //.type = SubGhzProtocolWeatherStation,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_315 | SubGhzProtocolFlag_868 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,

    .decoder = &subghz_protocol_tx_8300_decoder,
    .encoder = &subghz_protocol_tx_8300_encoder,
};

void* subghz_protocol_encoder_tx_8300_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_encoder_tx_8300* instance = malloc(sizeof(subghz_protocol_encoder_tx_8300));

    instance->base.protocol = &subghz_protocol_tx_8300;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 10;
    instance->encoder.size_upload = 72;
    instance->encoder.upload = malloc(instance->encoder.size_upload * sizeof(LevelDuration));
    instance->encoder.is_running = false;
    return instance;
}

void* subghz_protocol_decoder_tx_8300_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_decoder_TX_8300* instance = malloc(sizeof(subghz_protocol_decoder_TX_8300));
    instance->base.protocol = &subghz_protocol_tx_8300;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_tx_8300_free(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_TX_8300* instance = context;
    free(instance);
}

void subghz_protocol_decoder_tx_8300_reset(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_TX_8300* instance = context;
    instance->decoder.parser_step = TX_8300DecoderStepReset;
}

static bool subghz_protocol_tx_8300_check_crc(subghz_protocol_decoder_TX_8300* instance) {
    if(!instance->package_2) return false;
    if(instance->package_1 != ~instance->package_2) return false;

    uint16_t x = 0;
    uint16_t y = 0;
    for(int i = 0; i < 32; i += 4) {
        x += (instance->package_1 >> i) & 0x0F;
        y += (instance->package_1 >> i) & 0x05;
    }
    uint8_t crc = (~x & 0xF) << 4 | (~y & 0xF);
    return (crc == ((instance->decoder.decode_data) & 0xFF));
}

/**
 * Analysis of received data
 * @param instance Pointer to a SubGhzBlockGeneric* instance
 */
static void subghz_protocol_tx_8300_remote_controller(SubGhzBlockGeneric* instance) {
    instance->humidity = (((instance->data >> 28) & 0x0F) * 10) + ((instance->data >> 24) & 0x0F);
    instance->btn = WS_NO_BTN;
    if(!((instance->data >> 22) & 0x03))
        instance->battery_low = 0;
    else
        instance->battery_low = 1;
    instance->channel = (instance->data >> 20) & 0x03;
    instance->id = (instance->data >> 12) & 0x7F;

    float temp_raw = ((instance->data >> 8) & 0x0F) * 10.0f + ((instance->data >> 4) & 0x0F) +
                     (instance->data & 0x0F) * 0.1f;
    if(!((instance->data >> 19) & 1)) {
        instance->temp = temp_raw;
    } else {
        instance->temp = -temp_raw;
    }
}

void subghz_protocol_decoder_tx_8300_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    subghz_protocol_decoder_TX_8300* instance = context;

    switch(instance->decoder.parser_step) {
    case TX_8300DecoderStepReset:
        if((level) && (DURATION_DIFF(duration, subghz_protocol_tx_8300_const.te_short * 2) <
                       subghz_protocol_tx_8300_const.te_delta)) {
            instance->decoder.parser_step = TX_8300DecoderStepCheckPreambule;
        }
        break;

    case TX_8300DecoderStepCheckPreambule:
        if((!level) && ((DURATION_DIFF(duration, subghz_protocol_tx_8300_const.te_short * 2) <
                         subghz_protocol_tx_8300_const.te_delta) ||
                        (DURATION_DIFF(duration, subghz_protocol_tx_8300_const.te_short * 3) <
                         subghz_protocol_tx_8300_const.te_delta))) {
            instance->decoder.parser_step = TX_8300DecoderStepSaveDuration;
            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 1;
            instance->package_1 = 0;
            instance->package_2 = 0;
        } else {
            instance->decoder.parser_step = TX_8300DecoderStepReset;
        }
        break;

    case TX_8300DecoderStepSaveDuration:
        if(level) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = TX_8300DecoderStepCheckDuration;
        } else {
            instance->decoder.parser_step = TX_8300DecoderStepReset;
        }
        break;

    case TX_8300DecoderStepCheckDuration:
        if(!level) {
            if(duration >= ((uint32_t)subghz_protocol_tx_8300_const.te_short * 5)) {
                //Found syncPostfix
                if((instance->decoder.decode_count_bit ==
                    subghz_protocol_tx_8300_const.min_count_bit_for_found) &&
                   subghz_protocol_tx_8300_check_crc(instance)) {
                    instance->generic.data = instance->package_1;
                    instance->generic.data_count_bit = instance->decoder.decode_count_bit;
                    subghz_protocol_tx_8300_remote_controller(&instance->generic);
                    if(instance->base.callback)
                        instance->base.callback(&instance->base, instance->base.context);
                }
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 1;
                instance->decoder.parser_step = TX_8300DecoderStepReset;
                break;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_tx_8300_const.te_short) <
                 subghz_protocol_tx_8300_const.te_delta) &&
                (DURATION_DIFF(duration, subghz_protocol_tx_8300_const.te_long) <
                 subghz_protocol_tx_8300_const.te_delta * 2)) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 1);
                instance->decoder.parser_step = TX_8300DecoderStepSaveDuration;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_tx_8300_const.te_short) <
                 subghz_protocol_tx_8300_const.te_delta) &&
                (DURATION_DIFF(duration, subghz_protocol_tx_8300_const.te_short) <
                 subghz_protocol_tx_8300_const.te_delta)) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 0);
                instance->decoder.parser_step = TX_8300DecoderStepSaveDuration;
            } else {
                instance->decoder.parser_step = TX_8300DecoderStepReset;
            }

            if(instance->decoder.decode_count_bit == TX_8300_PACKAGE_SIZE) {
                instance->package_1 = instance->decoder.decode_data;
                instance->decoder.decode_data = 0;
            } else if(instance->decoder.decode_count_bit == TX_8300_PACKAGE_SIZE * 2) {
                instance->package_2 = instance->decoder.decode_data;
                instance->decoder.decode_data = 0;
            }

        } else {
            instance->decoder.parser_step = TX_8300DecoderStepReset;
        }
        break;
    }
}

void subghz_protocol_encoder_tx_8300_free(void* context) {
    furi_assert(context);
    subghz_protocol_encoder_tx_8300* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

uint8_t subghz_protocol_decoder_tx_8300_get_hash_data(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_TX_8300* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_tx_8300_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    subghz_protocol_decoder_TX_8300* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

static bool subghz_protocol_encoder_tx_8300_get_upload(subghz_protocol_encoder_tx_8300* instance) {
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
                level_duration_make(true, (uint32_t)subghz_protocol_tx_8300_const.te_long);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_tx_8300_const.te_short);
        } else {
            //send bit 0
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_protocol_tx_8300_const.te_short);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_tx_8300_const.te_long);
        }
    }
    if(bit_read(instance->generic.data, 0)) {
        //send bit 1
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_tx_8300_const.te_long);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_tx_8300_const.te_short +
                subghz_protocol_tx_8300_const.te_long * 7);
    } else {
        //send bit 0
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_tx_8300_const.te_short);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_tx_8300_const.te_long +
                subghz_protocol_tx_8300_const.te_long * 7);
    }
    return true;
}

LevelDuration subghz_protocol_encoder_tx_8300_yield(void* context) {
    subghz_protocol_encoder_tx_8300* instance = context;

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

void subghz_protocol_encoder_tx_8300_stop(void* context) {
    subghz_protocol_encoder_tx_8300* instance = context;
    instance->encoder.is_running = false;
}

SubGhzProtocolStatus subghz_protocol_decoder_tx_8300_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_decoder_TX_8300* instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, subghz_protocol_tx_8300_const.min_count_bit_for_found);
}

SubGhzProtocolStatus subghz_protocol_encoder_tx_8300_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_encoder_tx_8300* instance = context;
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;
    do {
        ret = subghz_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            subghz_protocol_tx_8300_const.min_count_bit_for_found);
        if(ret != SubGhzProtocolStatusOk) {
            break;
        }
        //optional parameter parameter
        flipper_format_read_uint32(
            flipper_format, "Repeat", (uint32_t*)&instance->encoder.repeat, 1);

        if(!subghz_protocol_encoder_tx_8300_get_upload(instance)) {
            ret = SubGhzProtocolStatusErrorEncoderGetUpload;
            break;
        }
        instance->encoder.is_running = true;

    } while(false);

    return ret;
}

void subghz_protocol_decoder_tx_8300_get_string(void* context, FuriString* output) {
    furi_assert(context);
    subghz_protocol_decoder_TX_8300* instance = context;
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
