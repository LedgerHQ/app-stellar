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

#define SIGNER_KEY_TYPE_ED25519 0
#define SIGNER_KEY_TYPE_PRE_AUTH_TX 1
#define SIGNER_KEY_TYPE_HASH_X 2

static const uint8_t PUBLIC_KEY_TYPE_ED25519 = 0;
static const uint8_t MEMO_TEXT_MAX_SIZE = 28;
static const uint8_t DATA_NAME_MAX_SIZE = 64;
static const uint8_t DATA_VALUE_MAX_SIZE = 64;
static const uint8_t HOME_DOMAIN_MAX_SIZE = 32;


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

void checkPadding(uint8_t *buffer, uint8_t offset, uint8_t length) {
    uint8_t i;
    for (i = 0; i < length - offset; i++) {
        if (buffer[offset + i] != 0x00) {
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

uint8_t parseMemo(uint8_t *buffer, tx_content_t *txContent) {
    uint8_t memoType = (uint8_t) readUInt32Block(buffer);
    buffer += 4;
    switch (memoType) {
        case MEMO_TYPE_NONE:
            memcpy(txContent->txDetails[0], "[none]", 7);
            return 4;
        case MEMO_TYPE_ID:
            print_int(readUInt64Block(buffer), txContent->txDetails[0]);
            return 4 + 8; // type + value
        case MEMO_TYPE_TEXT: {
            uint8_t size = readUInt32Block(buffer);
            if (size > MEMO_TEXT_MAX_SIZE) {
                THROW(0x6c2f);
            }
            buffer += 4;
            memcpy(txContent->txDetails[0], buffer, size);
            txContent->txDetails[0][size] = '\0';
            checkPadding(buffer, size, numBytes(size)); // security check
            return 4 + 4 + numBytes(size); // type + size + text
        }
        case MEMO_TYPE_HASH:
        case MEMO_TYPE_RETURN:
            print_hash_summary(buffer, txContent->txDetails[0]);
            return 4 + 32; // type + hash block
        default:
            THROW(0x6c21); // unknown memo type
    }
}

uint8_t parseAsset(uint8_t *buffer, char *asset, char *issuer) {
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
            if (issuer) {
                char tmp[57];
                public_key_to_address(buffer, tmp);
                print_short_summary(tmp, issuer);
            }
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
            if (issuer) {
                char tmp[57];
                public_key_to_address(buffer, tmp);
                print_short_summary(tmp, issuer);
            }
            return 4 + 12 + 36; // type + code12 + accountId
        }
        default:
            THROW(0x6c28); // unknown asset type
    }
}

uint16_t parseCreateAccountOpXdr(uint8_t *buffer, tx_content_t *txContent) {
    txContent->opType = OPERATION_TYPE_CREATE_ACCOUNT;

    uint32_t destinationAccountType = readUInt32Block(buffer);
    if (destinationAccountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c27);
    }
    uint16_t offset = 4;

    print_public_key(buffer + offset, txContent->opDetails[0]);
    PRINTF("account id: %s\n", txContent->opDetails[0]);
    offset += 32;

    uint64_t amount = readUInt64Block(buffer + offset);
    print_amount(amount, "XLM", txContent->opDetails[1]);
    PRINTF("starting balance: %s\n", txContent->opDetails[1]);
    offset += 8;

    return offset;
}

uint16_t parsePaymentOpXdr(uint8_t *buffer, tx_content_t *txContent) {
    txContent->opType = OPERATION_TYPE_PAYMENT;

    uint32_t destinationAccountType = readUInt32Block(buffer);
    if (destinationAccountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c27);
    }
    uint16_t offset = 4;

    print_public_key(buffer + offset, txContent->opDetails[1]);
    PRINTF("destination: %s\n", txContent->opDetails[1]);
    offset += 32;

    char asset[13];
    offset += parseAsset(buffer + offset, asset, NULL);

    uint64_t amount = readUInt64Block(buffer + offset);
    print_amount(amount, asset, txContent->opDetails[0]);
    PRINTF("amount: %s\n", txContent->opDetails[0]);
    offset += 8;

    return offset;
}

