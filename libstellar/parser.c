#include <stdint.h>
#include <string.h>

#include "stellar/parser.h"
#include "stellar/types.h"
#include "stellar/network.h"

#ifdef TEST
#include <stdio.h>
#else
#include "os.h"
#endif

#define PARSER_CHECK(x)         \
    {                           \
        if (!(x)) return false; \
    }

bool read_scval_advance(buffer_t *buffer);

static bool buffer_advance(buffer_t *buffer, size_t num_bytes) {
    return buffer_seek_cur(buffer, num_bytes);
}

static bool buffer_read_bytes(buffer_t *buffer, const uint8_t **data, size_t size) {
    PARSER_CHECK(buffer_can_read(buffer, size))
    *data = buffer->ptr + buffer->offset;
    PARSER_CHECK(buffer_advance(buffer, size))
    return true;
}

static int64_t read_i64_be(const uint8_t *ptr, size_t offset) {
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result = (result << 8) | ptr[offset + i];
    }
    return (int64_t) result;
}

static int32_t read_i32_be(const uint8_t *ptr, size_t offset) {
    uint32_t result = 0;
    for (int i = 0; i < 4; i++) {
        result = (result << 8) | ptr[offset + i];
    }
    return (int32_t) result;
}

bool parse_bool(buffer_t *buffer, bool *b) {
    uint32_t val;

    if (!parse_uint32(buffer, &val)) {
        return false;
    }
    if (val != 0 && val != 1) {
        return false;
    }
    *b = val == 1 ? true : false;
    return true;
}

bool parse_uint64(buffer_t *buffer, uint64_t *n) {
    return buffer_read_u64(buffer, n, BE);
}

bool parse_int64(buffer_t *buffer, int64_t *n) {
    if (!buffer_can_read(buffer, 8)) {
        *n = 0;
        return false;
    }
    *n = read_i64_be(buffer->ptr, buffer->offset);
    return buffer_seek_cur(buffer, 8);
}

bool parse_uint32(buffer_t *buffer, uint32_t *n) {
    return buffer_read_u32(buffer, n, BE);
}

bool parse_int32(buffer_t *buffer, int32_t *n) {
    if (!buffer_can_read(buffer, 4)) {
        *n = 0;
        return false;
    }
    *n = read_i32_be(buffer->ptr, buffer->offset);
    return buffer_seek_cur(buffer, 4);
}

static bool num_bytes(size_t size, size_t *out_size) {
    size_t remainder = size % 4;
    if (remainder == 0) {
        *out_size = size;
        return true;
    }
    if (size > SIZE_MAX - 4) {
        // size + 4 would overflow
        return false;
    }
    *out_size = size + 4 - remainder;
    return true;
}

static bool check_padding(const uint8_t *buffer, size_t offset, size_t length) {
    size_t i;
    for (i = 0; i < length - offset; i++) {
        if (buffer[offset + i] != 0x00) {
            return false;
        }
    }
    return true;
}

static bool parse_binary_string_ptr(buffer_t *buffer,
                                    const uint8_t **string,
                                    size_t *out_len,
                                    size_t max_length) {
    /* max_length does not include terminal null character */
    uint32_t size;

    PARSER_CHECK(parse_uint32(buffer, &size))
    if (max_length != 0 && size > max_length) {
        return false;
    }

    size_t data_size = 0;
    PARSER_CHECK(num_bytes(size, &data_size))
    PARSER_CHECK(buffer_read_bytes(buffer, string, data_size))
    PARSER_CHECK(check_padding(*string, size, data_size))  // security check
    if (out_len) {
        *out_len = size;
    }
    return true;
}

typedef bool (*xdr_type_reader)(buffer_t *, void *);

