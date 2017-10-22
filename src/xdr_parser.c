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

#include "stlr_utils.h"
#include "xdr_parser.h"

static const uint8_t ASSET_TYPE_NATIVE = 0;
static const uint8_t PUBLIC_KEY_TYPE_ED25519 = 0;

uint32_t readUInt32Block(char *buffer) {
    return (buffer[3] & 0xFF) + ((buffer[2] & 0xFF) << 8) + ((buffer[1] & 0xFF) <<  16) + ((buffer[0] & 0xFF) << 24);
}

uint64_t readUInt64Block(char *buffer) {
    return (buffer[7] & 0xFF) + ((buffer[6] & 0xFF) << 8) + ((buffer[5] & 0xFF) <<  16) + ((buffer[4] & 0xFF) << 24)
        + (buffer[3] & 0xFF) + ((buffer[2] & 0xFF) << 8) + ((buffer[1] & 0xFF) <<  16) + ((buffer[0] & 0xFF) << 24);
}

void parsePaymentOpXdr(char *buffer, txContent_t *txContent) {
    uint32_t destinationAccountType = readUInt32Block(buffer);
    if (destinationAccountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c20);
    }
    buffer += 4;
    public_key_to_address((void *)buffer, txContent->destination);
    PRINTF("destination: %s\n", txContent->destination);
    char address_summary[14];
    summarize_address(txContent->destination, address_summary);
    PRINTF("summary: %s\n", address_summary);
    buffer += 8*4;
    uint32_t asset = readUInt32Block(buffer);
    if (asset != ASSET_TYPE_NATIVE) {
        THROW(0x6c20);
    }
    buffer += 4;
    txContent->amount = readUInt64Block(buffer);
    char amount_summary[22];
    print_amount(txContent->amount, amount_summary, 22);
    PRINTF("amount: %s\n", amount_summary);
}

void parseOpXdr(char *buffer, txContent_t *txContent) {
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

void parseOpsXdr(char *buffer, txContent_t *txContent) {
    uint32_t operationsCount = readUInt32Block(buffer);
    if (operationsCount != 1) {
        THROW(0x6c20);
    }
    buffer += 4;
    parseOpXdr(buffer, txContent);
}

void parseTxXdr(char *buffer, txContent_t *txContent) {
    buffer += 8*4; // skip networkId
    buffer += 4; // skip envelopeType
    uint32_t sourceAccountType = readUInt32Block(buffer);
    if (sourceAccountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c20);
    }
    buffer += 4;
    public_key_to_address((void *)buffer, txContent->source);
    PRINTF("source: %s\n", txContent->source);
    buffer += 8*4;
    txContent->fee = readUInt32Block(buffer);
    char fee_summary[22];
    print_amount(txContent->fee, fee_summary, 22);
    PRINTF("fee: %s\n", fee_summary);
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