uint16_t parsePathPaymentOpXdr(uint8_t *buffer, tx_content_t *txContent) {
    txContent->opType = OPERATION_TYPE_PATH_PAYMENT;

    char asset[13];
    uint16_t offset = parseAsset(buffer, asset, NULL);

    uint64_t send = readUInt64Block(buffer + offset);
    print_amount(send, asset, txContent->opDetails[0]);
    PRINTF("send: %s\n", txContent->opDetails[0]);
    offset += 8;

    uint32_t destinationAccountType = readUInt32Block(buffer + offset);
    if (destinationAccountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c2b);
    }
    offset += 4;

    print_public_key(buffer + offset, txContent->opDetails[1]);
    PRINTF("destination: %s\n", txContent->opDetails[1]);
    offset += 32;

    offset += parseAsset(buffer + offset, asset, NULL);

    uint64_t receive = readUInt64Block(buffer + offset);
    print_amount(receive, asset, txContent->opDetails[2]);
    PRINTF("receive: %s\n", txContent->opDetails[2]);
    offset += 8;

    uint32_t pathArrayLength = readUInt32Block(buffer + offset);
    offset += 4;
    uint8_t i;
    for (i = 0; i < pathArrayLength; i++) {
        offset += parseAsset(buffer + offset, asset, NULL);
        uint8_t len = strlen(txContent->opDetails[3]);
        if (i > 0) {
            strcpy(txContent->opDetails[3]+len, ", ");
            len += 2;
        }
        strcpy(txContent->opDetails[3]+len, asset);
    }
    PRINTF("path: %s\n", txContent->opDetails[3]);

    return offset;
}

uint16_t parseAllowTrustOpXdr(uint8_t *buffer, tx_content_t *txContent) {
    uint32_t trustorAccountType = readUInt32Block(buffer);
    if (trustorAccountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c2b);
    }
    uint16_t offset = 4;

    print_public_key(buffer + offset, txContent->opDetails[1]);
    PRINTF("trustor: %s\n", txContent->opDetails[1]);
    offset += 32;

    uint32_t assetType = readUInt32Block(buffer + offset);
    offset += 4;

    switch (assetType) {
        case ASSET_TYPE_CREDIT_ALPHANUM4: {
            memcpy(txContent->opDetails[0], buffer + offset, 4);
            txContent->opDetails[0][4] = '\0';
            offset += 4;
            break;
        }
        case ASSET_TYPE_CREDIT_ALPHANUM12: {
            memcpy(txContent->opDetails[0], buffer + offset, 12);
            txContent->opDetails[0][12] = '\0';
            offset += 12;
            break;
        }
        default:
            THROW(0x6c28); // unknown asset type
    }
    PRINTF("asset: %s\n", txContent->opDetails[0]);

    if (readUInt32Block(buffer + offset)) {
        txContent->opType = OPERATION_TYPE_ALLOW_TRUST;
    } else {
        txContent->opType = OPERATION_TYPE_REVOKE_TRUST;
    }

    return offset;
}

uint16_t parseAccountMergeOpXdr(uint8_t *buffer, tx_content_t *txContent) {
    txContent->opType = OPERATION_TYPE_ACCOUNT_MERGE;

    uint32_t destinationAccountType = readUInt32Block(buffer);
    if (destinationAccountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c2b);
    }
    uint16_t offset = 4;

    print_public_key(buffer + offset, txContent->opDetails[0]);
    PRINTF("destination: %s\n", txContent->opDetails[0]);
    offset += 32;

    return offset;
}

uint16_t parseManageDataOpXdr(uint8_t *buffer, tx_content_t *txContent) {

    uint32_t size = readUInt32Block(buffer);
    if (size > DATA_NAME_MAX_SIZE) {
        THROW(0x6c2f);
    }
    uint16_t offset = 4;

    char dataName[size];
    memcpy(dataName, buffer + offset, size);
    dataName[size] = '\0';
    print_summary(dataName, txContent->opDetails[0]);
    PRINTF("data name: %s\n", txContent->opDetails[0]);
    checkPadding(buffer + offset, size, numBytes(size)); // security check
    offset += numBytes(size);

    uint32_t hasValue = readUInt32Block(buffer + offset);
    offset += 4;

    if (hasValue) {
        size = readUInt32Block(buffer + offset);
        if (size > DATA_VALUE_MAX_SIZE) {
            THROW(0x6c2f);
        }
        offset += 4;
        strcpy(txContent->opDetails[1], "<binary data>");
        txContent->opType = OPERATION_TYPE_SET_DATA;
        checkPadding(buffer + offset, size, numBytes(size)); // security check
        offset += numBytes(size);
    } else {
        txContent->opType = OPERATION_TYPE_REMOVE_DATA;
    }
    return offset;
}

