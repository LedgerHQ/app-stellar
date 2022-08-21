/*****************************************************************************
 *   Ledger Stellar App.
 *   (c) 2022 Ledger SAS.
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
 *****************************************************************************/

#include <stdint.h>
#include <string.h>

#include "os.h"

#include "./transaction_parser.h"
#include "../types.h"
#include "../sw.h"
#include "../common/buffer.h"

#define PARSER_CHECK(x)         \
    {                           \
        if (!(x)) return false; \
    }

/* SHA256("Public Global Stellar Network ; September 2015") */
static const uint8_t NETWORK_ID_PUBLIC_HASH[32] = {
    0x7a, 0xc3, 0x39, 0x97, 0x54, 0x4e, 0x31, 0x75, 0xd2, 0x66, 0xbd, 0x02, 0x24, 0x39, 0xb2, 0x2c,
    0xdb, 0x16, 0x50, 0x8c, 0x01, 0x16, 0x3f, 0x26, 0xe5, 0xcb, 0x2a, 0x3e, 0x10, 0x45, 0xa9, 0x79};

/* SHA256("Test SDF Network ; September 2015") */
static const uint8_t NETWORK_ID_TEST_HASH[32] = {
    0xce, 0xe0, 0x30, 0x2d, 0x59, 0x84, 0x4d, 0x32, 0xbd, 0xca, 0x91, 0x5c, 0x82, 0x03, 0xdd, 0x44,
    0xb3, 0x3f, 0xbb, 0x7e, 0xdc, 0x19, 0x05, 0x1e, 0xa3, 0x7a, 0xbe, 0xdf, 0x28, 0xec, 0xd4, 0x72};

static bool buffer_advance(buffer_t *buffer, size_t num_bytes) {
    return buffer_seek_cur(buffer, num_bytes);
}

static bool buffer_read32(buffer_t *buffer, uint32_t *n) {
    return buffer_read_u32(buffer, n, BE);
}

