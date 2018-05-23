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
uint8_t network_id;

uint32_t read_uint32_block(uint8_t *buffer) {
    return buffer[3] + (buffer[2] << 8) + (buffer[1] <<  16) + (buffer[0] << 24);
}

uint64_t read_uint64_block(uint8_t *buffer) {
    uint64_t i1 = buffer[3] + (buffer[2] << 8) + (buffer[1] <<  16) + (buffer[0] << 24);
    buffer += 4;
    uint32_t i2 = buffer[3] + (buffer[2] << 8) + (buffer[1] <<  16) + (buffer[0] << 24);
    return i2 | (i1 << 32);
}

uint8_t num_bytes(uint8_t size) {
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
             THROW(0x6c20);
        }
    }
}

uint8_t parse_network(uint8_t *buffer, tx_details_t *txDetails) {
    if (memcmp(buffer, NETWORK_ID_PUBLIC_HASH, 32) == 0) {
        network_id = txDetails->network = NETWORK_TYPE_PUBLIC;
    } else if (memcmp(buffer, NETWORK_ID_TEST_HASH, 32) == 0) {
        network_id = txDetails->network = NETWORK_TYPE_TEST;
    } else {
        network_id = txDetails->network = NETWORK_TYPE_UNKNOWN;
    }
    return 32;
}

uint8_t parse_time_bounds(uint8_t *buffer, tx_details_t *txDetails) {
    txDetails->hasTimeBounds = (read_uint32_block(buffer)) ? true : false;
    uint8_t offset = 4;
    if (txDetails->hasTimeBounds) {
        txDetails->timeBounds.minTime = read_uint64_block(buffer+offset);
        offset += 8;
        txDetails->timeBounds.maxTime = read_uint64_block(buffer+offset);
        return offset + 8;
    }
    return offset;
}

uint8_t parse_memo(uint8_t *buffer, tx_details_t *txDetails) {
    txDetails->memo.type = (uint8_t) read_uint32_block(buffer);
    buffer += 4;
    switch (txDetails->memo.type) {
        case MEMO_TYPE_NONE:
            strcpy(txDetails->memo.data, "[none]");
            return 4;
        case MEMO_TYPE_ID:
            print_uint(read_uint64_block(buffer), txDetails->memo.data);
            return 4 + 8; // type + value
        case MEMO_TYPE_TEXT: {
            uint8_t size = read_uint32_block(buffer);
            if (size > MEMO_TEXT_MAX_SIZE) {
                THROW(0x6c20);
            }
            buffer += 4;
            memcpy(txDetails->memo.data, buffer, size);
            txDetails->memo.data[size] = '\0';
            check_padding(buffer, size, num_bytes(size)); // security check
            return 4 + 4 + num_bytes(size); // type + size + text
        }
        case MEMO_TYPE_HASH:
        case MEMO_TYPE_RETURN:
            print_binary(buffer, txDetails->memo.data, 32);
            return 4 + 32; // type + hash block
        default:
            THROW(0x6c20); // unknown memo type
            return 0;
    }
}

uint8_t parse_asset(uint8_t *buffer, asset_t *asset) {
    asset->type = read_uint32_block(buffer);
    buffer += 4;
    switch (asset->type) {
        case ASSET_TYPE_NATIVE: {
            print_native_asset_code(network_id, asset->code);
            return 4; // type
        }
        case ASSET_TYPE_CREDIT_ALPHANUM4: {
            memcpy(asset->code, buffer, 4);
            asset->code[4] = '\0';
            buffer += 4;
            uint32_t accountType = read_uint32_block(buffer);
            if (accountType != PUBLIC_KEY_TYPE_ED25519) {
                THROW(0x6c20);
            }
            buffer += 4;
            asset->issuer = buffer;
            return 4 + 4 + 36; // type + code4 + accountId
        }
        case ASSET_TYPE_CREDIT_ALPHANUM12: {
            memcpy(asset->code, buffer, 12);
            asset->code[12] = '\0';
            buffer += 12;
            uint32_t accountType = read_uint32_block(buffer);
            if (accountType != PUBLIC_KEY_TYPE_ED25519) {
                THROW(0x6c20);
            }
            buffer += 4;
            asset->issuer = buffer;
            return 4 + 12 + 36; // type + code12 + accountId
        }
        default:
            THROW(0x6c20); // unknown asset type
            return 0;
    }
}

