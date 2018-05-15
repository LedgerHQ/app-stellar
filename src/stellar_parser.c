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

static const uint8_t NETWORK_ID_PUBLIC_HASH[64] = {0x7a, 0xc3, 0x39, 0x97, 0x54, 0x4e, 0x31, 0x75,
                                                   0xd2, 0x66, 0xbd, 0x02, 0x24, 0x39, 0xb2, 0x2c,
                                                   0xdb, 0x16, 0x50, 0x8c, 0x01, 0x16, 0x3f, 0x26,
                                                   0xe5, 0xcb, 0x2a, 0x3e, 0x10, 0x45, 0xa9, 0x79};

static const uint8_t NETWORK_ID_TEST_HASH[64] = {0xce, 0xe0, 0x30, 0x2d, 0x59, 0x84, 0x4d, 0x32,
                                                 0xbd, 0xca, 0x91, 0x5c, 0x82, 0x03, 0xdd, 0x44,
                                                 0xb3, 0x3f, 0xbb, 0x7e, 0xdc, 0x19, 0x05, 0x1e,
                                                 0xa3, 0x7a, 0xbe, 0xdf, 0x28, 0xec, 0xd4, 0x72};


static const char * NETWORK_NAMES[3] = { "Public", "Test", "Unknown" };

uint32_t read_uint32_block(uint8_t *buffer) {
    return buffer[3] + (buffer[2] << 8) + (buffer[1] <<  16) + (buffer[0] << 24);
}

