#include "ambient_weather.h"
#include <lib/toolbox/manchester_decoder.h>

#define TAG "subghz_protocol_ambient_weather"

// PORTING E PICCOLE MODIFICHE A CURA DI HAISENTECK
/*
 * Help
 * https://github.com/merbanan/rtl_433/blob/master/src/devices/ambient_weather.c
 * 
 * Decode Ambient Weather F007TH, F012TH, TF 30.3208.02, SwitchDoc F016TH.
 * Devices supported:
 * - Ambient Weather F007TH Thermo-Hygrometer.
 * - Ambient Weather F012TH Indoor/Display Thermo-Hygrometer.
 * - TFA senders 30.3208.02 from the TFA "Klima-Monitor" 30.3054,
 * - SwitchDoc Labs F016TH.
 * This decoder handles the 433mhz/868mhz thermo-hygrometers.
 * The 915mhz (WH*) family of devices use different modulation/encoding.
 * Byte 0   Byte 1   Byte 2   Byte 3   Byte 4   Byte 5
 * xxxxMMMM IIIIIIII BCCCTTTT TTTTTTTT HHHHHHHH MMMMMMMM
 * - x: Unknown 0x04 on F007TH/F012TH
 * - M: Model Number?, 0x05 on F007TH/F012TH/SwitchDocLabs F016TH
 * - I: ID byte (8 bits), volatie, changes at power up,
 * - B: Battery Low
 * - C: Channel (3 bits 1-8) - F007TH set by Dip switch, F012TH soft setting
 * - T: Temperature 12 bits - Fahrenheit * 10 + 400
 * - H: Humidity (8 bits)
 * - M: Message integrity check LFSR Digest-8, gen 0x98, key 0x3e, init 0x64
 * 
 * three repeats without gap
 * full preamble is 0x00145 (the last bits might not be fixed, e.g. 0x00146)
 * and on decoding also 0xffd45
 */

#define AMBIENT_WEATHER_PACKET_HEADER_1 0xFFD440000000000 //0xffd45 .. 0xffd46
#define AMBIENT_WEATHER_PACKET_HEADER_2 0x001440000000000 //0x00145 .. 0x00146
#define AMBIENT_WEATHER_PACKET_HEADER_MASK 0xFFFFC0000000000

static const SubGhzBlockConst subghz_protocol_ambient_weather_const = {
    .te_short = 500,
    .te_long = 1000,
    .te_delta = 120,
    .min_count_bit_for_found = 48,
};

struct subghz_protocol_DecoderAmbient_Weather {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
    ManchesterState manchester_saved_state;
    uint16_t header_count;
};

struct subghz_protocol_Encoder_Ambient_Weather {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

const SubGhzProtocolDecoder subghz_protocol_ambient_weather_decoder = {
    .alloc = subghz_protocol_decoder_ambient_weather_alloc,
    .free = subghz_protocol_decoder_ambient_weather_free,

    .feed = subghz_protocol_decoder_ambient_weather_feed,
    .reset = subghz_protocol_decoder_ambient_weather_reset,

    .get_hash_data = subghz_protocol_decoder_ambient_weather_get_hash_data,
    .serialize = subghz_protocol_decoder_ambient_weather_serialize,
    .deserialize = subghz_protocol_decoder_ambient_weather_deserialize,
    .get_string = subghz_protocol_decoder_ambient_weather_get_string,
};

const SubGhzProtocolEncoder subghz_protocol_ambient_weather_encoder = {
    .alloc = subghz_protocol_encoder_ambient_weather_alloc,
    .free = subghz_protocol_encoder_ambient_weather_free,

    .deserialize = subghz_protocol_encoder_ambient_weather_deserialize,
    .stop = subghz_protocol_encoder_ambient_weather_stop,
    .yield = subghz_protocol_encoder_ambient_weather_yield,
};

const SubGhzProtocol subghz_protocol_ambient_weather = {
    .name = subghz_protocol_AMBIENT_WEATHER_NAME,
    //.type = SubGhzProtocolWeatherStation,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_315 | SubGhzProtocolFlag_868 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,

    .decoder = &subghz_protocol_ambient_weather_decoder,
    .encoder = &subghz_protocol_ambient_weather_encoder,
};

void* subghz_protocol_encoder_ambient_weather_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_Encoder_Ambient_Weather* instance = malloc(sizeof(subghz_protocol_Encoder_Ambient_Weather));