static bool buffer_read64(buffer_t *buffer, uint64_t *n) {
    return buffer_read_u64(buffer, n, BE);
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

bool parse_binary_string_ptr(buffer_t *buffer,
                             const uint8_t **string,
                             size_t *out_len,
                             size_t max_length) {
    /* max_length does not include terminal null character */
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
    *string = (uint8_t *) buffer->ptr + buffer->offset;
    if (out_len) {
        *out_len = size;
    }
    PARSER_CHECK(buffer_advance(buffer, num_bytes(size)))
    return true;
}

typedef bool (*xdr_type_reader)(buffer_t *, void *);

bool parse_optional_type(buffer_t *buffer, xdr_type_reader reader, void *dst, bool *opted) {
    bool is_present;

    if (!buffer_read_bool(buffer, &is_present)) {
        return false;
    }
    if (is_present) {
        if (opted) {
            *opted = true;
        }
        return reader(buffer, dst);
    } else {
        if (opted) {
            *opted = false;
        }
        return true;
    }
}

bool parse_signer_key(buffer_t *buffer, signer_key_t *key) {
    uint32_t signer_type;

    PARSER_CHECK(buffer_read32(buffer, &signer_type))
    key->type = signer_type;

    switch (signer_type) {
        case SIGNER_KEY_TYPE_ED25519:
            PARSER_CHECK(buffer_can_read(buffer, RAW_ED25519_PUBLIC_KEY_SIZE))
            key->ed25519 = buffer->ptr + buffer->offset;
            PARSER_CHECK(buffer_advance(buffer, RAW_ED25519_PUBLIC_KEY_SIZE))
            return true;
        case SIGNER_KEY_TYPE_PRE_AUTH_TX:
            PARSER_CHECK(buffer_can_read(buffer, RAW_PRE_AUTH_TX_KEY_SIZE))
            key->pre_auth_tx = buffer->ptr + buffer->offset;
            PARSER_CHECK(buffer_advance(buffer, RAW_PRE_AUTH_TX_KEY_SIZE))
            return true;
        case SIGNER_KEY_TYPE_HASH_X:
            PARSER_CHECK(buffer_can_read(buffer, RAW_HASH_X_KEY_SIZE))
            key->hash_x = buffer->ptr + buffer->offset;
            PARSER_CHECK(buffer_advance(buffer, RAW_HASH_X_KEY_SIZE))
            return true;
        case SIGNER_KEY_TYPE_ED25519_SIGNED_PAYLOAD:
            PARSER_CHECK(buffer_can_read(buffer, RAW_ED25519_PUBLIC_KEY_SIZE))
            key->ed25519_signed_payload.ed25519 = buffer->ptr + buffer->offset;
            PARSER_CHECK(buffer_advance(buffer, RAW_ED25519_PUBLIC_KEY_SIZE))
            uint32_t payload_length;
            PARSER_CHECK(buffer_read32(buffer, &payload_length))
            // valid length [1, 64]
            if (payload_length == 0 || payload_length > 64) {
                return false;
            }
            key->ed25519_signed_payload.payload_len = payload_length;
            payload_length += (4 - payload_length % 4) % 4;
            PARSER_CHECK(buffer_can_read(buffer, payload_length))
            key->ed25519_signed_payload.payload = buffer->ptr + buffer->offset;
            PARSER_CHECK(buffer_advance(buffer, payload_length))
            return true;
        default:
            return false;
    }
}

bool parse_account_id(buffer_t *buffer, const uint8_t **account_id) {
    uint32_t account_type;

    PARSER_CHECK(buffer_read32(buffer, &account_type) || account_type != PUBLIC_KEY_TYPE_ED25519)
    PARSER_CHECK(buffer_can_read(buffer, RAW_ED25519_PUBLIC_KEY_SIZE))
    *account_id = buffer->ptr + buffer->offset;
    PARSER_CHECK(buffer_advance(buffer, RAW_ED25519_PUBLIC_KEY_SIZE))
    return true;
}

bool parse_muxed_account(buffer_t *buffer, muxed_account_t *muxed_account) {
    uint32_t crypto_key_type;
    PARSER_CHECK(buffer_read32(buffer, &crypto_key_type))
    muxed_account->type = crypto_key_type;

    switch (muxed_account->type) {
        case KEY_TYPE_ED25519:
            PARSER_CHECK(buffer_can_read(buffer, RAW_ED25519_PUBLIC_KEY_SIZE))
            muxed_account->ed25519 = buffer->ptr + buffer->offset;
            PARSER_CHECK(buffer_advance(buffer, RAW_ED25519_PUBLIC_KEY_SIZE))
            return true;
        case KEY_TYPE_MUXED_ED25519:
            PARSER_CHECK(buffer_read64(buffer, &muxed_account->med25519.id))
            PARSER_CHECK(buffer_can_read(buffer, RAW_ED25519_PUBLIC_KEY_SIZE))
            muxed_account->med25519.ed25519 = buffer->ptr + buffer->offset;
            PARSER_CHECK(buffer_advance(buffer, RAW_ED25519_PUBLIC_KEY_SIZE))
            return true;
        default:
            return false;
    }
}

bool parse_time_bounds(buffer_t *buffer, time_bounds_t *bounds) {
    PARSER_CHECK(buffer_read64(buffer, &bounds->min_time))
    return buffer_read64(buffer, &bounds->max_time);
}

bool parse_ledger_bounds(buffer_t *buffer, ledger_bounds_t *ledger_bounds) {
    PARSER_CHECK(buffer_read32(buffer, (uint32_t *) &ledger_bounds->min_ledger))
    PARSER_CHECK(buffer_read32(buffer, (uint32_t *) &ledger_bounds->max_ledger))
    return true;
}

bool parse_extra_signers(buffer_t *buffer) {
    uint32_t length;
    PARSER_CHECK(buffer_read32(buffer, &length))
    if (length > 2) {  // maximum length is 2
        return false;
    }
    signer_key_t signer_key;
    for (uint32_t i = 0; i < length; i++) {
        PARSER_CHECK(parse_signer_key(buffer, &signer_key))
    }
    return true;
}

bool parse_preconditions(buffer_t *buffer, preconditions_t *cond) {
    uint32_t precondition_type;
    PARSER_CHECK(buffer_read32(buffer, &precondition_type))
    switch (precondition_type) {
        case PRECOND_NONE:
            cond->time_bounds_present = false;
            cond->min_seq_num_present = false;
            cond->ledger_bounds_present = false;
            cond->min_seq_ledger_gap = 0;
            cond->min_seq_age = 0;
            return true;
        case PRECOND_TIME:
            cond->time_bounds_present = true;
            PARSER_CHECK(parse_time_bounds(buffer, &cond->time_bounds))
            cond->min_seq_num_present = false;
            cond->ledger_bounds_present = false;
            cond->min_seq_ledger_gap = 0;
            cond->min_seq_age = 0;
            return true;
        case PRECOND_V2:
            PARSER_CHECK(parse_optional_type(buffer,
                                             (xdr_type_reader) parse_time_bounds,
                                             &cond->time_bounds,
                                             &cond->time_bounds_present))
            PARSER_CHECK(parse_optional_type(buffer,
                                             (xdr_type_reader) parse_ledger_bounds,
                                             &cond->ledger_bounds,
                                             &cond->ledger_bounds_present))
            PARSER_CHECK(parse_optional_type(buffer,
                                             (xdr_type_reader) buffer_read64,
                                             (uint64_t *) &cond->min_seq_num,
                                             &cond->min_seq_num_present))
            PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &cond->min_seq_age))
            PARSER_CHECK(buffer_read32(buffer, &cond->min_seq_ledger_gap))
            PARSER_CHECK(parse_extra_signers(buffer))
            return true;
        default:
            return false;
    }
}

bool parse_memo(buffer_t *buffer, memo_t *memo) {
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
        case MEMO_TEXT: {
            size_t size;
            PARSER_CHECK(
                parse_binary_string_ptr(buffer, (const uint8_t **) &memo->text.text, &size, 28))
            memo->text.text_size = size;
            return true;
        }
        case MEMO_HASH:
            PARSER_CHECK(buffer_can_read(buffer, HASH_SIZE))
            memo->hash = buffer->ptr + buffer->offset;
            buffer->offset += HASH_SIZE;
            return true;
        case MEMO_RETURN:
            PARSER_CHECK(buffer_can_read(buffer, HASH_SIZE))
            memo->return_hash = buffer->ptr + buffer->offset;
            buffer->offset += HASH_SIZE;
            return true;
        default:
            return false;  // unknown memo type
    }
}