uint64_t read_uint64_block(uint8_t *buffer) {
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

void check_padding(uint8_t *buffer, uint8_t offset, uint8_t length) {
    uint8_t i;
    for (i = 0; i < length - offset; i++) {
        if (buffer[offset + i] != 0x00) {
             THROW(0x6c2e);
        }
    }
}

uint8_t parse_time_bounds(uint8_t *buffer, tx_content_t *txContent) {
    uint32_t timeBounds = read_uint32_block(buffer);
    if (timeBounds != 0) {
        txContent->timeBounds = true;
        return 4 + 8 + 8;
    } else {
        txContent->timeBounds = false;
        return 4;
    }
}

uint8_t parse_memo(uint8_t *buffer, tx_content_t *txContent) {
    uint8_t memoType = (uint8_t) read_uint32_block(buffer);
    buffer += 4;
    switch (memoType) {
        case MEMO_TYPE_NONE:
            memcpy(txContent->txDetails[0], "[none]", 7);
            return 4;
        case MEMO_TYPE_ID:
            print_uint(read_uint64_block(buffer), txContent->txDetails[0]);
            return 4 + 8; // type + value
        case MEMO_TYPE_TEXT: {
            uint8_t size = read_uint32_block(buffer);
            if (size > MEMO_TEXT_MAX_SIZE) {
                THROW(0x6c2f);
            }
            buffer += 4;
            memcpy(txContent->txDetails[0], buffer, size);
            txContent->txDetails[0][size] = '\0';
            check_padding(buffer, size, numBytes(size)); // security check
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

uint8_t parse_asset(uint8_t *buffer, char *code, char *issuer, char *nativeCode) {
    uint32_t assetType = read_uint32_block(buffer);
    buffer += 4;
    switch (assetType) {
        case ASSET_TYPE_NATIVE: {
            strcpy(code, nativeCode);
            return 4; // type
        }
        case ASSET_TYPE_CREDIT_ALPHANUM4: {
            memcpy(code, buffer, 4);
            code[4] = '\0';
            buffer += 4;
            uint32_t accountType = read_uint32_block(buffer);
            if (accountType != PUBLIC_KEY_TYPE_ED25519) {
                THROW(0x6c23);
            }
            buffer += 4;
            if (issuer) {
                print_public_key(buffer, issuer, 3, 4);
            }
            return 4 + 4 + 36; // type + code4 + accountId
        }
        case ASSET_TYPE_CREDIT_ALPHANUM12: {
            memcpy(code, buffer, 12);
            code[12] = '\0';
            buffer += 12;
            uint32_t accountType = read_uint32_block(buffer);
            if (accountType != PUBLIC_KEY_TYPE_ED25519) {
                THROW(0x6c22);
            }
            buffer += 4;
            if (issuer) {
                print_public_key(buffer, issuer, 3, 4);
            }
            return 4 + 12 + 36; // type + code12 + accountId
        }
        default:
            THROW(0x6c28); // unknown asset type
    }
}

char *get_native_asset_code(tx_content_t *txContent) {
    if (txContent->network == NETWORK_TYPE_UNKNOWN) {
        return "native";
    }
    return "XLM";
}

uint16_t parse_create_account(uint8_t *buffer, tx_content_t *txContent) {
    txContent->opType = OPERATION_TYPE_CREATE_ACCOUNT;

    uint32_t accountType = read_uint32_block(buffer);
    if (accountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c27);
    }
    uint16_t offset = 4;

    print_public_key_long(buffer + offset, txContent->opDetails[0]);
    PRINTF("account id: %s\n", txContent->opDetails[0]);
    offset += 32;

    uint64_t amount = read_uint64_block(buffer + offset);
    print_amount(amount, get_native_asset_code(txContent), txContent->opDetails[1]);
    PRINTF("starting balance: %s\n", txContent->opDetails[1]);
    offset += 8;

    return offset;
}

uint16_t parse_payment(uint8_t *buffer, tx_content_t *txContent) {
    txContent->opType = OPERATION_TYPE_PAYMENT;

    uint32_t accountType = read_uint32_block(buffer);
    if (accountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c27);
    }
    uint16_t offset = 4;

    print_public_key_long(buffer + offset, txContent->opDetails[1]);
    PRINTF("destination: %s\n", txContent->opDetails[1]);
    offset += 32;

    char assetCode[13];
    offset += parse_asset(buffer + offset, assetCode, NULL, get_native_asset_code(txContent));

    uint64_t amount = read_uint64_block(buffer + offset);
    print_amount(amount, assetCode, txContent->opDetails[0]);
    PRINTF("amount: %s\n", txContent->opDetails[0]);
    offset += 8;

    return offset;
}

uint16_t parse_path_payment(uint8_t *buffer, tx_content_t *txContent) {
    txContent->opType = OPERATION_TYPE_PATH_PAYMENT;

    char assetCode[13];
    uint16_t offset = parse_asset(buffer, assetCode, NULL, get_native_asset_code(txContent));

    uint64_t send = read_uint64_block(buffer + offset);
    print_amount(send, assetCode, txContent->opDetails[0]);
    PRINTF("send: %s\n", txContent->opDetails[0]);
    offset += 8;

    uint32_t accountType = read_uint32_block(buffer + offset);
    if (accountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c2b);
    }
    offset += 4;

    print_public_key_long(buffer + offset, txContent->opDetails[1]);
    PRINTF("destination: %s\n", txContent->opDetails[1]);
    offset += 32;

    offset += parse_asset(buffer + offset, assetCode, NULL, get_native_asset_code(txContent));

    uint64_t receive = read_uint64_block(buffer + offset);
    print_amount(receive, assetCode, txContent->opDetails[2]);
    PRINTF("receive: %s\n", txContent->opDetails[2]);
    offset += 8;

    uint32_t pathArrayLength = read_uint32_block(buffer + offset);
    offset += 4;
    uint8_t i;
    for (i = 0; i < pathArrayLength; i++) {
        offset += parse_asset(buffer + offset, assetCode, NULL, get_native_asset_code(txContent));
        uint8_t len = strlen(txContent->opDetails[3]);
        if (i > 0) {
            strcpy(txContent->opDetails[3]+len, ", ");
            len += 2;
        }
        strcpy(txContent->opDetails[3]+len, assetCode);
    }
    PRINTF("path: %s\n", txContent->opDetails[3]);

    return offset;
}

uint16_t parse_allow_trust(uint8_t *buffer, tx_content_t *txContent) {
    uint32_t trustorAccountType = read_uint32_block(buffer);
    if (trustorAccountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c2b);
    }
    uint16_t offset = 4;

    print_public_key_short(buffer + offset, txContent->opDetails[1]);
    PRINTF("trustor: %s\n", txContent->opDetails[1]);
    offset += 32;

    uint32_t assetType = read_uint32_block(buffer + offset);
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

    if (read_uint32_block(buffer + offset)) {
        txContent->opType = OPERATION_TYPE_ALLOW_TRUST;
    } else {
        txContent->opType = OPERATION_TYPE_REVOKE_TRUST;
    }

    return offset;
}