static bool parse_optional_type(buffer_t *buffer, xdr_type_reader reader, void *dst, bool *opted) {
    bool is_present;

    PARSER_CHECK(parse_bool(buffer, &is_present))
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

bool parse_scv_symbol(buffer_t *buffer, scv_symbol_t *symbol) {
    PARSER_CHECK(
        parse_binary_string_ptr(buffer, &symbol->symbol, &symbol->size, SCV_SYMBOL_MAX_SIZE))
    return true;
}

bool parse_scv_string(buffer_t *buffer, scv_string_t *string) {
    PARSER_CHECK(parse_binary_string_ptr(buffer, &string->string, &string->size, 0))
    return true;
}

static bool parse_signer_key(buffer_t *buffer, signer_key_t *key) {
    PARSER_CHECK(parse_uint32(buffer, &key->type))
    switch (key->type) {
        case SIGNER_KEY_TYPE_ED25519:
            PARSER_CHECK(buffer_read_bytes(buffer, &key->ed25519, RAW_ED25519_PUBLIC_KEY_SIZE))
            return true;
        case SIGNER_KEY_TYPE_PRE_AUTH_TX:
            PARSER_CHECK(buffer_read_bytes(buffer, &key->pre_auth_tx, RAW_PRE_AUTH_TX_KEY_SIZE))
            return true;
        case SIGNER_KEY_TYPE_HASH_X:
            PARSER_CHECK(buffer_read_bytes(buffer, &key->hash_x, RAW_HASH_X_KEY_SIZE))
            return true;
        case SIGNER_KEY_TYPE_ED25519_SIGNED_PAYLOAD:
            PARSER_CHECK(buffer_read_bytes(buffer,
                                           &key->ed25519_signed_payload.ed25519,
                                           RAW_ED25519_PUBLIC_KEY_SIZE))
            PARSER_CHECK(parse_uint32(buffer, &key->ed25519_signed_payload.payload_len))
            // valid length [1, 64]
            if (key->ed25519_signed_payload.payload_len == 0 ||
                key->ed25519_signed_payload.payload_len > 64) {
                return false;
            }
            size_t data_size = 0;
            PARSER_CHECK(num_bytes(key->ed25519_signed_payload.payload_len, &data_size))
            PARSER_CHECK(buffer_read_bytes(buffer, &key->ed25519_signed_payload.payload, data_size))
            return true;
        default:
            return false;
    }
}

static bool parse_account_id(buffer_t *buffer, const uint8_t **account_id) {
    uint32_t account_type;

    PARSER_CHECK(parse_uint32(buffer, &account_type))
    if (account_type != PUBLIC_KEY_TYPE_ED25519) {
        return false;
    }
    PARSER_CHECK(buffer_read_bytes(buffer, account_id, RAW_ED25519_PUBLIC_KEY_SIZE))
    return true;
}

static bool parse_muxed_account(buffer_t *buffer, muxed_account_t *muxed_account) {
    PARSER_CHECK(parse_uint32(buffer, &muxed_account->type))

    switch (muxed_account->type) {
        case KEY_TYPE_ED25519:
            PARSER_CHECK(
                buffer_read_bytes(buffer, &muxed_account->ed25519, RAW_ED25519_PUBLIC_KEY_SIZE))
            return true;
        case KEY_TYPE_MUXED_ED25519:
            PARSER_CHECK(parse_uint64(buffer, &muxed_account->med25519.id))
            PARSER_CHECK(buffer_read_bytes(buffer,
                                           &muxed_account->med25519.ed25519,
                                           RAW_ED25519_PUBLIC_KEY_SIZE))
            return true;
        default:
            return false;
    }
}

static bool parse_time_bounds(buffer_t *buffer, time_bounds_t *bounds) {
    PARSER_CHECK(parse_uint64(buffer, &bounds->min_time))
    PARSER_CHECK(parse_uint64(buffer, &bounds->max_time))
    return true;
}

static bool parse_ledger_bounds(buffer_t *buffer, ledger_bounds_t *ledger_bounds) {
    PARSER_CHECK(parse_uint32(buffer, &ledger_bounds->min_ledger))
    PARSER_CHECK(parse_uint32(buffer, &ledger_bounds->max_ledger))
    return true;
}

static bool parse_extra_signers(buffer_t *buffer) {
    uint32_t length;
    PARSER_CHECK(parse_uint32(buffer, &length))
    if (length > 2) {  // maximum length is 2
        return false;
    }
    signer_key_t signer_key;
    for (uint32_t i = 0; i < length; i++) {
        PARSER_CHECK(parse_signer_key(buffer, &signer_key))
    }
    return true;
}

static bool parse_preconditions(buffer_t *buffer, preconditions_t *cond) {
    uint32_t precondition_type;
    PARSER_CHECK(parse_uint32(buffer, &precondition_type))
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
                                             (xdr_type_reader) parse_int64,
                                             &cond->min_seq_num,
                                             &cond->min_seq_num_present))
            PARSER_CHECK(parse_int64(buffer, &cond->min_seq_age))
            PARSER_CHECK(parse_uint32(buffer, &cond->min_seq_ledger_gap))
            PARSER_CHECK(parse_extra_signers(buffer))
            return true;
        default:
            return false;
    }
}

static bool parse_memo(buffer_t *buffer, memo_t *memo) {
    PARSER_CHECK(parse_uint32(buffer, &memo->type))
    switch (memo->type) {
        case MEMO_NONE:
            return true;
        case MEMO_ID:
            return parse_uint64(buffer, &memo->id);
        case MEMO_TEXT: {
            PARSER_CHECK(parse_binary_string_ptr(buffer,
                                                 &memo->text.text,
                                                 &memo->text.text_size,
                                                 MEMO_TEXT_MAX_SIZE))
            return true;
        }
        case MEMO_HASH:
            return buffer_read_bytes(buffer, &memo->hash, HASH_SIZE);
        case MEMO_RETURN:
            return buffer_read_bytes(buffer, &memo->return_hash, HASH_SIZE);
        default:
            return false;  // unknown memo type
    }
}

static bool parse_alpha_num4_asset(buffer_t *buffer, alpha_num4_t *asset) {
    PARSER_CHECK(buffer_read_bytes(buffer, &asset->asset_code, 4))
    PARSER_CHECK(parse_account_id(buffer, &asset->issuer))
    return true;
}

static bool parse_alpha_num12_asset(buffer_t *buffer, alpha_num12_t *asset) {
    PARSER_CHECK(buffer_read_bytes(buffer, &asset->asset_code, 12))
    PARSER_CHECK(parse_account_id(buffer, &asset->issuer))
    return true;
}

static bool parse_asset(buffer_t *buffer, asset_t *asset) {
    PARSER_CHECK(parse_uint32(buffer, &asset->type))
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

static bool parse_trust_line_asset(buffer_t *buffer, trust_line_asset_t *asset) {
    PARSER_CHECK(parse_uint32(buffer, &asset->type))
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
            PARSER_CHECK(
                buffer_read_bytes(buffer, &asset->liquidity_pool_id, LIQUIDITY_POOL_ID_SIZE))
            return true;
        }
        default:
            return false;  // unknown asset type
    }
}

static bool parse_liquidity_pool_parameters(
    buffer_t *buffer,
    liquidity_pool_parameters_t *liquidity_pool_parameters) {
    uint32_t liquidity_pool_type;
    PARSER_CHECK(parse_uint32(buffer, &liquidity_pool_type))
    switch (liquidity_pool_type) {
        case LIQUIDITY_POOL_CONSTANT_PRODUCT: {
            PARSER_CHECK(parse_asset(buffer, &liquidity_pool_parameters->constant_product.asset_a))
            PARSER_CHECK(parse_asset(buffer, &liquidity_pool_parameters->constant_product.asset_b))
            PARSER_CHECK(parse_int32(buffer, &liquidity_pool_parameters->constant_product.fee))
            return true;
        }
        default:
            return false;
    }
}