    instance->base.protocol = &subghz_protocol_ambient_weather;
    instance->generic.protocol_name = instance->base.protocol->name;

    instance->encoder.repeat = 3;
    instance->encoder.size_upload = 48;
    instance->encoder.upload = malloc(instance->encoder.size_upload * sizeof(LevelDuration));
    instance->encoder.is_running = false;
    return instance;
}

void* subghz_protocol_decoder_ambient_weather_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    subghz_protocol_DecoderAmbient_Weather* instance = malloc(sizeof(subghz_protocol_DecoderAmbient_Weather));
    instance->base.protocol = &subghz_protocol_ambient_weather;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void subghz_protocol_decoder_ambient_weather_free(void* context) {
    furi_assert(context);
    subghz_protocol_DecoderAmbient_Weather* instance = context;
    free(instance);
}

void subghz_protocol_decoder_ambient_weather_reset(void* context) {
    furi_assert(context);
    subghz_protocol_DecoderAmbient_Weather* instance = context;
    manchester_advance(
        instance->manchester_saved_state,
        ManchesterEventReset,
        &instance->manchester_saved_state,
        NULL);
}

static bool subghz_protocol_ambient_weather_check_crc(subghz_protocol_DecoderAmbient_Weather* instance) {
    uint8_t msg[] = {
        instance->decoder.decode_data >> 40,
        instance->decoder.decode_data >> 32,
        instance->decoder.decode_data >> 24,
        instance->decoder.decode_data >> 16,
        instance->decoder.decode_data >> 8};

    uint8_t crc = subghz_protocol_blocks_lfsr_digest8(msg, 5, 0x98, 0x3e) ^ 0x64;
    return (crc == (uint8_t)(instance->decoder.decode_data & 0xFF));
}

/**
 * Analysis of received data
 * @param instance Pointer to a SubGhzBlockGeneric* instance
 */
static void subghz_protocol_ambient_weather_remote_controller(SubGhzBlockGeneric* instance) {
    instance->id = (instance->data >> 32) & 0xFF;
    instance->battery_low = (instance->data >> 31) & 1;
    instance->channel = ((instance->data >> 28) & 0x07) + 1;
    instance->temp =
        locale_fahrenheit_to_celsius(((float)((instance->data >> 16) & 0x0FFF) - 400.0f) / 10.0f);
    instance->humidity = (instance->data >> 8) & 0xFF;
    instance->btn = WS_NO_BTN;

    // ToDo maybe it won't be needed
    /*
    Sanity checks to reduce false positives and other bad data
    Packets with Bad data often pass the MIC check.
    - humidity > 100 (such as 255) and
    - temperatures > 140 F (such as 369.5 F and 348.8 F
    Specs in the F007TH and F012TH manuals state the range is:
    - Temperature: -40 to 140 F
    - Humidity: 10 to 99%
    @todo - sanity check b[0] "model number"
    - 0x45 - F007TH and F012TH
    - 0x?5 - SwitchDocLabs F016TH temperature sensor (based on comment b[0] & 0x0f == 5)
    - ? - TFA 30.3208.02
    if (instance->humidity < 0 || instance->humidity > 100) {
        ERROR;
    }

    if (instance->temp < -40.0 || instance->temp > 140.0) {
         ERROR;
    }
    */
}

void subghz_protocol_decoder_ambient_weather_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    subghz_protocol_DecoderAmbient_Weather* instance = context;

    ManchesterEvent event = ManchesterEventReset;
    if(!level) {
        if(DURATION_DIFF(duration, subghz_protocol_ambient_weather_const.te_short) <
           subghz_protocol_ambient_weather_const.te_delta) {
            event = ManchesterEventShortLow;
        } else if(
            DURATION_DIFF(duration, subghz_protocol_ambient_weather_const.te_long) <
            subghz_protocol_ambient_weather_const.te_delta * 2) {
            event = ManchesterEventLongLow;
        }
    } else {
        if(DURATION_DIFF(duration, subghz_protocol_ambient_weather_const.te_short) <
           subghz_protocol_ambient_weather_const.te_delta) {
            event = ManchesterEventShortHigh;
        } else if(
            DURATION_DIFF(duration, subghz_protocol_ambient_weather_const.te_long) <
            subghz_protocol_ambient_weather_const.te_delta * 2) {
            event = ManchesterEventLongHigh;
        }
    }
    if(event != ManchesterEventReset) {
        bool data;
        bool data_ok = manchester_advance(
            instance->manchester_saved_state, event, &instance->manchester_saved_state, &data);

        if(data_ok) {
            instance->decoder.decode_data = (instance->decoder.decode_data << 1) | !data;
        }

        if(((instance->decoder.decode_data & AMBIENT_WEATHER_PACKET_HEADER_MASK) ==
            AMBIENT_WEATHER_PACKET_HEADER_1) ||
           ((instance->decoder.decode_data & AMBIENT_WEATHER_PACKET_HEADER_MASK) ==
            AMBIENT_WEATHER_PACKET_HEADER_2)) {
            if(subghz_protocol_ambient_weather_check_crc(instance)) {
                instance->generic.data = instance->decoder.decode_data;
                instance->generic.data_count_bit =
                    subghz_protocol_ambient_weather_const.min_count_bit_for_found;
                subghz_protocol_ambient_weather_remote_controller(&instance->generic);
                if(instance->base.callback)
                    instance->base.callback(&instance->base, instance->base.context);
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
            }
        }
    } else {
        instance->decoder.decode_data = 0;
        instance->decoder.decode_count_bit = 0;
        manchester_advance(
            instance->manchester_saved_state,
            ManchesterEventReset,
            &instance->manchester_saved_state,
            NULL);
    }
}

