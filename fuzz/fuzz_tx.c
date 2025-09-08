#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "stellar/parser.h"
#include "stellar/formatter.h"

#define DETAIL_CAPTION_MAX_LENGTH 21
#define DETAIL_VALUE_MAX_LENGTH   105

static bool plugin_check_presence(const uint8_t *contract_address);
static stellar_plugin_result_t plugin_init_contract(const uint8_t *contract_address);
static stellar_plugin_result_t plugin_query_data_pair_count(const uint8_t *contract_address,
                                                            uint8_t *data_pair_count);
static stellar_plugin_result_t plugin_query_data_pair(const uint8_t *contract_address,
                                                      uint8_t data_pair_index,
                                                      char *caption,
                                                      uint8_t caption_len,
                                                      char *value,
                                                      uint8_t value_len);

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    envelope_t envelope;
    bool data_exists = true;
    bool is_op_header = false;
    char detail_caption[DETAIL_CAPTION_MAX_LENGTH];
    char detail_value[DETAIL_VALUE_MAX_LENGTH];
    uint8_t signing_key[] = {0xe9, 0x33, 0x88, 0xbb, 0xfd, 0x2f, 0xbd, 0x11, 0x80, 0x6d, 0xd0,
                             0xbd, 0x59, 0xce, 0xa9, 0x7,  0x9e, 0x7c, 0xc7, 0xc,  0xe7, 0xb1,
                             0xe1, 0x54, 0xf1, 0x14, 0xcd, 0xfe, 0x4e, 0x46, 0x6e, 0xcd};

    memset(&envelope, 0, sizeof(envelope_t));
    if (parse_transaction_envelope(data, size, &envelope)) {
        formatter_data_t tx_fdata = {
            .raw_data = data,
            .raw_data_len = size,
            .envelope = &envelope,
            .caption = detail_caption,
            .value = detail_value,
            .signing_key = signing_key,
            .caption_len = DETAIL_CAPTION_MAX_LENGTH,
            .value_len = DETAIL_VALUE_MAX_LENGTH,
            .display_sequence = true,
        };
        reset_formatter();

        while (true) {
            if (!get_next_data(&tx_fdata, true, &data_exists, &is_op_header)) {
                break;
            }

            if (!data_exists) {
                break;
            }
        }
    }

    memset(&envelope, 0, sizeof(envelope_t));
    if (parse_soroban_authorization_envelope(data, size, &envelope)) {
        formatter_data_t auth_fdata = {
            .raw_data = data,
            .raw_data_len = size,
            .envelope = &envelope,
            .caption = detail_caption,
            .value = detail_value,
            .signing_key = signing_key,
            .caption_len = DETAIL_CAPTION_MAX_LENGTH,
            .value_len = DETAIL_VALUE_MAX_LENGTH,
            .display_sequence = true,
            .plugin_check_presence = &plugin_check_presence,
            .plugin_init_contract = &plugin_init_contract,
            .plugin_query_data_pair_count = &plugin_query_data_pair_count,
            .plugin_query_data_pair = &plugin_query_data_pair,
        };

        reset_formatter();

        while (true) {
            if (!get_next_data(&auth_fdata, true, &data_exists, &is_op_header)) {
                break;
            }

            if (!data_exists) {
                break;
            }
        }
    }

    return 0;
}

static bool plugin_check_presence(const uint8_t *contract_address) {
    uint8_t expected[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return memcmp(contract_address, expected, 32) == 0;
}

stellar_plugin_result_t plugin_init_contract(const uint8_t *contract_address) {
    // Built-in token plugin
    if (plugin_check_presence(contract_address)) {
        return STELLAR_PLUGIN_RESULT_OK;
    }
    return STELLAR_PLUGIN_RESULT_UNAVAILABLE;
}

stellar_plugin_result_t plugin_query_data_pair_count(const uint8_t *contract_address,
                                                     uint8_t *data_pair_count) {
    // Built-in token plugin
    if (plugin_check_presence(contract_address)) {
        *data_pair_count = 3;
        return STELLAR_PLUGIN_RESULT_OK;
    }
    return STELLAR_PLUGIN_RESULT_UNAVAILABLE;
}

stellar_plugin_result_t plugin_query_data_pair(const uint8_t *contract_address,
                                               uint8_t data_pair_index,
                                               char *caption,
                                               uint8_t caption_len,
                                               char *value,
                                               uint8_t value_len) {
    if (!plugin_check_presence(contract_address)) {
        return STELLAR_PLUGIN_RESULT_UNAVAILABLE;
    }
    switch (data_pair_index) {
        case 0:
            strncpy(caption, "caption 0", caption_len);
            strncpy(value, "value 0", value_len);
            break;
        case 1:
            strncpy(caption, "caption 1", caption_len);
            strncpy(value, "value 1", value_len);
            break;
        case 2:
            strncpy(caption, "caption 2", caption_len);
            strncpy(value, "value 2", value_len);
            break;
        default:
            return STELLAR_PLUGIN_RESULT_ERROR;
    }
    return STELLAR_PLUGIN_RESULT_OK;
}
