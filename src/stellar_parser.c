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

/* SHA256("Public Global Stellar Network ; September 2015") */
static const uint8_t NETWORK_ID_PUBLIC_HASH[32] = {
    0x7a, 0xc3, 0x39, 0x97, 0x54, 0x4e, 0x31, 0x75, 0xd2, 0x66, 0xbd, 0x02, 0x24, 0x39, 0xb2, 0x2c,
    0xdb, 0x16, 0x50, 0x8c, 0x01, 0x16, 0x3f, 0x26, 0xe5, 0xcb, 0x2a, 0x3e, 0x10, 0x45, 0xa9, 0x79};

/* SHA256("Test SDF Network ; September 2015") */
static const uint8_t NETWORK_ID_TEST_HASH[32] = {
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

bool buffer_read64(buffer_t *buffer, uint64_t *n) {
    if (buffer->size - buffer->offset < 8) {
        *n = 0;
        return false;
    }

    const uint8_t *ptr = buffer->ptr + buffer->offset;
    uint32_t i1 = ptr[3] + (ptr[2] << 8u) + (ptr[1] << 16u) + (ptr[0] << 24u);
    uint32_t i2 = ptr[7] + (ptr[6] << 8u) + (ptr[5] << 16u) + (ptr[4] << 24u);
    *n = i2 | ((uint64_t) i1 << 32u);
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

#define PARSER_CHECK(x)         \
    {                           \
        if (!(x)) return false; \
    }

static bool parse_signer_key(buffer_t *buffer, SignerKey *key) {
    uint32_t signerType;

    PARSER_CHECK(buffer_read32(buffer, &signerType));
    key->type = signerType;

    switch (signerType) {
        case SIGNER_KEY_TYPE_ED25519:
        case SIGNER_KEY_TYPE_PRE_AUTH_TX:
        case SIGNER_KEY_TYPE_HASH_X:
            PARSER_CHECK(buffer_can_read(buffer, 32));
            key->data = buffer->ptr + buffer->offset;
            buffer_advance(buffer, 32);
            return true;
        case SIGNER_KEY_TYPE_ED25519_SIGNED_PAYLOAD:
            PARSER_CHECK(buffer_can_read(buffer, 32));
            key->data = buffer->ptr + buffer->offset;
            buffer_advance(buffer, 32);
            uint32_t payloadLength;
            PARSER_CHECK(buffer_read32(buffer, &payloadLength));
            payloadLength += (4 - payloadLength % 4) % 4;
            PARSER_CHECK(buffer_can_read(buffer, payloadLength));
            buffer_advance(buffer, payloadLength);
            return true;
        default:
            return false;
    }
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

static bool parse_muxed_account(buffer_t *buffer, MuxedAccount *muxed_account) {
    uint32_t cryptoKeyType;
    PARSER_CHECK(buffer_read32(buffer, &cryptoKeyType))
    muxed_account->type = cryptoKeyType;

    switch (muxed_account->type) {
        case KEY_TYPE_ED25519:
            PARSER_CHECK(buffer_can_read(buffer, 32));
            muxed_account->ed25519 = buffer->ptr + buffer->offset;
            buffer_advance(buffer, 32);
            return true;
        case KEY_TYPE_MUXED_ED25519:
            PARSER_CHECK(buffer_read64(buffer, &muxed_account->med25519.id));
            PARSER_CHECK(buffer_can_read(buffer, 32));
            muxed_account->med25519.ed25519 = buffer->ptr + buffer->offset;
            buffer_advance(buffer, 32);
            return true;
        default:
            return false;
    }
}

static bool parse_network(buffer_t *buffer, uint8_t *network) {
    if (!buffer_can_read(buffer, HASH_SIZE)) {
        return false;
    }
    if (memcmp(buffer->ptr, NETWORK_ID_PUBLIC_HASH, HASH_SIZE) == 0) {
        network_id = *network = NETWORK_TYPE_PUBLIC;
    } else if (memcmp(buffer->ptr, NETWORK_ID_TEST_HASH, HASH_SIZE) == 0) {
        network_id = *network = NETWORK_TYPE_TEST;
    } else {
        network_id = *network = NETWORK_TYPE_UNKNOWN;
    }
    buffer_advance(buffer, HASH_SIZE);
    return true;
}

static bool parse_time_bounds(buffer_t *buffer, TimeBounds *bounds) {
    if (!buffer_read64(buffer, &bounds->minTime)) {
        return false;
    }
    return buffer_read64(buffer, &bounds->maxTime);
}

static bool parse_ledger_bounds(buffer_t *buffer, LedgerBounds *ledgerBounds) {
    PARSER_CHECK(buffer_read32(buffer, (uint32_t *) &ledgerBounds->minLedger));
    PARSER_CHECK(buffer_read32(buffer, (uint32_t *) &ledgerBounds->maxLedger));
    return true;
}

static bool parse_extra_signers(buffer_t *buffer) {
    uint32_t length;
    PARSER_CHECK(buffer_read32(buffer, &length));
    if (length > 2) {  // maximum length is 2
        return false;
    }

    SignerKey signerKey;
    for (uint32_t i = 0; i < length; i++) {
        PARSER_CHECK(parse_signer_key(buffer, &signerKey));
    }
    return true;
}

static bool parse_preconditions(buffer_t *buffer, Preconditions *cond) {
    uint32_t preconditionType;
    PARSER_CHECK(buffer_read32(buffer, &preconditionType));
    switch (preconditionType) {
        case PRECOND_NONE:
            return true;
        case PRECOND_TIME:
            cond->hasTimeBounds = true;
            PARSER_CHECK(parse_time_bounds(buffer, &cond->timeBounds));
            return true;
        case PRECOND_V2:
            PARSER_CHECK(parse_optional_type(buffer,
                                             (xdr_type_parser) parse_time_bounds,
                                             &cond->timeBounds,
                                             &cond->hasTimeBounds));
            PARSER_CHECK(parse_optional_type(buffer,
                                             (xdr_type_parser) parse_ledger_bounds,
                                             &cond->ledgerBounds,
                                             &cond->hasLedgerBounds));
            PARSER_CHECK(parse_optional_type(buffer,
                                             (xdr_type_parser) buffer_read64,
                                             (uint64_t *) &cond->minSeqNum,
                                             &cond->hasMinSeqNum));
            PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &cond->minSeqAge));
            PARSER_CHECK(buffer_read32(buffer, &cond->minSeqLedgerGap));
            PARSER_CHECK(parse_extra_signers(buffer));
            return true;
        default:
            return false;
    }
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

static bool parse_memo(buffer_t *buffer, Memo *memo) {
    uint32_t type;

    if (!buffer_read32(buffer, &type)) {
        return 0;
    }
    memo->type = type;
    switch (memo->type) {
        case MEMO_NONE:
            return true;
        case MEMO_ID:
            return buffer_read64(buffer, &memo->id);
        case MEMO_TEXT:
            return parse_string_ptr(buffer, &memo->text, NULL, MEMO_TEXT_MAX_SIZE);
        case MEMO_HASH:
        case MEMO_RETURN:
            if (buffer->size - buffer->offset < HASH_SIZE) {
                return false;
            }
            memo->hash = buffer->ptr + buffer->offset;
            buffer->offset += HASH_SIZE;
            return true;
        default:
            return false;  // unknown memo type
    }
}

static bool parse_alpha_num4_asset(buffer_t *buffer, AlphaNum4 *asset) {
    PARSER_CHECK(buffer_can_read(buffer, 4));
    asset->assetCode = (const char *) buffer->ptr + buffer->offset;
    buffer_advance(buffer, 4);
    PARSER_CHECK(parse_account_id(buffer, &asset->issuer));
    return true;
}

static bool parse_alpha_num12_asset(buffer_t *buffer, AlphaNum12 *asset) {
    PARSER_CHECK(buffer_can_read(buffer, 12));
    asset->assetCode = (const char *) buffer->ptr + buffer->offset;
    buffer_advance(buffer, 12);
    PARSER_CHECK(parse_account_id(buffer, &asset->issuer));
    return true;
}

static bool parse_asset(buffer_t *buffer, Asset *asset) {
    uint32_t assetType;

    PARSER_CHECK(buffer_read32(buffer, &assetType));
    asset->type = assetType;
    switch (asset->type) {
        case ASSET_TYPE_NATIVE: {
            return true;
        }
        case ASSET_TYPE_CREDIT_ALPHANUM4: {
            return parse_alpha_num4_asset(buffer, &asset->alphaNum4);
        }
        case ASSET_TYPE_CREDIT_ALPHANUM12: {
            return parse_alpha_num12_asset(buffer, &asset->alphaNum12);
        }
        default:
            return false;  // unknown asset type
    }
}

static bool parse_trust_line_asset(buffer_t *buffer, TrustLineAsset *asset) {
    uint32_t assetType;

    PARSER_CHECK(buffer_read32(buffer, &assetType));
    asset->type = assetType;
    switch (asset->type) {
        case ASSET_TYPE_NATIVE: {
            return true;
        }
        case ASSET_TYPE_CREDIT_ALPHANUM4: {
            return parse_alpha_num4_asset(buffer, &asset->alphaNum4);
        }
        case ASSET_TYPE_CREDIT_ALPHANUM12: {
            return parse_alpha_num12_asset(buffer, &asset->alphaNum12);
        }
        case ASSET_TYPE_POOL_SHARE: {
            return buffer_read_bytes(buffer, asset->liquidityPoolID, LIQUIDITY_POOL_ID_SIZE);
        }
        default:
            return false;  // unknown asset type
    }
}

static bool parse_liquidity_pool_parameters(buffer_t *buffer,
                                            LiquidityPoolParameters *liquidityPoolParameters) {
    uint32_t liquidityPoolType;
    PARSER_CHECK(buffer_read32(buffer, &liquidityPoolType));
    switch (liquidityPoolType) {
        case LIQUIDITY_POOL_CONSTANT_PRODUCT: {
            PARSER_CHECK(parse_asset(buffer, &liquidityPoolParameters->constantProduct.assetA));
            PARSER_CHECK(parse_asset(buffer, &liquidityPoolParameters->constantProduct.assetB));
            PARSER_CHECK(
                buffer_read32(buffer, (uint32_t *) &liquidityPoolParameters->constantProduct.fee));
            return true;
        }
        default:
            return false;
    }
}

static bool parse_change_trust_asset(buffer_t *buffer, ChangeTrustAsset *asset) {
    uint32_t assetType;

    if (!buffer_read32(buffer, &assetType)) {
        return false;
    }
    asset->type = assetType;
    switch (asset->type) {
        case ASSET_TYPE_NATIVE: {
            return true;
        }
        case ASSET_TYPE_CREDIT_ALPHANUM4: {
            return parse_alpha_num4_asset(buffer, &asset->alphaNum4);
        }
        case ASSET_TYPE_CREDIT_ALPHANUM12: {
            return parse_alpha_num12_asset(buffer, &asset->alphaNum12);
        }
        case ASSET_TYPE_POOL_SHARE: {
            return parse_liquidity_pool_parameters(buffer, &asset->liquidityPool);
        }
        default:
            return false;  // unknown asset type
    }
}

static bool parse_create_account(buffer_t *buffer, CreateAccountOp *createAccount) {
    if (!parse_account_id(buffer, &createAccount->destination)) {
        return false;
    }
    return buffer_read64(buffer, (uint64_t *) &createAccount->startingBalance);
}

static bool parse_payment(buffer_t *buffer, PaymentOp *payment) {
    if (!parse_muxed_account(buffer, &payment->destination)) {
        return false;
    }

    if (!parse_asset(buffer, &payment->asset)) {
        return false;
    }

    return buffer_read64(buffer, (uint64_t *) &payment->amount);
}

static bool parse_path_payment_strict_receive(buffer_t *buffer, PathPaymentStrictReceiveOp *op) {
    uint32_t pathLen;

    PARSER_CHECK(parse_asset(buffer, &op->sendAsset));
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->sendMax));
    PARSER_CHECK(parse_muxed_account(buffer, &op->destination));
    PARSER_CHECK(parse_asset(buffer, &op->destAsset));
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->destAmount));

    PARSER_CHECK(buffer_read32(buffer, (uint32_t *) &pathLen));
    op->pathLen = pathLen;
    if (op->pathLen > 5) {
        return false;
    }
    for (int i = 0; i < op->pathLen; i++) {
        PARSER_CHECK(parse_asset(buffer, &op->path[i]));
    }
    return true;
}

