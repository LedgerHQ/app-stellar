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

#include "stellar_api.h"
#include "stellar_types.h"
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#ifndef TEST
#include "os.h"
#endif

typedef struct {
    const uint8_t *ptr;
    size_t size;
    off_t offset;
} buffer_t;

/* SHA256("Public Global Stellar Network ; September 2015") */
static const uint8_t NETWORK_ID_PUBLIC_HASH[64] = {
    0x7a, 0xc3, 0x39, 0x97, 0x54, 0x4e, 0x31, 0x75, 0xd2, 0x66, 0xbd, 0x02, 0x24, 0x39, 0xb2, 0x2c,
    0xdb, 0x16, 0x50, 0x8c, 0x01, 0x16, 0x3f, 0x26, 0xe5, 0xcb, 0x2a, 0x3e, 0x10, 0x45, 0xa9, 0x79};

/* SHA256("Test SDF Network ; September 2015") */
static const uint8_t NETWORK_ID_TEST_HASH[64] = {
    0xce, 0xe0, 0x30, 0x2d, 0x59, 0x84, 0x4d, 0x32, 0xbd, 0xca, 0x91, 0x5c, 0x82, 0x03, 0xdd, 0x44,
    0xb3, 0x3f, 0xbb, 0x7e, 0xdc, 0x19, 0x05, 0x1e, 0xa3, 0x7a, 0xbe, 0xdf, 0x28, 0xec, 0xd4, 0x72};
uint8_t network_id;

static bool buffer_can_read(const buffer_t *buffer, size_t num_bytes) {
    return buffer->size - buffer->offset >= num_bytes;
}
static void buffer_advance(buffer_t *buffer, size_t num_bytes) {
    buffer->offset += num_bytes;
}

static bool buffer_read32(buffer_t *buffer, uint32_t *n) {
    if (!buffer_can_read(buffer, 4)) {
        *n = 0;
        return false;
    }

    const uint8_t *ptr = buffer->ptr + buffer->offset;
    *n = ptr[3] + (ptr[2] << 8u) + (ptr[1] << 16u) + (ptr[0] << 24u);
    buffer_advance(buffer, 4);
    return true;
}

static bool buffer_read64(buffer_t *buffer, uint64_t *n) {
    if (buffer->size - buffer->offset < 8) {
        *n = 0;
        return false;
    }

    const uint8_t *ptr = buffer->ptr + buffer->offset;
    uint64_t i1 = ptr[3] + (ptr[2] << 8u) + (ptr[1] << 16u) + (ptr[0] << 24u);
    uint32_t i2 = ptr[7] + (ptr[6] << 8u) + (ptr[5] << 16u) + (ptr[4] << 24u);
    *n = i2 | (i1 << 32u);
    buffer->offset += 8;
    return true;
}

static bool buffer_read_bool(buffer_t *buffer, bool *b) {
    uint32_t val;

    if (!buffer_read32(buffer, &val)) {
        return false;
    }
    if (val != 0 && val != 1) {
        return false;
    }
    *b = val == 1 ? true : false;
    return true;
}

static bool buffer_read_bytes(buffer_t *buffer, uint8_t *out, size_t size) {
    if (buffer->size - buffer->offset < size) {
        return false;
    }
    memcpy(out, buffer->ptr + buffer->offset, size);
    buffer->offset += size;
    return true;
}

static size_t num_bytes(size_t size) {
    size_t remainder = size % 4;
    if (remainder == 0) {
        return size;
    }
    return size + 4 - remainder;
}

static bool check_padding(const uint8_t *buffer, size_t offset, size_t length) {
    unsigned int i;
    for (i = 0; i < length - offset; i++) {
        if (buffer[offset + i] != 0x00) {
            return false;
        }
    }
    return true;
}

bool parse_account_id(buffer_t *buffer, const uint8_t **account_id) {
    uint32_t accountType;

    if (!buffer_read32(buffer, &accountType) || accountType != PUBLIC_KEY_TYPE_ED25519) {
        return false;
    }
    if (!buffer_can_read(buffer, 32)) {
        return false;
    }
    *account_id = buffer->ptr + buffer->offset;
    buffer_advance(buffer, 32);
    return true;
}