bool parse_alpha_num4_asset(buffer_t *buffer, alpha_num4_t *asset) {
    PARSER_CHECK(buffer_can_read(buffer, 4))
    asset->asset_code = (const char *) buffer->ptr + buffer->offset;
    PARSER_CHECK(buffer_advance(buffer, 4))
    PARSER_CHECK(parse_account_id(buffer, &asset->issuer))
    return true;
}

bool parse_alpha_num12_asset(buffer_t *buffer, alpha_num12_t *asset) {
    PARSER_CHECK(buffer_can_read(buffer, 12))
    asset->asset_code = (const char *) buffer->ptr + buffer->offset;
    PARSER_CHECK(buffer_advance(buffer, 12))
    PARSER_CHECK(parse_account_id(buffer, &asset->issuer))
    return true;
}

bool parse_asset(buffer_t *buffer, asset_t *asset) {
    uint32_t asset_type;

    PARSER_CHECK(buffer_read32(buffer, &asset_type))
    asset->type = asset_type;
    switch (asset->type) {
        case ASSET_TYPE_NATIVE: {
            return true;
        }
        case ASSET_TYPE_CREDIT_ALPHANUM4: {
            return parse_alpha_num4_asset(buffer, &asset->alpha_num4);
        }
        case ASSET_TYPE_CREDIT_ALPHANUM12: {
            return parse_alpha_num12_asset(buffer, &asset->alpha_num12);
        }
        default:
            return false;  // unknown asset type
    }
}

bool parse_trust_line_asset(buffer_t *buffer, trust_line_asset_t *asset) {
    uint32_t asset_type;

    PARSER_CHECK(buffer_read32(buffer, &asset_type))
    asset->type = asset_type;
    switch (asset->type) {
        case ASSET_TYPE_NATIVE: {
            return true;
        }
        case ASSET_TYPE_CREDIT_ALPHANUM4: {
            return parse_alpha_num4_asset(buffer, &asset->alpha_num4);
        }
        case ASSET_TYPE_CREDIT_ALPHANUM12: {
            return parse_alpha_num12_asset(buffer, &asset->alpha_num12);
        }
        case ASSET_TYPE_POOL_SHARE: {
            PARSER_CHECK(buffer_can_read(buffer, LIQUIDITY_POOL_ID_SIZE))
            asset->liquidity_pool_id = buffer->ptr + buffer->offset;
            PARSER_CHECK(buffer_advance(buffer, LIQUIDITY_POOL_ID_SIZE))
            return true;
        }
        default:
            return false;  // unknown asset type
    }
}

bool parse_liquidity_pool_parameters(buffer_t *buffer,
                                     liquidity_pool_parameters_t *liquidity_pool_parameters) {
    uint32_t liquidity_pool_type;
    PARSER_CHECK(buffer_read32(buffer, &liquidity_pool_type))
    switch (liquidity_pool_type) {
        case LIQUIDITY_POOL_CONSTANT_PRODUCT: {
            PARSER_CHECK(parse_asset(buffer, &liquidity_pool_parameters->constant_product.asset_a))
            PARSER_CHECK(parse_asset(buffer, &liquidity_pool_parameters->constant_product.asset_b))
            PARSER_CHECK(
                buffer_read32(buffer,
                              (uint32_t *) &liquidity_pool_parameters->constant_product.fee))
            return true;
        }
        default:
            return false;
    }
}

bool parse_change_trust_asset(buffer_t *buffer, change_trust_asset_t *asset) {
    uint32_t asset_type;

    PARSER_CHECK(buffer_read32(buffer, &asset_type))
    asset->type = asset_type;
    switch (asset->type) {
        case ASSET_TYPE_NATIVE: {
            return true;
        }
        case ASSET_TYPE_CREDIT_ALPHANUM4: {
            return parse_alpha_num4_asset(buffer, &asset->alpha_num4);
        }
        case ASSET_TYPE_CREDIT_ALPHANUM12: {
            return parse_alpha_num12_asset(buffer, &asset->alpha_num12);
        }
        case ASSET_TYPE_POOL_SHARE: {
            return parse_liquidity_pool_parameters(buffer, &asset->liquidity_pool);
        }
        default:
            return false;  // unknown asset type
    }
}

bool parse_create_account(buffer_t *buffer, create_account_op_t *create_account_op) {
    PARSER_CHECK(parse_account_id(buffer, &create_account_op->destination))
    return buffer_read64(buffer, (uint64_t *) &create_account_op->starting_balance);
}

bool parse_payment(buffer_t *buffer, payment_op_t *payment_op) {
    PARSER_CHECK(parse_muxed_account(buffer, &payment_op->destination))

    PARSER_CHECK(parse_asset(buffer, &payment_op->asset))

    return buffer_read64(buffer, (uint64_t *) &payment_op->amount);
}

