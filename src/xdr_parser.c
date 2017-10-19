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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "os.h"
#include "base32.h"
#include "crc16.h"
#include "stlr_utils.h"
#include "xdr_parser.h"

static const uint8_t ASSET_TYPE_NATIVE = 0;
static const uint8_t PUBLIC_KEY_TYPE_ED25519 = 0;


void printHexBlocks(uint8_t *buffer, int size) {
    int i = 0;
    for (i = 0; i < size; i++) {
        if (i % 4 == 0) {
            if (i > 0) {
                printf("]");
            }
            printf("\n[");
        } else if (i > 0) {
            printf(":");
        }
        printf("%02hhx", buffer[i]);
    }
    printf("]\n");
}

uint32_t readUInt32Block(uint8_t *buffer) {
    return buffer[3] + (buffer[2] << 8) + (buffer[1] <<  16) + (buffer[0] << 24);
}

uint64_t readUInt64Block(uint8_t *buffer) {
    return buffer[7] + (buffer[6] << 8) + (buffer[5] <<  16) + (buffer[4] << 24)
        + buffer[3] + (buffer[2] << 8) + (buffer[1] <<  16) + (buffer[0] << 24);
}

void parsePaymentOpXdr(uint8_t *buffer, txContent_t *txContent) {
    uint32_t destinationAccountType = readUInt32Block(buffer);
    if (destinationAccountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c20);
    }
    buffer += 4;
    public_key_to_address((uint8_t*)buffer, txContent->destination);
//    printf("destination: %s\n", txContent->destination);
    buffer += 8*4;
    uint32_t asset = readUInt32Block(buffer);
    if (asset != ASSET_TYPE_NATIVE) {
        THROW(0x6c20);
    }
    buffer += 4;
    txContent->amount = readUInt64Block(buffer);
//    printf("amount: %lu\n", txContent->amount);
}

void parseOpXdr(uint8_t *buffer, txContent_t *txContent) {
    uint32_t hasAccountId = readUInt32Block(buffer);
    if (hasAccountId) {
        THROW(0x6c20);
    }
    buffer += 4;
    uint32_t operationType = readUInt32Block(buffer);
    if (operationType != 1) {
        THROW(0x6c20);
    }
    buffer += 4;
    parsePaymentOpXdr(buffer, txContent);
}

void parseOpsXdr(uint8_t *buffer, txContent_t *txContent) {
    uint32_t operationsCount = readUInt32Block(buffer);
    if (operationsCount != 1) {
        THROW(0x6c20);
    }
    buffer += 4;
    parseOpXdr(buffer, txContent);
}

void parseTxXdr(uint8_t *buffer, int size, txContent_t *txContent) {
    buffer += 8*4; // skip networkId
    buffer += 4; // skip envelopeType
    uint32_t sourceAccountType = readUInt32Block(buffer);
    if (sourceAccountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c20);
    }
    buffer += 4;
    public_key_to_address((uint8_t*)buffer, txContent->source);
//    printf("source: %s\n", txContent->source);
    buffer += 8*4;
    txContent->fee = readUInt32Block(buffer);
    buffer += 4;
    buffer += 8; // skip seqNum
    uint32_t timeBounds = readUInt32Block(buffer);
    if (timeBounds != 0) {
        THROW(0x6c20);
    }
    buffer += 4;
    uint32_t memoType = readUInt32Block(buffer);
    if (memoType != 0) {
        THROW(0x6c20);
    }
    buffer += 4; // skip memoType
    parseOpsXdr(buffer, txContent);
}