uint16_t parse_account_merge(uint8_t *buffer, tx_content_t *txContent) {
    txContent->opType = OPERATION_TYPE_ACCOUNT_MERGE;

    if (txContent->opSource[0] != '\0') {
        strcpy(txContent->opDetails[0], txContent->opSource);
        txContent->opSource[0] = '\0'; // don't show separately
    } else {
        strcpy(txContent->opDetails[0], txContent->txDetails[3]);
    }

    uint32_t accountType = read_uint32_block(buffer);
    if (accountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c2b);
    }
    uint16_t offset = 4;

    print_public_key_long(buffer + offset, txContent->opDetails[1]);
    PRINTF("destination: %s\n", txContent->opDetails[1]);
    offset += 32;

    return offset;
}

uint16_t parse_manage_data(uint8_t *buffer, tx_content_t *txContent) {

    uint32_t size = read_uint32_block(buffer);
    if (size > DATA_NAME_MAX_SIZE) {
        THROW(0x6c2f);
    }
    uint16_t offset = 4;

    char dataName[size];
    memcpy(dataName, buffer + offset, size);
    dataName[size] = '\0';
    print_summary(dataName, txContent->opDetails[0], 12, 12);
    PRINTF("data name: %s\n", txContent->opDetails[0]);
    check_padding(buffer + offset, size, numBytes(size)); // security check
    offset += numBytes(size);

    uint32_t hasValue = read_uint32_block(buffer + offset);
    offset += 4;

    if (hasValue) {
        size = read_uint32_block(buffer + offset);
        if (size > DATA_VALUE_MAX_SIZE) {
            THROW(0x6c2f);
        }
        offset += 4;
        strcpy(txContent->opDetails[1], "<binary data>");
        txContent->opType = OPERATION_TYPE_SET_DATA;
        check_padding(buffer + offset, size, numBytes(size)); // security check
        offset += numBytes(size);
    } else {
        txContent->opType = OPERATION_TYPE_REMOVE_DATA;
    }
    return offset;
}

