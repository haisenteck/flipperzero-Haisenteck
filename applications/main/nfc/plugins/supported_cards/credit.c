#include "nfc_supported_card_plugin.h"
#include <flipper_application/flipper_application.h>
#include <nfc/nfc_device.h>
#include <nfc/helpers/nfc_util.h>
#include <nfc/protocols/mf_classic/mf_classic_poller_sync.h>
#include <stdint.h>

#define TAG "CREDIT"

#define KEY_LENGTH 6
#define UID_LENGTH 4

#define MONEY_BLOCK_ID 0x3A
#define MONEY_BLOCK_SECTOR 14
#define KEY_BLOCK_ID 0x3B
#define KEY_BLOCK_SECTOR 14

typedef uint8_t cardpass[KEY_LENGTH];
typedef uint8_t carduid[UID_LENGTH];

#define DEBUGF(tag, format, ...) FURI_LOG_D(tag, format, ##__VA_ARGS__)
#define INFOF(tag, format, ...) FURI_LOG_I(tag, format, ##__VA_ARGS__)
#define WARNF(tag, format, ...) FURI_LOG_W(tag, format, ##__VA_ARGS__)

uint8_t KeyAMidMap[5][16] = {
    {0x2, 0x0, 0x0, 0x2, 0x2, 0x8, 0x8, 0xA, 0xA, 0x8, 0x8, 0xA, 0xA, 0x0, 0x0, 0x2},
    {0x2, 0x1, 0x0, 0x7, 0x6, 0x5, 0x4, 0xB, 0xA, 0x9, 0x8, 0xF, 0xE, 0xD, 0xC, 0x3},
    {0x3, 0x0, 0x1, 0x6, 0x7, 0x4, 0x5, 0xA, 0xB, 0x8, 0x9, 0xE, 0xF, 0xC, 0xD, 0x2},
    {0x1, 0x2, 0x3, 0xC, 0xD, 0xE, 0xF, 0x8, 0x9, 0xA, 0xB, 0x4, 0x5, 0x6, 0x7, 0x0},
    {0xF, 0xE, 0x9, 0x8, 0xB, 0xA, 0x5, 0x4, 0x7, 0x6, 0x1, 0x0, 0x3, 0x2, 0xD, 0xC},
};

void GenKeyA(const carduid uid, cardpass pwd)
{
    pwd[0] = (uint16_t)uid[0] + 0xFB;
    pwd[1] = (uint16_t)uid[1] + 0xF8;
    // pwd[2] = ((0xE ^ ((uid[2] & 0x10) >> 4)) << 4) + (uid[2] & 0x8) ^ 0x7;
    pwd[2] = (((0xE ^ ((uid[2] & 0x10) >> 4)) << 4) + (uid[2] & 0x8)) ^ 0x7;
    pwd[3] = (KeyAMidMap[0][uid[3] >> 4] << 4) + KeyAMidMap[1][uid[3] & 0x0F];
    pwd[4] = (KeyAMidMap[2][uid[0] >> 4] << 4) + KeyAMidMap[3][uid[0] & 0x0F];
    pwd[5] = (KeyAMidMap[4][uid[3] >> 4] << 4) + (uid[3] & 0x5);

    return;
}

cardpass DEFALUT_KEYB = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct
{
    uint64_t a;
    uint64_t b;
} MfClassicKeyPair;

static MfClassicKeyPair credit_1k_keys[] = {
    {.a = 0xffffffffffff, .b = 0xffffffffffff}, // 000
    {.a = 0x000000000000, .b = 0xffffffffffff}, // 001
    {.a = 0xffffffffffff, .b = 0xffffffffffff}, // 002
    {.a = 0xffffffffffff, .b = 0xffffffffffff}, // 003
    {.a = 0xffffffffffff, .b = 0xffffffffffff}, // 004
    {.a = 0xffffffffffff, .b = 0xffffffffffff}, // 005
    {.a = 0xffffffffffff, .b = 0xffffffffffff}, // 006
    {.a = 0xffffffffffff, .b = 0xffffffffffff}, // 007
    {.a = 0xffffffffffff, .b = 0xffffffffffff}, // 008
    {.a = 0xffffffffffff, .b = 0xffffffffffff}, // 009
    {.a = 0xffffffffffff, .b = 0xffffffffffff}, // 010
    {.a = 0xffffffffffff, .b = 0xffffffffffff}, // 011
    {.a = 0xffffffffffff, .b = 0xffffffffffff}, // 012
    {.a = 0x000000000000, .b = 0xffffffffffff}, // 013
    {.a = 0xffffffffffff, .b = 0xffffffffffff}, // 014
    {.a = 0xffffffffffff, .b = 0xffffffffffff}, // 015
};

