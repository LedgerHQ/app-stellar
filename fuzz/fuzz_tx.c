#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "stellar/parser.h"
#include "stellar/formatter.h"

#define DETAIL_CAPTION_MAX_LENGTH 21
#define DETAIL_VALUE_MAX_LENGTH   105

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
