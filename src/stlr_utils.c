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
#include <stdlib.h>
#include <string.h>
#include "os.h"
#include "cx.h"
#include "crc16.h"
#include "base32.h"

/**
 * read the generated ed25516 public key
 */
void read_public_key(cx_ecfp_public_key_t *publicKey, uint8_t *out) {
    uint8_t i;
    for (i = 0; i < 32; i++) {
        out[i] = publicKey->W[64 - i];
    }
    if ((publicKey->W[32] & 1) != 0) {
        out[31] |= 0x80;
    }
}

/**
 * convert the raw public key to a stellar address
 */
void public_key_to_address(uint8_t *in, char *out) {
    char *buffer = (char*) malloc(35);
    buffer[0] = 6 << 3; // version bit 'G'
    int i;
    for (i = 0; i < 32; i++) {
        buffer[i+1] = in[i];
    }
    short crc = crc16(buffer, 33); // checksum
    buffer[33] = crc;
    buffer[34] = crc >> 8;
    base32_encode(buffer, 35, out);
    out[56] = '\0';
}

void summarize_address(char *in, char *out) {
    strncpy(out, in, 5);
    strncpy(out + 5, "...", 3);
    strncpy(out + 8, in + 52, 5);
}
