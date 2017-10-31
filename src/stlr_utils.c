/*******************************************************************************
 *   Ledger Stellar App
 *   (c) 2017 LeNonDupe
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

void summarize_address(char *in, char *out) {
    memcpy(out, in, 5);
    out[5] = '.';
    out[6] = '.';
    out[7] = '.';
    memcpy(out + 8, in + 52, 4);
    out[12] = '\0';
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
    // strip trailing .
    if (out[j] == '.') j--;
    // qualify amount
    out[++j] = ' ';
    strncpy(out + ++j, asset, strlen(asset));
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