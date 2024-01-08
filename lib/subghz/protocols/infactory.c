#include "infactory.h"

#define TAG "subghz_protocolInfactory"

// PORTING E PICCOLE MODIFICHE A CURA DI HAISENTECK
/*
 * Help
 * https://github.com/merbanan/rtl_433/blob/master/src/devices/infactory.c
 *
 * Analysis using Genuino (see http://gitlab.com/hp-uno, e.g. uno_log_433):
 * Observed On-Off-Key (OOK) data pattern:
 *     preamble            syncPrefix        data...(40 bit)                        syncPostfix
 *     HHLL HHLL HHLL HHLL HLLLLLLLLLLLLLLLL (HLLLL HLLLLLLLL HLLLL HLLLLLLLL ....) HLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
 * Breakdown:
 * - four preamble pairs '1'/'0' each with a length of ca. 1000us
 * - syncPre, syncPost, data0, data1 have a '1' start pulse of ca. 500us
 * - syncPre pulse before dataPtr has a '0' pulse length of ca. 8000us
 * - data0 (0-bits) have then a '0' pulse length of ca. 2000us
 * - data1 (1-bits) have then a '0' pulse length of ca. 4000us
 * - syncPost after dataPtr has a '0' pulse length of ca. 16000us
 * This analysis is the reason for the new r_device definitions below.
 * NB: pulse_slicer_ppm does not use .gap_limit if .tolerance is set.
 * 
 * Outdoor sensor, transmits temperature and humidity data
 * - inFactory NC-3982-913/NX-5817-902, Pearl (for FWS-686 station)
 * - nor-tec 73383 (weather station + sensor), Schou Company AS, Denmark
 * - DAY 73365 (weather station + sensor), Schou Company AS, Denmark
 * Known brand names: inFactory, nor-tec, GreenBlue, DAY. Manufacturer in China.
 * Transmissions includes an id. Every 60 seconds the sensor transmits 6 packets:
 *     0000 1111 | 0011 0000 | 0101 1100 | 1110 0111 | 0110 0001
 *     iiii iiii | cccc ub?? | tttt tttt | tttt hhhh | hhhh ??nn
 * - i: identification; changes on battery switch
 * - c: CRC-4; CCITT checksum, see below for computation specifics
 * - u: unknown; (sometimes set at power-on, but not always)
 * - b: battery low; flag to indicate low battery voltage
 * - h: Humidity; BCD-encoded, each nibble is one digit, 'A0' means 100%rH
 * - t: Temperature; in °F as binary number with one decimal place + 90 °F offset
 * - n: Channel; Channel number 1 - 3
 * 
 */

static const SubGhzBlockConst subghz_protocol_infactory_const = {
    .te_short = 500,
    .te_long = 2000,
    .te_delta = 150,
    .min_count_bit_for_found = 40,
};

struct subghz_protocol_decoder_infactory {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;

    uint16_t header_count;
};

struct subghz_protocol_encoder_infactory {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

typedef enum {
    InfactoryDecoderStepReset = 0,
    InfactoryDecoderStepCheckPreambule,
    InfactoryDecoderStepSaveDuration,
    InfactoryDecoderStepCheckDuration,
} InfactoryDecoderStep;

const SubGhzProtocolDecoder subghz_protocol_infactory_decoder = {
    .alloc = subghz_protocol_decoder_infactory_alloc,
    .free = subghz_protocol_decoder_infactory_free,

    .feed = subghz_protocol_decoder_infactory_feed,
    .reset = subghz_protocol_decoder_infactory_reset,

    .get_hash_data = subghz_protocol_decoder_infactory_get_hash_data,
    .serialize = subghz_protocol_decoder_infactory_serialize,
    .deserialize = subghz_protocol_decoder_infactory_deserialize,
    .get_string = subghz_protocol_decoder_infactory_get_string,
};

const SubGhzProtocolEncoder subghz_protocol_infactory_encoder = {
    .alloc = subghz_protocol_encoder_infactory_alloc,
    .free = subghz_protocol_encoder_infactory_free,

    .deserialize = subghz_protocol_encoder_infactory_deserialize,
    .stop = subghz_protocol_encoder_infactory_stop,
    .yield = subghz_protocol_encoder_infactory_yield,
};

const SubGhzProtocol subghz_protocol_infactory = {
    .name = subghz_protocol_INFACTORY_NAME,
    .type = SubGhzProtocolWeatherStation,
    //.type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_315 | SubGhzProtocolFlag_868 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save,

    .decoder = &subghz_protocol_infactory_decoder,
    .encoder = &subghz_protocol_infactory_encoder,
};

void* subghz_protocol_encoder_infactory_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_encoder_infactory* instance = malloc(sizeof(subghz_protocol_encoder_infactory));