static bool parse_change_trust_asset(buffer_t *buffer, change_trust_asset_t *asset) {
    PARSER_CHECK(parse_uint32(buffer, &asset->type))
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

static bool parse_create_account(buffer_t *buffer, create_account_op_t *create_account_op) {
    PARSER_CHECK(parse_account_id(buffer, &create_account_op->destination))
    PARSER_CHECK(parse_int64(buffer, &create_account_op->starting_balance))
    return true;
}

static bool parse_payment(buffer_t *buffer, payment_op_t *payment_op) {
    PARSER_CHECK(parse_muxed_account(buffer, &payment_op->destination))
    PARSER_CHECK(parse_asset(buffer, &payment_op->asset))
    PARSER_CHECK(parse_int64(buffer, &payment_op->amount))
    return true;
}

static bool parse_path_payment_strict_receive(buffer_t *buffer,
                                              path_payment_strict_receive_op_t *op) {
    uint32_t path_len;

    PARSER_CHECK(parse_asset(buffer, &op->send_asset))
    PARSER_CHECK(parse_int64(buffer, &op->send_max))
    PARSER_CHECK(parse_muxed_account(buffer, &op->destination))
    PARSER_CHECK(parse_asset(buffer, &op->dest_asset))
    PARSER_CHECK(parse_int64(buffer, &op->dest_amount))

    PARSER_CHECK(parse_uint32(buffer, &path_len))
    if (path_len > PATH_PAYMENT_MAX_PATH_LENGTH) {
        return false;
    }
    for (uint32_t i = 0; i < path_len; i++) {
        asset_t tmp_asset;
        PARSER_CHECK(parse_asset(buffer, &tmp_asset))
    }
    return true;
}

static bool parse_allow_trust(buffer_t *buffer, allow_trust_op_t *op) {
    PARSER_CHECK(parse_account_id(buffer, &op->trustor))
    PARSER_CHECK(parse_uint32(buffer, &op->asset_type))

    switch (op->asset_type) {
        case ASSET_TYPE_CREDIT_ALPHANUM4:
            PARSER_CHECK(buffer_read_bytes(buffer, &op->asset_code, 4))
            break;
        case ASSET_TYPE_CREDIT_ALPHANUM12:
            PARSER_CHECK(buffer_read_bytes(buffer, &op->asset_code, 12))
            break;
        default:
            return false;  // unknown asset type
    }

    PARSER_CHECK(parse_uint32(buffer, &op->authorize))
    return true;
}

static bool parse_account_merge(buffer_t *buffer, account_merge_op_t *op) {
    return parse_muxed_account(buffer, &op->destination);
}

static bool parse_manage_data(buffer_t *buffer, manage_data_op_t *op) {
    PARSER_CHECK(
        parse_binary_string_ptr(buffer, &op->data_name, &op->data_name_size, DATA_NAME_MAX_SIZE))

    bool has_value;
    PARSER_CHECK(parse_bool(buffer, &has_value))
    if (has_value) {
        PARSER_CHECK(parse_binary_string_ptr(buffer,
                                             &op->data_value,
                                             &op->data_value_size,
                                             DATA_VALUE_MAX_SIZE))
    } else {
        op->data_value_size = 0;
    }
    return true;
}

static bool parse_price(buffer_t *buffer, price_t *price) {
    PARSER_CHECK(parse_int32(buffer, &price->n))
    PARSER_CHECK(parse_int32(buffer, &price->d))
    // Denominator cannot be null, as it would lead to a division by zero.
    return price->d != 0;
}

static bool parse_manage_sell_offer(buffer_t *buffer, manage_sell_offer_op_t *op) {
    PARSER_CHECK(parse_asset(buffer, &op->selling))
    PARSER_CHECK(parse_asset(buffer, &op->buying))
    PARSER_CHECK(parse_int64(buffer, &op->amount))
    PARSER_CHECK(parse_price(buffer, &op->price))
    PARSER_CHECK(parse_int64(buffer, &op->offer_id))
    return true;
}

static bool parse_manage_buy_offer(buffer_t *buffer, manage_buy_offer_op_t *op) {
    PARSER_CHECK(parse_asset(buffer, &op->selling))
    PARSER_CHECK(parse_asset(buffer, &op->buying))
    PARSER_CHECK(parse_int64(buffer, &op->buy_amount))
    PARSER_CHECK(parse_price(buffer, &op->price))
    PARSER_CHECK(parse_int64(buffer, &op->offer_id))
    return true;
}

static bool parse_create_passive_sell_offer(buffer_t *buffer, create_passive_sell_offer_op_t *op) {
    PARSER_CHECK(parse_asset(buffer, &op->selling))
    PARSER_CHECK(parse_asset(buffer, &op->buying))
    PARSER_CHECK(parse_int64(buffer, &op->amount))
    PARSER_CHECK(parse_price(buffer, &op->price))
    return true;
}

static bool parse_change_trust(buffer_t *buffer, change_trust_op_t *op) {
    PARSER_CHECK(parse_change_trust_asset(buffer, &op->line))
    PARSER_CHECK(parse_int64(buffer, &op->limit));
    // Limit cannot be negative
    return op->limit >= 0;
}

static bool parse_signer(buffer_t *buffer, signer_t *signer) {
    PARSER_CHECK(parse_signer_key(buffer, &signer->key))
    PARSER_CHECK(parse_uint32(buffer, &signer->weight))
    return true;
}

static bool parse_set_options(buffer_t *buffer, set_options_op_t *set_options) {
    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) parse_account_id,
                                     &set_options->inflation_destination,
                                     &set_options->inflation_destination_present))

    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) parse_uint32,
                                     &set_options->clear_flags,
                                     &set_options->clear_flags_present))
    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) parse_uint32,
                                     &set_options->set_flags,
                                     &set_options->set_flags_present))
    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) parse_uint32,
                                     &set_options->master_weight,
                                     &set_options->master_weight_present))
    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) parse_uint32,
                                     &set_options->low_threshold,
                                     &set_options->low_threshold_present))
    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) parse_uint32,
                                     &set_options->medium_threshold,
                                     &set_options->medium_threshold_present))
    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) parse_uint32,
                                     &set_options->high_threshold,
                                     &set_options->high_threshold_present))

    uint32_t home_domain_present;
    PARSER_CHECK(parse_uint32(buffer, &home_domain_present))
    set_options->home_domain_present = home_domain_present ? true : false;
    if (set_options->home_domain_present) {
        PARSER_CHECK(parse_binary_string_ptr(buffer,
                                             &set_options->home_domain,
                                             &set_options->home_domain_size,
                                             HOME_DOMAIN_MAX_SIZE))
    } else {
        set_options->home_domain_size = 0;
    }

    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) parse_signer,
                                     &set_options->signer,
                                     &set_options->signer_present))
    return true;
}