static bool parse_allow_trust(buffer_t *buffer, AllowTrustOp *op) {
    uint32_t assetType;

    if (!parse_account_id(buffer, &op->trustor)) {
        return false;
    }

    if (!buffer_read32(buffer, &assetType)) {
        return false;
    }

    switch (assetType) {
        case ASSET_TYPE_CREDIT_ALPHANUM4: {
            if (!buffer_read_bytes(buffer, (uint8_t *) op->assetCode, 4)) {
                return false;
            }
            op->assetCode[4] = '\0';
            break;
        }
        case ASSET_TYPE_CREDIT_ALPHANUM12: {
            if (!buffer_read_bytes(buffer, (uint8_t *) op->assetCode, 12)) {
                return false;
            }
            op->assetCode[12] = '\0';
            break;
        }
        default:
            return false;  // unknown asset type
    }

    return buffer_read32(buffer, &op->authorize);
}

static bool parse_account_merge(buffer_t *buffer, MuxedAccount *destination) {
    return parse_muxed_account(buffer, destination);
}

static bool parse_manage_data(buffer_t *buffer, ManageDataOp *op) {
    size_t size;

    if (!parse_string_ptr(buffer, (const char **) &op->dataName, &size, DATA_NAME_MAX_SIZE)) {
        return false;
    }
    op->dataNameSize = size;

    // DataValue* dataValue;
    bool hasValue;
    if (!buffer_read_bool(buffer, &hasValue)) {
        return false;
    }
    if (hasValue) {
        if (!parse_string_ptr(buffer, (const char **) &op->dataValue, &size, DATA_VALUE_MAX_SIZE)) {
            return false;
        }
        op->dataValueSize = size;
    } else {
        op->dataValueSize = 0;
    }
    return true;
}