uint16_t parseOfferOpXdr(uint8_t *buffer, tx_content_t *txContent, uint32_t opType) {
    char selling[13];
    uint16_t offset = parseAsset(buffer, selling, NULL);

    offset += parseAsset(buffer + offset, txContent->opDetails[1], NULL);
    PRINTF("buying: %s\n", txContent->opDetails[1]);

    uint64_t amount  = readUInt64Block(buffer + offset);
    if (amount > 0) {
        print_amount(amount, selling, txContent->opDetails[3]);
    }
    PRINTF("selling: %s\n", txContent->opDetails[3]);
    offset += 8;

    uint32_t numerator = readUInt32Block(buffer + offset);
    offset += 4;

    uint32_t denominator = readUInt32Block(buffer + offset);
    offset += 4;

    uint64_t price = ((uint64_t)numerator * 10000000) / denominator;
    print_amount(price, NULL, txContent->opDetails[2]);
    PRINTF("price: %s\n", txContent->opDetails[2]);

    if (opType == XDR_OPERATION_TYPE_MANAGE_OFFER) {
        uint64_t offerId = readUInt64Block(buffer + offset);
        offset += 8;
        if (offerId == 0) {
            txContent->opType = OPERATION_TYPE_CREATE_OFFER;
            strcpy(txContent->opDetails[0], "non-passive");
        } else {
            print_int(offerId, txContent->opDetails[0]);
            if (amount == 0) {
                txContent->opType = OPERATION_TYPE_REMOVE_OFFER;
            } else {
                txContent->opType = OPERATION_TYPE_CHANGE_OFFER;
            }
        }
    } else {
        txContent->opType = OPERATION_TYPE_CREATE_OFFER;
        strcpy(txContent->opDetails[0], "passive");
    }

    return offset;
}

uint16_t parseChangeTrustOpXdr(uint8_t *buffer, tx_content_t *txContent) {
    char code[13];
    char issuer[9];
    uint16_t offset = parseAsset(buffer, code, issuer);
    print_asset(code, issuer, txContent->opDetails[0]);
    PRINTF("asset: %s\n", txContent->opDetails[0]);

    uint64_t limit = readUInt64Block(buffer + offset);
    offset += 8;
    if (limit == 0) {
        txContent->opType = OPERATION_TYPE_REMOVE_TRUST;
    } else {
        if (limit == 9223372036854775807) {
            strcpy(txContent->opDetails[1], "max");
        } else {
            print_amount(limit, NULL, txContent->opDetails[1]);
        }
        PRINTF("limit: %s\n", txContent->opDetails[1]);
        txContent->opType = OPERATION_TYPE_CHANGE_TRUST;
    }
    return offset;
}

uint8_t printBits(uint8_t *buffer, char *out, char *prefix) {
    uint32_t bitsPresent = readUInt32Block(buffer);
    buffer += 4;
    if (bitsPresent) {
        uint32_t bits = readUInt32Block(buffer);
        buffer += 4;
        if (bits) {
            uint8_t i = strlen(out);
            if (i > 0) {
                out[i++] = ';';
                out[i++] = ' ';
            }
            strcpy(out+i, prefix);
            i += strlen(prefix);
            print_bits(bits, out+i);
        }
        return 8;
    } else {
        return 4;
    }
}