static bool parse_bump_sequence(buffer_t *buffer, bump_sequence_op_t *op) {
    return parse_int64(buffer, &op->bump_to);
}

static bool parse_path_payment_strict_send(buffer_t *buffer, path_payment_strict_send_op_t *op) {
    uint32_t path_len;

    PARSER_CHECK(parse_asset(buffer, &op->send_asset))
    PARSER_CHECK(parse_int64(buffer, &op->send_amount))
    PARSER_CHECK(parse_muxed_account(buffer, &op->destination))
    PARSER_CHECK(parse_asset(buffer, &op->dest_asset))
    PARSER_CHECK(parse_int64(buffer, &op->dest_min))
    PARSER_CHECK(parse_uint32(buffer, &path_len))
    if (path_len > PATH_PAYMENT_MAX_PATH_LENGTH) {
        return false;
    }
    for (uint32_t i = 0; i < path_len; i++) {
        asset_t tmp_asset;
        PARSER_CHECK(parse_asset(buffer, &tmp_asset))
    }
    return true;
}

static bool parse_claimant_predicate(buffer_t *buffer) {
    // Currently, does not support displaying claimant details.
    // So here we will not store the parsed data, just to ensure that the data can be parsed
    // correctly.
    uint32_t claim_predicate_type;
    uint32_t predicates_len;
    bool not_predicate_present;
    int64_t abs_before;
    int64_t rel_before;
    PARSER_CHECK(parse_uint32(buffer, &claim_predicate_type))
    switch (claim_predicate_type) {
        case CLAIM_PREDICATE_UNCONDITIONAL:
            return true;
        case CLAIM_PREDICATE_AND:
        case CLAIM_PREDICATE_OR:
            PARSER_CHECK(parse_uint32(buffer, &predicates_len))
            if (predicates_len != 2) {
                return false;
            }
            PARSER_CHECK(parse_claimant_predicate(buffer))
            PARSER_CHECK(parse_claimant_predicate(buffer))
            return true;
        case CLAIM_PREDICATE_NOT:
            PARSER_CHECK(parse_bool(buffer, &not_predicate_present))
            if (not_predicate_present) {
                PARSER_CHECK(parse_claimant_predicate(buffer))
            }
            return true;
        case CLAIM_PREDICATE_BEFORE_ABSOLUTE_TIME:
            PARSER_CHECK(parse_int64(buffer, &abs_before))
            return true;
        case CLAIM_PREDICATE_BEFORE_RELATIVE_TIME:
            PARSER_CHECK(parse_int64(buffer, &rel_before))
            return true;
        default:
            return false;
    }
}

static bool parse_claimant(buffer_t *buffer, claimant_t *claimant) {
    PARSER_CHECK(parse_uint32(buffer, &claimant->type))
    switch (claimant->type) {
        case CLAIMANT_TYPE_V0:
            PARSER_CHECK(parse_account_id(buffer, &claimant->v0.destination))
            PARSER_CHECK(parse_claimant_predicate(buffer))
            return true;
        default:
            return false;
    }
}

static bool parse_create_claimable_balance(buffer_t *buffer, create_claimable_balance_op_t *op) {
    uint32_t claimant_len;
    PARSER_CHECK(parse_asset(buffer, &op->asset))
    PARSER_CHECK(parse_int64(buffer, &op->amount))
    PARSER_CHECK(parse_uint32(buffer, &claimant_len))
    if (claimant_len > CLAIMANTS_MAX_LENGTH) {
        return false;
    }
    op->claimant_len = claimant_len;
    for (int i = 0; i < op->claimant_len; i++) {
        PARSER_CHECK(parse_claimant(buffer, &op->claimants[i]))
    }
    return true;
}

static bool parse_claimable_balance_id(buffer_t *buffer,
                                       claimable_balance_id_t *claimable_balance_id_t) {
    PARSER_CHECK(parse_uint32(buffer, &claimable_balance_id_t->type))
    switch (claimable_balance_id_t->type) {
        case CLAIMABLE_BALANCE_ID_TYPE_V0:
            PARSER_CHECK(
                buffer_read_bytes(buffer, &claimable_balance_id_t->v0, CLAIMABLE_BALANCE_ID_SIZE))
            return true;
        default:
            return false;
    }
}

static bool parse_claim_claimable_balance(buffer_t *buffer, claim_claimable_balance_op_t *op) {
    PARSER_CHECK(parse_claimable_balance_id(buffer, &op->balance_id))
    return true;
}

static bool parse_begin_sponsoring_future_reserves(buffer_t *buffer,
                                                   begin_sponsoring_future_reserves_op_t *op) {
    PARSER_CHECK(parse_account_id(buffer, &op->sponsored_id))
    return true;
}

bool parse_sc_address(buffer_t *buffer, sc_address_t *sc_address) {
    PARSER_CHECK(parse_uint32(buffer, &sc_address->type))
    switch (sc_address->type) {
        case SC_ADDRESS_TYPE_ACCOUNT:
            PARSER_CHECK(parse_account_id(buffer, &sc_address->address))
            return true;
        case SC_ADDRESS_TYPE_CONTRACT:
            PARSER_CHECK(buffer_read_bytes(buffer, &sc_address->address, RAW_CONTRACT_KEY_SIZE))
            return true;
        default:
            return false;
    }
}