static bool parse_price(buffer_t *buffer, Price *price) {
    // FIXME: must correctly read int32_t
    PARSER_CHECK(buffer_read32(buffer, (uint32_t *) &price->n));
    PARSER_CHECK(buffer_read32(buffer, (uint32_t *) &price->d));

    // Denominator cannot be null, as it would lead to a division by zero.
    return price->d != 0;
}

static bool parse_manage_sell_offer(buffer_t *buffer, ManageSellOfferOp *op) {
    PARSER_CHECK(parse_asset(buffer, &op->selling));
    PARSER_CHECK(parse_asset(buffer, &op->buying));
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->amount));
    PARSER_CHECK(parse_price(buffer, &op->price));

    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->offerID));
    return true;
}

static bool parse_manage_buy_offer(buffer_t *buffer, ManageBuyOfferOp *op) {
    PARSER_CHECK(parse_asset(buffer, &op->selling));
    PARSER_CHECK(parse_asset(buffer, &op->buying));
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->buyAmount));
    PARSER_CHECK(parse_price(buffer, &op->price));

    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->offerID));
    return true;
}

static bool parse_create_passive_sell_offer(buffer_t *buffer, CreatePassiveSellOfferOp *op) {
    PARSER_CHECK(parse_asset(buffer, &op->selling));
    PARSER_CHECK(parse_asset(buffer, &op->buying));
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->amount));
    PARSER_CHECK(parse_price(buffer, &op->price));
    return true;
}