uint16_t parse_create_account(uint8_t *buffer, create_account_op_t *createAccount) {
    uint32_t accountType = read_uint32_block(buffer);
    if (accountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c20);
    }
    uint16_t offset = 4;

    createAccount->accountId = buffer + offset;
    offset += 32;

    createAccount->amount = read_uint64_block(buffer + offset);

    return offset + 8;
}

uint16_t parse_payment(uint8_t *buffer, payment_op_t *payment) {
    uint32_t accountType = read_uint32_block(buffer);
    if (accountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c20);
    }
    uint16_t offset = 4;

    payment->destination = buffer + offset;
    offset += 32;

    offset += parse_asset(buffer + offset, &payment->asset);

    payment->amount = read_uint64_block(buffer + offset);

    return offset + 8;
}

uint16_t parse_path_payment(uint8_t *buffer, path_payment_op_t *pathPayment) {
    uint16_t offset = parse_asset(buffer, &pathPayment->sourceAsset);

    pathPayment->sendMax = read_uint64_block(buffer + offset);
    offset += 8;

    uint32_t accountType = read_uint32_block(buffer + offset);
    if (accountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c20);
    }
    offset += 4;

    pathPayment->destination = buffer+offset;
    offset += 32;

    offset += parse_asset(buffer + offset, &pathPayment->destAsset);

    pathPayment->destAmount = read_uint64_block(buffer + offset);
    offset += 8;

    pathPayment->pathLen = read_uint32_block(buffer + offset);
    offset += 4;
    uint8_t i;
    for (i = 0; i < pathPayment->pathLen; i++) {
        offset += parse_asset(buffer + offset, &pathPayment->path[i]);
    }

    return offset;
}

uint16_t parse_allow_trust(uint8_t *buffer, allow_trust_op_t *allowTrust) {
    uint32_t trusteeAccountType = read_uint32_block(buffer);
    if (trusteeAccountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c20);
    }
    uint16_t offset = 4;

    allowTrust->trustee = buffer + offset;
    offset += 32;

    uint32_t assetType = read_uint32_block(buffer + offset);
    offset += 4;

    switch (assetType) {
        case ASSET_TYPE_CREDIT_ALPHANUM4: {
            memcpy(allowTrust->assetCode, buffer + offset, 4);
            allowTrust->assetCode[4] = '\0';
            offset += 4;
            break;
        }
        case ASSET_TYPE_CREDIT_ALPHANUM12: {
            memcpy(allowTrust->assetCode, buffer + offset, 12);
            allowTrust->assetCode[12] = '\0';
            offset += 12;
            break;
        }
        default: THROW(0x6c20); // unknown asset type
    }

    allowTrust->authorize = (read_uint32_block(buffer + offset)) ? true : false;

    return offset + 4;
}

uint16_t parse_account_merge(uint8_t *buffer, account_merge_op_t *accountMerge) {
    uint32_t accountType = read_uint32_block(buffer);
    if (accountType != PUBLIC_KEY_TYPE_ED25519) {
        THROW(0x6c20);
    }
    uint16_t offset = 4;

    accountMerge->destination = buffer + offset;

    return offset + 32;
}

uint16_t parse_manage_data(uint8_t *buffer, manage_data_op_t *manageData) {
    uint32_t size = read_uint32_block(buffer);

    if (size > DATA_NAME_MAX_SIZE) {
        THROW(0x6c20);
    }
    uint16_t offset = 4;

    memcpy(manageData->dataName, buffer + offset, size);
    manageData->dataName[size] = '\0';

    check_padding(buffer + offset, size, num_bytes(size)); // security check
    offset += num_bytes(size);

    manageData->hasValue = (read_uint32_block(buffer + offset)) ? true : false;
    offset += 4;

    if (manageData->hasValue) {
        manageData->dataSize = read_uint32_block(buffer + offset);
        if (manageData->dataSize > DATA_VALUE_MAX_SIZE) {
            THROW(0x6c20);
        }
        offset += 4;

        memcpy(manageData->dataValue, buffer+offset, manageData->dataSize);
        check_padding(buffer + offset, manageData->dataSize, num_bytes(size)); // security check
        offset += num_bytes(manageData->dataSize);
    }

    return offset;
}