    instance->base.protocol = &subghz_protocol_infactory;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 10;
    instance->encoder.size_upload = 40;
    instance->encoder.upload = malloc(instance->encoder.size_upload * sizeof(LevelDuration));
    instance->encoder.is_running = false;
    return instance;
}

void* subghz_protocol_decoder_infactory_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_decoder_infactory* instance = malloc(sizeof(subghz_protocol_decoder_infactory));
    instance->base.protocol = &subghz_protocol_infactory;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_infactory_free(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_infactory* instance = context;
    free(instance);
}

void subghz_protocol_decoder_infactory_reset(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_infactory* instance = context;
    instance->decoder.parser_step = InfactoryDecoderStepReset;
}

static bool subghz_protocol_infactory_check_crc(subghz_protocol_decoder_infactory* instance) {
    uint8_t msg[] = {
        instance->decoder.decode_data >> 32,
        (((instance->decoder.decode_data >> 24) & 0x0F) | (instance->decoder.decode_data & 0x0F)
                                                              << 4),
        instance->decoder.decode_data >> 16,
        instance->decoder.decode_data >> 8,
        instance->decoder.decode_data};

    uint8_t crc =
        subghz_protocol_blocks_crc4(msg, 4, 0x13, 0); // Koopmann 0x9, CCITT-4; FP-4; ITU-T G.704
    crc ^= msg[4] >> 4; // last nibble is only XORed
    return (crc == ((instance->decoder.decode_data >> 28) & 0x0F));
}

/**
 * Analysis of received data
 * @param instance Pointer to a SubGhzBlockGeneric* instance
 */
static void subghz_protocol_infactory_remote_controller(SubGhzBlockGeneric* instance) {
    instance->id = instance->data >> 32;
    instance->battery_low = (instance->data >> 26) & 1;
    instance->btn = WS_NO_BTN;
    instance->temp =
        locale_fahrenheit_to_celsius(((float)((instance->data >> 12) & 0x0FFF) - 900.0f) / 10.0f);
    instance->humidity =
        (((instance->data >> 8) & 0x0F) * 10) + ((instance->data >> 4) & 0x0F); // BCD, 'A0'=100%rH
    instance->channel = instance->data & 0x03;
}