static bool parse_change_trust(buffer_t *buffer, ChangeTrustOp *op) {
    if (!parse_change_trust_asset(buffer, &op->line)) {
        return false;
    }
    return buffer_read64(buffer, &op->limit);
}

static bool parse_signer(buffer_t *buffer, signer_t *signer) {
    PARSER_CHECK(parse_signer_key(buffer, &signer->key));
    PARSER_CHECK(buffer_read32(buffer, &signer->weight));
    return true;
}

static bool parse_set_options(buffer_t *buffer, SetOptionsOp *setOptions) {
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

    uint32_t homeDomainPresent;
    if (!buffer_read32(buffer, &homeDomainPresent)) {
        return false;
    }
    setOptions->homeDomainPresent = homeDomainPresent ? true : false;
    if (setOptions->homeDomainPresent) {
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

static bool parse_bump_sequence(buffer_t *buffer, BumpSequenceOp *op) {
    return buffer_read64(buffer, (uint64_t *) &op->bumpTo);
}

static bool parse_path_payment_strict_send(buffer_t *buffer, PathPaymentStrictSendOp *op) {
    uint32_t pathLen;

    PARSER_CHECK(parse_asset(buffer, &op->sendAsset));
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->sendAmount));
    PARSER_CHECK(parse_muxed_account(buffer, &op->destination));
    PARSER_CHECK(parse_asset(buffer, &op->destAsset));
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->destMin));
    PARSER_CHECK(buffer_read32(buffer, (uint32_t *) &pathLen));
    op->pathLen = pathLen;
    if (op->pathLen > 5) {
        return false;
    }
    for (int i = 0; i < op->pathLen; i++) {
        PARSER_CHECK(parse_asset(buffer, &op->path[i]));
    }
    return true;
}