static bool read_scval_vec_advance(buffer_t *buffer) {
    uint32_t vec_len;
    PARSER_CHECK(parse_uint32(buffer, &vec_len))
    for (uint32_t i = 0; i < vec_len; i++) {
        PARSER_CHECK(read_scval_advance(buffer))
    }
    return true;
}

static bool rad_scval_map_advance(buffer_t *buffer) {
    uint32_t map_len;
    PARSER_CHECK(parse_uint32(buffer, &map_len))
    for (uint32_t i = 0; i < map_len; i++) {
        PARSER_CHECK(read_scval_advance(buffer))
        PARSER_CHECK(read_scval_advance(buffer))
    }
    return true;
}

static bool read_contract_executable_advance(buffer_t *buffer) {
    uint32_t type;
    PARSER_CHECK(parse_uint32(buffer, &type))
    switch (type) {
        case CONTRACT_EXECUTABLE_WASM:
            PARSER_CHECK(buffer_advance(buffer, HASH_SIZE))  // code
            break;
        case CONTRACT_EXECUTABLE_STELLAR_ASSET:
            // void
            break;
        default:
            return false;
    }
    return true;
}

bool read_scval_advance(buffer_t *buffer) {
    uint32_t sc_type;
    PARSER_CHECK(parse_uint32(buffer, &sc_type))

    switch (sc_type) {
        case SCV_BOOL:
            PARSER_CHECK(buffer_advance(buffer, 4))
            break;
        case SCV_VOID:
            break;  // void
        case SCV_ERROR:
            return false;  // not implemented
        case SCV_U32:
        case SCV_I32:
            PARSER_CHECK(buffer_advance(buffer, 4))
            break;
        case SCV_U64:
        case SCV_I64:
        case SCV_TIMEPOINT:
        case SCV_DURATION:
            PARSER_CHECK(buffer_advance(buffer, 8))
            break;
        case SCV_U128:
        case SCV_I128:
            PARSER_CHECK(buffer_advance(buffer, 16))
            break;
        case SCV_U256:
        case SCV_I256:
            PARSER_CHECK(buffer_advance(buffer, 32))
            break;
        case SCV_BYTES:
        case SCV_STRING:
        case SCV_SYMBOL: {
            size_t data_size = 0;
            PARSER_CHECK(parse_uint32(buffer, &data_size))
            PARSER_CHECK(num_bytes(data_size, &data_size))
            PARSER_CHECK(buffer_advance(buffer, data_size))
            break;
        }
        case SCV_VEC: {
            bool vec_exists;
            PARSER_CHECK(parse_bool(buffer, &vec_exists))
            if (vec_exists) {
                read_scval_vec_advance(buffer);
            }
            break;
        }
        case SCV_MAP: {
            bool map_exists;
            PARSER_CHECK(parse_bool(buffer, &map_exists))
            if (map_exists) {
                rad_scval_map_advance(buffer);
            }
            break;
        }
        case SCV_ADDRESS: {
            sc_address_t sc_address;
            PARSER_CHECK(parse_sc_address(buffer, &sc_address));
            break;
        }
        case SCV_CONTRACT_INSTANCE: {
            PARSER_CHECK(read_contract_executable_advance(buffer))
            bool map_exists;
            PARSER_CHECK(parse_bool(buffer, &map_exists))
            if (map_exists) {
                rad_scval_map_advance(buffer);
            }
            break;
        }
        case SCV_LEDGER_KEY_CONTRACT_INSTANCE:
            break;  // void
        case SCV_LEDGER_KEY_NONCE:
            PARSER_CHECK(buffer_advance(buffer, 8))
            break;
        default:
            return false;
    }
    return true;
}

static bool parse_ledger_key(buffer_t *buffer, ledger_key_t *ledger_key) {
    PARSER_CHECK(parse_uint32(buffer, &ledger_key->type))
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
            PARSER_CHECK(parse_int64(buffer, &ledger_key->offer.offer_id))
            return true;
        case DATA:
            PARSER_CHECK(parse_account_id(buffer, &ledger_key->data.account_id))
            PARSER_CHECK(parse_binary_string_ptr(buffer,
                                                 &ledger_key->data.data_name,
                                                 &ledger_key->data.data_name_size,
                                                 DATA_NAME_MAX_SIZE))
            return true;
        case CLAIMABLE_BALANCE:
            PARSER_CHECK(
                parse_claimable_balance_id(buffer, &ledger_key->claimable_balance.balance_id))
            return true;
        case LIQUIDITY_POOL:
            PARSER_CHECK(buffer_read_bytes(buffer,
                                           &ledger_key->liquidity_pool.liquidity_pool_id,
                                           LIQUIDITY_POOL_ID_SIZE))

            return true;
        default:
            return false;
    }
}