void subghz_protocol_encoder_ambient_weather_free(void* context) {
    furi_assert(context);
    subghz_protocol_Encoder_Ambient_Weather* instance = context;
    free(instance->encoder.upload);
    free(instance);
}

uint8_t subghz_protocol_decoder_ambient_weather_get_hash_data(void* context) {
    furi_assert(context);
    subghz_protocol_DecoderAmbient_Weather* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_ambient_weather_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    subghz_protocol_DecoderAmbient_Weather* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

static bool subghz_protocol_encoder_ambient_weather_get_upload(subghz_protocol_Encoder_Ambient_Weather* instance) {
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
                level_duration_make(true, (uint32_t)subghz_protocol_ambient_weather_const.te_long);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_ambient_weather_const.te_short);
        } else {
            //send bit 0
            instance->encoder.upload[index++] =
                level_duration_make(true, (uint32_t)subghz_protocol_ambient_weather_const.te_short);
            instance->encoder.upload[index++] =
                level_duration_make(false, (uint32_t)subghz_protocol_ambient_weather_const.te_long);
        }
    }
    if(bit_read(instance->generic.data, 0)) {
        //send bit 1
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_ambient_weather_const.te_long);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_ambient_weather_const.te_short +
                subghz_protocol_ambient_weather_const.te_long * 7);
    } else {
        //send bit 0
        instance->encoder.upload[index++] =
            level_duration_make(true, (uint32_t)subghz_protocol_ambient_weather_const.te_short);
        instance->encoder.upload[index++] = level_duration_make(
            false,
            (uint32_t)subghz_protocol_ambient_weather_const.te_long +
                subghz_protocol_ambient_weather_const.te_long * 7);
    }
    return true;
}

LevelDuration subghz_protocol_encoder_ambient_weather_yield(void* context) {
    subghz_protocol_Encoder_Ambient_Weather* instance = context;

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

void subghz_protocol_encoder_ambient_weather_stop(void* context) {
    subghz_protocol_Encoder_Ambient_Weather* instance = context;
    instance->encoder.is_running = false;
}

SubGhzProtocolStatus subghz_protocol_decoder_ambient_weather_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_DecoderAmbient_Weather* instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic,
        flipper_format,
        subghz_protocol_ambient_weather_const.min_count_bit_for_found);
}


SubGhzProtocolStatus subghz_protocol_encoder_ambient_weather_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    subghz_protocol_Encoder_Ambient_Weather* instance = context;
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;
    do {
        ret = subghz_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            subghz_protocol_ambient_weather_const.min_count_bit_for_found);
        if(ret != SubGhzProtocolStatusOk) {
            break;
        }
        //optional parameter parameter
        flipper_format_read_uint32(
            flipper_format, "Repeat", (uint32_t*)&instance->encoder.repeat, 1);

        if(!subghz_protocol_encoder_ambient_weather_get_upload(instance)) {
            ret = SubGhzProtocolStatusErrorEncoderGetUpload;
            break;
        }
        instance->encoder.is_running = true;

    } while(false);

    return ret;
}

void subghz_protocol_decoder_ambient_weather_get_string(void* context, FuriString* output) {
    furi_assert(context);
    subghz_protocol_DecoderAmbient_Weather* instance = context;
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