void debug_print_read_block(const MfClassicData *data, uint8_t block_num)
{
    MfClassicBlock raw = data->block[block_num];
    DEBUGF(
        TAG,
        "Block %d: %02X%02X %02X%02X %02X%02X %02X%02X",
        block_num,
        raw.data[0],
        raw.data[1],
        raw.data[2],
        raw.data[3],
        raw.data[4],
        raw.data[5],
        raw.data[6],
        raw.data[7]);
    DEBUGF(
        TAG,
        "Block %d: %02X%02X %02X%02X %02X%02X %02X%02X",
        block_num,
        raw.data[8],
        raw.data[9],
        raw.data[10],
        raw.data[11],
        raw.data[12],
        raw.data[13],
        raw.data[14],
        raw.data[15]);
}

MfClassicDeviceKeys refactor_MfClassicKeyPair_to_MfClassicDeviceKeys(MfClassicKeyPair *keypair, uint8_t sectors)
{
    MfClassicDeviceKeys keys = {};
    for (size_t i = 0; i < sectors; i++)
    {
        nfc_util_num2bytes(keypair[i].a, sizeof(MfClassicKey), keys.key_a[i].data);
        FURI_BIT_SET(keys.key_a_mask, i);
        nfc_util_num2bytes(keypair[i].b, sizeof(MfClassicKey), keys.key_b[i].data);
        FURI_BIT_SET(keys.key_b_mask, i);
    }
    return keys;
}

#ifdef TAG
#undef TAG
#define TAG "CREDIT READ"
static bool credit_read(Nfc *nfc, NfcDevice *device)
{
    DEBUGF(TAG, "Entering credit KDF Read");
    furi_assert(nfc);
    furi_assert(device);

    bool is_read = false;
    MfClassicData *data = mf_classic_alloc();
    nfc_device_copy_data(device, NfcProtocolMfClassic, data);

    do
    {
        MfClassicType type = MfClassicType1k;
        MfClassicError error = mf_classic_poller_sync_detect_type(nfc, &type);
        // Error protocol
        if (error != MfClassicErrorNone)
        {
            DEBUGF(TAG, "Failed to detect type, MfClassicError ID: %d", error);
            is_read = false;
            break;
        }
        data->type = type;

        size_t uid_len;
        const uint8_t *uid = mf_classic_get_uid(data, &uid_len);

        DEBUGF(TAG, "UID len: %d", uid_len);
        if (uid_len != UID_LENGTH)
        {
            INFOF(TAG, "UID length is not 4 bytes");
            is_read = false;
            break;
        };
        DEBUGF(TAG, "UID identified: %02X%02X%02X%02X", uid[0], uid[1], uid[2], uid[3]);

        cardpass key;
        GenKeyA(uid, key);
        uint64_t num_key = nfc_util_bytes2num(key, KEY_LENGTH);
        DEBUGF(TAG, "Key generated for UID: %012llX", num_key);

        uint8_t sectors = mf_classic_get_total_sectors_num(data->type);
        if (sectors < MONEY_BLOCK_SECTOR)
        {
            INFOF(TAG, "Card is not hot water card, the total sectors is %d", sectors);
            is_read = false;
            break;
        }
        credit_1k_keys[MONEY_BLOCK_SECTOR].a = num_key;

        MfClassicDeviceKeys keys = refactor_MfClassicKeyPair_to_MfClassicDeviceKeys(credit_1k_keys, sectors);

        error = mf_classic_poller_sync_read(nfc, &keys, data);
        if (error != MfClassicErrorNone)
        {
            WARNF(TAG, "Failed to verify data");
            is_read = false;
            break;
        }

        nfc_device_set_data(device, NfcProtocolMfClassic, data); // Send data to device
        is_read = true;
    } while (false);
    mf_classic_free(data);
    DEBUGF(TAG, "Exiting credit KDF Read, is_read: %s", is_read ? "true" : "false");
    return is_read;
}
#endif