static bool parse_revoke_sponsorship(buffer_t *buffer, revoke_sponsorship_op_t *op) {
    PARSER_CHECK(parse_uint32(buffer, &op->type))
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

static bool parse_clawback(buffer_t *buffer, clawback_op_t *op) {
    PARSER_CHECK(parse_asset(buffer, &op->asset))
    PARSER_CHECK(parse_muxed_account(buffer, &op->from))
    PARSER_CHECK(parse_int64(buffer, &op->amount))
    return true;
}

static bool parse_clawback_claimable_balance(buffer_t *buffer,
                                             clawback_claimable_balance_op_t *op) {
    PARSER_CHECK(parse_claimable_balance_id(buffer, &op->balance_id))
    return true;
}

static bool parse_set_trust_line_flags(buffer_t *buffer, set_trust_line_flags_op_t *op) {
    PARSER_CHECK(parse_account_id(buffer, &op->trustor))
    PARSER_CHECK(parse_asset(buffer, &op->asset))
    PARSER_CHECK(parse_uint32(buffer, &op->clear_flags))
    PARSER_CHECK(parse_uint32(buffer, &op->set_flags))
    return true;
}

static bool parse_liquidity_pool_deposit(buffer_t *buffer, liquidity_pool_deposit_op_t *op) {
    PARSER_CHECK(buffer_read_bytes(buffer, &op->liquidity_pool_id, LIQUIDITY_POOL_ID_SIZE))
    PARSER_CHECK(parse_int64(buffer, &op->max_amount_a))
    PARSER_CHECK(parse_int64(buffer, &op->max_amount_b))
    PARSER_CHECK(parse_price(buffer, &op->min_price))
    PARSER_CHECK(parse_price(buffer, &op->max_price))
    return true;
}

static bool parse_liquidity_pool_withdraw(buffer_t *buffer, liquidity_pool_withdraw_op_t *op) {
    PARSER_CHECK(buffer_read_bytes(buffer, &op->liquidity_pool_id, LIQUIDITY_POOL_ID_SIZE))
    PARSER_CHECK(parse_int64(buffer, &op->amount))
    PARSER_CHECK(parse_int64(buffer, &op->min_amount_a))
    PARSER_CHECK(parse_int64(buffer, &op->min_amount_b))
    return true;
}

static bool parse_extension_point_v0(buffer_t *buffer) {
    uint32_t v;
    PARSER_CHECK(parse_uint32(buffer, &v))
    if (v != 0) {
        return false;
    }
    return true;
}

static bool parse_restore_footprint(buffer_t *buffer, restore_footprint_op_t *op) {
    (void) op;
    PARSER_CHECK(parse_extension_point_v0(buffer))
    return true;
}

static bool read_parse_soroban_credentials_advance(buffer_t *buffer, uint32_t *credentials_type) {
    PARSER_CHECK(parse_uint32(buffer, credentials_type))
    switch (*credentials_type) {
        case SOROBAN_CREDENTIALS_SOURCE_ACCOUNT:
            // void
            break;
        case SOROBAN_CREDENTIALS_ADDRESS: {
            sc_address_t address;
            PARSER_CHECK(parse_sc_address(buffer, &address))
            PARSER_CHECK(buffer_advance(buffer, 8))   // nonce
            PARSER_CHECK(buffer_advance(buffer, 4))   // signatureExpirationLedger
            PARSER_CHECK(read_scval_advance(buffer))  // signature
            break;
        }
        default:
            return false;
    }
    return true;
}

static bool read_create_contract_args_advance(buffer_t *buffer) {
    // contract_id_preimage
    uint32_t type;
    PARSER_CHECK(parse_uint32(buffer, &type))
    switch (type) {
        case CONTRACT_ID_PREIMAGE_FROM_ADDRESS: {
            sc_address_t address;
            PARSER_CHECK(parse_sc_address(buffer, &address))
            PARSER_CHECK(buffer_advance(buffer, 32))  // salt
            break;
        }
        case CONTRACT_ID_PREIMAGE_FROM_ASSET: {
            asset_t asset;
            PARSER_CHECK(parse_asset(buffer, &asset))
            break;
        }
        default:
            return false;
    }

    // executable
    PARSER_CHECK(read_contract_executable_advance(buffer))
    return true;
}

static bool parse_invoke_contract_args(buffer_t *buffer, invoke_contract_args_t *args) {
    // contractAddress
    PARSER_CHECK(parse_sc_address(buffer, &args->address))
    // functionName
    PARSER_CHECK(parse_binary_string_ptr(buffer,
                                         &args->function.name,
                                         &args->function.name_size,
                                         SCV_SYMBOL_MAX_SIZE))

    // args
    uint32_t args_len;
    PARSER_CHECK(parse_uint32(buffer, &args_len))

    args->parameters_length = args_len;
    args->parameters_position = buffer->offset;

    if (args_len > HOST_FUNCTION_ARGS_MAX_LENGTH) {
        // We dont support more than 10 arguments
        return false;
    }

    // PRINTF("function_name.text_size=%d, function_name.text=%s, args->parameters_length=%d\n",
    //        args->function.name_size,
    //        args->function.name,
    //        args->parameters_length);

    for (uint32_t i = 0; i < args_len; i++) {
        PARSER_CHECK(read_scval_advance(buffer))
    }
    return true;
}

static bool read_soroban_authorized_function_advance(buffer_t *buffer) {
    uint32_t type;
    PARSER_CHECK(parse_uint32(buffer, &type))
    switch (type) {
        case SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN: {
            // contractFn
            invoke_contract_args_t args;
            PARSER_CHECK(parse_invoke_contract_args(buffer, &args));
            break;
        }
        case SOROBAN_AUTHORIZED_FUNCTION_TYPE_CREATE_CONTRACT_HOST_FN:
            // createContractHostFn
            PARSER_CHECK(read_create_contract_args_advance(buffer));
            break;
        default:
            return false;
    }
    return true;
}

static bool read_soroban_authorized_invocation_advance(buffer_t *buffer,
                                                       uint8_t *count,
                                                       size_t *positions) {
    if (count != NULL && positions != NULL) {
        if (*count >= MAX_SUB_INVOCATIONS_SIZE) {
            return false;
        }
        positions[(*count)++] = buffer->offset;
    }
    // function
    PARSER_CHECK(read_soroban_authorized_function_advance(buffer))

    // subInvocations
    uint32_t len;
    PARSER_CHECK(parse_uint32(buffer, &len))
    for (uint32_t i = 0; i < len; i++) {
        PARSER_CHECK(read_soroban_authorized_invocation_advance(buffer, count, positions))
    }
    return true;
}

static bool parse_invoke_host_function(buffer_t *buffer, invoke_host_function_op_t *op) {
    // hostFunction
    PARSER_CHECK(parse_uint32(buffer, &op->host_function_type));
    PRINTF("host_function_type=%d\n", op->host_function_type);
    switch (op->host_function_type) {
        case HOST_FUNCTION_TYPE_INVOKE_CONTRACT:
            PARSER_CHECK(parse_invoke_contract_args(buffer, &op->invoke_contract_args))
            break;
        case HOST_FUNCTION_TYPE_CREATE_CONTRACT:
            PARSER_CHECK(read_create_contract_args_advance(buffer))
            break;
        case HOST_FUNCTION_TYPE_UPLOAD_CONTRACT_WASM: {
            size_t data_size = 0;
            PARSER_CHECK(parse_uint32(buffer, &data_size))
            PARSER_CHECK(num_bytes(data_size, &data_size))
            PARSER_CHECK(buffer_advance(buffer, data_size))
            break;
        }
        default:
            return false;
    }

    // auth<>
    uint32_t auth_len;
    uint8_t sub_invocations_count = 0;
    PARSER_CHECK(parse_uint32(buffer, &auth_len))
    for (uint32_t i = 0; i < auth_len; i++) {
        // PARSER_CHECK(read_soroban_authorization_entry_advance(buffer,
        //                                                       &sub_invocations_count,
        //                                                       op->sub_invocation_positions))
        // PARSER_CHECK(read_soroban_authorized_invocation_advance(buffer, count, positions))
        // 1. read credentials
        uint32_t credentials_type;
        PARSER_CHECK(read_parse_soroban_credentials_advance(buffer, &credentials_type))
        // 2. read rootInvocation.function
        PARSER_CHECK(read_soroban_authorized_function_advance(buffer))
        // 3. read rootInvocation.subInvocations
        // subInvocations
        uint32_t len;
        PARSER_CHECK(parse_uint32(buffer, &len))
        for (uint32_t j = 0; j < len; j++) {
            if (credentials_type == SOROBAN_CREDENTIALS_SOURCE_ACCOUNT) {
                PARSER_CHECK(
                    read_soroban_authorized_invocation_advance(buffer,
                                                               &sub_invocations_count,
                                                               op->sub_invocation_positions))
            } else {
                PARSER_CHECK(read_soroban_authorized_invocation_advance(buffer, NULL, NULL))
            }
        }
    }
    op->sub_invocations_count = sub_invocations_count;

    PRINTF("sub_invocations_count=%d\n", sub_invocations_count);
    // for (uint8_t i = 0; i < 16; i++) {
    //     PRINTF("sub_invocation_positions[%d]=%d\n", i, op->sub_invocation_positions[i]);
    // }
    return true;
}

static bool parse_extend_footprint_ttl(buffer_t *buffer, extend_footprint_ttl_op_t *op) {
    PARSER_CHECK(parse_extension_point_v0(buffer))
    PARSER_CHECK(parse_uint32(buffer, &op->extend_to))
    return true;
}

static bool parse_operation(buffer_t *buffer, operation_t *operation) {
    PRINTF("parse_operation: offset=%d\n", buffer->offset);
    explicit_bzero(operation, sizeof(operation_t));

    PARSER_CHECK(parse_optional_type(buffer,
                                     (xdr_type_reader) parse_muxed_account,
                                     &operation->source_account,
                                     &operation->source_account_present))
    PRINTF("operation->source_account_present: %d\n", operation->source_account_present);

    PARSER_CHECK(parse_uint32(buffer, &operation->type))
    PRINTF("sizeof operation->type: %d\n", sizeof(operation->type));

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
        case OPERATION_INVOKE_HOST_FUNCTION: {
            return parse_invoke_host_function(buffer, &operation->invoke_host_function_op);
        }
        case OPERATION_EXTEND_FOOTPRINT_TTL:
            return parse_extend_footprint_ttl(buffer, &operation->extend_footprint_ttl_op);
        case OPERATION_RESTORE_FOOTPRINT:
            return parse_restore_footprint(buffer, &operation->restore_footprint_op);
        default:
            return false;
    }
    return false;
}