static bool parse_network(buffer_t *buffer, tx_details_t *txDetails) {
    if (!buffer_can_read(buffer, 32)) {
        return false;
    }
    if (memcmp(buffer->ptr, NETWORK_ID_PUBLIC_HASH, 32) == 0) {
        network_id = txDetails->network = NETWORK_TYPE_PUBLIC;
    } else if (memcmp(buffer->ptr, NETWORK_ID_TEST_HASH, 32) == 0) {
        network_id = txDetails->network = NETWORK_TYPE_TEST;
    } else {
        network_id = txDetails->network = NETWORK_TYPE_UNKNOWN;
    }
    buffer_advance(buffer, 32);
    return true;
}

static bool parse_time_bounds(buffer_t *buffer, time_bounds_t *bounds) {
    if (!buffer_read64(buffer, &bounds->minTime)) {
        return false;
    }
    return buffer_read64(buffer, &bounds->maxTime);
}

/* TODO: max_length does not include terminal null character */
static bool parse_string(buffer_t *buffer, char *string, size_t max_length) {
    uint32_t size;

    if (!buffer_read32(buffer, &size)) {
        return false;
    }
    if (size > max_length || !buffer_can_read(buffer, num_bytes(size))) {
        return false;
    }
    if (!check_padding(buffer->ptr + buffer->offset, size,
                       num_bytes(size))) {  // security check
        return false;
    }
    memcpy(string, buffer->ptr + buffer->offset, size);
    string[size] = '\0';
    buffer_advance(buffer, num_bytes(size));
    return true;
}

/* TODO: max_length does not include terminal null character */
static bool parse_string_ptr(buffer_t *buffer,
                             const char **string,
                             size_t *out_len,
                             size_t max_length) {
    uint32_t size;

    if (!buffer_read32(buffer, &size)) {
        return false;
    }
    if (size > max_length || !buffer_can_read(buffer, num_bytes(size))) {
        return false;
    }
    if (!check_padding(buffer->ptr + buffer->offset, size,
                       num_bytes(size))) {  // security check
        return false;
    }
    *string = (char *) buffer->ptr + buffer->offset;
    if (out_len) {
        *out_len = size;
    }
    buffer_advance(buffer, num_bytes(size));
    return true;
}

static bool parse_memo(buffer_t *buffer, tx_details_t *txDetails) {
    uint32_t type;
    uint64_t memo_id;

    if (!buffer_read32(buffer, &type)) {
        return 0;
    }
    txDetails->memo.type = type;
    switch (txDetails->memo.type) {
        case MEMO_TYPE_NONE:
            strcpy(txDetails->memo.data, "[none]");
            return true;
        case MEMO_TYPE_ID:
            if (!buffer_read64(buffer, &memo_id)) {
                return false;
            }
            print_uint(memo_id, txDetails->memo.data);
            return true;
        case MEMO_TYPE_TEXT: {
            return parse_string(buffer, txDetails->memo.data, MEMO_TEXT_MAX_SIZE);
        }
        case MEMO_TYPE_HASH:
        case MEMO_TYPE_RETURN:
            if (buffer->size - buffer->offset < HASH_SIZE) {
                return false;
            }
            print_binary(buffer->ptr + buffer->offset, txDetails->memo.data, HASH_SIZE);
            buffer->offset += HASH_SIZE;
            return true;
        default:
            return false;  // unknown memo type
    }
}

static bool parse_asset(buffer_t *buffer, asset_t *asset) {
    uint32_t assetType;

    if (!buffer_read32(buffer, &assetType)) {
        return false;
    }
    asset->type = assetType;
    switch (asset->type) {
        case ASSET_TYPE_NATIVE: {
            print_native_asset_code(network_id, asset->code);
            return true;
        }
        case ASSET_TYPE_CREDIT_ALPHANUM4: {
            if (!buffer_read_bytes(buffer, (uint8_t *) asset->code, 4)) {
                return false;
            }
            asset->code[4] = '\0';
            return parse_account_id(buffer, &asset->issuer);
        }
        case ASSET_TYPE_CREDIT_ALPHANUM12: {
            if (!buffer_read_bytes(buffer, (uint8_t *) asset->code, 12)) {
                return false;
            }
            asset->code[12] = '\0';
            return parse_account_id(buffer, &asset->issuer);
        }
        default:
            return false;  // unknown asset type
    }
}

static bool parse_create_account(buffer_t *buffer, create_account_op_t *createAccount) {
    if (!parse_account_id(buffer, &createAccount->accountId)) {
        return false;
    }
    return buffer_read64(buffer, &createAccount->amount);
}

