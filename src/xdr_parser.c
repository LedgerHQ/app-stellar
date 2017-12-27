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
#include "xdr_parser.h"

static const uint8_t PUBLIC_KEY_TYPE_ED25519 = 0;
static const uint8_t MEMO_TEXT_MAX_SIZE = 28;

uint32_t readUInt32Block(uint8_t *buffer) {
    return buffer[3] + (buffer[2] << 8) + (buffer[1] <<  16) + (buffer[0] << 24);
}

uint64_t readUInt64Block(uint8_t *buffer) {
    uint64_t i1 = buffer[3] + (buffer[2] << 8) + (buffer[1] <<  16) + (buffer[0] << 24);
    buffer += 4;
    uint32_t i2 = buffer[3] + (buffer[2] << 8) + (buffer[1] <<  16) + (buffer[0] << 24);
    return i2 | (i1 << 32);
}

uint8_t numBytes(uint8_t size) {
   uint8_t remainder = size % 4;
   if (remainder == 0) {
      return size;
   }
   return size + 4 - remainder;
}

void checkPadding(uint8_t *buffer, uint8_t dataLength, uint8_t totalLength) {
    uint8_t i;
    for (i = 0; i < totalLength - dataLength; i++) {
        if (buffer[dataLength + i] != 0x00) {
             THROW(0x6c2e);
        }
    }
}

uint8_t skipTimeBounds(uint8_t *buffer) {
    uint32_t timeBounds = readUInt32Block(buffer);
    if (timeBounds != 0) {
        return 4 + 8 + 8; // timebounds on + unint64 + uint64
    } else {
        return 4;  // timebounds off
    }
}

uint8_t parseMemo(uint8_t *buffer, txContent_t *txContent) {
    uint8_t memoType = (uint8_t) readUInt32Block(buffer);
    buffer += 4;
    switch (memoType) {
        case MEMO_TYPE_NONE:
            memcpy(txContent->memo, "[none]", 7);
            return 4;
        case MEMO_TYPE_ID:
            print_id(readUInt64Block(buffer), txContent->memo, 22);
            return 4 + 8; // type + value
        case MEMO_TYPE_TEXT: {
            uint8_t size = readUInt32Block(buffer);
            if (size > MEMO_TEXT_MAX_SIZE) {
                THROW(0x6c2f);
            }
            buffer += 4;
            memcpy(txContent->memo, buffer, size);
            txContent->memo[size] = '\0';
            checkPadding(buffer, size, numBytes(size)); // security check
            return 4 + 4 + numBytes(size); // type + size + text
        }
        case MEMO_TYPE_HASH:
        case MEMO_TYPE_RETURN:
            memcpy(txContent->memo, "[hash]", 7);
            return 4 + 32; // type + hash block
        default:
            THROW(0x6c21); // unknown memo type
    }
}

uint8_t parseAsset(uint8_t *buffer, char *asset) {
    uint32_t assetType = readUInt32Block(buffer);
    buffer += 4;
    switch (assetType) {
        case ASSET_TYPE_NATIVE: {
            strcpy(asset, "XLM");
            asset[3] = '\0';
            return 4; // type
        }
        case ASSET_TYPE_CREDIT_ALPHANUM4: {
            memcpy(asset, buffer, 4);
            asset[4] = '\0';
            buffer += 4;
            uint32_t sourceAccountType = readUInt32Block(buffer);
            if (sourceAccountType != PUBLIC_KEY_TYPE_ED25519) {
                THROW(0x6c23);
            }
            buffer += 4;
            return 4 + 4 + 36; // type + code4 + accountId
        }
        case ASSET_TYPE_CREDIT_ALPHANUM12: {
            memcpy(asset, buffer, 12);
            asset[12] = '\0';
            buffer += 12;
            uint32_t sourceAccountType = readUInt32Block(buffer);
            if (sourceAccountType != PUBLIC_KEY_TYPE_ED25519) {
                THROW(0x6c22);
            }
            buffer += 4;
            return 4 + 12 + 36; // type + code12 + accountId
        }
        default:
            THROW(0x6c28); // unknown memo type
    }
}

