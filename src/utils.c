/*****************************************************************************
 *   Ledger Stellar App.
 *   (c) 2022 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <bolos_target.h>

#include "./utils.h"
#include "./common/base32.h"
#include "./common/base58.h"
#include "./common/format.h"

#define MUXED_ACCOUNT_MED_25519_SIZE  43
#define BINARY_MAX_SIZE               36
#define AMOUNT_WITH_COMMAS_MAX_LENGTH 24  // 922,337,203,685.4775807

static const char BASE64_ALPHABET[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int BASE64_MOD_TABLE[] = {0, 2, 1};

bool base64_encode(const uint8_t *data, size_t in_len, char *out, size_t out_len) {
    size_t encoded_len = 4 * ((in_len + 2) / 3);
    if (encoded_len > out_len) {
        return false;
    }

    for (unsigned int i = 0, j = 0; i < in_len;) {
        uint32_t octet_a = i < in_len ? data[i++] : 0;
        uint32_t octet_b = i < in_len ? data[i++] : 0;
        uint32_t octet_c = i < in_len ? data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        out[j++] = BASE64_ALPHABET[(triple >> 3 * 6) & 0x3F];
        out[j++] = BASE64_ALPHABET[(triple >> 2 * 6) & 0x3F];
        out[j++] = BASE64_ALPHABET[(triple >> 1 * 6) & 0x3F];
        out[j++] = BASE64_ALPHABET[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < BASE64_MOD_TABLE[in_len % 3]; i++) {
        out[encoded_len - 1 - i] = '=';
    }

    out[encoded_len] = '\0';
    return true;
}

uint16_t crc16(const uint8_t *input_str, int num_bytes) {
    uint16_t crc;
    crc = 0;
    while (--num_bytes >= 0) {
        crc = crc ^ (uint32_t) *input_str++ << 8;
        int i = 8;
        do {
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        } while (--i);
    }
    return crc;
}

bool encode_key(const uint8_t *in, uint8_t version_byte, char *out, uint8_t out_len) {
    if (out_len < 56 + 1) {
        return false;
    }
    uint8_t buffer[35];
    buffer[0] = version_byte;
    for (uint8_t i = 0; i < 32; i++) {
        buffer[i + 1] = in[i];
    }
    uint16_t crc = crc16(buffer, 33);  // checksum
    buffer[33] = crc;
    buffer[34] = crc >> 8;
    if (base32_encode(buffer, 35, (uint8_t *) out, 56) == -1) {
        return false;
    }
    out[56] = '\0';
    return true;
}

bool encode_ed25519_public_key(const uint8_t raw_public_key[static RAW_ED25519_PUBLIC_KEY_SIZE],
                               char *out,
                               size_t out_len) {
    return encode_key(raw_public_key, VERSION_BYTE_ED25519_PUBLIC_KEY, out, out_len);
}

bool encode_hash_x_key(const uint8_t raw_hash_x[static RAW_HASH_X_KEY_SIZE],
                       char *out,
                       size_t out_len) {
    return encode_key(raw_hash_x, VERSION_BYTE_HASH_X, out, out_len);
}

bool encode_pre_auth_x_key(const uint8_t raw_pre_auth_tx[static RAW_PRE_AUTH_TX_KEY_SIZE],
                           char *out,
                           size_t out_len) {
    return encode_key(raw_pre_auth_tx, VERSION_BYTE_PRE_AUTH_TX_KEY, out, out_len);
}

bool encode_contract(const uint8_t raw_contract[static RAW_CONTRACT_KEY_SIZE],
                     char *out,
                     size_t out_len) {
    return encode_key(raw_contract, VERSION_BYTE_CONTRACT, out, out_len);
}

bool encode_ed25519_signed_payload(const ed25519_signed_payload_t *signed_payload,
                                   char *out,
                                   size_t out_len) {
    if (out_len < 166) {  // (103 * 8 + 4) / 5
        return false;
    }
    if (signed_payload->payload_len > 64 || signed_payload->payload_len <= 0) {
        return false;
    }
    uint8_t data_len = RAW_ED25519_PUBLIC_KEY_SIZE + 4 + signed_payload->payload_len +
                       ((4 - signed_payload->payload_len % 4) % 4);
    uint8_t buffer[1 + 32 + 4 + 64 + 2] = {0};
    buffer[0] = VERSION_BYTE_ED25519_SIGNED_PAYLOAD;
    for (uint8_t i = 0; i < 32; i++) {
        buffer[i + 1] = signed_payload->ed25519[i];
    }
    buffer[36] = signed_payload->payload_len;
    for (uint8_t i = 0; i < signed_payload->payload_len; i++) {
        buffer[i + 37] = signed_payload->payload[i];
    }
    uint16_t crc = crc16(buffer, data_len + 1);  // checksum
    buffer[1 + data_len] = crc;
    buffer[1 + data_len + 1] = crc >> 8;
    int ret = base32_encode(buffer, data_len + 3, (uint8_t *) out, out_len);
    if (ret == -1) {
        return false;
    }
    out[ret] = '\0';
    return true;
}

bool encode_muxed_account(const muxed_account_t *raw_muxed_account, char *out, size_t out_len) {
    if (raw_muxed_account->type == KEY_TYPE_ED25519) {
        return encode_ed25519_public_key(raw_muxed_account->ed25519, out, out_len);
    } else {
        if (out_len < ENCODED_MUXED_ACCOUNT_KEY_LENGTH) {
            return false;
        }
        uint8_t buffer[MUXED_ACCOUNT_MED_25519_SIZE];
        buffer[0] = VERSION_BYTE_MUXED_ACCOUNT;
        memcpy(buffer + 1, raw_muxed_account->med25519.ed25519, RAW_ED25519_PUBLIC_KEY_SIZE);
        for (int i = 0; i < 8; i++) {
            buffer[33 + i] = raw_muxed_account->med25519.id >> 8 * (7 - i);
        }
        uint16_t crc = crc16(buffer, MUXED_ACCOUNT_MED_25519_SIZE - 2);  // checksum
        buffer[41] = crc;
        buffer[42] = crc >> 8;
        if (base32_encode(buffer,
                          MUXED_ACCOUNT_MED_25519_SIZE,
                          (uint8_t *) out,
                          ENCODED_MUXED_ACCOUNT_KEY_LENGTH) == -1) {
            return false;
        }
        out[69] = '\0';
        return true;
    }
}

bool print_summary(const char *in,
                   char *out,
                   size_t out_len,
                   uint8_t num_chars_l,
                   uint8_t num_chars_r) {
    uint8_t result_len = num_chars_l + num_chars_r + 2;
    if (out_len < result_len + 1) {
        return false;
    }
    uint16_t in_len = strlen(in);
    if (in_len > result_len) {
        memcpy(out, in, num_chars_l);
        out[num_chars_l] = '.';
        out[num_chars_l + 1] = '.';
        memcpy(out + num_chars_l + 2, in + in_len - num_chars_r, num_chars_r);
        out[result_len] = '\0';
    } else {
        memcpy(out, in, in_len);
        out[in_len] = '\0';
    }
    return true;
}

bool print_binary(const uint8_t *in,
                  size_t in_len,
                  char *out,
                  size_t out_len,
                  uint8_t num_chars_l,
                  uint8_t num_chars_r) {
    if (num_chars_l > 0) {
        char buffer[BINARY_MAX_SIZE * 2 + 1];  // FIXME
        if (!format_hex(in, in_len, buffer, sizeof(buffer))) {
            return false;
        }
        return print_summary(buffer, out, out_len, num_chars_l, num_chars_r);
    }
    return format_hex(in, in_len, out, out_len);
}

bool print_account_id(const account_id_t account_id,
                      char *out,
                      size_t out_len,
                      uint8_t num_chars_l,
                      uint8_t num_chars_r) {
    if (num_chars_l > 0) {
        char buffer[ENCODED_ED25519_PUBLIC_KEY_LENGTH];
        if (!encode_ed25519_public_key(account_id, buffer, sizeof(buffer))) {
            return false;
        }
        return print_summary(buffer, out, out_len, num_chars_l, num_chars_r);
    }
    return encode_ed25519_public_key(account_id, out, out_len);
}

bool print_contract_id(const uint8_t *contract_id,
                       char *out,
                       size_t out_len,
                       uint8_t num_chars_l,
                       uint8_t num_chars_r) {
    if (num_chars_l > 0) {
        char buffer[ENCODED_CONTRACT_KEY_LENGTH];
        if (!encode_contract(contract_id, buffer, sizeof(buffer))) {
            return false;
        }
        return print_summary(buffer, out, out_len, num_chars_l, num_chars_r);
    }
    return encode_contract(contract_id, out, out_len);
}

bool print_hash_x_key(const uint8_t raw_hash_x[static RAW_HASH_X_KEY_SIZE],
                      char *out,
                      size_t out_len,
                      uint8_t num_chars_l,
                      uint8_t num_chars_r) {
    if (num_chars_l > 0) {
        char buffer[ENCODED_HASH_X_KEY_LENGTH];
        if (!encode_hash_x_key(raw_hash_x, buffer, sizeof(buffer))) {
            return false;
        }
        return print_summary(buffer, out, out_len, num_chars_l, num_chars_r);
    }
    return encode_hash_x_key(raw_hash_x, out, out_len);
}

bool print_pre_auth_x_key(const uint8_t raw_pre_auth_tx[static RAW_PRE_AUTH_TX_KEY_SIZE],
                          char *out,
                          size_t out_len,
                          uint8_t num_chars_l,
                          uint8_t num_chars_r) {
    if (num_chars_l > 0) {
        char buffer[ENCODED_PRE_AUTH_TX_KEY_LENGTH];
        if (!encode_pre_auth_x_key(raw_pre_auth_tx, buffer, sizeof(buffer))) {
            return false;
        }
        return print_summary(buffer, out, out_len, num_chars_l, num_chars_r);
    }
    return encode_pre_auth_x_key(raw_pre_auth_tx, out, out_len);
}

bool print_ed25519_signed_payload(const ed25519_signed_payload_t *signed_payload,
                                  char *out,
                                  size_t out_len,
                                  uint8_t num_chars_l,
                                  uint8_t num_chars_r) {
    if (num_chars_l + num_chars_r + 2 + 1 > out_len) {
        return false;
    }
    char tmp[166];
    if (!encode_ed25519_signed_payload(signed_payload, tmp, sizeof(tmp))) {
        return false;
    };
    return print_summary(tmp, out, out_len, num_chars_l, num_chars_r);
}

bool print_sc_address(const sc_address_t *sc_address,
                      char *out,
                      size_t out_len,
                      uint8_t num_chars_l,
                      uint8_t num_chars_r) {
    if (sc_address->type == SC_ADDRESS_TYPE_ACCOUNT) {
        return print_account_id(sc_address->address, out, out_len, num_chars_l, num_chars_r);
    } else {
        return print_contract_id(sc_address->address, out, out_len, num_chars_l, num_chars_r);
    }
    return true;
}

bool print_muxed_account(const muxed_account_t *muxed_account,
                         char *out,
                         size_t out_len,
                         uint8_t num_chars_l,
                         uint8_t num_chars_r) {
    if (num_chars_l > 0) {
        char buffer[ENCODED_MUXED_ACCOUNT_KEY_LENGTH];
        if (!encode_muxed_account(muxed_account, buffer, sizeof(buffer))) {
            return false;
        }
        return print_summary(buffer, out, out_len, num_chars_l, num_chars_r);
    }
    return encode_muxed_account(muxed_account, out, out_len);
}

bool print_claimable_balance_id(const claimable_balance_id_t *claimable_balance_id_t,
                                char *out,
                                size_t out_len,
                                uint8_t num_chars_l,
                                uint8_t num_chars_r) {
    if (out_len < 36 * 2 + 1) {
        return false;
    }
    uint8_t data[36];
    // enum is 1 byte
    data[0] = '\0';
    data[1] = '\0';
    data[2] = '\0';
    data[3] = claimable_balance_id_t->type;
    memcpy(data + 4, claimable_balance_id_t->v0, 32);
    return print_binary(data, 36, out, out_len, num_chars_l, num_chars_r);
}

bool print_uint(uint64_t num, char *out, size_t out_len) {
    char buffer[AMOUNT_MAX_LENGTH];
    uint64_t d_val = num;
    size_t i, j;

    if (num == 0) {
        if (out_len < 2) {
            return false;
        }
        if (strlcpy(out, "0", out_len) >= out_len) {
            return false;
        }
        return true;
    }

    memset(buffer, 0, AMOUNT_MAX_LENGTH);
    for (i = 0; d_val > 0; i++) {
        if (i >= AMOUNT_MAX_LENGTH) {
            return false;
        }
        buffer[i] = (d_val % 10) + '0';
        d_val /= 10;
    }
    if (out_len <= i) {
        return false;
    }
    // reverse order
    for (j = 0; j < i; j++) {
        out[j] = buffer[i - j - 1];
    }
    out[i] = '\0';
    return true;
}

bool print_int(int64_t num, char *out, size_t out_len) {
    if (out_len == 0) {
        return false;
    }
    if (num < 0) {
        uint64_t n;

        out[0] = '-';
        if (num == INT64_MIN) {
            n = (uint64_t) num;
        } else {
            n = -num;
        }
        return print_uint(n, out + 1, out_len - 1);
    }
    return print_uint(num, out, out_len);
}

bool print_time(uint64_t seconds, char *out, size_t out_len) {
    if (seconds > 253402300799) {
        // valid range 1970-01-01 00:00:00 - 9999-12-31 23:59:59
        return false;
    }
    char time_str[20] = {0};  // 1970-01-01 00:00:00

    if (out_len < sizeof(time_str)) {
        return false;
    }
    struct tm tm;
    if (!gmtime_r((time_t *) &seconds, &tm)) {
        return false;
    };

    if (snprintf(time_str,
                 sizeof(time_str),
                 "%04d-%02d-%02d %02d:%02d:%02d",
                 tm.tm_year + 1900,
                 tm.tm_mon + 1,
                 tm.tm_mday,
                 tm.tm_hour,
                 tm.tm_min,
                 tm.tm_sec) < 0) {
        return false;
    };
    if (strlcpy(out, time_str, out_len) >= out_len) {
        return false;
    }
    return true;
}

bool print_asset_name(const asset_t *asset, uint8_t network_id, char *out, size_t out_len) {
    switch (asset->type) {
        case ASSET_TYPE_NATIVE:
            if (network_id == NETWORK_TYPE_UNKNOWN) {
                if (strlcpy(out, "native", out_len) >= out_len) {
                    return false;
                }
            } else {
                if (strlcpy(out, "XLM", out_len) >= out_len) {
                    return false;
                }
            }
            return true;
        case ASSET_TYPE_CREDIT_ALPHANUM4:
            for (int i = 0; i < 4; i++) {
                out[i] = asset->alpha_num4.asset_code[i];
                if (out[i] == 0) {
                    break;
                }
            }
            out[4] = 0;
            return true;
        case ASSET_TYPE_CREDIT_ALPHANUM12:
            for (int i = 0; i < 12; i++) {
                out[i] = asset->alpha_num12.asset_code[i];
                if (out[i] == 0) {
                    break;
                }
            }
            out[12] = 0;
            return true;
        default:
            return false;
    }
}

bool print_asset(const asset_t *asset, uint8_t network_id, char *out, size_t out_len) {
    char asset_code[12 + 1];
    char asset_issuer[3 + 2 + 4 + 1];
    print_asset_name(asset, network_id, asset_code, sizeof(asset_code));

    switch (asset->type) {
        case ASSET_TYPE_CREDIT_ALPHANUM4:
            print_account_id(asset->alpha_num4.issuer, asset_issuer, sizeof(asset_issuer), 3, 4);
            break;
        case ASSET_TYPE_CREDIT_ALPHANUM12:
            print_account_id(asset->alpha_num12.issuer, asset_issuer, sizeof(asset_issuer), 3, 4);
            break;
        default:
            break;
    }
    if (strlcpy(out, asset_code, out_len) >= out_len) {
        return false;
    }
    if (asset->type != ASSET_TYPE_NATIVE) {
        strlcat(out, "@", out_len);
        strlcat(out, asset_issuer, out_len);
    }
    return true;
}

bool print_flag(const char *flag, char *out, size_t out_len) {
    if (out[0]) {
        if (strlcat(out, ", ", out_len) >= out_len) {
            return false;
        }
    }
    if (strlcat(out, flag, out_len) >= out_len) {
        return false;
    }
    return true;
}

bool print_account_flags(uint32_t flags, char *out, size_t out_len) {
    if (flags & 0x01u) {
        if (!print_flag("AUTH_REQUIRED", out, out_len)) {
            return false;
        }
    }
    if (flags & 0x02u) {
        if (!print_flag("AUTH_REVOCABLE", out, out_len)) {
            return false;
        }
    }
    if (flags & 0x04u) {
        if (!print_flag("AUTH_IMMUTABLE", out, out_len)) {
            return false;
        }
    }
    if (flags & 0x08u) {
        if (!print_flag("AUTH_CLAWBACK_ENABLED", out, out_len)) {
            return false;
        }
    }
    return true;
}

bool print_trust_line_flags(uint32_t flags, char *out, size_t out_len) {
    if (flags & AUTHORIZED_FLAG) {
        if (!print_flag("AUTHORIZED", out, out_len)) {
            return false;
        }
    }
    if (flags & AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG) {
        if (!print_flag("AUTHORIZED_TO_MAINTAIN_LIABILITIES", out, out_len)) {
            return false;
        }
    }
    if (flags & TRUSTLINE_CLAWBACK_ENABLED_FLAG) {
        if (!print_flag("TRUSTLINE_CLAWBACK_ENABLED", out, out_len)) {
            return false;
        }
    }
    return true;
}

bool print_allow_trust_flags(uint32_t flag, char *out, size_t out_len) {
    if (flag & AUTHORIZED_FLAG) {
        if (strlcpy(out, "AUTHORIZED", out_len) >= out_len) {
            return false;
        }
    } else if (flag & AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG) {
        if (strlcpy(out, "AUTHORIZED_TO_MAINTAIN_LIABILITIES", out_len) >= out_len) {
            return false;
        }
    } else {
        if (strlcpy(out, "UNAUTHORIZED", out_len) >= out_len) {
            return false;
        }
    }
    return true;
}

bool print_amount(uint64_t amount,
                  const asset_t *asset,
                  uint8_t network_id,
                  char *out,
                  size_t out_len) {
    char buffer[AMOUNT_WITH_COMMAS_MAX_LENGTH] = {0};
    uint64_t d_val = amount;
    int i;

    for (i = 0; d_val > 0 || i < 9; i++) {
        // len('100.0000001') == 11
        if (i >= 11 && i < AMOUNT_WITH_COMMAS_MAX_LENGTH && (i - 11) % 4 == 0) {
            buffer[i] = ',';
            i += 1;
        }
        if (i >= AMOUNT_WITH_COMMAS_MAX_LENGTH) {
            return false;
        }
        if (d_val > 0) {
            buffer[i] = (d_val % 10) + '0';
            d_val /= 10;
        } else {
            buffer[i] = '0';
        }
        if (i == 6) {  // stroops to xlm: 1 xlm = 10000000 stroops
            i += 1;
            buffer[i] = '.';
        }
        if (i >= AMOUNT_WITH_COMMAS_MAX_LENGTH) {
            return false;
        }
    }

    // reverse order
    for (int j = 0; j < i / 2; j++) {
        char c = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = c;
    }

    // strip trailing 0s
    i -= 1;
    while (buffer[i] == '0') {
        buffer[i] = 0;
        i -= 1;
    }
    // strip trailing .
    if (buffer[i] == '.') buffer[i] = 0;
    if (strlcpy(out, buffer, out_len) >= out_len) {
        return false;
    }

    char asset_info[23];  // BANANANANANA@GBD..KHK4, 12 + 1 + 3 + 2 + 4 = 22

    if (asset) {
        // qualify amount
        if (!print_asset(asset, network_id, asset_info, 23)) {
            return false;
        };
        strlcat(out, " ", out_len);
        strlcat(out, asset_info, out_len);
    }
    return true;
}

bool is_printable_binary(const uint8_t *str, size_t str_len) {
    for (size_t i = 0; i < str_len; i++) {
        if (str[i] > 0x7e || str[i] < 0x20) {
            return false;
        }
    }
    return true;
}