static bool parse_payment(buffer_t *buffer, payment_op_t *payment) {
    if (!parse_account_id(buffer, &payment->destination)) {
        return false;
    }

    if (!parse_asset(buffer, &payment->asset)) {
        return false;
    }

    return buffer_read64(buffer, &payment->amount);
}

static bool parse_path_payment(buffer_t *buffer, path_payment_op_t *pathPayment) {
    uint32_t pathLen;

    if (!parse_asset(buffer, &pathPayment->sourceAsset)) {
        return false;
    }
    if (!buffer_read64(buffer, &pathPayment->sendMax)) {
        return false;
    }
    if (!parse_account_id(buffer, &pathPayment->destination)) {
        return false;
    }

    if (!parse_asset(buffer, &pathPayment->destAsset)) {
        return false;
    }

    if (!buffer_read64(buffer, &pathPayment->destAmount)) {
        return false;
    }

    if (!buffer_read32(buffer, &pathLen)) {
        return false;
    }
    pathPayment->pathLen = pathLen;
    if (pathPayment->pathLen > 5) {
        return false;
    }
    for (int i = 0; i < pathPayment->pathLen; i++) {
        if (!parse_asset(buffer, &pathPayment->path[i])) {
            return false;
        }
    }
    return true;
}

static bool parse_allow_trust(buffer_t *buffer, allow_trust_op_t *allowTrust) {
    uint32_t assetType;

    if (!parse_account_id(buffer, &allowTrust->trustee)) {
        return false;
    }

    if (!buffer_read32(buffer, &assetType)) {
        return false;
    }

    switch (assetType) {
        case ASSET_TYPE_CREDIT_ALPHANUM4: {
            if (!buffer_read_bytes(buffer, (uint8_t *) allowTrust->assetCode, 4)) {
                return false;
            }
            allowTrust->assetCode[4] = '\0';
            break;
        }
        case ASSET_TYPE_CREDIT_ALPHANUM12: {
            if (!buffer_read_bytes(buffer, (uint8_t *) allowTrust->assetCode, 12)) {
                return false;
            }
            allowTrust->assetCode[12] = '\0';
            break;
        }
        default:
            return false;  // unknown asset type
    }

    return buffer_read_bool(buffer, &allowTrust->authorize);
}

static bool parse_account_merge(buffer_t *buffer, account_merge_op_t *accountMerge) {
    return parse_account_id(buffer, &accountMerge->destination);
}

static bool parse_manage_data(buffer_t *buffer, manage_data_op_t *manageData) {
    size_t size;

    if (!parse_string_ptr(buffer,
                          (const char **) &manageData->dataName,
                          &size,
                          DATA_NAME_MAX_SIZE)) {
        return false;
    }
    manageData->dataNameSize = size;

    // DataValue* dataValue;
    bool hasValue;
    if (!buffer_read_bool(buffer, &hasValue)) {
        return false;
    }
    if (hasValue) {
        if (!parse_string_ptr(buffer,
                              (const char **) &manageData->dataValue,
                              &size,
                              DATA_VALUE_MAX_SIZE)) {
            return false;
        }
        manageData->dataValueSize = size;
    } else {
        manageData->dataValueSize = 0;
    }
    return true;
}

static bool parse_offer(buffer_t *buffer, manage_offer_op_t *manageOffer) {
    if (!parse_asset(buffer, &manageOffer->selling)) {
        return false;
    }
    if (!parse_asset(buffer, &manageOffer->buying)) {
        return false;
    }
    if (!buffer_read64(buffer, &manageOffer->amount)) {
        return false;
    }
    if (!buffer_read32(buffer, &manageOffer->price.numerator)) {
        return false;
    }
    if (!buffer_read32(buffer, &manageOffer->price.denominator)) {
        return false;
    }
    // Denominator cannot be null, as it would lead to a division by zero.
    return manageOffer->price.denominator != 0;
}

static bool parse_active_offer(buffer_t *buffer, manage_offer_op_t *manageOffer) {
    manageOffer->active = true;
    if (!parse_offer(buffer, manageOffer)) {
        return false;
    }
    return buffer_read64(buffer, &manageOffer->offerId);
}

static bool parse_active_sell_offer(buffer_t *buffer, manage_offer_op_t *manageOffer) {
    manageOffer->buy = false;
    return parse_active_offer(buffer, manageOffer);
}