void parseCreateAccountOpXdr(uint8_t *buffer, txContent_t *txContent) {
    uint32_t destinationAccountType = readUInt32Block(buffer);
    if (destinationAccountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c27);
    }
    buffer += 4;
    char destination[57];
    public_key_to_address(buffer, destination);
    print_summary(destination, txContent->destination);
    PRINTF("destination: %s\n", txContent->destination);
    buffer += 8*4;
    uint64_t amount = readUInt64Block(buffer);
    print_amount(amount, "XLM", txContent->amount, 22);
    PRINTF("amount: %s\n", txContent->amount);
}

void parsePaymentOpXdr(uint8_t *buffer, txContent_t *txContent) {
    uint32_t destinationAccountType = readUInt32Block(buffer);
    if (destinationAccountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c27);
    }
    buffer += 4;
    char destination[57];
    public_key_to_address(buffer, destination);
    print_summary(destination, txContent->destination);
    PRINTF("destination: %s\n", txContent->destination);
    buffer += 8*4;
    char asset[13];
    buffer += parseAsset(buffer, asset);
    uint64_t amount = readUInt64Block(buffer);
    print_amount(amount, asset, txContent->amount, 22);
    PRINTF("amount: %s\n", txContent->amount);
}

void parsePathPaymentOpXdr(uint8_t *buffer, txContent_t *txContent) {
    char asset[13];
    buffer += parseAsset(buffer, asset);
    uint64_t amount = readUInt64Block(buffer);
    buffer += 8;
    print_amount(amount, asset, txContent->amount, 22);
    PRINTF("send: %s\n", txContent->amount);
    uint32_t sourceAccountType = readUInt32Block(buffer);
    if (sourceAccountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c2b);
    }
    buffer += 4;
    char destination[57];
    public_key_to_address(buffer, destination);
    print_summary(destination, txContent->destination);
    PRINTF("destination: %s\n", txContent->destination);
    buffer += 32;
    buffer += parseAsset(buffer, asset);
    amount = readUInt64Block(buffer);
    buffer += 8;
    print_amount(amount, asset, txContent->amount_alt, 22);
    PRINTF("receive: %s\n", txContent->amount_alt);
}

void parseSetOptionsOpXdr(uint8_t *buffer, txContent_t *txContent) {
    // TODO
}

void parseAllowTrustOpXdr(uint8_t *buffer, txContent_t *txContent) {
    // TODO
}

void parseAccountMergeOpXdr(uint8_t *buffer, txContent_t *txContent) {
    // TODO
}

void parseInflationOpXdr(uint8_t *buffer, txContent_t *txContent) {
    // TODO
}

void parseManageDataOpXdr(uint8_t *buffer, txContent_t *txContent) {
    // TODO
}

uint8_t parseOfferOpXdr(uint8_t *buffer, txContent_t *txContent, uint32_t operationType) {
    char selling[13];
    buffer += parseAsset(buffer, selling);
    buffer += parseAsset(buffer, txContent->asset);
    uint64_t amount  = readUInt64Block(buffer);
    print_amount(amount, selling, txContent->amount, 22);
    PRINTF("selling: %s\n", txContent->amount);
    buffer += 8;
    uint32_t numerator = readUInt32Block(buffer);
    buffer += 4;
    uint32_t denominator = readUInt32Block(buffer);
    buffer += 4;
    uint64_t price = (numerator * 10000000) / denominator;
    print_amount(price, selling, txContent->amount_alt, 22);
    PRINTF("price: %s\n", txContent->amount_alt);

    if (operationType == XDR_OPERATION_TYPE_MANAGE_OFFER) {
        txContent->offerId = readUInt64Block(buffer);
        if (txContent->offerId == 0) {
            return OPERATION_TYPE_CREATE_OFFER;
        } else if (amount == 0) {
            return OPERATION_TYPE_DELETE_OFFER;
        } else {
            return OPERATION_TYPE_CHANGE_OFFER;
        }
    } else {
        return OPERATION_TYPE_CREATE_PASSIVE_OFFER;
    }
}

uint8_t parseChangeTrustOpXdr(uint8_t *buffer, txContent_t *txContent) {
    buffer += parseAsset(buffer, txContent->asset);
    PRINTF("asset: %s\n", txContent->asset);
    uint64_t amount = readUInt64Block(buffer);
    if (amount == 0) {
        return OPERATION_TYPE_REMOVE_TRUST;
    } else {
        return OPERATION_TYPE_CHANGE_TRUST;
    }
}