uint8_t printInt(uint8_t *buffer, char *out, char *prefix) {
    uint32_t intPresent = readUInt32Block(buffer);
    buffer += 4;
    if (intPresent) {
        uint32_t n = readUInt32Block(buffer);
        buffer += 4;
        uint8_t i = strlen(out);
        if (i > 0) {
            out[i++] = ';';
            out[i++] = ' ';
        }
        strcpy(out+i, prefix);
        i += strlen(prefix);
        print_int(n, out+i);
        return 8;
    } else {
        return 4;
    }
}

uint16_t parseSetOptionsOpXdr(uint8_t *buffer, tx_content_t *txContent) {
    txContent->opType = OPERATION_TYPE_SET_OPTIONS;

    uint32_t inflationDestinationPresent = readUInt32Block(buffer);
    uint16_t offset = 4;
    if (inflationDestinationPresent) {
        uint32_t inflationDestinationAccountType = readUInt32Block(buffer + offset);
        if (inflationDestinationAccountType != PUBLIC_KEY_TYPE_ED25519) {
            THROW(0x6c27);
        }
        offset += 4;
        print_public_key(buffer + offset, txContent->opDetails[0]);
        offset += 32;
    }
    PRINTF("inflation destination: %s\n", txContent->opDetails[0]);

    offset += printBits(buffer + offset, txContent->opDetails[1], "clear: ");
    offset += printBits(buffer + offset, txContent->opDetails[1], "set: ");
    PRINTF("flags: %s\n", txContent->opDetails[1]);

    offset += printInt(buffer + offset, txContent->opDetails[2], "master weight: ");
    offset += printInt(buffer + offset, txContent->opDetails[2], "low: ");
    offset += printInt(buffer + offset, txContent->opDetails[2], "med: ");
    offset += printInt(buffer + offset, txContent->opDetails[2], "high: ");
    PRINTF("thresholds: %s\n", txContent->opDetails[2]);

    uint32_t homeDomainPresent = readUInt32Block(buffer + offset);
    offset += 4;
    if (homeDomainPresent) {
        uint32_t size = readUInt32Block(buffer + offset);
        if (size > HOME_DOMAIN_MAX_SIZE) {
            THROW(0x6c2f);
        }
        offset += 4;
        memcpy(txContent->opDetails[3], buffer + offset, size);
        txContent->opDetails[3][size] = '\0';
        checkPadding(buffer + offset, size, numBytes(size)); // security check
        offset += numBytes(size);
    }
    PRINTF("home domain: %s\n", txContent->opDetails[3]);

    uint32_t signerPresent = readUInt32Block(buffer + offset);
    offset += 4;
    if (signerPresent) {
        uint32_t signerType = readUInt32Block(buffer + offset);
        offset += 4;

        switch (signerType) {
            case SIGNER_KEY_TYPE_ED25519: {
                strcpy(txContent->opDetails[4], "pk: ");
                char signer[57];
                public_key_to_address(buffer + offset, signer);
                print_summary(signer, txContent->opDetails[4]+strlen(txContent->opDetails[4]));
                break;
            }
            case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
                strcpy(txContent->opDetails[4], "pre-auth: ");
                print_hash_summary(buffer + offset, txContent->opDetails[4]+strlen(txContent->opDetails[4]));
                break;
            }
            case SIGNER_KEY_TYPE_HASH_X: {
                strcpy(txContent->opDetails[4], "hash(x): ");
                print_hash_summary(buffer + offset, txContent->opDetails[4]+strlen(txContent->opDetails[4]));
                break;
            }
            default: THROW(0x6cdd);
        }
        offset += 32;

        uint32_t weight = readUInt32Block(buffer + offset);
        strcpy(txContent->opDetails[4]+strlen(txContent->opDetails[4]), "; weight: ");
        print_int(weight, txContent->opDetails[4]+strlen(txContent->opDetails[4]));
        PRINTF("signer: %s\n", txContent->opDetails[4]);
        offset += 4;
    }
    return offset;
}