static bool parse_transaction_source(buffer_t *buffer, muxed_account_t *source) {
    return parse_muxed_account(buffer, source);
}

static bool parse_transaction_fee(buffer_t *buffer, uint32_t *fee) {
    return parse_uint32(buffer, fee);
}

static bool parse_transaction_sequence(buffer_t *buffer, sequence_number_t *sequence_number) {
    return parse_int64(buffer, sequence_number);
}

static bool parse_transaction_preconditions(buffer_t *buffer, preconditions_t *preconditions) {
    return parse_preconditions(buffer, preconditions);
}

static bool parse_transaction_memo(buffer_t *buffer, memo_t *memo) {
    return parse_memo(buffer, memo);
}

static bool parse_transaction_operation_len(buffer_t *buffer, uint8_t *operations_count) {
    uint32_t len;
    PARSER_CHECK(parse_uint32(buffer, &len))
    if (len > MAX_OPS) {
        return false;
    }
    *operations_count = len;
    return true;
}

static bool check_operations(buffer_t *buffer, uint8_t op_count) {
    PRINTF("check_operations: op_count=%d, offset=%d\n", op_count, buffer->offset);
    operation_t op;
    for (uint8_t i = 0; i < op_count; i++) {
        PARSER_CHECK(parse_operation(buffer, &op))
    }
    return true;
}

static bool parse_transaction_details(buffer_t *buffer, transaction_details_t *transaction) {
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

    // check ops is valid
    // size_t offset = buffer->offset;
    // PARSER_CHECK(check_operations(buffer, transaction->operations_count))
    // buffer->offset = offset;

    return true;
}

static bool parse_fee_bump_transaction_fee_source(buffer_t *buffer, muxed_account_t *fee_source) {
    return parse_muxed_account(buffer, fee_source);
}

static bool parse_fee_bump_transaction_fee(buffer_t *buffer, int64_t *fee) {
    return parse_int64(buffer, fee);
}

