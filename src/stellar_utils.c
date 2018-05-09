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
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "stellar_types.h"
#include "stellar_api.h"

#ifndef TEST
#include "os.h"
#endif


static const uint8_t TEST_NETWORK_ID_HASH[64] = {0xce, 0xe0, 0x30, 0x2d, 0x59, 0x84, 0x4d, 0x32,
                                                 0xbd, 0xca, 0x91, 0x5c, 0x82, 0x03, 0xdd, 0x44,
                                                 0xb3, 0x3f, 0xbb, 0x7e, 0xdc, 0x19, 0x05, 0x1e,
                                                 0xa3, 0x7a, 0xbe, 0xdf, 0x28, 0xec, 0xd4, 0x72};

static const uint8_t PUBLIC_NETWORK_ID_HASH[64] = {0x7a, 0xc3, 0x39, 0x97, 0x54, 0x4e, 0x31, 0x75,
                                                   0xd2, 0x66, 0xbd, 0x02, 0x24, 0x39, 0xb2, 0x2c,
                                                   0xdb, 0x16, 0x50, 0x8c, 0x01, 0x16, 0x3f, 0x26,
                                                   0xe5, 0xcb, 0x2a, 0x3e, 0x10, 0x45, 0xa9, 0x79};

static const char hexChars[] = "0123456789ABCDEF";

static const uint8_t AMOUNT_MAX_SIZE = 22;

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

void print_summary(char *in, char *out, uint8_t numCharsL, uint8_t numCharsR) {
    uint8_t outLength = numCharsL + numCharsR + 2;
    uint16_t inLength = strlen(in);
    if (inLength > outLength) {
        memcpy(out, in, numCharsL);
        out[numCharsL] = '.';
        out[numCharsL+1] = '.';
        memcpy(out + numCharsL+2, in + inLength - numCharsR, numCharsR);
        out[outLength] = '\0';
    } else {
        memcpy(out, in, inLength);
    }
}

void print_hash(uint8_t *in, char *out) {
    uint8_t i, j;
    for (i = 0, j = 0; i < 32; i+=1, j+=2) {
        out[j] = hexChars[in[i] / 16];
        out[j+1] = hexChars[in[i] % 16];
    }
    out[j] = '\0';
}

void print_hash_summary(uint8_t *in, char *out) {
    uint8_t i, j;
    for (i = 0, j = 0; i < 4; i+=1, j+=2) {
        out[j] = hexChars[in[i] / 16];
        out[j+1] = hexChars[in[i] % 16];
    }
    out[j++] = '.';
    out[j++] = '.';
    for (i = 28; i < 32; i+=1, j+=2) {
        out[j] = hexChars[in[i] / 16];
        out[j+1] = hexChars[in[i] % 16];
    }
    out[j] = '\0';
}

void print_public_key(uint8_t *in, char *out, uint8_t numCharsL, uint8_t numCharsR) {
    char buffer[57];
    public_key_to_address(in, buffer);
    print_summary(buffer, out, numCharsL, numCharsR);
}

void print_amount(uint64_t amount, char *asset, char *out) {
    char buffer[AMOUNT_MAX_SIZE];
    uint64_t dVal = amount;
    int i, j;

    memset(buffer, 0, AMOUNT_MAX_SIZE);
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
        if (i >= AMOUNT_MAX_SIZE) {
            THROW(0x6700);
        }
    }
    // reverse order
    for (i -= 1, j = 0; i >= 0 && j < AMOUNT_MAX_SIZE-1; i--, j++) {
        out[j] = buffer[i];
    }
    // strip trailing 0s
    for (j -= 1; j > 0; j--) {
        if (out[j] != '0') break;
    }
    j += 1;

    // strip trailing .
    if (out[j-1] == '.') j -= 1;

    if (asset) {
        // qualify amount
        out[j++] = ' ';
        strcpy(out + j, asset);
        out[j+strlen(asset)] = '\0';
    } else {
        out[j] = '\0';
    }

}

void print_int(uint64_t l, char *out) {
    char buffer[AMOUNT_MAX_SIZE];
    uint64_t dVal = l;
    int i, j;

    memset(buffer, 0, AMOUNT_MAX_SIZE);
    for (i = 0; dVal > 0; i++) {
        buffer[i] = (dVal % 10) + '0';
        dVal /= 10;
        if (i >= AMOUNT_MAX_SIZE) {
            THROW(0x6700);
        }
    }
    // reverse order
    for (i -= 1, j = 0; i >= 0 && j < AMOUNT_MAX_SIZE-1; i--, j++) {
        out[j] = buffer[i];
    }
    out[j] = '\0';
}

void print_bits(uint32_t in, char *out) {
    out[2] = (in & 0x01) ? '1' : '0';
    out[1] = (in & 0x02) ? '1' : '0';
    out[0] = (in & 0x04) ? '1' : '0';
    out[3] = '\0';
}

void print_network_id(uint8_t *in, char *out) {
    if (memcmp(in, PUBLIC_NETWORK_ID_HASH, 32) == 0) {
        strcpy(out, "Public");
    } else if (memcmp(in, TEST_NETWORK_ID_HASH, 32) == 0) {
        strcpy(out, "Test");
    } else {
        strcpy(out, "Unknown");
    }
}

void print_asset(char *code, char *issuer, char *out) {
    uint8_t offset = strlen(code);
    strcpy(out, code);
    out[offset] = '@';
    strcpy(out+offset+1, issuer);
}

unsigned short crc16(char *ptr, int count) {
   int  crc;
   char i;
   crc = 0;
   while (--count >= 0) {
      crc = crc ^ (int) *ptr++ << 8;
      i = 8;
      do
      {
         if (crc & 0x8000)
            crc = crc << 1 ^ 0x1021;
         else
            crc = crc << 1;
      } while(--i);
   }
   return (crc);
}

/**
 * adapted from https://stash.forgerock.org/projects/OPENAM/repos/forgerock-authenticator-ios/browse/ForgeRock-Authenticator/base32.c
 */
int base32_encode(const uint8_t *data, int length, char *result, int bufSize) {
    int count = 0;
    int quantum = 8;

    if (length < 0 || length > (1 << 28)) {
        return -1;
    }

    if (length > 0) {
        int buffer = data[0];
        int next = 1;
        int bitsLeft = 8;

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

            int index = 0x1F & (buffer >> (bitsLeft - 5));
            bitsLeft -= 5;
            result[count++] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"[index];

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