uint16_t parse_offer(uint8_t *buffer, manage_offer_op_t *manageOffer) {
    uint16_t offset = parse_asset(buffer, &manageOffer->selling);
    offset += parse_asset(buffer + offset, &manageOffer->buying);

    manageOffer->amount  = read_uint64_block(buffer + offset);
    offset += 8;

    manageOffer->price.numerator = read_uint32_block(buffer + offset);
    offset += 4;
    manageOffer->price.denominator = read_uint32_block(buffer + offset);
    offset += 4;

    return offset;
}

uint16_t parse_active_offer(uint8_t *buffer, manage_offer_op_t *manageOffer) {
    manageOffer->active = true;
    uint16_t offset = parse_offer(buffer, manageOffer);
    manageOffer->offerId = read_uint64_block(buffer + offset);
    return offset + 8;
}

uint16_t parse_passive_offer(uint8_t *buffer, manage_offer_op_t *manageOffer) {
    manageOffer->active = false;
    manageOffer->offerId = 0;
    return parse_offer(buffer, manageOffer);
}

uint16_t parse_change_trust(uint8_t *buffer, change_trust_op_t *changeTrust) {
    uint16_t offset = parse_asset(buffer, &changeTrust->asset);
    changeTrust->limit = read_uint64_block(buffer + offset);
    return offset + 8;
}

uint16_t parse_set_options(uint8_t *buffer, set_options_op_t *setOptions) {

    setOptions->inflationDestinationPresent = (read_uint32_block(buffer)) ? true : false;
    uint16_t offset = 4;
    if (setOptions->inflationDestinationPresent) {
        uint32_t accountType = read_uint32_block(buffer + offset);
        if (accountType != PUBLIC_KEY_TYPE_ED25519) {
            THROW(0x6c20);
        }
        offset += 4;
        setOptions->inflationDestination = buffer + offset;
        offset += 32;
    }

    setOptions->clearFlagsPresent = (read_uint32_block(buffer + offset)) ? true : false;
    offset += 4;
    if (setOptions->clearFlagsPresent) {
        setOptions->clearFlags = read_uint32_block(buffer + offset);
        offset += 4;
    }

    setOptions->setFlagsPresent = (read_uint32_block(buffer + offset)) ? true : false;
    offset += 4;
    if (setOptions->setFlagsPresent) {
        setOptions->setFlags = read_uint32_block(buffer + offset);
        offset += 4;
    }

    setOptions->masterWeightPresent = (read_uint32_block(buffer + offset)) ? true : false;
    offset += 4;
    if (setOptions->masterWeightPresent) {
        setOptions->masterWeight = read_uint32_block(buffer + offset);
        offset += 4;
    }

    setOptions->lowThresholdPresent = (read_uint32_block(buffer + offset)) ? true : false;
    offset += 4;
    if (setOptions->lowThresholdPresent) {
        setOptions->lowThreshold = read_uint32_block(buffer + offset);
        offset += 4;
    }

    setOptions->mediumThresholdPresent = (read_uint32_block(buffer + offset)) ? true : false;
    offset += 4;
    if (setOptions->mediumThresholdPresent) {
        setOptions->mediumThreshold = read_uint32_block(buffer + offset);
        offset += 4;
    }

    setOptions->highThresholdPresent = (read_uint32_block(buffer + offset)) ? true : false;
    offset += 4;
    if (setOptions->highThresholdPresent) {
        setOptions->highThreshold = read_uint32_block(buffer + offset);
        offset += 4;
    }

    setOptions->homeDomainPresent = (read_uint32_block(buffer + offset)) ? true : false;
    offset += 4;
    if (setOptions->homeDomainPresent) {
        uint32_t size = read_uint32_block(buffer + offset);
        if (size > HOME_DOMAIN_MAX_SIZE) {
            THROW(0x6c20);
        }
        offset += 4;
        memcpy(setOptions->homeDomain, buffer + offset, size);
        setOptions->homeDomain[size] = '\0';
        check_padding(buffer + offset, size, num_bytes(size)); // security check
        offset += num_bytes(size);
    }

    setOptions->signerPresent = (read_uint32_block(buffer + offset)) ? true : false;
    offset += 4;
    if (setOptions->signerPresent) {
        setOptions->signer.type = read_uint32_block(buffer + offset);
        offset += 4;

        setOptions->signer.data = buffer+offset;
        offset += 32;

        setOptions->signer.weight = read_uint32_block(buffer + offset);
        offset += 4;
    }
    return offset;
}

