/*******************************************************************************
 *   Ledger Stellar App
 *   (c) 2017-2018 Ledger
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
 ********************************************************************************/
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "stellar_types.h"
#include "stellar_api.h"

#include "bolos_target.h"

static const char hexAlphabet[] = "0123456789ABCDEF";
static const char base32Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
static const char base64Alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int base64ModTable[] = {0, 2, 1};

bool parse_bip32_path(uint8_t *path,
                      size_t path_length,
                      uint32_t *path_parsed,
                      size_t path_parsed_length) {
    if ((path_length < 0x01) || (path_length > path_parsed_length)) {
        return false;
    }

    for (size_t i = 0; i < path_length; i++) {
        path_parsed[i] = (path[0] << 24u) | (path[1] << 16u) | (path[2] << 8u) | (path[3]);
        path += 4;
    }

    return true;
}

unsigned short crc16(char *ptr, int count) {
    int crc;
    crc = 0;
    while (--count >= 0) {
        crc = crc ^ (int) *ptr++ << 8;
        int i = 8;
        do {
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        } while (--i);
    }
    return (crc);
}

/**
 * adapted from
 * https://stash.forgerock.org/projects/OPENAM/repos/forgerock-authenticator-ios/browse/ForgeRock-Authenticator/base32.c
 */
int base32_encode(const uint8_t *data, int length, char *result, int bufSize) {
    int count = 0;

    if (length < 0 || length > (1 << 28)) {
        return -1;
    }

    if (length > 0) {
        int buffer = data[0];
        int next = 1;
        int bitsLeft = 8;
        int quantum = 8;

        while (count < bufSize && (bitsLeft > 0 || next < length)) {
            if (bitsLeft < 5) {
                if (next < length) {
                    buffer <<= 8;
                    buffer |= data[next++] & 0xFF;
                    bitsLeft += 8;
                } else {
                    int pad = 5 - bitsLeft;
                    buffer <<= pad;
                    bitsLeft += pad;
                }
            }

            int idx = 0x1F & (buffer >> (bitsLeft - 5));
            bitsLeft -= 5;
            result[count++] = base32Alphabet[idx];

            // Track the characters which make up a single quantum of 8 characters
            quantum--;
            if (quantum == 0) {
                quantum = 8;
            }
        }

        // If the number of encoded characters does not make a full quantum, insert padding
        if (quantum != 8) {
            while (quantum > 0 && count < bufSize) {
                result[count++] = '=';
                quantum--;
            }
        }
    }

    // Finally check if we exceeded buffer size.
    if (count < bufSize) {
        result[count] = '\000';
        return count;
    } else {
        return -1;
    }
}

void base64_encode(const uint8_t *data, int inLen, char *out) {
    size_t outLen = 4 * ((inLen + 2) / 3);

    for (int i = 0, j = 0; i < inLen;) {
        uint32_t octet_a = i < inLen ? data[i++] : 0;
        uint32_t octet_b = i < inLen ? data[i++] : 0;
        uint32_t octet_c = i < inLen ? data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        out[j++] = base64Alphabet[(triple >> 3 * 6) & 0x3F];
        out[j++] = base64Alphabet[(triple >> 2 * 6) & 0x3F];
        out[j++] = base64Alphabet[(triple >> 1 * 6) & 0x3F];
        out[j++] = base64Alphabet[(triple >> 0 * 6) & 0x3F];
    }

    int i;
    for (i = 0; i < base64ModTable[inLen % 3]; i++) {
        out[outLen - 1 - i] = '=';
    }

    out[outLen] = '\0';
}

void encode_key(const uint8_t *in, char *out, uint8_t versionByte) {
    uint8_t buffer[35];
    buffer[0] = versionByte;
    int i;
    for (i = 0; i < 32; i++) {
        buffer[i + 1] = in[i];
    }
    short crc = crc16((char *) buffer, 33);  // checksum
    buffer[33] = crc;
    buffer[34] = crc >> 8;
    base32_encode(buffer, 35, out, 56);
    out[56] = '\0';
}

void encode_public_key(const uint8_t *in, char *out) {
    encode_key(in, out, 6 << 3);
}

void encode_pre_auth_key(const uint8_t *in, char *out) {
    encode_key(in, out, 19 << 3);
}

void encode_hash_x_key(const uint8_t *in, char *out) {
    encode_key(in, out, 23 << 3);
}

void print_summary(const char *in, char *out, uint8_t numCharsL, uint8_t numCharsR) {
    uint8_t outLength = numCharsL + numCharsR + 2;
    uint16_t inLength = strlen(in);
    if (inLength > outLength) {
        memcpy(out, in, numCharsL);
        out[numCharsL] = '.';
        out[numCharsL + 1] = '.';
        memcpy(out + numCharsL + 2, in + inLength - numCharsR, numCharsR);
        out[outLength] = '\0';
    } else {
        memcpy(out, in, inLength);
    }
}