static bool parse_claimant_predicate(buffer_t *buffer) {
    // Currently, does not support displaying claimant details.
    // So here we will not store the parsed data, just to ensure that the data can be parsed
    // correctly.
    uint32_t claimPredicateType;
    uint32_t predicatesLen;
    bool notPredicatePresent;
    int64_t absBefore;
    int64_t relBefore;
    PARSER_CHECK(buffer_read32(buffer, &claimPredicateType));
    switch (claimPredicateType) {
        case CLAIM_PREDICATE_UNCONDITIONAL:
            return true;
        case CLAIM_PREDICATE_AND:
        case CLAIM_PREDICATE_OR:
            PARSER_CHECK(buffer_read32(buffer, &predicatesLen));
            if (predicatesLen != 2) {
                return false;
            }
            PARSER_CHECK(parse_claimant_predicate(buffer));
            PARSER_CHECK(parse_claimant_predicate(buffer));
            return true;
        case CLAIM_PREDICATE_NOT:
            PARSER_CHECK(buffer_read_bool(buffer, &notPredicatePresent));
            if (notPredicatePresent) {
                PARSER_CHECK(parse_claimant_predicate(buffer));
            }
            return true;
        case CLAIM_PREDICATE_BEFORE_ABSOLUTE_TIME:
            PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &absBefore));
            return true;
        case CLAIM_PREDICATE_BEFORE_RELATIVE_TIME:
            PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &relBefore));
            return true;
        default:
            return false;
    }
}

static bool parse_claimant(buffer_t *buffer, Claimant *claimant) {
    uint32_t claimantType;
    PARSER_CHECK(buffer_read32(buffer, &claimantType));
    claimant->type = claimantType;

    switch (claimant->type) {
        case CLAIMANT_TYPE_V0:
            PARSER_CHECK(parse_account_id(buffer, &claimant->v0.destination));
            PARSER_CHECK(parse_claimant_predicate(buffer));
            return true;
        default:
            return false;
    }
}

static bool parse_create_claimable_balance(buffer_t *buffer, CreateClaimableBalanceOp *op) {
    uint32_t claimantLen;
    PARSER_CHECK(parse_asset(buffer, &op->asset));
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->amount));
    PARSER_CHECK(buffer_read32(buffer, (uint32_t *) &claimantLen));
    if (claimantLen > 10) {
        return false;
    }
    op->claimantLen = claimantLen;
    for (int i = 0; i < op->claimantLen; i++) {
        PARSER_CHECK(parse_claimant(buffer, &op->claimants[i]));
    }
    return true;
}
static bool parse_claimable_balance_id(buffer_t *buffer, ClaimableBalanceID *claimableBalanceID) {
    uint32_t claimableBalanceIDType;
    PARSER_CHECK(buffer_read32(buffer, &claimableBalanceIDType));
    claimableBalanceID->type = claimableBalanceIDType;

    switch (claimableBalanceID->type) {
        case CLAIMABLE_BALANCE_ID_TYPE_V0:
            PARSER_CHECK(buffer_read_bytes(buffer, claimableBalanceID->v0, 32));
            return true;
        default:
            return false;
    }
}

