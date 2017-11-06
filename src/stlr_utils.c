/*******************************************************************************
 *   Ledger Stellar App
 *   (c) 2017 Ledger
 *
 *  adapted from https://github.com/mjg59/tpmtotp/blob/master/base32.h
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
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "stlr_utils.h"
#include "crc16.h"
#include "base32.h"

static const uint8_t TEST_NETWORK_ID_HASH[64] = {0xce, 0xe0, 0x30, 0x2d, 0x59, 0x84, 0x4d, 0x32,
                                                 0xbd, 0xca, 0x91, 0x5c, 0x82, 0x03, 0xdd, 0x44,
                                                 0xb3, 0x3f, 0xbb, 0x7e, 0xdc, 0x19, 0x05, 0x1e,
                                                 0xa3, 0x7a, 0xbe, 0xdf, 0x28, 0xec, 0xd4, 0x72};

static const uint8_t PUBLIC_NETWORK_ID_HASH[64] = {0x7a, 0xc3, 0x39, 0x97, 0x54, 0x4e, 0x31, 0x75,
                                                   0xd2, 0x66, 0xbd, 0x02, 0x24, 0x39, 0xb2, 0x2c,
                                                   0xdb, 0x16, 0x50, 0x8c, 0x01, 0x16, 0x3f, 0x26,
                                                   0xe5, 0xcb, 0x2a, 0x3e, 0x10, 0x45, 0xa9, 0x79};

static const char hexChars[] = "0123456789ABCDEF";

/**
 * convert the raw public key to a stellar address
 */
void public_key_to_address(uint8_t *in, char *out) {
    uint8_t buffer[35];
    buffer[0] = 6 << 3; // version bit 'G'
    int i;
    for (i = 0; i < 32; i++) {
        buffer[i+1] = in[i];
    }
    short crc = crc16((char *)buffer, 33); // checksum
    buffer[33] = crc;
    buffer[34] = crc >> 8;
    base32_encode(buffer, 35, out, 56);
    out[56] = '\0';
}

void print_summary(char *in, char *out) {
    size_t len = strlen(in);
    if (strlen(in) > 12) {
        memcpy(out, in, 5);
        out[5] = '.';
        out[6] = '.';
        out[7] = '.';
        memcpy(out + 8, in + len - 4, 4);
        out[12] = '\0';
    } else {
        memcpy(out, in, len);
    }
}

void print_amount(uint64_t amount, char *asset, char *out, uint8_t len) {
    char buffer[len];
    uint64_t dVal = amount;
    int i, j;

    memset(buffer, 0, len);
    for (i = 0; dVal > 0 || i < 9; i++) {
        if (dVal > 0) {
            buffer[i] = (dVal % 10) + '0';
            dVal /= 10;
        } else {
            buffer[i] = '0';
        }
        if (i == 6) { // stroops to xlm: 1 xlm = 10000000 stroops
            i += 1;
            buffer[i] = '.';
        }
        if (i >= len) {
            THROW(0x6700);
        }
    }
    // reverse order
    for (i -= 1, j = 0; i >= 0 && j < len-1; i--, j++) {
        out[j] = buffer[i];
    }
    // strip trailing 0s
    for (j -= 1; j > 0; j--) {
        if (out[j] != '0') break;
    }
    j += 1;

    // strip trailing .
    if (out[j-1] == '.') j -= 1;

    // qualify amount
    out[j++] = ' ';
    strncpy(out + j, asset, strlen(asset));
    out[j+strlen(asset)] = '\0';

}

void print_id_memo(uint64_t id, char *out, uint8_t len) {
    char buffer[len];
    uint64_t dVal = id;
    int i, j;

    memset(buffer, 0, len);
    for (i = 0; dVal > 0; i++) {
        buffer[i] = (dVal % 10) + '0';
        dVal /= 10;
        if (i >= len) {
            THROW(0x6700);
        }
    }
    // reverse order
    for (i -= 1, j = 0; i >= 0 && j < len-1; i--, j++) {
        out[j] = buffer[i];
    }
    out[j] = '\0';
}

void print_network_id(uint8_t *in, char *out) {
    if (memcmp(in, PUBLIC_NETWORK_ID_HASH, 32) == 0) {
        strncpy(out, "Public", 7);
    } else if (memcmp(in, TEST_NETWORK_ID_HASH, 32) == 0) {
        strncpy(out, "Test", 5);
    } else {
        strncpy(out, "Unknown", 8);
    }
}

void print_hash_summary(uint8_t *in, char *out) {
    uint8_t i, j;
    for (i = 0, j = 0; i < 3; i+=1, j+=2) {
        out[j] = hexChars[in[i] / 16];
        out[j+1] = hexChars[in[i] % 16];
    }
    out[j++] = '.';
    out[j++] = '.';
    out[j++] = '.';
    for (i = 61; i < 64; i+=1, j+=2) {
        out[j] = hexChars[in[i] / 16];
        out[j+1] = hexChars[in[i] % 16];
    }
    out[j] = '\0';
}