void print_binary(const uint8_t *in, char *out, uint8_t len) {
    out[0] = '0';
    out[1] = 'x';
    uint8_t i, j;
    for (i = 0, j = 2; i < len; i += 1, j += 2) {
        out[j] = hexAlphabet[in[i] / 16];
        out[j + 1] = hexAlphabet[in[i] % 16];
    }
    out[j] = '\0';
}

void print_binary_summary(const uint8_t *in, char *out, uint8_t len) {
    out[0] = '0';
    out[1] = 'x';
    if (2 + len * 2 > 18) {
        uint8_t i, j;
        for (i = 0, j = 2; i < 3; i += 1, j += 2) {
            out[j] = hexAlphabet[in[i] / 16];
            out[j + 1] = hexAlphabet[in[i] % 16];
        }
        out[j++] = '.';
        out[j++] = '.';
        for (i = len - 3; i < len; i += 1, j += 2) {
            out[j] = hexAlphabet[in[i] / 16];
            out[j + 1] = hexAlphabet[in[i] % 16];
        }
        out[j] = '\0';
    } else {
        print_binary(in, out, len);
        return;
    }
}

void print_public_key(AccountID in, char *out, uint8_t numCharsL, uint8_t numCharsR) {
    if (numCharsL > 0) {
        char buffer[57];
        encode_public_key(in, buffer);
        print_summary(buffer, out, numCharsL, numCharsR);
    } else {
        encode_public_key(in, out);
    }
}

void encode_muxed_account(const MuxedAccount *in, char *out) {
    if (in->type == KEY_TYPE_ED25519) {
        encode_public_key(in->ed25519, out);
    } else {
        uint8_t buffer[43];
        buffer[0] = 12 << 3;
        int i;
        for (i = 0; i < 32; i++) {
            buffer[i + 1] = in->med25519.ed25519[i];
        }
        for (i = 0; i < 8; i++) {
            buffer[33 + i] = in->med25519.id >> 8 * (7 - i);
        }
        short crc = crc16((char *) buffer, 41);  // checksum
        buffer[41] = crc;
        buffer[42] = crc >> 8;
        base32_encode(buffer, 43, out, 69);
        out[69] = '\0';
    }
}

void print_muxed_account(const MuxedAccount *in, char *out, uint8_t numCharsL, uint8_t numCharsR) {
    if (numCharsL > 0) {
        char buffer[70];
        encode_muxed_account(in, buffer);
        print_summary(buffer, out, numCharsL, numCharsR);
    } else {
        encode_muxed_account(in, out);
    }
}

int print_asset_name(const Asset *asset, uint8_t network_id, char *out, size_t out_len) {
    switch (asset->type) {
        case ASSET_TYPE_NATIVE:
            print_native_asset_code(network_id, out, out_len);
            return 0;
        case ASSET_TYPE_CREDIT_ALPHANUM4:
            for (int i = 0; i < 4; i++) {
                out[i] = asset->alphaNum4.assetCode[i];
                if (out[i] == 0) {
                    break;
                }
            }
            out[4] = 0;
            return 0;
        case ASSET_TYPE_CREDIT_ALPHANUM12:
            for (int i = 0; i < 12; i++) {
                out[i] = asset->alphaNum12.assetCode[i];
                if (out[i] == 0) {
                    break;
                }
            }
            out[12] = 0;
            return 0;
        default:
            return -1;
    }
}