static bool parse_claim_claimable_balance(buffer_t *buffer, ClaimClaimableBalanceOp *op) {
    PARSER_CHECK(parse_claimable_balance_id(buffer, &op->balanceID));
    return true;
}

static bool parse_begin_sponsoring_future_reserves(buffer_t *buffer,
                                                   BeginSponsoringFutureReservesOp *op) {
    PARSER_CHECK(parse_account_id(buffer, &op->sponsoredID));
    return true;
}

static bool parse_ledger_key(buffer_t *buffer, LedgerKey *ledgerKey) {
    uint32_t ledgerEntryType;
    PARSER_CHECK(buffer_read32(buffer, &ledgerEntryType));
    ledgerKey->type = ledgerEntryType;
    switch (ledgerKey->type) {
        case ACCOUNT:
            PARSER_CHECK(parse_account_id(buffer, &ledgerKey->account.accountID));
            return true;
        case TRUSTLINE:
            PARSER_CHECK(parse_account_id(buffer, &ledgerKey->trustLine.accountID));
            PARSER_CHECK(parse_trust_line_asset(buffer, &ledgerKey->trustLine.asset));
            return true;
        case OFFER:
            PARSER_CHECK(parse_account_id(buffer, &ledgerKey->offer.sellerID));
            PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &ledgerKey->offer.offerID));
            return true;
        case DATA:
            PARSER_CHECK(parse_account_id(buffer, &ledgerKey->data.accountID));
            PARSER_CHECK(parse_string_ptr(buffer,
                                          (const char **) &ledgerKey->data.dataName,
                                          (size_t *) &ledgerKey->data.dataNameSize,
                                          DATA_VALUE_MAX_SIZE));
            return true;
        case CLAIMABLE_BALANCE:
            PARSER_CHECK(
                parse_claimable_balance_id(buffer, &ledgerKey->claimableBalance.balanceId));
            return true;
        case LIQUIDITY_POOL:
            PARSER_CHECK(buffer_read_bytes(buffer,
                                           ledgerKey->liquidityPool.liquidityPoolID,
                                           LIQUIDITY_POOL_ID_SIZE));
            return true;
        default:
            return false;
    }
}

static bool parse_revoke_sponsorship(buffer_t *buffer, RevokeSponsorshipOp *op) {
    uint32_t revokeSponsorshipType;
    PARSER_CHECK(buffer_read32(buffer, &revokeSponsorshipType))
    op->type = revokeSponsorshipType;

    switch (op->type) {
        case REVOKE_SPONSORSHIP_LEDGER_ENTRY:
            PARSER_CHECK(parse_ledger_key(buffer, &op->ledgerKey));
            return true;
        case REVOKE_SPONSORSHIP_SIGNER:
            PARSER_CHECK(parse_account_id(buffer, &op->signer.accountID));
            PARSER_CHECK(parse_signer_key(buffer, &op->signer.signerKey));
            return true;
        default:
            return false;
    }
}

static bool parse_clawback(buffer_t *buffer, ClawbackOp *op) {
    PARSER_CHECK(parse_asset(buffer, &op->asset));
    PARSER_CHECK(parse_muxed_account(buffer, &op->from));
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->amount));
    return true;
}

static bool parse_clawback_claimable_balance(buffer_t *buffer, ClawbackClaimableBalanceOp *op) {
    PARSER_CHECK(parse_claimable_balance_id(buffer, &op->balanceID));
    return true;
}

static bool parse_set_trust_line_flags(buffer_t *buffer, SetTrustLineFlagsOp *op) {
    PARSER_CHECK(parse_account_id(buffer, &op->trustor));
    PARSER_CHECK(parse_asset(buffer, &op->asset));
    PARSER_CHECK(buffer_read32(buffer, &op->clearFlags));
    PARSER_CHECK(buffer_read32(buffer, &op->setFlags));
    return true;
}