void parseOpXdr(uint8_t *buffer, txContent_t *txContent) {
    uint32_t hasAccountId = readUInt32Block(buffer);
    buffer += 4;
    if (hasAccountId) {
        uint32_t sourceAccountType = readUInt32Block(buffer);
        if (sourceAccountType != PUBLIC_KEY_TYPE_ED25519) {
            THROW(0x6c2b);
        }
        buffer += 4;
        char source[57];
        public_key_to_address(buffer, source);
        print_summary(source, txContent->source);
        PRINTF("operation source: %s\n", txContent->source);
        buffer += 8*4;
    }
    uint32_t operationType = readUInt32Block(buffer);
    buffer += 4;
    switch (operationType) {
        case XDR_OPERATION_TYPE_CREATE_ACCOUNT: {
            txContent->operationType = OPERATION_TYPE_CREATE_ACCOUNT;
            parseCreateAccountOpXdr(buffer, txContent);
            break;
        }
        case XDR_OPERATION_TYPE_PAYMENT: {
            txContent->operationType = OPERATION_TYPE_PAYMENT;
            parsePaymentOpXdr(buffer, txContent);
            break;
        }
        case XDR_OPERATION_TYPE_PATH_PAYMENT: {
            txContent->operationType = OPERATION_TYPE_PATH_PAYMENT;
            parsePathPaymentOpXdr(buffer, txContent);
            break;
        }
        case XDR_OPERATION_TYPE_CREATE_PASSIVE_OFFER:
        case XDR_OPERATION_TYPE_MANAGE_OFFER: {
            txContent->operationType = parseOfferOpXdr(buffer, txContent, operationType);
            break;
        }
        case XDR_OPERATION_TYPE_SET_OPTIONS: {
            txContent->operationType = OPERATION_TYPE_SET_OPTIONS;
            parseSetOptionsOpXdr(buffer, txContent);
            break;
        }
        case XDR_OPERATION_TYPE_CHANGE_TRUST: {
            txContent->operationType = parseChangeTrustOpXdr(buffer, txContent);
            break;
        }
        case XDR_OPERATION_TYPE_ALLOW_TRUST: {
            txContent->operationType = OPERATION_TYPE_ALLOW_TRUST;
            parseAllowTrustOpXdr(buffer, txContent);
            break;
        }
        case XDR_OPERATION_TYPE_ACCOUNT_MERGE: {
            txContent->operationType = OPERATION_TYPE_ACCOUNT_MERGE;
            parseAccountMergeOpXdr(buffer, txContent);
            break;
        }
        case XDR_OPERATION_TYPE_INFLATION: {
            txContent->operationType = OPERATION_TYPE_INFLATION;
            parseInflationOpXdr(buffer, txContent);
            break;
        }
        case XDR_OPERATION_TYPE_MANAGE_DATA: {
            txContent->operationType = OPERATION_TYPE_MANAGE_DATA;
            parseManageDataOpXdr(buffer, txContent);
            break;
        }
        default: {
            txContent->operationType = OPERATION_TYPE_UNKNOWN;
        }
    }
}

void parseOpsXdr(uint8_t *buffer, txContent_t *txContent) {
    uint32_t operationsCount = readUInt32Block(buffer);
    if (operationsCount != 1) {
        THROW(0x6c25);
    }
    buffer += 4;
    parseOpXdr(buffer, txContent);
}

void parseTxXdr(uint8_t *buffer, txContent_t *txContent) {
    print_network_id(buffer, txContent->networkId);
    buffer += 32;
    buffer += 4; // skip envelopeType
    uint32_t sourceAccountType = readUInt32Block(buffer);
    if (sourceAccountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c26);
    }
    buffer += 4;
    char source[57];
    public_key_to_address(buffer, source);
    print_summary(source, txContent->source);
    PRINTF("transaction source: %s\n", txContent->source);
    buffer += 8*4;
    uint32_t fee = readUInt32Block(buffer);
    print_amount(fee, "XLM", txContent->fee, 22);
    PRINTF("fee: %s\n", txContent->fee);
    buffer += 4;
    buffer += 8; // skip seqNum
    buffer += skipTimeBounds(buffer);
    buffer += parseMemo(buffer, txContent);
    PRINTF("memo: %s\n", txContent->memo);
//    printHexBlocks(buffer, 20);
    parseOpsXdr(buffer, txContent);
}