bool parse_path_payment_strict_receive(buffer_t *buffer, path_payment_strict_receive_op_t *op) {
    uint32_t path_len;

    PARSER_CHECK(parse_asset(buffer, &op->send_asset))
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->send_max))
    PARSER_CHECK(parse_muxed_account(buffer, &op->destination))
    PARSER_CHECK(parse_asset(buffer, &op->dest_asset))
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->dest_amount))

    PARSER_CHECK(buffer_read32(buffer, (uint32_t *) &path_len))
    if (path_len > PATH_PAYMENT_MAX_PATH_LENGTH) {
        return false;
    }
    return true;
}

bool parse_allow_trust(buffer_t *buffer, allow_trust_op_t *op) {
    uint32_t asset_type;

    PARSER_CHECK(parse_account_id(buffer, &op->trustor))
    PARSER_CHECK(buffer_read32(buffer, &asset_type))

    switch (asset_type) {
        case ASSET_TYPE_CREDIT_ALPHANUM4: {
            PARSER_CHECK(buffer_read_bytes(buffer, (uint8_t *) op->asset_code, 4))
            op->asset_code[4] = '\0';  // FIXME: it's OK?
            break;
        }
        case ASSET_TYPE_CREDIT_ALPHANUM12: {
            PARSER_CHECK(buffer_read_bytes(buffer, (uint8_t *) op->asset_code, 12))
            op->asset_code[12] = '\0';
            break;
        }
        default:
            return false;  // unknown asset type
    }

    return buffer_read32(buffer, &op->authorize);
}

bool parse_account_merge(buffer_t *buffer, account_merge_op_t *op) {
    return parse_muxed_account(buffer, &op->destination);
}

bool parse_manage_data(buffer_t *buffer, manage_data_op_t *op) {
    size_t size;

    PARSER_CHECK(parse_binary_string_ptr(buffer,
                                         (const uint8_t **) &op->data_name,
                                         &size,
                                         DATA_NAME_MAX_SIZE))
    op->data_name_size = size;

    bool has_value;
    PARSER_CHECK(buffer_read_bool(buffer, &has_value))
    if (has_value) {
        PARSER_CHECK(parse_binary_string_ptr(buffer,
                                             (const uint8_t **) &op->data_value,
                                             &size,
                                             DATA_VALUE_MAX_SIZE))
        op->data_value_size = size;
    } else {
        op->data_value_size = 0;
    }
    return true;
}

bool parse_price(buffer_t *buffer, price_t *price) {
    PARSER_CHECK(buffer_read32(buffer, (uint32_t *) &price->n))
    PARSER_CHECK(buffer_read32(buffer, (uint32_t *) &price->d))

    // Denominator cannot be null, as it would lead to a division by zero.
    return price->d != 0;
}

bool parse_manage_sell_offer(buffer_t *buffer, manage_sell_offer_op_t *op) {
    PARSER_CHECK(parse_asset(buffer, &op->selling))
    PARSER_CHECK(parse_asset(buffer, &op->buying))
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->amount))
    PARSER_CHECK(parse_price(buffer, &op->price))
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->offer_id))
    return true;
}

bool parse_manage_buy_offer(buffer_t *buffer, manage_buy_offer_op_t *op) {
    PARSER_CHECK(parse_asset(buffer, &op->selling))
    PARSER_CHECK(parse_asset(buffer, &op->buying))
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->buy_amount))
    PARSER_CHECK(parse_price(buffer, &op->price))
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->offer_id))
    return true;
}

bool parse_create_passive_sell_offer(buffer_t *buffer, create_passive_sell_offer_op_t *op) {
    PARSER_CHECK(parse_asset(buffer, &op->selling))
    PARSER_CHECK(parse_asset(buffer, &op->buying))
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->amount))
    PARSER_CHECK(parse_price(buffer, &op->price))
    return true;
}

bool parse_change_trust(buffer_t *buffer, change_trust_op_t *op) {
    PARSER_CHECK(parse_change_trust_asset(buffer, &op->line))
    return buffer_read64(buffer, &op->limit);
}

bool parse_signer(buffer_t *buffer, signer_t *signer) {
    PARSER_CHECK(parse_signer_key(buffer, &signer->key))
    PARSER_CHECK(buffer_read32(buffer, &signer->weight))
    return true;
}