void subghz_protocol_decoder_infactory_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    subghz_protocol_decoder_infactory* instance = context;

    switch(instance->decoder.parser_step) {
    case InfactoryDecoderStepReset:
        if((level) && (DURATION_DIFF(duration, subghz_protocol_infactory_const.te_short * 2) <
                       subghz_protocol_infactory_const.te_delta * 2)) {
            instance->decoder.parser_step = InfactoryDecoderStepCheckPreambule;
            instance->decoder.te_last = duration;
            instance->header_count = 0;
        }
        break;

    case InfactoryDecoderStepCheckPreambule:
        if(level) {
            instance->decoder.te_last = duration;
        } else {
            if((DURATION_DIFF(instance->decoder.te_last, subghz_protocol_infactory_const.te_short * 2) <
                subghz_protocol_infactory_const.te_delta * 2) &&
               (DURATION_DIFF(duration, subghz_protocol_infactory_const.te_short * 2) <
                subghz_protocol_infactory_const.te_delta * 2)) {
                //Found preambule
                instance->header_count++;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_infactory_const.te_short) <
                 subghz_protocol_infactory_const.te_delta) &&
                (DURATION_DIFF(duration, subghz_protocol_infactory_const.te_short * 16) <
                 subghz_protocol_infactory_const.te_delta * 8)) {
                //Found syncPrefix
                if(instance->header_count > 3) {
                    instance->decoder.parser_step = InfactoryDecoderStepSaveDuration;
                    instance->decoder.decode_data = 0;
                    instance->decoder.decode_count_bit = 0;
                }
            } else {
                instance->decoder.parser_step = InfactoryDecoderStepReset;
            }
        }
        break;

    case InfactoryDecoderStepSaveDuration:
        if(level) {
            instance->decoder.te_last = duration;
            instance->decoder.parser_step = InfactoryDecoderStepCheckDuration;
        } else {
            instance->decoder.parser_step = InfactoryDecoderStepReset;
        }
        break;

    case InfactoryDecoderStepCheckDuration:
        if(!level) {
            if(duration >= ((uint32_t)subghz_protocol_infactory_const.te_short * 30)) {
                //Found syncPostfix
                if((instance->decoder.decode_count_bit ==
                    subghz_protocol_infactory_const.min_count_bit_for_found) &&
                   subghz_protocol_infactory_check_crc(instance)) {
                    instance->generic.data = instance->decoder.decode_data;
                    instance->generic.data_count_bit = instance->decoder.decode_count_bit;
                    subghz_protocol_infactory_remote_controller(&instance->generic);
                    if(instance->base.callback)
                        instance->base.callback(&instance->base, instance->base.context);
                }
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                instance->decoder.parser_step = InfactoryDecoderStepReset;
                break;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_infactory_const.te_short) <
                 subghz_protocol_infactory_const.te_delta) &&
                (DURATION_DIFF(duration, subghz_protocol_infactory_const.te_long) <
                 subghz_protocol_infactory_const.te_delta * 2)) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 0);
                instance->decoder.parser_step = InfactoryDecoderStepSaveDuration;
            } else if(
                (DURATION_DIFF(instance->decoder.te_last, subghz_protocol_infactory_const.te_short) <
                 subghz_protocol_infactory_const.te_delta) &&
                (DURATION_DIFF(duration, subghz_protocol_infactory_const.te_long * 2) <
                 subghz_protocol_infactory_const.te_delta * 4)) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 1);
                instance->decoder.parser_step = InfactoryDecoderStepSaveDuration;
            } else {
                instance->decoder.parser_step = InfactoryDecoderStepReset;
            }
        } else {
            instance->decoder.parser_step = InfactoryDecoderStepReset;
        }
        break;
    }
}

void subghz_protocol_encoder_infactory_free(void* context) {
    furi_assert(context);
    subghz_protocol_encoder_infactory* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

uint8_t subghz_protocol_decoder_infactory_get_hash_data(void* context) {
    furi_assert(context);
    subghz_protocol_decoder_infactory* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_infactory_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    subghz_protocol_decoder_infactory* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

static bool subghz_protocol_encoder_infactory_get_upload(subghz_protocol_encoder_infactory* instance) {
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
                level_duration_make(true, (uint32_t)subghz_protocol_infactory_const.te_long);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_infactory_const.te_short);
        } else {
            //send bit 0
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_protocol_infactory_const.te_short);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_infactory_const.te_long);
        }
    }
    if(bit_read(instance->generic.data, 0)) {
        //send bit 1
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_infactory_const.te_long);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_infactory_const.te_short +
                subghz_protocol_infactory_const.te_long * 7);
    } else {
        //send bit 0
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_infactory_const.te_short);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_infactory_const.te_long +
                subghz_protocol_infactory_const.te_long * 7);
    }
    return true;
}

LevelDuration subghz_protocol_encoder_infactory_yield(void* context) {
    subghz_protocol_encoder_infactory* instance = context;

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

void subghz_protocol_encoder_infactory_stop(void* context) {
    subghz_protocol_encoder_infactory* instance = context;
    instance->encoder.is_running = false;
}

SubGhzProtocolStatus subghz_protocol_decoder_infactory_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_decoder_infactory* instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, subghz_protocol_infactory_const.min_count_bit_for_found);
}

SubGhzProtocolStatus subghz_protocol_encoder_infactory_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_encoder_infactory* instance = context;
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;
    do {
        ret = subghz_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            subghz_protocol_infactory_const.min_count_bit_for_found);
        if(ret != SubGhzProtocolStatusOk) {
            break;
        }
        //optional parameter parameter
        flipper_format_read_uint32(
            flipper_format, "Repeat", (uint32_t*)&instance->encoder.repeat, 1);

        if(!subghz_protocol_encoder_infactory_get_upload(instance)) {
            ret = SubGhzProtocolStatusErrorEncoderGetUpload;
            break;
        }
        instance->encoder.is_running = true;

    } while(false);

    return ret;
}

void subghz_protocol_decoder_infactory_get_string(void* context, FuriString* output) {
    furi_assert(context);
    subghz_protocol_decoder_infactory* instance = context;
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