static bool parse_active_buy_offer(buffer_t *buffer, manage_offer_op_t *manageOffer) {
    manageOffer->buy = true;
    return parse_active_offer(buffer, manageOffer);
}

static bool parse_passive_offer(buffer_t *buffer, manage_offer_op_t *manageOffer) {
    manageOffer->active = false;
    manageOffer->offerId = 0;
    return parse_offer(buffer, manageOffer);
}

static bool parse_change_trust(buffer_t *buffer, change_trust_op_t *changeTrust) {
    if (!parse_asset(buffer, &changeTrust->asset)) {
        return false;
    }
    return buffer_read64(buffer, &changeTrust->limit);
}

typedef bool (*xdr_type_parser)(buffer_t *, void *);

static bool parse_optional_type(buffer_t *buffer, xdr_type_parser parser, void *dst, bool *opted) {
    bool isPresent;

    if (!buffer_read_bool(buffer, &isPresent)) {
        return false;
    }
    if (isPresent) {
        if (opted) {
            *opted = true;
        }
        return parser(buffer, dst);
    } else {
        if (opted) {
            *opted = false;
        }
        return true;
    }
}

static bool parse_signer(buffer_t *buffer, signer_t *signer) {
    uint32_t signerType;

    if (!buffer_read32(buffer, &signerType)) {
        return false;
    }
    signer->type = signerType;

    if (buffer->size - buffer->offset < 32) {
        return false;
    }
    signer->data = buffer->ptr + buffer->offset;
    buffer_advance(buffer, 32);

    return buffer_read32(buffer, &signer->weight);
}

static bool parse_set_options(buffer_t *buffer, set_options_op_t *setOptions) {
    uint32_t tmp;

    if (!parse_optional_type(buffer,
                             (xdr_type_parser) parse_account_id,
                             &setOptions->inflationDestination,
                             &setOptions->inflationDestinationPresent)) {
        return false;
    }

    if (!parse_optional_type(buffer,
                             (xdr_type_parser) buffer_read32,
                             &setOptions->clearFlags,
                             NULL)) {
        return false;
    }
    if (!parse_optional_type(buffer,
                             (xdr_type_parser) buffer_read32,
                             &setOptions->setFlags,
                             NULL)) {
        return false;
    }
    if (!parse_optional_type(buffer,
                             (xdr_type_parser) buffer_read32,
                             &setOptions->masterWeight,
                             &setOptions->masterWeightPresent)) {
        return false;
    }
    if (!parse_optional_type(buffer,
                             (xdr_type_parser) buffer_read32,
                             &setOptions->lowThreshold,
                             &setOptions->lowThresholdPresent)) {
        return false;
    }
    if (!parse_optional_type(buffer,
                             (xdr_type_parser) buffer_read32,
                             &setOptions->mediumThreshold,
                             &setOptions->mediumThresholdPresent)) {
        return false;
    }
    if (!parse_optional_type(buffer,
                             (xdr_type_parser) buffer_read32,
                             &setOptions->highThreshold,
                             &setOptions->highThresholdPresent)) {
        return false;
    }
    if (!buffer_read32(buffer, &tmp)) {
        return false;
    }
    bool homeDomainPresent = tmp ? true : false;
    if (homeDomainPresent) {
        if (!buffer_read32(buffer, &setOptions->homeDomainSize) ||
            setOptions->homeDomainSize > HOME_DOMAIN_MAX_SIZE) {
            return false;
        }
        if (buffer->size - buffer->offset < num_bytes(setOptions->homeDomainSize)) {
            return false;
        }
        setOptions->homeDomain = buffer->ptr + buffer->offset;
        if (!check_padding(setOptions->homeDomain,
                           setOptions->homeDomainSize,
                           num_bytes(setOptions->homeDomainSize))) {  // security check
            return false;
        }
        buffer->offset += num_bytes(setOptions->homeDomainSize);
    } else {
        setOptions->homeDomainSize = 0;
    }

    return parse_optional_type(buffer,
                               (xdr_type_parser) parse_signer,
                               &setOptions->signer,
                               &setOptions->signerPresent);
}

static bool parse_bump_sequence(buffer_t *buffer, bump_sequence_op_t *bumpSequence) {
    return buffer_read64(buffer, (uint64_t *) &bumpSequence->bumpTo);
}