int print_amount(uint64_t amount,
                 const Asset *asset,
                 uint8_t network_id,
                 char *out,
                 size_t out_len) {
    char buffer[AMOUNT_MAX_SIZE] = {0};
    uint64_t dVal = amount;
    int i;

    for (i = 0; dVal > 0 || i < 9; i++) {
        if (dVal > 0) {
            buffer[i] = (dVal % 10) + '0';
            dVal /= 10;
        } else {
            buffer[i] = '0';
        }
        if (i == 6) {  // stroops to xlm: 1 xlm = 10000000 stroops
            i += 1;
            buffer[i] = '.';
        }
        if (i >= AMOUNT_MAX_SIZE) {
            return -1;
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
    strlcpy(out, buffer, out_len);

    char assetInfo[23];  // BANANANANANA@GBD..KHK4, 12 + 1 + 3 + 2 + 4 = 22

    if (asset) {
        // qualify amount
        print_asset_t(asset, network_id, assetInfo, 23);
        strlcat(out, " ", out_len);
        strlcat(out, assetInfo, out_len);
    }
    return 0;
}

int print_int(int64_t l, char *out, size_t out_len) {
    if (out_len == 0) {
        return -1;
    }
    if (l < 0) {
        out[0] = '-';
        return print_uint(-l, out + 1, out_len - 1);
    }
    return print_uint(l, out, out_len);
}

int print_uint(uint64_t l, char *out, size_t out_len) {
    char buffer[AMOUNT_MAX_SIZE];
    uint64_t dVal = l;
    size_t i, j;

    if (l == 0) {
        if (out_len < 2) {
            return -1;
        }
        strlcpy(out, "0", out_len);
        return 0;
    }

    memset(buffer, 0, AMOUNT_MAX_SIZE);
    for (i = 0; dVal > 0; i++) {
        if (i >= AMOUNT_MAX_SIZE) {
            return -1;
        }
        buffer[i] = (dVal % 10) + '0';
        dVal /= 10;
    }
    if (out_len <= i) {
        return -1;
    }
    // reverse order
    for (j = 0; j < i; j++) {
        out[j] = buffer[i - j - 1];
    }
    out[i] = '\0';
    return 0;
}

void print_asset_t(const Asset *asset, uint8_t network_id, char *out, size_t out_len) {
    char issuer[12];
    char asset_name[12 + 1];

    print_asset_name(asset, network_id, asset_name, sizeof(asset_name));

    switch (asset->type) {
        case ASSET_TYPE_CREDIT_ALPHANUM4:
            print_public_key(asset->alphaNum4.issuer, issuer, 3, 4);
            break;
        case ASSET_TYPE_CREDIT_ALPHANUM12:
            print_public_key(asset->alphaNum12.issuer, issuer, 3, 4);
            break;
        default:
            break;
    }

    bool isNative = asset->type == ASSET_TYPE_NATIVE ? true : false;
    print_asset(asset_name, issuer, isNative, out, out_len);
}

void print_asset(const char *code, char *issuer, bool isNative, char *out, size_t out_len) {
    strlcpy(out, code, out_len);
    if (!isNative) {
        strlcat(out, "@", out_len);
        strlcat(out, issuer, out_len);
    }
}

static void print_flag(char *flag, char *out, size_t out_len) {
    if (out[0]) {
        strlcat(out, ", ", out_len);
    }
    strlcat(out, flag, out_len);
}

void print_flags(uint32_t flags, char *out, size_t out_len) {
    if (flags & 0x01u) {
        print_flag("Auth required", out, out_len);
    }
    if (flags & 0x02u) {
        print_flag("Auth revocable", out, out_len);
    }
    if (flags & 0x04u) {
        print_flag("Auth immutable", out, out_len);
    }
}

void print_trust_line_flags(uint32_t flags, char *out, size_t out_len) {
    if (flags & AUTHORIZED_FLAG) {
        print_flag("AUTHORIZED", out, out_len);
    }
    if (flags & AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG) {
        print_flag("AUTHORIZED_TO_MAINTAIN_LIABILITIES", out, out_len);
    }
    if (flags & TRUSTLINE_CLAWBACK_ENABLED_FLAG) {
        print_flag("TRUSTLINE_CLAWBACK_ENABLED", out, out_len);
    }
}

void print_native_asset_code(uint8_t network, char *out, size_t out_len) {
    if (network == NETWORK_TYPE_UNKNOWN) {
        strlcpy(out, "native", out_len);
    } else {
        strlcpy(out, "XLM", out_len);
    }
}

void print_claimable_balance_id(const ClaimableBalanceID *claimableBalanceID, char *out) {
    size_t data_len = 36;
    uint8_t data[data_len];
    memcpy(data, &claimableBalanceID->type, 4);
    memcpy(data + 4, claimableBalanceID->v0, 32);
    print_binary(data, out, data_len);
}

bool print_time(uint64_t timestamp_in_seconds, char *out, size_t out_len) {
    if (timestamp_in_seconds > 253402300799) {
        // valid range 1970-01-01 00:00:00 - 9999-12-31 23:59:59
        return false;
    }
    char strTime[20] = {0};  // 1970-01-01 00:00:00
    struct tm tm;
    if (!gmtime_r((time_t *) &timestamp_in_seconds, &tm)) {
        return false;
    };

    if (snprintf(strTime,
                 sizeof(strTime),
                 "%04d-%02d-%02d %02d:%02d:%02d",
                 tm.tm_year + 1900,
                 tm.tm_mon + 1,
                 tm.tm_mday,
                 tm.tm_hour,
                 tm.tm_min,
                 tm.tm_sec) < 0) {
        return false;
    };
    strlcpy(out, strTime, out_len);
    return true;
}