uint16_t parse_bump_sequence(uint8_t *buffer, bump_sequence_op_t *bumpSequence) {
    bumpSequence->bumpTo = (int64_t) read_uint64_block(buffer);
    return 8;
}

uint16_t parse_op_xdr(uint8_t *buffer, operation_details_t *opDetails) {

    opDetails->sourcePresent = (read_uint32_block(buffer)) ? true : false;
    uint16_t offset = 4;
    if (opDetails->sourcePresent) {
        uint32_t accountType = read_uint32_block(buffer + offset);
        if (accountType != PUBLIC_KEY_TYPE_ED25519) {
            THROW(0x6c20);
        }
        offset += 4;
        opDetails->source = buffer+offset;
        offset += 32;
    }

    opDetails->type = read_uint32_block(buffer+offset);
    offset += 4;
    switch (opDetails->type) {
        case XDR_OPERATION_TYPE_CREATE_ACCOUNT: {
            return parse_create_account(buffer + offset, &opDetails->op.createAccount) + offset;
        }
        case XDR_OPERATION_TYPE_PAYMENT: {
            return parse_payment(buffer + offset, &opDetails->op.payment) + offset;
        }
        case XDR_OPERATION_TYPE_PATH_PAYMENT: {
            return parse_path_payment(buffer + offset, &opDetails->op.pathPayment) + offset;
        }
        case XDR_OPERATION_TYPE_CREATE_PASSIVE_OFFER: {
            return parse_passive_offer(buffer + offset, &opDetails->op.manageOffer) + offset;
        }
        case XDR_OPERATION_TYPE_MANAGE_OFFER: {
            return parse_active_offer(buffer + offset, &opDetails->op.manageOffer) + offset;
        }
        case XDR_OPERATION_TYPE_SET_OPTIONS: {
            return parse_set_options(buffer + offset, &opDetails->op.setOptions) + offset;
        }
        case XDR_OPERATION_TYPE_CHANGE_TRUST: {
            return parse_change_trust(buffer + offset, &opDetails->op.changeTrust) + offset;
        }
        case XDR_OPERATION_TYPE_ALLOW_TRUST: {
            return parse_allow_trust(buffer + offset, &opDetails->op.allowTrust) + offset;
        }
        case XDR_OPERATION_TYPE_ACCOUNT_MERGE: {
            return parse_account_merge(buffer + offset, &opDetails->op.accountMerge) + offset;
        }
        case XDR_OPERATION_TYPE_INFLATION: {
            break;
        }
        case XDR_OPERATION_TYPE_MANAGE_DATA: {
            return parse_manage_data(buffer + offset, &opDetails->op.manageData) + offset;
        }
        case XDR_OPERATION_TYPE_BUMP_SEQUENCE: {
            return parse_bump_sequence(buffer + offset, &opDetails->op.bumpSequence) + offset;
        }
        default: THROW(0x6c24);
    }
    return offset;
}

void parse_tx_xdr(uint8_t *buffer, tx_context_t *txCtx) {
    uint16_t offset = txCtx->offset;
    if (offset == 0) {
        MEMCLEAR(txCtx->txDetails);

        offset += parse_network(buffer, &txCtx->txDetails);

        offset += 4; // skip envelopeType

        uint32_t accountType = read_uint32_block(buffer + offset);
        if (accountType != PUBLIC_KEY_TYPE_ED25519) {
            THROW(0x6c20);
        }
        offset += 4;

        txCtx->txDetails.source = buffer + offset;
        offset += 32;

        txCtx->txDetails.fee = read_uint32_block(buffer + offset);
        offset += 4;

        txCtx->txDetails.sequenceNumber = (int64_t) read_uint64_block(buffer + offset);
        offset += 8;

        offset += parse_time_bounds(buffer + offset, &txCtx->txDetails);
        offset += parse_memo(buffer + offset, &txCtx->txDetails);

        txCtx->opCount = read_uint32_block(buffer + offset);
        if (txCtx->opCount > MAX_OPS) {
            THROW(0x6c20);
        }
        offset += 4;

        txCtx->opIdx = 0;
    }
//    printHexBlocks(buffer+offset, 20);

    offset = parse_op_xdr(buffer + offset, &txCtx->opDetails) + offset;

    txCtx->opIdx += 1;
    if (txCtx->opCount == txCtx->opIdx) {
        offset = 0; // start from beginning next time
    }
    txCtx->offset = offset;
}