static bool parse_fee_bump_transaction_details(
    buffer_t *buffer,
    fee_bump_transaction_details_t *fee_bump_transaction) {
    PARSER_CHECK(parse_fee_bump_transaction_fee_source(buffer, &fee_bump_transaction->fee_source))
    PARSER_CHECK(parse_fee_bump_transaction_fee(buffer, &fee_bump_transaction->fee))
    return true;
}

// static bool parse_fee_bump_transaction_ext(buffer_t *buffer)
// {
//     uint32_t ext;
//     PARSER_CHECK(parse_uint32(buffer, &ext))
//     if (ext != 0)
//     {
//         return false;
//     }
//     return true;
// }

static bool parse_network(buffer_t *buffer, uint8_t *network) {
    const uint8_t *network_id;
    PARSER_CHECK(buffer_read_bytes(buffer, &network_id, HASH_SIZE))

    if (memcmp(network_id, NETWORK_ID_PUBLIC_HASH, HASH_SIZE) == 0) {
        *network = NETWORK_TYPE_PUBLIC;
    } else if (memcmp(network_id, NETWORK_ID_TEST_HASH, HASH_SIZE) == 0) {
        *network = NETWORK_TYPE_TEST;
    } else {
        *network = NETWORK_TYPE_UNKNOWN;
    }
    return true;
}

bool parse_transaction_envelope(const uint8_t *data, size_t data_len, envelope_t *envelope) {
    PRINTF("parse_transaction_envelope\n");
    buffer_t buffer = {
        .ptr = data,
        .size = data_len,
        .offset = 0,
    };

    explicit_bzero(&envelope->tx_details, sizeof(tx_details_t));
    uint32_t envelope_type;
    PARSER_CHECK(parse_network(&buffer, &envelope->network))
    PARSER_CHECK(parse_uint32(&buffer, &envelope_type))
    envelope->type = envelope_type;
    switch (envelope_type) {
        case ENVELOPE_TYPE_TX:
            PARSER_CHECK(parse_transaction_details(&buffer, &envelope->tx_details.tx))
            break;
        case ENVELOPE_TYPE_TX_FEE_BUMP:
            PARSER_CHECK(
                parse_fee_bump_transaction_details(&buffer, &envelope->tx_details.fee_bump_tx))
            uint32_t inner_envelope_type;
            PARSER_CHECK(parse_uint32(&buffer, &inner_envelope_type))
            if (inner_envelope_type != ENVELOPE_TYPE_TX) {
                return false;
            }
            PARSER_CHECK(parse_transaction_details(&buffer, &envelope->tx_details.tx))
            break;
        default:
            return false;
    }

    envelope->tx_details.tx.operation_position = buffer.offset;

    // check all operations are valid
    PARSER_CHECK(check_operations(&buffer, envelope->tx_details.tx.operations_count));
    return true;
}

bool parse_transaction_operation(const uint8_t *data,
                                 size_t data_len,
                                 envelope_t *envelope,
                                 uint8_t operation_index) {
    buffer_t buffer = {
        .ptr = data,
        .size = data_len,
        .offset = envelope->tx_details.tx.operation_position,
    };
    for (uint8_t i = 0; i <= operation_index; i++) {
        PARSER_CHECK(parse_operation(&buffer, &envelope->tx_details.tx.op_details));
    }
    envelope->tx_details.tx.operation_index = operation_index;
    return true;
}

bool parse_auth_function(buffer_t *buffer, uint32_t *type, invoke_contract_args_t *args) {
    // function
    PARSER_CHECK(parse_uint32(buffer, type))
    switch (*type) {
        case SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN: {
            // contractFn
            PARSER_CHECK(parse_invoke_contract_args(buffer, args))
            break;
        }
        case SOROBAN_AUTHORIZED_FUNCTION_TYPE_CREATE_CONTRACT_HOST_FN:
            // createContractHostFn
            PARSER_CHECK(read_create_contract_args_advance(buffer))
            break;
        default:
            return false;
    }
    return true;
}

bool parse_soroban_authorization_envelope(const uint8_t *data,
                                          size_t data_len,
                                          envelope_t *envelope) {
    PRINTF("parse_soroban_authorization_envelope\n");
    buffer_t buffer = {
        .ptr = data,
        .size = data_len,
        .offset = 0,
    };

    explicit_bzero(&envelope->soroban_authorization, sizeof(soroban_authorization_t));

    uint32_t envelope_type;
    PARSER_CHECK(parse_uint32(&buffer, &envelope_type))
    if (envelope_type != ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
        return false;
    }
    envelope->type = envelope_type;

    PARSER_CHECK(parse_network(&buffer, &envelope->network))
    PARSER_CHECK(parse_uint64(&buffer, &envelope->soroban_authorization.nonce))
    PARSER_CHECK(
        parse_uint32(&buffer, &envelope->soroban_authorization.signature_expiration_ledger))

    // function
    PARSER_CHECK(parse_auth_function(&buffer,
                                     &envelope->soroban_authorization.auth_function_type,
                                     &envelope->soroban_authorization.invoke_contract_args))
    // subInvocations
    uint32_t len;
    uint8_t sub_invocations_count = 0;
    PARSER_CHECK(parse_uint32(&buffer, &len))
    for (uint32_t i = 0; i < len; i++) {
        PARSER_CHECK(read_soroban_authorized_invocation_advance(
            &buffer,
            &sub_invocations_count,
            envelope->soroban_authorization.sub_invocation_positions));
    }
    envelope->soroban_authorization.sub_invocations_count = sub_invocations_count;
    PRINTF("sub_invocations_count=%d\n", sub_invocations_count);
    // for (uint8_t i = 0; i < 16; i++) {
    //     PRINTF("sub_invocation_positions[%d]=%d\n",
    //            i,
    //            envelope->soroban_authorization.sub_invocation_positions[i]);
    // }
    return true;
}