static bool parse_op_xdr(buffer_t *buffer, operation_details_t *opDetails) {
    uint32_t opType;

    if (!parse_optional_type(buffer,
                             (xdr_type_parser) parse_account_id,
                             &opDetails->source,
                             &opDetails->sourcePresent)) {
        return false;
    }

    if (!buffer_read32(buffer, &opType)) {
        return false;
    }
    opDetails->type = opType;
    switch (opDetails->type) {
        case XDR_OPERATION_TYPE_CREATE_ACCOUNT: {
            return parse_create_account(buffer, &opDetails->op.createAccount);
        }
        case XDR_OPERATION_TYPE_PAYMENT: {
            return parse_payment(buffer, &opDetails->op.payment);
        }
        case XDR_OPERATION_TYPE_PATH_PAYMENT: {
            return parse_path_payment(buffer, &opDetails->op.pathPayment);
        }
        case XDR_OPERATION_TYPE_CREATE_PASSIVE_OFFER: {
            return parse_passive_offer(buffer, &opDetails->op.manageOffer);
        }
        case XDR_OPERATION_TYPE_MANAGE_SELL_OFFER: {
            return parse_active_sell_offer(buffer, &opDetails->op.manageOffer);
        }
        case XDR_OPERATION_TYPE_SET_OPTIONS: {
            return parse_set_options(buffer, &opDetails->op.setOptions);
        }
        case XDR_OPERATION_TYPE_CHANGE_TRUST: {
            return parse_change_trust(buffer, &opDetails->op.changeTrust);
        }
        case XDR_OPERATION_TYPE_ALLOW_TRUST: {
            return parse_allow_trust(buffer, &opDetails->op.allowTrust);
        }
        case XDR_OPERATION_TYPE_ACCOUNT_MERGE: {
            return parse_account_merge(buffer, &opDetails->op.accountMerge);
        }
        case XDR_OPERATION_TYPE_INFLATION: {
            return true;
        }
        case XDR_OPERATION_TYPE_MANAGE_DATA: {
            return parse_manage_data(buffer, &opDetails->op.manageData);
        }
        case XDR_OPERATION_TYPE_BUMP_SEQUENCE: {
            return parse_bump_sequence(buffer, &opDetails->op.bumpSequence);
        }
        case XDR_OPERATION_TYPE_MANAGE_BUY_OFFER: {
            return parse_active_buy_offer(buffer, &opDetails->op.manageOffer);
        }
        default:
            return false;  // Unknown operation
    }
}

#define ENVELOPE_TYPE_TX 2

bool parse_tx_xdr(const uint8_t *data, size_t size, tx_context_t *txCtx) {
    buffer_t buffer = {data, size, 0};
    uint32_t envelopeType;

    uint16_t offset = txCtx->offset;
    buffer.offset = txCtx->offset;

    if (offset == 0) {
        MEMCLEAR(txCtx->txDetails);

        if (!parse_network(&buffer, &txCtx->txDetails)) {
            return false;
        }
        if (!buffer_read32(&buffer, &envelopeType) || envelopeType != ENVELOPE_TYPE_TX) {
            return false;
        }

        // account used to run the transaction
        if (!parse_account_id(&buffer, &txCtx->txDetails.source)) {
            return false;
        }

        // the fee the sourceAccount will pay
        uint32_t fees;
        if (!buffer_read32(&buffer, &fees)) {
            return false;
        }
        txCtx->txDetails.fee = fees;

        // sequence number to consume in the account
        if (!buffer_read64(&buffer, (uint64_t *) &txCtx->txDetails.sequenceNumber)) {
            return false;
        }

        // validity range (inclusive) for the last ledger close time
        if (!parse_optional_type(&buffer,
                                 (xdr_type_parser) parse_time_bounds,
                                 &txCtx->txDetails.timeBounds,
                                 &txCtx->txDetails.hasTimeBounds)) {
            return false;
        }

        if (!parse_memo(&buffer, &txCtx->txDetails)) {
            return false;
        }

        uint32_t opCount;
        if (!buffer_read32(&buffer, &opCount)) {
            return false;
        }
        txCtx->opCount = opCount;
        if (txCtx->opCount > MAX_OPS) {
            return false;
        }
        txCtx->opIdx = 0;
    }

    if (!parse_op_xdr(&buffer, &txCtx->opDetails)) {
        return false;
    }
    offset = buffer.offset;

    txCtx->opIdx += 1;
    txCtx->offset = offset;
    return true;
}