bool parse_set_options(buffer_t *buffer, set_options_op_t *set_options) {
    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) parse_account_id,
                                     &set_options->inflation_destination,
                                     &set_options->inflation_destination_present))

    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) buffer_read32,
                                     &set_options->clear_flags,
                                     &set_options->clear_flags_present))
    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) buffer_read32,
                                     &set_options->set_flags,
                                     &set_options->set_flags_present))
    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) buffer_read32,
                                     &set_options->master_weight,
                                     &set_options->master_weight_present))
    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) buffer_read32,
                                     &set_options->low_threshold,
                                     &set_options->low_threshold_present))
    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) buffer_read32,
                                     &set_options->medium_threshold,
                                     &set_options->medium_threshold_present))
    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) buffer_read32,
                                     &set_options->high_threshold,
                                     &set_options->high_threshold_present))

    uint32_t home_domain_present;
    PARSER_CHECK(buffer_read32(buffer, &home_domain_present))
    set_options->home_domain_present = home_domain_present ? true : false;
    if (set_options->home_domain_present) {
        if (!buffer_read32(buffer, &set_options->home_domain_size) ||
            set_options->home_domain_size > HOME_DOMAIN_MAX_SIZE) {
            return false;
        }
        PARSER_CHECK(buffer_can_read(buffer, num_bytes(set_options->home_domain_size)))
        set_options->home_domain = buffer->ptr + buffer->offset;
        PARSER_CHECK(check_padding(set_options->home_domain,
                                   set_options->home_domain_size,
                                   num_bytes(set_options->home_domain_size)))  // security check
        buffer->offset += num_bytes(set_options->home_domain_size);
    } else {
        set_options->home_domain_size = 0;
    }

    return parse_optional_type(buffer,
                               (xdr_type_reader) parse_signer,
                               &set_options->signer,
                               &set_options->signer_present);
}

bool parse_bump_sequence(buffer_t *buffer, bump_sequence_op_t *op) {
    return buffer_read64(buffer, (uint64_t *) &op->bump_to);
}

bool parse_path_payment_strict_send(buffer_t *buffer, path_payment_strict_send_op_t *op) {
    uint32_t path_len;

    PARSER_CHECK(parse_asset(buffer, &op->send_asset))
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->send_amount))
    PARSER_CHECK(parse_muxed_account(buffer, &op->destination))
    PARSER_CHECK(parse_asset(buffer, &op->dest_asset))
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->dest_min))
    PARSER_CHECK(buffer_read32(buffer, (uint32_t *) &path_len))
    if (path_len > PATH_PAYMENT_MAX_PATH_LENGTH) {
        return false;
    }
    return true;
}

bool parse_claimant_predicate(buffer_t *buffer) {
    // Currently, does not support displaying claimant details.
    // So here we will not store the parsed data, just to ensure that the data can be parsed
    // correctly.
    uint32_t claim_predicate_type;
    uint32_t predicates_len;
    bool not_predicate_present;
    int64_t abs_before;
    int64_t rel_before;
    PARSER_CHECK(buffer_read32(buffer, &claim_predicate_type))
    switch (claim_predicate_type) {
        case CLAIM_PREDICATE_UNCONDITIONAL:
            return true;
        case CLAIM_PREDICATE_AND:
        case CLAIM_PREDICATE_OR:
            PARSER_CHECK(buffer_read32(buffer, &predicates_len))
            if (predicates_len != 2) {
                return false;
            }
            PARSER_CHECK(parse_claimant_predicate(buffer))
            PARSER_CHECK(parse_claimant_predicate(buffer))
            return true;
        case CLAIM_PREDICATE_NOT:
            PARSER_CHECK(buffer_read_bool(buffer, &not_predicate_present))
            if (not_predicate_present) {
                PARSER_CHECK(parse_claimant_predicate(buffer))
            }
            return true;
        case CLAIM_PREDICATE_BEFORE_ABSOLUTE_TIME:
            PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &abs_before))
            return true;
        case CLAIM_PREDICATE_BEFORE_RELATIVE_TIME:
            PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &rel_before))
            return true;
        default:
            return false;
    }
}

bool parse_claimant(buffer_t *buffer, claimant_t *claimant) {
    uint32_t claimant_type;
    PARSER_CHECK(buffer_read32(buffer, &claimant_type))
    claimant->type = claimant_type;

    switch (claimant->type) {
        case CLAIMANT_TYPE_V0:
            PARSER_CHECK(parse_account_id(buffer, &claimant->v0.destination))
            PARSER_CHECK(parse_claimant_predicate(buffer))
            return true;
        default:
            return false;
    }
}

bool parse_create_claimable_balance(buffer_t *buffer, create_claimable_balance_op_t *op) {
    uint32_t claimant_len;
    PARSER_CHECK(parse_asset(buffer, &op->asset))
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->amount))
    PARSER_CHECK(buffer_read32(buffer, (uint32_t *) &claimant_len))
    if (claimant_len > CLAIMANTS_MAX_LENGTH) {
        return false;
    }
    op->claimant_len = claimant_len;
    for (int i = 0; i < op->claimant_len; i++) {
        PARSER_CHECK(parse_claimant(buffer, &op->claimants[i]))
    }
    return true;
}
bool parse_claimable_balance_id(buffer_t *buffer, claimable_balance_id *claimable_balance_id) {
    uint32_t claimable_balance_id_type;
    PARSER_CHECK(buffer_read32(buffer, &claimable_balance_id_type))
    claimable_balance_id->type = claimable_balance_id_type;

    switch (claimable_balance_id->type) {
        case CLAIMABLE_BALANCE_ID_TYPE_V0:
            PARSER_CHECK(buffer_can_read(buffer, CLAIMABLE_BALANCE_ID_SIZE))
            claimable_balance_id->v0 = buffer->ptr + buffer->offset;
            PARSER_CHECK(buffer_advance(buffer, CLAIMABLE_BALANCE_ID_SIZE))
            return true;
        default:
            return false;
    }
}