uint16_t parse_offer(uint8_t *buffer, tx_content_t *txContent, uint32_t opType) {
    char selling[13];
    uint16_t offset = parse_asset(buffer, selling, NULL, get_native_asset_code(txContent));

    char buyingCode[13];
    char buyingIssuer[10];
    offset += parse_asset(buffer + offset, buyingCode, buyingIssuer, get_native_asset_code(txContent));
    print_asset(buyingCode, buyingIssuer, txContent->opDetails[1]);
    PRINTF("buying: %s\n", txContent->opDetails[1]);

    uint64_t amount  = read_uint64_block(buffer + offset);
    if (amount > 0) {
        print_amount(amount, selling, txContent->opDetails[3]);
    }
    PRINTF("selling: %s\n", txContent->opDetails[3]);
    offset += 8;

    uint32_t numerator = read_uint32_block(buffer + offset);
    offset += 4;

    uint32_t denominator = read_uint32_block(buffer + offset);
    offset += 4;

    uint64_t price = ((uint64_t)numerator * 10000000) / denominator;
    print_amount(price, buyingCode, txContent->opDetails[2]);
    PRINTF("price: %s\n", txContent->opDetails[2]);

    if (opType == XDR_OPERATION_TYPE_MANAGE_OFFER) {
        uint64_t offerId = read_uint64_block(buffer + offset);
        offset += 8;
        if (offerId == 0) {
            txContent->opType = OPERATION_TYPE_CREATE_OFFER;
            strcpy(txContent->opDetails[0], "non-passive");
        } else {
            print_uint(offerId, txContent->opDetails[0]);
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

uint16_t parse_change_trust(uint8_t *buffer, tx_content_t *txContent) {
    char code[13];
    char issuer[9];
    uint16_t offset = parse_asset(buffer, code, issuer, get_native_asset_code(txContent));
    print_asset(code, issuer, txContent->opDetails[0]);
    PRINTF("asset: %s\n", txContent->opDetails[0]);

    uint64_t limit = read_uint64_block(buffer + offset);
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

uint8_t parse_bits(uint8_t *buffer, char *out, char *prefix) {
    uint32_t bitsPresent = read_uint32_block(buffer);
    buffer += 4;
    if (bitsPresent) {
        uint32_t bits = read_uint32_block(buffer);
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

uint8_t parse_int(uint8_t *buffer, char *out, char *prefix) {
    uint32_t intPresent = read_uint32_block(buffer);
    buffer += 4;
    if (intPresent) {
        uint32_t n = read_uint32_block(buffer);
        buffer += 4;
        uint8_t i = strlen(out);
        if (i > 0) {
            out[i++] = ';';
            out[i++] = ' ';
        }
        strcpy(out+i, prefix);
        i += strlen(prefix);
        print_uint(n, out+i);
        return 8;
    } else {
        return 4;
    }
}

uint16_t parse_set_options(uint8_t *buffer, tx_content_t *txContent) {
    txContent->opType = OPERATION_TYPE_SET_OPTIONS;

    uint32_t inflationDestinationPresent = read_uint32_block(buffer);
    uint16_t offset = 4;
    if (inflationDestinationPresent) {
        uint32_t accountType = read_uint32_block(buffer + offset);
        if (accountType != PUBLIC_KEY_TYPE_ED25519) {
            THROW(0x6c27);
        }
        offset += 4;
        print_public_key_short(buffer + offset, txContent->opDetails[0]);
        offset += 32;
    }
    PRINTF("inflation destination: %s\n", txContent->opDetails[0]);

    offset += parse_bits(buffer + offset, txContent->opDetails[1], "clear: ");
    offset += parse_bits(buffer + offset, txContent->opDetails[1], "set: ");
    PRINTF("flags: %s\n", txContent->opDetails[1]);

    offset += parse_int(buffer + offset, txContent->opDetails[2], "master weight: ");
    offset += parse_int(buffer + offset, txContent->opDetails[2], "low: ");
    offset += parse_int(buffer + offset, txContent->opDetails[2], "med: ");
    offset += parse_int(buffer + offset, txContent->opDetails[2], "high: ");
    PRINTF("thresholds: %s\n", txContent->opDetails[2]);

    uint32_t homeDomainPresent = read_uint32_block(buffer + offset);
    offset += 4;
    if (homeDomainPresent) {
        uint32_t size = read_uint32_block(buffer + offset);
        if (size > HOME_DOMAIN_MAX_SIZE) {
            THROW(0x6c2f);
        }
        offset += 4;
        memcpy(txContent->opDetails[3], buffer + offset, size);
        txContent->opDetails[3][size] = '\0';
        check_padding(buffer + offset, size, numBytes(size)); // security check
        offset += numBytes(size);
    }
    PRINTF("home domain: %s\n", txContent->opDetails[3]);

    uint32_t signerPresent = read_uint32_block(buffer + offset);
    offset += 4;
    if (signerPresent) {
        uint32_t signerType = read_uint32_block(buffer + offset);
        offset += 4;

        switch (signerType) {
            case SIGNER_KEY_TYPE_ED25519: {
                strcpy(txContent->opDetails[4], "pk: ");
                print_public_key(buffer + offset, txContent->opDetails[4]+4, 12, 12);
                break;
            }
            case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
                strcpy(txContent->opDetails[4], "pre-auth: ");
                print_hash_summary(buffer + offset, txContent->opDetails[4]+10);
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

        uint32_t weight = read_uint32_block(buffer + offset);
        strcpy(txContent->opDetails[4]+strlen(txContent->opDetails[4]), "; weight: ");
        print_uint(weight, txContent->opDetails[4]+strlen(txContent->opDetails[4]));
        PRINTF("signer: %s\n", txContent->opDetails[4]);
        offset += 4;
    }
    return offset;
}

uint16_t parse_bump_sequence(uint8_t *buffer, tx_content_t *txContent) {
    txContent->opType = OPERATION_TYPE_BUMP_SEQUENCE;

    int64_t bumpTo = (int64_t) read_uint64_block(buffer);
    print_int(bumpTo, txContent->opDetails[0]);

    return 8;
}

uint16_t parse_op_xdr(uint8_t *buffer, tx_content_t *txContent) {
    MEMCLEAR(txContent->opDetails);
    MEMCLEAR(txContent->opSource);

    txContent->opIdx += 1;

    uint32_t hasAccountId = read_uint32_block(buffer);
    uint16_t offset = 4;
    if (hasAccountId) {
        uint32_t accountType = read_uint32_block(buffer + offset);
        if (accountType != PUBLIC_KEY_TYPE_ED25519) {
            THROW(0x6c2b);
        }
        offset += 4;
        print_public_key_short(buffer+offset, txContent->opSource);
        PRINTF("operation source: %s\n", txContent->opSource);
        offset += 32;
    }
    uint32_t opType = read_uint32_block(buffer+offset);
    offset += 4;
    switch (opType) {
        case XDR_OPERATION_TYPE_CREATE_ACCOUNT: {
            return parse_create_account(buffer + offset, txContent) + offset;
        }
        case XDR_OPERATION_TYPE_PAYMENT: {
            return parse_payment(buffer + offset, txContent) + offset;
        }
        case XDR_OPERATION_TYPE_PATH_PAYMENT: {
            return parse_path_payment(buffer + offset, txContent) + offset;
        }
        case XDR_OPERATION_TYPE_CREATE_PASSIVE_OFFER:
        case XDR_OPERATION_TYPE_MANAGE_OFFER: {
            return parse_offer(buffer + offset, txContent, opType) + offset;
        }
        case XDR_OPERATION_TYPE_SET_OPTIONS: {
            return parse_set_options(buffer + offset, txContent) + offset;
        }
        case XDR_OPERATION_TYPE_CHANGE_TRUST: {
            return parse_change_trust(buffer + offset, txContent) + offset;
        }
        case XDR_OPERATION_TYPE_ALLOW_TRUST: {
            return parse_allow_trust(buffer + offset, txContent) + offset;
        }
        case XDR_OPERATION_TYPE_ACCOUNT_MERGE: {
            return parse_account_merge(buffer + offset, txContent) + offset;
        }
        case XDR_OPERATION_TYPE_INFLATION: {
            txContent->opType = OPERATION_TYPE_INFLATION;
            strcpy(txContent->opDetails[0], "Run now");
            break;
        }
        case XDR_OPERATION_TYPE_MANAGE_DATA: {
            return parse_manage_data(buffer + offset, txContent) + offset;
        }
        case XDR_OPERATION_TYPE_BUMP_SEQUENCE: {
            return parse_bump_sequence(buffer + offset, txContent) + offset;
        }
        default: THROW(0x6c24);
    }
    return offset;
}

uint16_t parse_tx_xdr(uint8_t *buffer, tx_content_t *txContent, uint16_t offset) {
    if (offset == 0) {
        MEMCLEAR(txContent->txDetails);

        if (memcmp(buffer, NETWORK_ID_PUBLIC_HASH, 32) == 0) {
            txContent->network = NETWORK_TYPE_PUBLIC;
        } else if (memcmp(buffer, NETWORK_ID_TEST_HASH, 32) == 0) {
            txContent->network = NETWORK_TYPE_TEST;
        } else {
            txContent->network = NETWORK_TYPE_UNKNOWN;
        }
        strcpy(txContent->txDetails[2], ((char *)PIC(NETWORK_NAMES[txContent->network])));
        offset += 32;

        offset += 4; // skip envelopeType

        uint32_t accountType = read_uint32_block(buffer + offset);
        if (accountType != PUBLIC_KEY_TYPE_ED25519) {
            THROW(0x6c26);
        }
        offset += 4;

        print_public_key_short(buffer + offset, txContent->txDetails[3]);
        PRINTF("transaction source: %s\n", txContent->txDetails[3]);
        offset += 32;

        uint32_t fee = read_uint32_block(buffer + offset);
        print_amount(fee, get_native_asset_code(txContent), txContent->txDetails[1]);
        PRINTF("fee: %s\n", txContent->txDetails[1]);
        offset += 4;

        offset += 8; // skip seqNum
        offset += parse_time_bounds(buffer + offset, txContent);
        offset += parse_memo(buffer + offset, txContent);
        PRINTF("memo: %s\n", txContent->txDetails[0]);

        txContent->opCount = read_uint32_block(buffer + offset);
        PRINTF("op count: %d\n\n", txContent->opCount);

        if (txContent->opCount > MAX_OPS) {
            THROW(0x6c30);
        }

        offset += 4;

        txContent->opIdx = 0;
    }
//    printHexBlocks(buffer+offset, 20);
    offset = parse_op_xdr(buffer + offset, txContent) + offset;
    if (txContent->opCount == txContent->opIdx) {
        return 0; // start from beginning next time
    }
    return offset;
}