static bool parse_liquidity_pool_deposit(buffer_t *buffer, LiquidityPoolDepositOp *op) {
    PARSER_CHECK(buffer_read_bytes(buffer, op->liquidityPoolID, LIQUIDITY_POOL_ID_SIZE));
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->maxAmountA));
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->maxAmountB));
    PARSER_CHECK(parse_price(buffer, &op->minPrice));
    PARSER_CHECK(parse_price(buffer, &op->maxPrice));
    return true;
}

static bool parse_liquidity_pool_withdraw(buffer_t *buffer, LiquidityPoolWithdrawOp *op) {
    PARSER_CHECK(buffer_read_bytes(buffer, op->liquidityPoolID, LIQUIDITY_POOL_ID_SIZE));
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->amount));
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->minAmountA));
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->minAmountB));
    return true;
}

static bool parse_operation(buffer_t *buffer, Operation *opDetails) {
    MEMCLEAR(*opDetails);
    uint32_t opType;

    if (!parse_optional_type(buffer,
                             (xdr_type_parser) parse_muxed_account,
                             &opDetails->sourceAccount,
                             &opDetails->sourceAccountPresent)) {
        return false;
    }

    if (!buffer_read32(buffer, &opType)) {
        return false;
    }
    opDetails->type = opType;
    switch (opDetails->type) {
        case XDR_OPERATION_TYPE_CREATE_ACCOUNT: {
            return parse_create_account(buffer, &opDetails->createAccount);
        }
        case XDR_OPERATION_TYPE_PAYMENT: {
            return parse_payment(buffer, &opDetails->payment);
        }
        case XDR_OPERATION_TYPE_PATH_PAYMENT_STRICT_RECEIVE: {
            return parse_path_payment_strict_receive(buffer,
                                                     &opDetails->pathPaymentStrictReceiveOp);
        }
        case XDR_OPERATION_TYPE_CREATE_PASSIVE_SELL_OFFER: {
            return parse_create_passive_sell_offer(buffer, &opDetails->createPassiveSellOfferOp);
        }
        case XDR_OPERATION_TYPE_MANAGE_SELL_OFFER: {
            return parse_manage_sell_offer(buffer, &opDetails->manageSellOfferOp);
        }
        case XDR_OPERATION_TYPE_SET_OPTIONS: {
            return parse_set_options(buffer, &opDetails->setOptionsOp);
        }
        case XDR_OPERATION_TYPE_CHANGE_TRUST: {
            return parse_change_trust(buffer, &opDetails->changeTrustOp);
        }
        case XDR_OPERATION_TYPE_ALLOW_TRUST: {
            return parse_allow_trust(buffer, &opDetails->allowTrustOp);
        }
        case XDR_OPERATION_TYPE_ACCOUNT_MERGE: {
            return parse_account_merge(buffer, &opDetails->destination);
        }
        case XDR_OPERATION_TYPE_INFLATION: {
            return true;
        }
        case XDR_OPERATION_TYPE_MANAGE_DATA: {
            return parse_manage_data(buffer, &opDetails->manageDataOp);
        }
        case XDR_OPERATION_TYPE_BUMP_SEQUENCE: {
            return parse_bump_sequence(buffer, &opDetails->bumpSequenceOp);
        }
        case XDR_OPERATION_TYPE_MANAGE_BUY_OFFER: {
            return parse_manage_buy_offer(buffer, &opDetails->manageBuyOfferOp);
        }
        case XDR_OPERATION_TYPE_PATH_PAYMENT_STRICT_SEND: {
            return parse_path_payment_strict_send(buffer, &opDetails->pathPaymentStrictSendOp);
        }
        case XDR_OPERATION_TYPE_CREATE_CLAIMABLE_BALANCE: {
            return parse_create_claimable_balance(buffer, &opDetails->createClaimableBalanceOp);
        }
        case XDR_OPERATION_TYPE_CLAIM_CLAIMABLE_BALANCE: {
            return parse_claim_claimable_balance(buffer, &opDetails->claimClaimableBalanceOp);
        }
        case XDR_OPERATION_TYPE_BEGIN_SPONSORING_FUTURE_RESERVES: {
            return parse_begin_sponsoring_future_reserves(
                buffer,
                &opDetails->beginSponsoringFutureReservesOp);
        }
        case XDR_OPERATION_TYPE_END_SPONSORING_FUTURE_RESERVES: {
            return true;
        }
        case XDR_OPERATION_TYPE_REVOKE_SPONSORSHIP: {
            return parse_revoke_sponsorship(buffer, &opDetails->revokeSponsorshipOp);
        }
        case XDR_OPERATION_TYPE_CLAWBACK: {
            return parse_clawback(buffer, &opDetails->clawbackOp);
        }
        case XDR_OPERATION_TYPE_CLAWBACK_CLAIMABLE_BALANCE: {
            return parse_clawback_claimable_balance(buffer, &opDetails->clawbackClaimableBalanceOp);
        }
        case XDR_OPERATION_TYPE_SET_TRUST_LINE_FLAGS: {
            return parse_set_trust_line_flags(buffer, &opDetails->setTrustLineFlagsOp);
        }
        case XDR_OPERATION_TYPE_LIQUIDITY_POOL_DEPOSIT:
            return parse_liquidity_pool_deposit(buffer, &opDetails->liquidityPoolDepositOp);
        case XDR_OPERATION_TYPE_LIQUIDITY_POOL_WITHDRAW:
            return parse_liquidity_pool_withdraw(buffer, &opDetails->liquidityPoolWithdrawOp);
        default:
            return false;  // Unknown operation
    }
}