bool parse_claim_claimable_balance(buffer_t *buffer, claim_claimable_balance_op_t *op) {
    PARSER_CHECK(parse_claimable_balance_id(buffer, &op->balance_id))
    return true;
}

bool parse_begin_sponsoring_future_reserves(buffer_t *buffer,
                                            begin_sponsoring_future_reserves_op_t *op) {
    PARSER_CHECK(parse_account_id(buffer, &op->sponsored_id))
    return true;
}

bool parse_ledger_key(buffer_t *buffer, ledger_key_t *ledger_key) {
    uint32_t ledger_entry_type;
    PARSER_CHECK(buffer_read32(buffer, &ledger_entry_type))
    ledger_key->type = ledger_entry_type;
    switch (ledger_key->type) {
        case ACCOUNT:
            PARSER_CHECK(parse_account_id(buffer, &ledger_key->account.account_id))
            return true;
        case TRUSTLINE:
            PARSER_CHECK(parse_account_id(buffer, &ledger_key->trust_line.account_id))
            PARSER_CHECK(parse_trust_line_asset(buffer, &ledger_key->trust_line.asset))
            return true;
        case OFFER:
            PARSER_CHECK(parse_account_id(buffer, &ledger_key->offer.seller_id))
            PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &ledger_key->offer.offer_id))
            return true;
        case DATA:
            PARSER_CHECK(parse_account_id(buffer, &ledger_key->data.account_id))
            PARSER_CHECK(parse_binary_string_ptr(buffer,
                                                 (const uint8_t **) &ledger_key->data.data_name,
                                                 (size_t *) &ledger_key->data.data_name_size,
                                                 DATA_NAME_MAX_SIZE))
            return true;
        case CLAIMABLE_BALANCE:
            PARSER_CHECK(
                parse_claimable_balance_id(buffer, &ledger_key->claimable_balance.balance_id))
            return true;
        case LIQUIDITY_POOL:
            PARSER_CHECK(buffer_can_read(buffer, LIQUIDITY_POOL_ID_SIZE))
            ledger_key->liquidity_pool.liquidity_pool_id = buffer->ptr + buffer->offset;
            PARSER_CHECK(buffer_advance(buffer, LIQUIDITY_POOL_ID_SIZE))
            return true;
        default:
            return false;
    }
}

bool parse_revoke_sponsorship(buffer_t *buffer, revoke_sponsorship_op_t *op) {
    uint32_t revoke_sponsorship_type;
    PARSER_CHECK(buffer_read32(buffer, &revoke_sponsorship_type))
    op->type = revoke_sponsorship_type;

    switch (op->type) {
        case REVOKE_SPONSORSHIP_LEDGER_ENTRY:
            PARSER_CHECK(parse_ledger_key(buffer, &op->ledger_key))
            return true;
        case REVOKE_SPONSORSHIP_SIGNER:
            PARSER_CHECK(parse_account_id(buffer, &op->signer.account_id))
            PARSER_CHECK(parse_signer_key(buffer, &op->signer.signer_key))
            return true;
        default:
            return false;
    }
}

bool parse_clawback(buffer_t *buffer, clawback_op_t *op) {
    PARSER_CHECK(parse_asset(buffer, &op->asset))
    PARSER_CHECK(parse_muxed_account(buffer, &op->from))
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->amount))
    return true;
}

bool parse_clawback_claimable_balance(buffer_t *buffer, clawback_claimable_balance_op_t *op) {
    PARSER_CHECK(parse_claimable_balance_id(buffer, &op->balance_id))
    return true;
}

bool parse_set_trust_line_flags(buffer_t *buffer, set_trust_line_flags_op_t *op) {
    PARSER_CHECK(parse_account_id(buffer, &op->trustor))
    PARSER_CHECK(parse_asset(buffer, &op->asset))
    PARSER_CHECK(buffer_read32(buffer, &op->clear_flags))
    PARSER_CHECK(buffer_read32(buffer, &op->set_flags))
    return true;
}

bool parse_liquidity_pool_deposit(buffer_t *buffer, liquidity_pool_deposit_op_t *op) {
    PARSER_CHECK(buffer_can_read(buffer, LIQUIDITY_POOL_ID_SIZE))
    op->liquidity_pool_id = buffer->ptr + buffer->offset;
    PARSER_CHECK(buffer_advance(buffer, LIQUIDITY_POOL_ID_SIZE))
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->max_amount_a))
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->max_amount_b))
    PARSER_CHECK(parse_price(buffer, &op->min_price))
    PARSER_CHECK(parse_price(buffer, &op->max_price))
    return true;
}

bool parse_liquidity_pool_withdraw(buffer_t *buffer, liquidity_pool_withdraw_op_t *op) {
    PARSER_CHECK(buffer_can_read(buffer, LIQUIDITY_POOL_ID_SIZE))
    op->liquidity_pool_id = buffer->ptr + buffer->offset;
    PARSER_CHECK(buffer_advance(buffer, LIQUIDITY_POOL_ID_SIZE))
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->amount))
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->min_amount_a))
    PARSER_CHECK(buffer_read64(buffer, (uint64_t *) &op->min_amount_b))
    return true;
}