uint16_t parseOpXdr(uint8_t *buffer, tx_content_t *txContent) {
    os_memset(txContent->opDetails, 0, sizeof(txContent->opDetails));
    os_memset(txContent->opSource, 0, sizeof(txContent->opSource));

    txContent->opIdx += 1;

    uint32_t hasAccountId = readUInt32Block(buffer);
    uint16_t offset = 4;
    if (hasAccountId) {
        uint32_t sourceAccountType = readUInt32Block(buffer + offset);
        if (sourceAccountType != PUBLIC_KEY_TYPE_ED25519) {
            THROW(0x6c2b);
        }
        offset += 4;
        char tmp[57];
        public_key_to_address(buffer+offset, tmp);
        print_short_summary(tmp, txContent->opSource);
        PRINTF("operation source: %s\n", txContent->opSource);
        offset += 32;
    }
    uint32_t opType = readUInt32Block(buffer+offset);
    offset += 4;
    switch (opType) {
        case XDR_OPERATION_TYPE_CREATE_ACCOUNT: {
            return parseCreateAccountOpXdr(buffer + offset, txContent) + offset;
        }
        case XDR_OPERATION_TYPE_PAYMENT: {
            return parsePaymentOpXdr(buffer + offset, txContent) + offset;
        }
        case XDR_OPERATION_TYPE_PATH_PAYMENT: {
            return parsePathPaymentOpXdr(buffer + offset, txContent) + offset;
        }
        case XDR_OPERATION_TYPE_CREATE_PASSIVE_OFFER:
        case XDR_OPERATION_TYPE_MANAGE_OFFER: {
            return parseOfferOpXdr(buffer + offset, txContent, opType) + offset;
        }
        case XDR_OPERATION_TYPE_SET_OPTIONS: {
            return parseSetOptionsOpXdr(buffer + offset, txContent) + offset;
        }
        case XDR_OPERATION_TYPE_CHANGE_TRUST: {
            return parseChangeTrustOpXdr(buffer + offset, txContent) + offset;
        }
        case XDR_OPERATION_TYPE_ALLOW_TRUST: {
            return parseAllowTrustOpXdr(buffer + offset, txContent) + offset;
        }
        case XDR_OPERATION_TYPE_ACCOUNT_MERGE: {
            return parseAccountMergeOpXdr(buffer + offset, txContent) + offset;
        }
        case XDR_OPERATION_TYPE_INFLATION: {
            txContent->opType = OPERATION_TYPE_INFLATION;
            strcpy(txContent->opDetails[0], "Run now");
            break;
        }
        case XDR_OPERATION_TYPE_MANAGE_DATA: {
            return parseManageDataOpXdr(buffer + offset, txContent) + offset;
        }
        default: THROW(0x6c24);
    }
    return offset;
}

uint16_t parseTxXdr(uint8_t *buffer, tx_content_t *txContent, uint16_t offset) {
    if (offset == 0) {
        os_memset(txContent->txDetails, 0, sizeof(txContent->txDetails));

        print_network_id(buffer, txContent->txDetails[2]);
        offset += 32;

        offset += 4; // skip envelopeType

        uint32_t sourceAccountType = readUInt32Block(buffer + offset);
        if (sourceAccountType != PUBLIC_KEY_TYPE_ED25519) {
            THROW(0x6c26);
        }
        offset += 4;

        char tmp[57];
        public_key_to_address(buffer + offset, tmp);
        print_short_summary(tmp, txContent->txDetails[3]);
        PRINTF("transaction source: %s\n", txContent->txDetails[3]);
        offset += 32;

        uint32_t fee = readUInt32Block(buffer + offset);
        print_amount(fee, "XLM", txContent->txDetails[1]);
        PRINTF("fee: %s\n", txContent->txDetails[1]);
        offset += 4;

        offset += 8; // skip seqNum
        offset += skipTimeBounds(buffer + offset);
        offset += parseMemo(buffer + offset, txContent);
        PRINTF("memo: %s\n", txContent->txDetails[0]);

        txContent->opCount = readUInt32Block(buffer + offset);
        PRINTF("op count: %d\n\n", txContent->opCount);

        if (txContent->opCount > MAX_OPS) {
            THROW(0x6c30);
        }

        offset += 4;

        txContent->opIdx = 0;
    }
//    printHexBlocks(buffer+offset, 20);
    offset = parseOpXdr(buffer + offset, txContent) + offset;
    if (txContent->opCount == txContent->opIdx) {
        return 0; // start from beginning next time
    }
    return offset;
}