static bool parse_tx_details(buffer_t *buffer, TransactionDetails *transaction) {
    // account used to run the (inner)transaction
    PARSER_CHECK(parse_muxed_account(buffer, &transaction->sourceAccount));

    // the fee the sourceAccount will pay
    PARSER_CHECK(buffer_read32(buffer, &transaction->fee));

    // sequence number to consume in the account
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &transaction->sequenceNumber));

    // validity conditions
    PARSER_CHECK(parse_preconditions(buffer, &transaction->cond));

    PARSER_CHECK(parse_memo(buffer, &transaction->memo));
    uint32_t opCount;
    PARSER_CHECK(buffer_read32(buffer, &opCount));
    transaction->opCount = opCount;
    if (transaction->opCount > MAX_OPS) {
        return false;
    }
    transaction->opIdx = 0;
    return true;
}

static bool parse_fee_bump_tx_details(buffer_t *buffer,
                                      FeeBumpTransactionDetails *feeBumpTransaction) {
    PARSER_CHECK(parse_muxed_account(buffer, &feeBumpTransaction->feeSource));
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &feeBumpTransaction->fee));
    return true;
}

bool parse_tx_xdr(const uint8_t *data, size_t size, tx_context_t *txCtx) {
    buffer_t buffer = {data, size, 0};
    uint32_t envelopeType;

    uint16_t offset = txCtx->offset;
    buffer.offset = txCtx->offset;

    if (offset == 0) {
        MEMCLEAR(txCtx->txDetails);
        MEMCLEAR(txCtx->feeBumpTxDetails);
        PARSER_CHECK(parse_network(&buffer, &txCtx->network));
        PARSER_CHECK(buffer_read32(&buffer, &envelopeType));
        txCtx->envelopeType = envelopeType;
        switch (envelopeType) {
            case ENVELOPE_TYPE_TX:
                PARSER_CHECK(parse_tx_details(&buffer, &txCtx->txDetails));
                break;
            case ENVELOPE_TYPE_TX_FEE_BUMP:
                PARSER_CHECK(parse_fee_bump_tx_details(&buffer, &txCtx->feeBumpTxDetails));
                uint32_t innerEnvelopeType;
                PARSER_CHECK(buffer_read32(&buffer, &innerEnvelopeType));
                if (innerEnvelopeType != ENVELOPE_TYPE_TX) {
                    return false;
                }
                PARSER_CHECK(parse_tx_details(&buffer, &txCtx->txDetails));
                break;
            default:
                return false;
        }
    }

    if (!parse_operation(&buffer, &txCtx->txDetails.opDetails)) {
        return false;
    }
    offset = buffer.offset;
    txCtx->txDetails.opIdx += 1;
    txCtx->offset = offset;
    return true;
}