#ifdef TAG
#undef TAG
#define TAG "CREDIT PARSE"
static bool credit_parse(const NfcDevice *device, FuriString *parsed_data)
{
    DEBUGF(TAG, "Entering credit KDF Parse");
    furi_assert(device);
    furi_assert(parsed_data);

    bool parsed = false;
    const MfClassicData *data = nfc_device_get_data(device, NfcProtocolMfClassic);

    do
    {
        // UID data
        size_t uid_len;
        const uint8_t *uid = mf_classic_get_uid(data, &uid_len);
        // uint64_t _uid = nfc_util_bytes2num(uid, uid_len);

        // Card Pass
        cardpass keyx;
        GenKeyA(uid, keyx);
        uint64_t _key = nfc_util_bytes2num(keyx, KEY_LENGTH);
        DEBUGF(TAG, "UID len: %d", uid_len);
        DEBUGF(TAG, "UID identified: %02X%02X%02X%02X", uid[0], uid[1], uid[2], uid[3]);
        DEBUGF(TAG, "Key generated for UID: %012llX", _key);

        // Read block of key
        debug_print_read_block(data, (uint8_t)KEY_BLOCK_ID);
        MfClassicBlock block_3b = data->block[KEY_BLOCK_ID];
        uint64_t block_3b_num = nfc_util_bytes2num(block_3b.data, KEY_LENGTH);
        if (block_3b_num != _key) // if KeyA is not match
        {
            INFOF(
                TAG,
                "KeyA not match. KeyA in Card is %012llX, but generated is %012llx",
                block_3b_num,
                _key);
            break;
        }

        // Read block of money
        MfClassicBlock money_raw = data->block[MONEY_BLOCK_ID];
        uint64_t money = nfc_util_bytes2num_little_endian(money_raw.data, 2);
        DEBUGF(TAG, "Money: %012llX", money);
        DEBUGF(TAG, "Size: uint64_t: %d, double: %d", sizeof(uint64_t), sizeof(double));
        double final_money = money / 100.00;

        // parse data
        furi_string_cat_printf(parsed_data, "\e#Credit Card\n");
        furi_string_cat_printf(parsed_data, "UID: %02X%02X%02X%02X\n", uid[0], uid[1], uid[2], uid[3]);
        furi_string_cat_printf(parsed_data, "S14 KeyA: %012llX\n", _key);
        furi_string_cat_printf(
            parsed_data, "Def KeyB: %012llX\n", nfc_util_bytes2num(DEFALUT_KEYB, KEY_LENGTH));
        furi_string_cat_printf(parsed_data, "Fin Cred:  $%.03lf\n", final_money);

        parsed = true;
    } while (false);

    DEBUGF(TAG, "Exiting credit KDF Parse, parsed: %s", parsed ? "true" : "false");
    return parsed;
}
#endif

#ifdef TAG
#undef TAG
#define TAG "CREDIT VERIFY"
static bool credit_verify(Nfc *nfc)
{
    DEBUGF(TAG, "Entering credit KDF Verify");
    furi_assert(nfc);
    bool is_verified = false;

    do
    {
        // Allocate memory for key
        MfClassicKey key = {};

        // Read Block 1 for UID
        uint8_t base = mf_classic_get_first_block_num_of_sector(0);
        DEBUGF(TAG, "Base block: %d", base);
        nfc_util_num2bytes(credit_1k_keys[0].a, KEY_LENGTH, key.data);
        MfClassicKeyType type = MfClassicKeyTypeA;
        MfClassicBlock data_block = {};
        MfClassicError error = mf_classic_poller_sync_read_block(nfc, base, &key, type, &data_block);
        if (error != MfClassicErrorNone)
        {
            INFOF(TAG, "Failed to read block %d", base);
            break;
        }

        carduid uid = {data_block.data[0], data_block.data[1], data_block.data[2], data_block.data[3]};
        DEBUGF(TAG, "UID: %02X%02X%02X%02X", uid[0], uid[1], uid[2], uid[3]);

        // Generate KeyA 
        cardpass keya;
        GenKeyA(uid, keya);
        DEBUGF(
            TAG, "KeyA: %02X%02X%02X%02X%02X%02X", keya[0], keya[1], keya[2], keya[3], keya[4], keya[5]);
        uint64_t numx = nfc_util_bytes2num(keya, KEY_LENGTH);
        nfc_util_num2bytes(numx, KEY_LENGTH, key.data);

        error = mf_classic_poller_sync_read_block(nfc, MONEY_BLOCK_ID, &key, type, &data_block);
        if (error != MfClassicErrorNone)
        {
            INFOF(TAG, "Failed to read block %d", MONEY_BLOCK_ID);
            break;
        }
        is_verified = true;
    } while (false);
    return is_verified;
}
#endif

/* Actual implementation of app<>plugin interface */
static const NfcSupportedCardsPlugin credit_plugin = {
    // .protocol = NfcProtocolIso14443_4a,
    .protocol = NfcProtocolMfClassic,
    .verify = credit_verify,
    .read = credit_read,
    // KDF mode
    .parse = credit_parse,
};

/* Plugin descriptor to comply with basic plugin specification */
static const FlipperAppPluginDescriptor credit_plugin_descriptor = {
    .appid = NFC_SUPPORTED_CARD_PLUGIN_APP_ID,
    .ep_api_version = NFC_SUPPORTED_CARD_PLUGIN_API_VERSION,
    .entry_point = &credit_plugin,
};

/* Plugin entry point - must return a pointer to const descriptor  */
const FlipperAppPluginDescriptor *credit_plugin_ep()
{
    return &credit_plugin_descriptor;
}