bool parse_operation(buffer_t *buffer, operation_t *operation) {
    explicit_bzero(operation, sizeof(operation_t));
    uint32_t op_type;

    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) parse_muxed_account,
                                     &operation->source_account,
                                     &operation->source_account_present))

    PARSER_CHECK(buffer_read32(buffer, &op_type))
    operation->type = op_type;
    switch (operation->type) {
        case OPERATION_TYPE_CREATE_ACCOUNT: {
            return parse_create_account(buffer, &operation->create_account_op);
        }
        case OPERATION_TYPE_PAYMENT: {
            return parse_payment(buffer, &operation->payment_op);
        }
        case OPERATION_TYPE_PATH_PAYMENT_STRICT_RECEIVE: {
            return parse_path_payment_strict_receive(buffer,
                                                     &operation->path_payment_strict_receive_op);
        }
        case OPERATION_TYPE_CREATE_PASSIVE_SELL_OFFER: {
            return parse_create_passive_sell_offer(buffer,
                                                   &operation->create_passive_sell_offer_op);
        }
        case OPERATION_TYPE_MANAGE_SELL_OFFER: {
            return parse_manage_sell_offer(buffer, &operation->manage_sell_offer_op);
        }
        case OPERATION_TYPE_SET_OPTIONS: {
            return parse_set_options(buffer, &operation->set_options_op);
        }
        case OPERATION_TYPE_CHANGE_TRUST: {
            return parse_change_trust(buffer, &operation->change_trust_op);
        }
        case OPERATION_TYPE_ALLOW_TRUST: {
            return parse_allow_trust(buffer, &operation->allow_trust_op);
        }
        case OPERATION_TYPE_ACCOUNT_MERGE: {
            return parse_account_merge(buffer, &operation->account_merge_op);
        }
        case OPERATION_TYPE_INFLATION: {
            return true;
        }
        case OPERATION_TYPE_MANAGE_DATA: {
            return parse_manage_data(buffer, &operation->manage_data_op);
        }
        case OPERATION_TYPE_BUMP_SEQUENCE: {
            return parse_bump_sequence(buffer, &operation->bump_sequence_op);
        }
        case OPERATION_TYPE_MANAGE_BUY_OFFER: {
            return parse_manage_buy_offer(buffer, &operation->manage_buy_offer_op);
        }
        case OPERATION_TYPE_PATH_PAYMENT_STRICT_SEND: {
            return parse_path_payment_strict_send(buffer, &operation->path_payment_strict_send_op);
        }
        case OPERATION_TYPE_CREATE_CLAIMABLE_BALANCE: {
            return parse_create_claimable_balance(buffer, &operation->create_claimable_balance_op);
        }
        case OPERATION_TYPE_CLAIM_CLAIMABLE_BALANCE: {
            return parse_claim_claimable_balance(buffer, &operation->claim_claimable_balance_op);
        }
        case OPERATION_TYPE_BEGIN_SPONSORING_FUTURE_RESERVES: {
            return parse_begin_sponsoring_future_reserves(
                buffer,
                &operation->begin_sponsoring_future_reserves_op);
        }
        case OPERATION_TYPE_END_SPONSORING_FUTURE_RESERVES: {
            return true;
        }
        case OPERATION_TYPE_REVOKE_SPONSORSHIP: {
            return parse_revoke_sponsorship(buffer, &operation->revoke_sponsorship_op);
        }
        case OPERATION_TYPE_CLAWBACK: {
            return parse_clawback(buffer, &operation->clawback_op);
        }
        case OPERATION_TYPE_CLAWBACK_CLAIMABLE_BALANCE: {
            return parse_clawback_claimable_balance(buffer,
                                                    &operation->clawback_claimable_balance_op);
        }
        case OPERATION_TYPE_SET_TRUST_LINE_FLAGS: {
            return parse_set_trust_line_flags(buffer, &operation->set_trust_line_flags_op);
        }
        case OPERATION_TYPE_LIQUIDITY_POOL_DEPOSIT:
            return parse_liquidity_pool_deposit(buffer, &operation->liquidity_pool_deposit_op);
        case OPERATION_TYPE_LIQUIDITY_POOL_WITHDRAW:
            return parse_liquidity_pool_withdraw(buffer, &operation->liquidity_pool_withdraw_op);
        default:
            return false;
    }
    return false;
}

bool parse_transaction_source(buffer_t *buffer, muxed_account_t *source) {
    return parse_muxed_account(buffer, source);
}

bool parse_transaction_fee(buffer_t *buffer, uint32_t *fee) {
    return buffer_read32(buffer, fee);
}

bool parse_transaction_sequence(buffer_t *buffer, sequence_number_t *sequence_number) {
    return buffer_read64(buffer, (uint64_t *) sequence_number);
}

bool parse_transaction_preconditions(buffer_t *buffer, preconditions_t *preconditions) {
    return parse_preconditions(buffer, preconditions);
}

bool parse_transaction_memo(buffer_t *buffer, memo_t *memo) {
    return parse_memo(buffer, memo);
}

bool parse_transaction_operation_len(buffer_t *buffer, uint8_t *operations_count) {
    uint32_t len;
    PARSER_CHECK(buffer_read32(buffer, &len))
    if (len > MAX_OPS) {
        return false;
    }
    *operations_count = len;
    return true;
}

bool parse_transaction_details(buffer_t *buffer, transaction_details_t *transaction) {
    // account used to run the (inner)transaction
    PARSER_CHECK(parse_transaction_source(buffer, &transaction->source_account))

    // the fee the source_account will pay
    PARSER_CHECK(parse_transaction_fee(buffer, &transaction->fee))

    // sequence number to consume in the account
    PARSER_CHECK(parse_transaction_sequence(buffer, &transaction->sequence_number))

    // validity conditions
    PARSER_CHECK(parse_transaction_preconditions(buffer, &transaction->cond))

    PARSER_CHECK(parse_transaction_memo(buffer, &transaction->memo))
    PARSER_CHECK(parse_transaction_operation_len(buffer, &transaction->operations_count))
    return true;
}

bool parse_fee_bump_transaction_fee_source(buffer_t *buffer, muxed_account_t *fee_source) {
    return parse_muxed_account(buffer, fee_source);
}

bool parse_fee_bump_transaction_fee(buffer_t *buffer, int64_t *fee) {
    return buffer_read64(buffer, (uint64_t *) fee);
}

bool parse_fee_bump_transaction_details(buffer_t *buffer,
                                        fee_bump_transaction_details_t *fee_bump_transaction) {
    PARSER_CHECK(parse_fee_bump_transaction_fee_source(buffer, &fee_bump_transaction->fee_source))
    PARSER_CHECK(parse_fee_bump_transaction_fee(buffer, &fee_bump_transaction->fee))
    return true;
}

bool parse_fee_bump_transaction_ext(buffer_t *buffer) {
    uint32_t ext;
    PARSER_CHECK(buffer_read32(buffer, &ext))
    if (ext != 0) {
        return false;
    }
    return true;
}

bool parse_transaction_envelope_type(buffer_t *buffer, envelope_type_t *envelope_type) {
    uint32_t type;
    PARSER_CHECK(buffer_read32(buffer, &type))
    if (type != ENVELOPE_TYPE_TX && type != ENVELOPE_TYPE_TX_FEE_BUMP) {
        THROW(SW_UNKNOWN_ENVELOPE_TYPE);
        return false;
    }

    *envelope_type = type;
    return true;
}

bool parse_network(buffer_t *buffer, uint8_t *network) {
    PARSER_CHECK(buffer_can_read(buffer, HASH_SIZE))
    if (memcmp(buffer->ptr, NETWORK_ID_PUBLIC_HASH, HASH_SIZE) == 0) {
        *network = NETWORK_TYPE_PUBLIC;
    } else if (memcmp(buffer->ptr, NETWORK_ID_TEST_HASH, HASH_SIZE) == 0) {
        *network = NETWORK_TYPE_TEST;
    } else {
        *network = NETWORK_TYPE_UNKNOWN;
    }
    PARSER_CHECK(buffer_advance(buffer, HASH_SIZE))
    return true;
}

bool parse_tx_xdr(const uint8_t *data, size_t size, tx_ctx_t *tx_ctx) {
    buffer_t buffer = {data, size, 0};
    uint32_t envelope_type;

    uint16_t offset = tx_ctx->offset;
    buffer.offset = tx_ctx->offset;

    if (offset == 0) {
        explicit_bzero(&tx_ctx->tx_details, sizeof(transaction_details_t));
        explicit_bzero(&tx_ctx->fee_bump_tx_details, sizeof(fee_bump_transaction_details_t));
        PARSER_CHECK(parse_network(&buffer, &tx_ctx->network))
        PARSER_CHECK(buffer_read32(&buffer, &envelope_type))
        tx_ctx->envelope_type = envelope_type;
        switch (envelope_type) {
            case ENVELOPE_TYPE_TX:
                PARSER_CHECK(parse_transaction_details(&buffer, &tx_ctx->tx_details))
                break;
            case ENVELOPE_TYPE_TX_FEE_BUMP:
                PARSER_CHECK(
                    parse_fee_bump_transaction_details(&buffer, &tx_ctx->fee_bump_tx_details))
                uint32_t inner_envelope_type;
                PARSER_CHECK(buffer_read32(&buffer, &inner_envelope_type))
                if (inner_envelope_type != ENVELOPE_TYPE_TX) {
                    return false;
                }
                PARSER_CHECK(parse_transaction_details(&buffer, &tx_ctx->tx_details))
                break;
            default:
                THROW(SW_UNKNOWN_OP);
                return false;
        }
    }

    PARSER_CHECK(parse_operation(&buffer, &tx_ctx->tx_details.op_details))
    offset = buffer.offset;
    tx_ctx->tx_details.operation_index += 1;
    tx_ctx->offset = offset;
    return true;
}
