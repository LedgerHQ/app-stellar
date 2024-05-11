#include <string.h>
#include <stdbool.h>  // bool

#ifdef TEST
#include <stdio.h>
#define PIC(x) x
#else
#include "os.h"
#endif

#include "base64.h"
#include "stellar/formatter.h"
#include "stellar/parser.h"
#include "stellar/printer.h"
#include "stellar/plugin.h"

#if defined(TARGET_NANOS)
#define INVOKE_SMART_CONTRACT "Invoke Contract"
#else
#define INVOKE_SMART_CONTRACT "Invoke Smart Contract"
#endif

/*
 * the formatter prints the details and defines the order of the details
 * by setting the next formatter to be called
 */
typedef bool (*format_function_t)(formatter_data_t *fdata);

#define FORMATTER_CHECK(x)      \
    {                           \
        if (!(x)) return false; \
    }

#define STRLCPY(dst, src, size)               \
    {                                         \
        size_t len = strlcpy(dst, src, size); \
        if (len >= size) return false;        \
    }

#define STRLCAT(dst, src, size)               \
    {                                         \
        size_t len = strlcat(dst, src, size); \
        if (len >= size) return false;        \
    }

/*
 * Longest string will be "Operation ii of nn"
 */
#define OPERATION_CAPTION_MAX_LENGTH 20

/* 16 formatters in a row ought to be enough for everybody*/
#define MAX_FORMATTERS_PER_OPERATION 16

static const char *NETWORK_NAMES[3] = {"Public", "Testnet", "Unknown"};

static format_function_t formatter_stack[MAX_FORMATTERS_PER_OPERATION];
static int8_t formatter_index;
static uint8_t current_data_index;
static uint8_t parameters_index;
static uint8_t plugin_data_pair_count;

static bool format_sub_invocation_start(formatter_data_t *fdata);
static bool push_to_formatter_stack(format_function_t formatter) {
    if (formatter_index >= MAX_FORMATTERS_PER_OPERATION) {
        PRINTF("Formatter stack overflow\n");
        return false;
    }

    formatter_stack[formatter_index++] = formatter;

    return true;
}

static bool format_transaction_source(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Tx Source", fdata->caption_len);
    if (fdata->envelope->type == ENVELOPE_TYPE_TX &&
        fdata->envelope->tx_details.tx.source_account.type == KEY_TYPE_ED25519 &&
        memcmp(fdata->envelope->tx_details.tx.source_account.ed25519,
               fdata->signing_key,
               RAW_ED25519_PUBLIC_KEY_SIZE) == 0) {
        FORMATTER_CHECK(print_muxed_account(&fdata->envelope->tx_details.tx.source_account,
                                            fdata->value,
                                            fdata->value_len,
                                            6,
                                            6))
    } else {
        FORMATTER_CHECK(print_muxed_account(&fdata->envelope->tx_details.tx.source_account,
                                            fdata->value,
                                            fdata->value_len,
                                            0,
                                            0))
    }
    FORMATTER_CHECK(push_to_formatter_stack(NULL))
    return true;
}

static bool format_min_seq_ledger_gap(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Min Seq Ledger Gap", fdata->caption_len);
    FORMATTER_CHECK(print_uint64_num(fdata->envelope->tx_details.tx.cond.min_seq_ledger_gap,
                                     fdata->value,
                                     fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_transaction_source))
    return true;
}

static bool format_min_seq_ledger_gap_prepare(formatter_data_t *fdata) {
    if (fdata->envelope->tx_details.tx.cond.min_seq_ledger_gap == 0) {
        return format_transaction_source(fdata);
    } else {
        return format_min_seq_ledger_gap(fdata);
    }
}

static bool format_min_seq_age(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Min Seq Age", fdata->caption_len);
    FORMATTER_CHECK(print_uint64_num(fdata->envelope->tx_details.tx.cond.min_seq_age,
                                     fdata->value,
                                     fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_min_seq_ledger_gap_prepare))
    return true;
}

static bool format_min_seq_age_prepare(formatter_data_t *fdata) {
    if (fdata->envelope->tx_details.tx.cond.min_seq_age == 0) {
        format_min_seq_ledger_gap_prepare(fdata);
    } else {
        format_min_seq_age(fdata);
    }
    return true;
}

static bool format_min_seq_num(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Min Seq Num", fdata->caption_len);
    FORMATTER_CHECK(print_uint64_num(fdata->envelope->tx_details.tx.cond.min_seq_num,
                                     fdata->value,
                                     fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_min_seq_age_prepare))
    return true;
}

static bool format_min_seq_num_prepare(formatter_data_t *fdata) {
    if (!fdata->envelope->tx_details.tx.cond.min_seq_num_present ||
        fdata->envelope->tx_details.tx.cond.min_seq_num == 0) {
        return format_min_seq_age_prepare(fdata);
    } else {
        return format_min_seq_num(fdata);
    }
}

static bool format_ledger_bounds_max_ledger(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Ledger Bounds Max", fdata->caption_len);
    FORMATTER_CHECK(print_uint64_num(fdata->envelope->tx_details.tx.cond.ledger_bounds.max_ledger,
                                     fdata->value,
                                     fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_min_seq_num_prepare))
    return true;
}

static bool format_ledger_bounds_min_ledger(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Ledger Bounds Min", fdata->caption_len);
    FORMATTER_CHECK(print_uint64_num(fdata->envelope->tx_details.tx.cond.ledger_bounds.min_ledger,
                                     fdata->value,
                                     fdata->value_len))
    if (fdata->envelope->tx_details.tx.cond.ledger_bounds.max_ledger != 0) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_ledger_bounds_max_ledger))
    } else {
        FORMATTER_CHECK(push_to_formatter_stack(&format_min_seq_num_prepare))
    }
    return true;
}

static bool format_ledger_bounds(formatter_data_t *fdata) {
    if (!fdata->envelope->tx_details.tx.cond.ledger_bounds_present ||
        (fdata->envelope->tx_details.tx.cond.ledger_bounds.min_ledger == 0 &&
         fdata->envelope->tx_details.tx.cond.ledger_bounds.max_ledger == 0)) {
        return format_min_seq_num_prepare(fdata);
    } else if (fdata->envelope->tx_details.tx.cond.ledger_bounds.min_ledger != 0) {
        return format_ledger_bounds_min_ledger(fdata);
    } else {
        return format_ledger_bounds_max_ledger(fdata);
    }
    return true;
}

static bool format_time_bounds_max_time(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Valid Before (UTC)", fdata->caption_len);
    FORMATTER_CHECK(print_time(fdata->envelope->tx_details.tx.cond.time_bounds.max_time,
                               fdata->value,
                               fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_ledger_bounds))
    return true;
}

static bool format_time_bounds_min_time(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Valid After (UTC)", fdata->caption_len);
    FORMATTER_CHECK(print_time(fdata->envelope->tx_details.tx.cond.time_bounds.min_time,
                               fdata->value,
                               fdata->value_len))

    if (fdata->envelope->tx_details.tx.cond.time_bounds.max_time != 0) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_time_bounds_max_time))
    } else {
        FORMATTER_CHECK(push_to_formatter_stack(&format_ledger_bounds))
    }
    return true;
}

static bool format_time_bounds(formatter_data_t *fdata) {
    if (!fdata->envelope->tx_details.tx.cond.time_bounds_present ||
        (fdata->envelope->tx_details.tx.cond.time_bounds.min_time == 0 &&
         fdata->envelope->tx_details.tx.cond.time_bounds.max_time == 0)) {
        return format_ledger_bounds(fdata);
    } else if (fdata->envelope->tx_details.tx.cond.time_bounds.min_time != 0) {
        return format_time_bounds_min_time(fdata);
    } else {
        return format_time_bounds_max_time(fdata);
    }
    return true;
}

static bool format_sequence(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Sequence Num", fdata->caption_len);
    FORMATTER_CHECK(print_uint64_num(fdata->envelope->tx_details.tx.sequence_number,
                                     fdata->value,
                                     fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_time_bounds))
    return true;
}

static bool format_fee(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Max Fee", fdata->caption_len);
    asset_t asset = {.type = ASSET_TYPE_NATIVE};
    FORMATTER_CHECK(print_amount(fdata->envelope->tx_details.tx.fee,
                                 &asset,
                                 fdata->envelope->network,
                                 fdata->value,
                                 fdata->value_len))
    if (fdata->display_sequence) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_sequence))
    } else {
        FORMATTER_CHECK(push_to_formatter_stack(&format_time_bounds))
    }
    return true;
}

static bool format_memo(formatter_data_t *fdata) {
    memo_t *memo = &fdata->envelope->tx_details.tx.memo;
    switch (memo->type) {
        case MEMO_ID: {
            STRLCPY(fdata->caption, "Memo ID", fdata->caption_len);
            FORMATTER_CHECK(print_uint64_num(memo->id, fdata->value, fdata->value_len))
            break;
        }
        case MEMO_TEXT: {
            STRLCPY(fdata->caption, "Memo Text", fdata->caption_len);
            if (is_printable_binary(memo->text.text, memo->text.text_size)) {
                FORMATTER_CHECK(print_string(fdata->value,
                                             fdata->value_len,
                                             memo->text.text,
                                             memo->text.text_size))
            } else {
                char tmp[41];  // (28 / 3) * 4 = 37.33， 4 is for padding
                FORMATTER_CHECK(
                    base64_encode(memo->text.text, memo->text.text_size, tmp, fdata->value_len))
                STRLCPY(fdata->value, "Base64: ", fdata->value_len)
                STRLCAT(fdata->value, tmp, fdata->value_len)
            }
            break;
        }
        case MEMO_HASH: {
            STRLCPY(fdata->caption, "Memo Hash", fdata->caption_len);
            FORMATTER_CHECK(
                print_binary(memo->hash, HASH_SIZE, fdata->value, fdata->value_len, 0, 0))
            break;
        }
        case MEMO_RETURN: {
            STRLCPY(fdata->caption, "Memo Return", fdata->caption_len);
            FORMATTER_CHECK(
                print_binary(memo->hash, HASH_SIZE, fdata->value, fdata->value_len, 0, 0))
            break;
        }
        default:
            return false;
    }
    FORMATTER_CHECK(push_to_formatter_stack(&format_fee))
    return true;
}

static bool format_transaction_details(formatter_data_t *fdata) {
    switch (fdata->envelope->type) {
        case ENVELOPE_TYPE_TX_FEE_BUMP:
            STRLCPY(fdata->caption, "InnerTx", fdata->caption_len);
            break;
        case ENVELOPE_TYPE_TX:
            STRLCPY(fdata->caption, "Transaction", fdata->caption_len);
            break;
        default:
            return false;
    }
    STRLCPY(fdata->value, "Details", fdata->value_len);
    if (fdata->envelope->tx_details.tx.memo.type != MEMO_NONE) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_memo))
    } else {
        FORMATTER_CHECK(push_to_formatter_stack(&format_fee))
    }
    return true;
}

static bool format_operation_source(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Op Source", fdata->caption_len);
    if (fdata->envelope->type == ENVELOPE_TYPE_TX &&
        fdata->envelope->tx_details.tx.source_account.type == KEY_TYPE_ED25519 &&
        fdata->envelope->tx_details.tx.op_details.source_account.type == KEY_TYPE_ED25519 &&
        memcmp(fdata->envelope->tx_details.tx.source_account.ed25519,
               fdata->signing_key,
               RAW_ED25519_PUBLIC_KEY_SIZE) == 0 &&
        memcmp(fdata->envelope->tx_details.tx.op_details.source_account.ed25519,
               fdata->signing_key,
               RAW_ED25519_PUBLIC_KEY_SIZE) == 0) {
        FORMATTER_CHECK(
            print_muxed_account(&fdata->envelope->tx_details.tx.op_details.source_account,
                                fdata->value,
                                fdata->value_len,
                                6,
                                6))
    } else {
        FORMATTER_CHECK(
            print_muxed_account(&fdata->envelope->tx_details.tx.op_details.source_account,
                                fdata->value,
                                fdata->value_len,
                                0,
                                0))
    }
    FORMATTER_CHECK(push_to_formatter_stack(NULL))
    return true;
}

static bool format_operation_source_prepare(formatter_data_t *fdata) {
    if (fdata->envelope->tx_details.tx.op_details.source_account_present) {
        // If the source exists, when the user clicks the next button,
        // it will jump to the page showing the source
        FORMATTER_CHECK(push_to_formatter_stack(&format_operation_source))
    } else {
        FORMATTER_CHECK(push_to_formatter_stack(NULL))
    }
    return true;
}

static bool format_bump_sequence_bump_to(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Bump To", fdata->caption_len);
    FORMATTER_CHECK(
        print_int64_num(fdata->envelope->tx_details.tx.op_details.bump_sequence_op.bump_to,
                        fdata->value,
                        fdata->value_len))
    return format_operation_source_prepare(fdata);
}

static bool format_bump_sequence(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Bump Sequence", fdata->value_len);
    FORMATTER_CHECK(push_to_formatter_stack(&format_bump_sequence_bump_to))
    return true;
}

static bool format_inflation(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Inflation", fdata->value_len);
    return format_operation_source_prepare(fdata);
}

static bool format_account_merge_destination(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Destination", fdata->caption_len);
    FORMATTER_CHECK(
        print_muxed_account(&fdata->envelope->tx_details.tx.op_details.account_merge_op.destination,
                            fdata->value,
                            fdata->value_len,
                            0,
                            0))
    return format_operation_source_prepare(fdata);
}

static bool format_account_merge_detail(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Send", fdata->caption_len);
    STRLCPY(fdata->value, "All Funds", fdata->value_len);
    FORMATTER_CHECK(push_to_formatter_stack(&format_account_merge_destination))
    return true;
}

static bool format_account_merge(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Account Merge", fdata->value_len);
    FORMATTER_CHECK(push_to_formatter_stack(&format_account_merge_detail))
    return true;
}

static bool format_manage_data_value(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Data Value", fdata->caption_len);
    if (is_printable_binary(
            fdata->envelope->tx_details.tx.op_details.manage_data_op.data_value,
            fdata->envelope->tx_details.tx.op_details.manage_data_op.data_value_size)) {
        if (fdata->envelope->tx_details.tx.op_details.manage_data_op.data_value_size >=
            fdata->value_len) {
            return false;
        }
        FORMATTER_CHECK(
            print_string(fdata->value,
                         fdata->value_len,
                         fdata->envelope->tx_details.tx.op_details.manage_data_op.data_value,
                         fdata->envelope->tx_details.tx.op_details.manage_data_op.data_value_size))
    } else {
        char tmp[89];  // (64 / 3) * 4 = 85.33, 4 is for padding
        FORMATTER_CHECK(
            base64_encode(fdata->envelope->tx_details.tx.op_details.manage_data_op.data_value,
                          fdata->envelope->tx_details.tx.op_details.manage_data_op.data_value_size,
                          tmp,
                          sizeof(tmp)))
        STRLCPY(fdata->value, "Base64: ", fdata->value_len)
        STRLCAT(fdata->value, tmp, fdata->value_len)
    }
    return format_operation_source_prepare(fdata);
}

static bool format_manage_data(formatter_data_t *fdata) {
    FORMATTER_CHECK(
        print_string(fdata->value,
                     fdata->value_len,
                     fdata->envelope->tx_details.tx.op_details.manage_data_op.data_name,
                     fdata->envelope->tx_details.tx.op_details.manage_data_op.data_name_size))

    if (fdata->envelope->tx_details.tx.op_details.manage_data_op.data_value_size) {
        STRLCPY(fdata->caption, "Set Data", fdata->caption_len);
        FORMATTER_CHECK(push_to_formatter_stack(&format_manage_data_value))
    } else {
        STRLCPY(fdata->caption, "Remove Data", fdata->caption_len);
        return format_operation_source_prepare(fdata);
    }
    return true;
}

static bool format_allow_trust_authorize(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Authorize Flag", fdata->caption_len);
    FORMATTER_CHECK(
        print_allow_trust_flags(fdata->envelope->tx_details.tx.op_details.allow_trust_op.authorize,
                                fdata->value,
                                fdata->value_len))
    return format_operation_source_prepare(fdata);
}

static bool format_allow_trust_asset_code(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Asset Code", fdata->caption_len);
    switch (fdata->envelope->tx_details.tx.op_details.allow_trust_op.asset_type) {
        case ASSET_TYPE_CREDIT_ALPHANUM4: {
            FORMATTER_CHECK(
                print_string(fdata->value,
                             fdata->value_len,
                             fdata->envelope->tx_details.tx.op_details.allow_trust_op.asset_code,
                             4))

            break;
        }
        case ASSET_TYPE_CREDIT_ALPHANUM12: {
            FORMATTER_CHECK(
                print_string(fdata->value,
                             fdata->value_len,
                             fdata->envelope->tx_details.tx.op_details.allow_trust_op.asset_code,
                             12))
            break;
        }
        default:
            return false;  // unknown asset type
    }
    FORMATTER_CHECK(push_to_formatter_stack(&format_allow_trust_authorize))
    return true;
}

static bool format_allow_trust_trustor(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Trustor", fdata->caption_len);
    FORMATTER_CHECK(
        print_account_id(fdata->envelope->tx_details.tx.op_details.allow_trust_op.trustor,
                         fdata->value,
                         fdata->value_len,
                         0,
                         0))
    FORMATTER_CHECK(push_to_formatter_stack(&format_allow_trust_asset_code))
    return true;
}

static bool format_allow_trust(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Allow Trust", fdata->value_len);
    FORMATTER_CHECK(push_to_formatter_stack(&format_allow_trust_trustor))
    return true;
}

static bool format_set_option_signer_weight(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Weight", fdata->caption_len);
    FORMATTER_CHECK(
        print_uint64_num(fdata->envelope->tx_details.tx.op_details.set_options_op.signer.weight,
                         fdata->value,
                         fdata->value_len))
    return format_operation_source_prepare(fdata);
}

static bool print_signer_key_detail(signer_key_t *key, char *value, size_t value_len) {
    switch (key->type) {
        case SIGNER_KEY_TYPE_ED25519: {
            FORMATTER_CHECK(print_account_id(key->ed25519, value, value_len, 0, 0))
            break;
        }
        case SIGNER_KEY_TYPE_HASH_X: {
            FORMATTER_CHECK(print_hash_x_key(key->hash_x, value, value_len, 0, 0))
            break;
        }
        case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
            FORMATTER_CHECK(print_pre_auth_x_key(key->pre_auth_tx, value, value_len, 0, 0))
            break;
        }
        case SIGNER_KEY_TYPE_ED25519_SIGNED_PAYLOAD: {
            FORMATTER_CHECK(print_ed25519_signed_payload(&key->ed25519_signed_payload,
                                                         value,
                                                         value_len,
                                                         12,
                                                         12))
            break;
        }
        default:
            return false;
    }
    return true;
}

static bool format_set_option_signer_detail(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Signer Key", fdata->caption_len);
    signer_key_t *key = &fdata->envelope->tx_details.tx.op_details.set_options_op.signer.key;

    FORMATTER_CHECK(print_signer_key_detail(key, fdata->value, fdata->value_len))

    if (fdata->envelope->tx_details.tx.op_details.set_options_op.signer.weight != 0) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_set_option_signer_weight))
    } else {
        return format_operation_source_prepare(fdata);
    }
    return true;
}

static bool format_set_option_signer(formatter_data_t *fdata) {
    signer_t *signer = &fdata->envelope->tx_details.tx.op_details.set_options_op.signer;
    if (signer->weight) {
        STRLCPY(fdata->caption, "Add Signer", fdata->caption_len);
    } else {
        STRLCPY(fdata->caption, "Remove Signer", fdata->caption_len);
    }
    switch (signer->key.type) {
        case SIGNER_KEY_TYPE_ED25519: {
            STRLCPY(fdata->value, "Type Public Key", fdata->value_len);
            break;
        }
        case SIGNER_KEY_TYPE_HASH_X: {
            STRLCPY(fdata->value, "Type Hash(x)", fdata->value_len);
            break;
        }
        case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
            STRLCPY(fdata->value, "Type Pre-Auth", fdata->value_len);
            break;
        }
        case SIGNER_KEY_TYPE_ED25519_SIGNED_PAYLOAD: {
            STRLCPY(fdata->value, "Type Ed25519 Signed Payload", fdata->value_len);
            break;
        }
        default:
            return false;
    }
    FORMATTER_CHECK(push_to_formatter_stack(&format_set_option_signer_detail))
    return true;
}

static bool format_set_option_signer_prepare(formatter_data_t *fdata) {
    if (fdata->envelope->tx_details.tx.op_details.set_options_op.signer_present) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_set_option_signer))
    } else {
        return format_operation_source_prepare(fdata);
    }
    return true;
}

static bool format_set_option_home_domain(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Home Domain", fdata->caption_len);
    if (fdata->envelope->tx_details.tx.op_details.set_options_op.home_domain_size) {
        FORMATTER_CHECK(
            print_string(fdata->value,
                         fdata->value_len,
                         fdata->envelope->tx_details.tx.op_details.set_options_op.home_domain,
                         fdata->envelope->tx_details.tx.op_details.set_options_op.home_domain_size))
    } else {
        STRLCPY(fdata->value, "[remove home domain from account]", fdata->value_len);
    }
    format_set_option_signer_prepare(fdata);
    return true;
}

static bool format_set_option_home_domain_prepare(formatter_data_t *fdata) {
    if (fdata->envelope->tx_details.tx.op_details.set_options_op.home_domain_present) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_set_option_home_domain))
    } else {
        format_set_option_signer_prepare(fdata);
    }
    return true;
}

static bool format_set_option_high_threshold(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "High Threshold", fdata->caption_len);
    FORMATTER_CHECK(
        print_uint64_num(fdata->envelope->tx_details.tx.op_details.set_options_op.high_threshold,
                         fdata->value,
                         fdata->value_len))
    format_set_option_home_domain_prepare(fdata);
    return true;
}

static bool format_set_option_high_threshold_prepare(formatter_data_t *fdata) {
    if (fdata->envelope->tx_details.tx.op_details.set_options_op.high_threshold_present) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_set_option_high_threshold))
    } else {
        format_set_option_home_domain_prepare(fdata);
    }
    return true;
}

static bool format_set_option_medium_threshold(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Medium Threshold", fdata->caption_len);
    FORMATTER_CHECK(
        print_uint64_num(fdata->envelope->tx_details.tx.op_details.set_options_op.medium_threshold,
                         fdata->value,
                         fdata->value_len))
    format_set_option_high_threshold_prepare(fdata);
    return true;
}

static bool format_set_option_medium_threshold_prepare(formatter_data_t *fdata) {
    if (fdata->envelope->tx_details.tx.op_details.set_options_op.medium_threshold_present) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_set_option_medium_threshold))
    } else {
        format_set_option_high_threshold_prepare(fdata);
    }
    return true;
}

static bool format_set_option_low_threshold(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Low Threshold", fdata->caption_len);
    FORMATTER_CHECK(
        print_uint64_num(fdata->envelope->tx_details.tx.op_details.set_options_op.low_threshold,
                         fdata->value,
                         fdata->value_len))
    format_set_option_medium_threshold_prepare(fdata);
    return true;
}

static bool format_set_option_low_threshold_prepare(formatter_data_t *fdata) {
    if (fdata->envelope->tx_details.tx.op_details.set_options_op.low_threshold_present) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_set_option_low_threshold))
    } else {
        format_set_option_medium_threshold_prepare(fdata);
    }
    return true;
}

static bool format_set_option_master_weight(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Master Weight", fdata->caption_len);
    FORMATTER_CHECK(
        print_uint64_num(fdata->envelope->tx_details.tx.op_details.set_options_op.master_weight,
                         fdata->value,
                         fdata->value_len))
    format_set_option_low_threshold_prepare(fdata);
    return true;
}

static bool format_set_option_master_weight_prepare(formatter_data_t *fdata) {
    if (fdata->envelope->tx_details.tx.op_details.set_options_op.master_weight_present) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_set_option_master_weight))
    } else {
        format_set_option_low_threshold_prepare(fdata);
    }
    return true;
}

static bool format_set_option_set_flags(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Set Flags", fdata->caption_len);
    FORMATTER_CHECK(
        print_account_flags(fdata->envelope->tx_details.tx.op_details.set_options_op.set_flags,
                            fdata->value,
                            fdata->value_len))
    format_set_option_master_weight_prepare(fdata);
    return true;
}

static bool format_set_option_set_flags_prepare(formatter_data_t *fdata) {
    if (fdata->envelope->tx_details.tx.op_details.set_options_op.set_flags_present) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_set_option_set_flags))
    } else {
        format_set_option_master_weight_prepare(fdata);
    }
    return true;
}

static bool format_set_option_clear_flags(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Clear Flags", fdata->caption_len);
    FORMATTER_CHECK(
        print_account_flags(fdata->envelope->tx_details.tx.op_details.set_options_op.clear_flags,
                            fdata->value,
                            fdata->value_len))
    format_set_option_set_flags_prepare(fdata);
    return true;
}

static bool format_set_option_clear_flags_prepare(formatter_data_t *fdata) {
    if (fdata->envelope->tx_details.tx.op_details.set_options_op.clear_flags_present) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_set_option_clear_flags))
    } else {
        format_set_option_set_flags_prepare(fdata);
    }
    return true;
}

static bool format_set_option_inflation_destination(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Inflation Dest", fdata->caption_len);
    FORMATTER_CHECK(print_account_id(
        fdata->envelope->tx_details.tx.op_details.set_options_op.inflation_destination,
        fdata->value,
        fdata->value_len,
        0,
        0))
    format_set_option_clear_flags_prepare(fdata);
    return true;
}

static bool format_set_option_inflation_destination_prepare(formatter_data_t *fdata) {
    if (fdata->envelope->tx_details.tx.op_details.set_options_op.inflation_destination_present) {
        FORMATTER_CHECK(push_to_formatter_stack(format_set_option_inflation_destination))
    } else {
        format_set_option_clear_flags_prepare(fdata);
    }
    return true;
}

static bool format_set_options_empty_body(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "SET OPTIONS", fdata->caption_len);
    STRLCPY(fdata->value, "[BODY IS EMPTY]", fdata->value_len);
    return format_operation_source_prepare(fdata);
}

static bool is_empty_set_options_body(formatter_data_t *fdata) {
    return !(
        fdata->envelope->tx_details.tx.op_details.set_options_op.inflation_destination_present ||
        fdata->envelope->tx_details.tx.op_details.set_options_op.clear_flags_present ||
        fdata->envelope->tx_details.tx.op_details.set_options_op.set_flags_present ||
        fdata->envelope->tx_details.tx.op_details.set_options_op.master_weight_present ||
        fdata->envelope->tx_details.tx.op_details.set_options_op.low_threshold_present ||
        fdata->envelope->tx_details.tx.op_details.set_options_op.medium_threshold_present ||
        fdata->envelope->tx_details.tx.op_details.set_options_op.high_threshold_present ||
        fdata->envelope->tx_details.tx.op_details.set_options_op.home_domain_present ||
        fdata->envelope->tx_details.tx.op_details.set_options_op.signer_present);
}

static bool format_set_options(formatter_data_t *fdata) {
    // this operation is a special one among all operations, because all its
    // fields are optional.
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Set Options", fdata->value_len);
    if (is_empty_set_options_body(fdata)) {
        FORMATTER_CHECK(push_to_formatter_stack(format_set_options_empty_body))
    } else {
        format_set_option_inflation_destination_prepare(fdata);
    }
    return true;
}

static bool format_change_trust_limit(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Trust Limit", fdata->caption_len);
    FORMATTER_CHECK(print_amount(fdata->envelope->tx_details.tx.op_details.change_trust_op.limit,
                                 NULL,
                                 fdata->envelope->network,
                                 fdata->value,
                                 fdata->value_len))
    return format_operation_source_prepare(fdata);
}

static bool format_change_trust_detail_liquidity_pool_fee(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Pool Fee Rate", fdata->caption_len);

    uint8_t fee[4] = {0};
    for (int i = 0; i < 4; i++) {
        fee[i] = fdata->envelope->tx_details.tx.op_details.change_trust_op.line.liquidity_pool
                     .constant_product.fee >>
                 (8 * (3 - i));
    }
    FORMATTER_CHECK(print_int32(fee, 2, fdata->value, fdata->value_len, false))

    STRLCAT(fdata->value, "%", fdata->value_len);
    if (fdata->envelope->tx_details.tx.op_details.change_trust_op.limit &&
        fdata->envelope->tx_details.tx.op_details.change_trust_op.limit != INT64_MAX) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_change_trust_limit))
    } else {
        return format_operation_source_prepare(fdata);
    }
    return true;
}

static bool format_change_trust_detail_liquidity_pool_asset_b(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Asset B", fdata->caption_len);
    FORMATTER_CHECK(print_asset(&fdata->envelope->tx_details.tx.op_details.change_trust_op.line
                                     .liquidity_pool.constant_product.asset_b,
                                fdata->envelope->network,
                                fdata->value,
                                fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_change_trust_detail_liquidity_pool_fee))
    return true;
}

static bool format_change_trust_detail_liquidity_pool_asset_a(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Asset A", fdata->caption_len);
    FORMATTER_CHECK(print_asset(&fdata->envelope->tx_details.tx.op_details.change_trust_op.line
                                     .liquidity_pool.constant_product.asset_a,
                                fdata->envelope->network,
                                fdata->value,
                                fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_change_trust_detail_liquidity_pool_asset_b))
    return true;
}

static bool format_change_trust(formatter_data_t *fdata) {
    if (fdata->envelope->tx_details.tx.op_details.change_trust_op.limit) {
        STRLCPY(fdata->caption, "Change Trust", fdata->caption_len);
    } else {
        STRLCPY(fdata->caption, "Remove Trust", fdata->caption_len);
    }
    uint8_t asset_type = fdata->envelope->tx_details.tx.op_details.change_trust_op.line.type;
    switch (asset_type) {
        case ASSET_TYPE_CREDIT_ALPHANUM4:
        case ASSET_TYPE_CREDIT_ALPHANUM12:
            FORMATTER_CHECK(print_asset(
                (asset_t *) &fdata->envelope->tx_details.tx.op_details.change_trust_op.line,
                fdata->envelope->network,
                fdata->value,
                fdata->value_len))
            if (fdata->envelope->tx_details.tx.op_details.change_trust_op.limit &&
                fdata->envelope->tx_details.tx.op_details.change_trust_op.limit != INT64_MAX) {
                FORMATTER_CHECK(push_to_formatter_stack(&format_change_trust_limit))
            } else {
                return format_operation_source_prepare(fdata);
            }
            break;
        case ASSET_TYPE_POOL_SHARE:
            STRLCPY(fdata->value, "Liquidity Pool Asset", fdata->value_len);
            FORMATTER_CHECK(
                push_to_formatter_stack(&format_change_trust_detail_liquidity_pool_asset_a))
            break;
        default:
            return false;
    }
    return true;
}

static bool format_manage_sell_offer_price(formatter_data_t *fdata) {
    manage_sell_offer_op_t *op = &fdata->envelope->tx_details.tx.op_details.manage_sell_offer_op;
    STRLCPY(fdata->caption, "Price", fdata->caption_len);
    FORMATTER_CHECK(print_price(&op->price,
                                &op->buying,
                                &op->selling,
                                fdata->envelope->network,
                                fdata->value,
                                fdata->value_len))
    return format_operation_source_prepare(fdata);
}

static bool format_manage_sell_offer_sell(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Sell", fdata->caption_len);
    FORMATTER_CHECK(
        print_amount(fdata->envelope->tx_details.tx.op_details.manage_sell_offer_op.amount,
                     &fdata->envelope->tx_details.tx.op_details.manage_sell_offer_op.selling,
                     fdata->envelope->network,
                     fdata->value,
                     fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_manage_sell_offer_price))
    return true;
}

static bool format_manage_sell_offer_buy(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Buy", fdata->caption_len);
    FORMATTER_CHECK(
        print_asset(&fdata->envelope->tx_details.tx.op_details.manage_sell_offer_op.buying,
                    fdata->envelope->network,
                    fdata->value,
                    fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_manage_sell_offer_sell))
    return true;
}

static bool format_manage_sell_offer(formatter_data_t *fdata) {
    if (!fdata->envelope->tx_details.tx.op_details.manage_sell_offer_op.amount) {
        STRLCPY(fdata->caption, "Remove Offer", fdata->caption_len);
        FORMATTER_CHECK(print_uint64_num(
            fdata->envelope->tx_details.tx.op_details.manage_sell_offer_op.offer_id,
            fdata->value,
            fdata->value_len))
        return format_operation_source_prepare(fdata);
    } else {
        if (fdata->envelope->tx_details.tx.op_details.manage_sell_offer_op.offer_id) {
            STRLCPY(fdata->caption, "Change Offer", fdata->caption_len);
            FORMATTER_CHECK(print_uint64_num(
                fdata->envelope->tx_details.tx.op_details.manage_sell_offer_op.offer_id,
                fdata->value,
                fdata->value_len))
        } else {
            STRLCPY(fdata->caption, "Create Offer", fdata->caption_len);
        }
        FORMATTER_CHECK(push_to_formatter_stack(&format_manage_sell_offer_buy))
    }
    return true;
}

static bool format_manage_buy_offer_price(formatter_data_t *fdata) {
    manage_buy_offer_op_t *op = &fdata->envelope->tx_details.tx.op_details.manage_buy_offer_op;
    STRLCPY(fdata->caption, "Price", fdata->caption_len);
    FORMATTER_CHECK(print_price(&op->price,
                                &op->selling,
                                &op->buying,
                                fdata->envelope->network,
                                fdata->value,
                                fdata->value_len))
    return format_operation_source_prepare(fdata);
}

static bool format_manage_buy_offer_buy(formatter_data_t *fdata) {
    manage_buy_offer_op_t *op = &fdata->envelope->tx_details.tx.op_details.manage_buy_offer_op;

    STRLCPY(fdata->caption, "Buy", fdata->caption_len);
    FORMATTER_CHECK(print_amount(op->buy_amount,
                                 &op->buying,
                                 fdata->envelope->network,
                                 fdata->value,
                                 fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_manage_buy_offer_price))
    return true;
}

static bool format_manage_buy_offer_sell(formatter_data_t *fdata) {
    manage_buy_offer_op_t *op = &fdata->envelope->tx_details.tx.op_details.manage_buy_offer_op;

    STRLCPY(fdata->caption, "Sell", fdata->caption_len);
    FORMATTER_CHECK(
        print_asset(&op->selling, fdata->envelope->network, fdata->value, fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_manage_buy_offer_buy))
    return true;
}

static bool format_manage_buy_offer(formatter_data_t *fdata) {
    manage_buy_offer_op_t *op = &fdata->envelope->tx_details.tx.op_details.manage_buy_offer_op;

    if (op->buy_amount == 0) {
        STRLCPY(fdata->caption, "Remove Offer", fdata->caption_len);
        FORMATTER_CHECK(print_uint64_num(op->offer_id, fdata->value, fdata->value_len))
        return format_operation_source_prepare(fdata);
    } else {
        if (op->offer_id) {
            STRLCPY(fdata->caption, "Change Offer", fdata->caption_len);
            FORMATTER_CHECK(print_uint64_num(op->offer_id, fdata->value, fdata->value_len))
        } else {
            STRLCPY(fdata->caption, "Create Offer", fdata->caption_len);
        }
        FORMATTER_CHECK(push_to_formatter_stack(&format_manage_buy_offer_sell))
    }
    return true;
}

static bool format_create_passive_sell_offer_price(formatter_data_t *fdata) {
    create_passive_sell_offer_op_t *op =
        &fdata->envelope->tx_details.tx.op_details.create_passive_sell_offer_op;
    STRLCPY(fdata->caption, "Price", fdata->caption_len);
    FORMATTER_CHECK(print_price(&op->price,
                                &op->buying,
                                &op->selling,
                                fdata->envelope->network,
                                fdata->value,
                                fdata->value_len))
    return format_operation_source_prepare(fdata);
}

static bool format_create_passive_sell_offer_sell(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Sell", fdata->caption_len);
    FORMATTER_CHECK(print_amount(
        fdata->envelope->tx_details.tx.op_details.create_passive_sell_offer_op.amount,
        &fdata->envelope->tx_details.tx.op_details.create_passive_sell_offer_op.selling,
        fdata->envelope->network,
        fdata->value,
        fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_create_passive_sell_offer_price))
    return true;
}

static bool format_create_passive_sell_offer_buy(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Buy", fdata->caption_len);
    FORMATTER_CHECK(
        print_asset(&fdata->envelope->tx_details.tx.op_details.create_passive_sell_offer_op.buying,
                    fdata->envelope->network,
                    fdata->value,
                    fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_create_passive_sell_offer_sell))
    return true;
}

static bool format_create_passive_sell_offer(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Create Passive Sell Offer", fdata->value_len);
    FORMATTER_CHECK(push_to_formatter_stack(&format_create_passive_sell_offer_buy))
    return true;
}

static bool format_path_payment_strict_receive_receive(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Receive", fdata->caption_len);
    FORMATTER_CHECK(print_amount(
        fdata->envelope->tx_details.tx.op_details.path_payment_strict_receive_op.dest_amount,
        &fdata->envelope->tx_details.tx.op_details.path_payment_strict_receive_op.dest_asset,
        fdata->envelope->network,
        fdata->value,
        fdata->value_len))
    return format_operation_source_prepare(fdata);
}

static bool format_path_payment_strict_receive_destination(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Destination", fdata->caption_len);
    FORMATTER_CHECK(print_muxed_account(
        &fdata->envelope->tx_details.tx.op_details.path_payment_strict_receive_op.destination,
        fdata->value,
        fdata->value_len,
        0,
        0))
    FORMATTER_CHECK(push_to_formatter_stack(&format_path_payment_strict_receive_receive))
    return true;
}

static bool format_path_payment_strict_receive(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Send Max", fdata->caption_len);
    FORMATTER_CHECK(print_amount(
        fdata->envelope->tx_details.tx.op_details.path_payment_strict_receive_op.send_max,
        &fdata->envelope->tx_details.tx.op_details.path_payment_strict_receive_op.send_asset,
        fdata->envelope->network,
        fdata->value,
        fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_path_payment_strict_receive_destination))
    return true;
}

static bool format_path_payment_strict_send_receive(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Receive Min", fdata->caption_len);
    FORMATTER_CHECK(print_amount(
        fdata->envelope->tx_details.tx.op_details.path_payment_strict_send_op.dest_min,
        &fdata->envelope->tx_details.tx.op_details.path_payment_strict_send_op.dest_asset,
        fdata->envelope->network,
        fdata->value,
        fdata->value_len))
    return format_operation_source_prepare(fdata);
}

static bool format_path_payment_strict_send_destination(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Destination", fdata->caption_len);
    FORMATTER_CHECK(print_muxed_account(
        &fdata->envelope->tx_details.tx.op_details.path_payment_strict_send_op.destination,
        fdata->value,
        fdata->value_len,
        0,
        0))
    FORMATTER_CHECK(push_to_formatter_stack(&format_path_payment_strict_send_receive))
    return true;
}

static bool format_path_payment_strict_send(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Send", fdata->caption_len);
    FORMATTER_CHECK(print_amount(
        fdata->envelope->tx_details.tx.op_details.path_payment_strict_send_op.send_amount,
        &fdata->envelope->tx_details.tx.op_details.path_payment_strict_send_op.send_asset,
        fdata->envelope->network,
        fdata->value,
        fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_path_payment_strict_send_destination))
    return true;
}

static bool format_payment_destination(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Destination", fdata->caption_len);
    FORMATTER_CHECK(
        print_muxed_account(&fdata->envelope->tx_details.tx.op_details.payment_op.destination,
                            fdata->value,
                            fdata->value_len,
                            0,
                            0))
    return format_operation_source_prepare(fdata);
}

static bool format_payment(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Send", fdata->caption_len);
    FORMATTER_CHECK(print_amount(fdata->envelope->tx_details.tx.op_details.payment_op.amount,
                                 &fdata->envelope->tx_details.tx.op_details.payment_op.asset,
                                 fdata->envelope->network,
                                 fdata->value,
                                 fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_payment_destination))
    return true;
}

static bool format_create_account_amount(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Starting Balance", fdata->caption_len);
    asset_t asset = {.type = ASSET_TYPE_NATIVE};
    FORMATTER_CHECK(
        print_amount(fdata->envelope->tx_details.tx.op_details.create_account_op.starting_balance,
                     &asset,
                     fdata->envelope->network,
                     fdata->value,
                     fdata->value_len))
    return format_operation_source_prepare(fdata);
}

static bool format_create_account_destination(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Destination", fdata->caption_len);
    FORMATTER_CHECK(
        print_account_id(fdata->envelope->tx_details.tx.op_details.create_account_op.destination,
                         fdata->value,
                         fdata->value_len,
                         0,
                         0))
    FORMATTER_CHECK(push_to_formatter_stack(&format_create_account_amount))
    return true;
}

static bool format_create_account(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Create Account", fdata->value_len);
    FORMATTER_CHECK(push_to_formatter_stack(&format_create_account_destination))
    return true;
}

static bool format_create_claimable_balance_warning(formatter_data_t *fdata) {
    (void) fdata;
    // The claimant can be very complicated. I haven't figured out how to
    // display it for the time being, so let's display an WARNING here first.
    STRLCPY(fdata->caption, "WARNING", fdata->caption_len);
    STRLCPY(fdata->value,
            "Currently does not support displaying claimant details",
            fdata->value_len);
    return format_operation_source_prepare(fdata);
}

static bool format_create_claimable_balance_balance(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Balance", fdata->caption_len);
    FORMATTER_CHECK(
        print_amount(fdata->envelope->tx_details.tx.op_details.create_claimable_balance_op.amount,
                     &fdata->envelope->tx_details.tx.op_details.create_claimable_balance_op.asset,
                     fdata->envelope->network,
                     fdata->value,
                     fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_create_claimable_balance_warning))
    return true;
}

static bool format_create_claimable_balance(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Create Claimable Balance", fdata->value_len);
    FORMATTER_CHECK(push_to_formatter_stack(&format_create_claimable_balance_balance))
    return true;
}

static bool format_claim_claimable_balance_balance_id(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Balance ID", fdata->caption_len);
    FORMATTER_CHECK(print_claimable_balance_id(
        &fdata->envelope->tx_details.tx.op_details.claim_claimable_balance_op.balance_id,
        fdata->value,
        fdata->value_len,
        12,
        12))
    return format_operation_source_prepare(fdata);
}

static bool format_claim_claimable_balance(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Claim Claimable Balance", fdata->value_len);
    FORMATTER_CHECK(push_to_formatter_stack(&format_claim_claimable_balance_balance_id))
    return true;
}

static bool format_claim_claimable_balance_sponsored_id(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Sponsored ID", fdata->caption_len);
    FORMATTER_CHECK(print_account_id(
        fdata->envelope->tx_details.tx.op_details.begin_sponsoring_future_reserves_op.sponsored_id,
        fdata->value,
        fdata->value_len,
        0,
        0))
    return format_operation_source_prepare(fdata);
}

static bool format_begin_sponsoring_future_reserves(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Begin Sponsoring Future Reserves", fdata->value_len);
    FORMATTER_CHECK(push_to_formatter_stack(&format_claim_claimable_balance_sponsored_id))
    return true;
}

static bool format_end_sponsoring_future_reserves(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "End Sponsoring Future Reserves", fdata->value_len);
    return format_operation_source_prepare(fdata);
}

static bool format_revoke_sponsorship_account(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Account ID", fdata->caption_len);
    FORMATTER_CHECK(print_account_id(fdata->envelope->tx_details.tx.op_details.revoke_sponsorship_op
                                         .ledger_key.account.account_id,
                                     fdata->value,
                                     fdata->value_len,
                                     0,
                                     0))
    return format_operation_source_prepare(fdata);
}

static bool format_revoke_sponsorship_trust_line_asset(formatter_data_t *fdata) {
    if (fdata->envelope->tx_details.tx.op_details.revoke_sponsorship_op.ledger_key.trust_line.asset
            .type == ASSET_TYPE_POOL_SHARE) {
        STRLCPY(fdata->caption, "Liquidity Pool ID", fdata->caption_len);
        FORMATTER_CHECK(print_binary(fdata->envelope->tx_details.tx.op_details.revoke_sponsorship_op
                                         .ledger_key.trust_line.asset.liquidity_pool_id,
                                     LIQUIDITY_POOL_ID_SIZE,
                                     fdata->value,
                                     fdata->value_len,
                                     0,
                                     0))
    } else {
        STRLCPY(fdata->caption, "Asset", fdata->caption_len);
        FORMATTER_CHECK(print_asset((asset_t *) &fdata->envelope->tx_details.tx.op_details
                                        .revoke_sponsorship_op.ledger_key.trust_line.asset,
                                    fdata->envelope->network,
                                    fdata->value,
                                    fdata->value_len))
    }
    return format_operation_source_prepare(fdata);
}

static bool format_revoke_sponsorship_trust_line_account(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Account ID", fdata->caption_len);
    FORMATTER_CHECK(print_account_id(fdata->envelope->tx_details.tx.op_details.revoke_sponsorship_op
                                         .ledger_key.trust_line.account_id,
                                     fdata->value,
                                     fdata->value_len,
                                     0,
                                     0))
    FORMATTER_CHECK(push_to_formatter_stack(&format_revoke_sponsorship_trust_line_asset))
    return true;
}

static bool format_revoke_sponsorship_offer_offer_id(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Offer ID", fdata->caption_len);
    FORMATTER_CHECK(print_uint64_num(
        fdata->envelope->tx_details.tx.op_details.revoke_sponsorship_op.ledger_key.offer.offer_id,
        fdata->value,
        fdata->value_len))

    return format_operation_source_prepare(fdata);
}

static bool format_revoke_sponsorship_offer_seller_id(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Seller ID", fdata->caption_len);
    FORMATTER_CHECK(print_account_id(
        fdata->envelope->tx_details.tx.op_details.revoke_sponsorship_op.ledger_key.offer.seller_id,
        fdata->value,
        fdata->value_len,
        0,
        0))
    FORMATTER_CHECK(push_to_formatter_stack(&format_revoke_sponsorship_offer_offer_id))
    return true;
}

static bool format_revoke_sponsorship_data_data_name(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Data Name", fdata->caption_len);
    FORMATTER_CHECK(print_string(
        fdata->value,
        fdata->value_len,
        fdata->envelope->tx_details.tx.op_details.revoke_sponsorship_op.ledger_key.data.data_name,
        fdata->envelope->tx_details.tx.op_details.revoke_sponsorship_op.ledger_key.data
            .data_name_size))
    return format_operation_source_prepare(fdata);
}

static bool format_revoke_sponsorship_data_account(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Account ID", fdata->caption_len);
    FORMATTER_CHECK(print_account_id(
        fdata->envelope->tx_details.tx.op_details.revoke_sponsorship_op.ledger_key.data.account_id,
        fdata->value,
        fdata->value_len,
        0,
        0))
    FORMATTER_CHECK(push_to_formatter_stack(&format_revoke_sponsorship_data_data_name))
    return true;
}

static bool format_revoke_sponsorship_claimable_balance(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Balance ID", fdata->caption_len);
    FORMATTER_CHECK(
        print_claimable_balance_id(&fdata->envelope->tx_details.tx.op_details.revoke_sponsorship_op
                                        .ledger_key.claimable_balance.balance_id,
                                   fdata->value,
                                   fdata->value_len,
                                   0,
                                   0))
    return format_operation_source_prepare(fdata);
}

static bool format_revoke_sponsorship_liquidity_pool(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Liquidity Pool ID", fdata->caption_len);
    FORMATTER_CHECK(print_binary(fdata->envelope->tx_details.tx.op_details.revoke_sponsorship_op
                                     .ledger_key.liquidity_pool.liquidity_pool_id,
                                 LIQUIDITY_POOL_ID_SIZE,
                                 fdata->value,
                                 fdata->value_len,
                                 0,
                                 0))
    return format_operation_source_prepare(fdata);
}

static bool format_revoke_sponsorship_claimable_signer_signer_key_detail(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Signer Key", fdata->caption_len);
    signer_key_t *key =
        &fdata->envelope->tx_details.tx.op_details.revoke_sponsorship_op.signer.signer_key;

    FORMATTER_CHECK(print_signer_key_detail(key, fdata->value, fdata->value_len))
    return format_operation_source_prepare(fdata);
}

static bool format_revoke_sponsorship_claimable_signer_signer_key_type(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Signer Key Type", fdata->caption_len);
    switch (
        fdata->envelope->tx_details.tx.op_details.revoke_sponsorship_op.signer.signer_key.type) {
        case SIGNER_KEY_TYPE_ED25519: {
            STRLCPY(fdata->value, "Public Key", fdata->value_len);
            break;
        }
        case SIGNER_KEY_TYPE_HASH_X: {
            STRLCPY(fdata->value, "Hash(x)", fdata->value_len);
            break;
        }
        case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
            STRLCPY(fdata->value, "Pre-Auth", fdata->value_len);
            break;
        }
        case SIGNER_KEY_TYPE_ED25519_SIGNED_PAYLOAD: {
            STRLCPY(fdata->value, "Ed25519 Signed Payload", fdata->value_len);
            break;
        }
        default:
            return false;
    }

    FORMATTER_CHECK(
        push_to_formatter_stack(&format_revoke_sponsorship_claimable_signer_signer_key_detail))
    return true;
}

static bool format_revoke_sponsorship_claimable_signer_account(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Account ID", fdata->caption_len);
    FORMATTER_CHECK(print_account_id(
        fdata->envelope->tx_details.tx.op_details.revoke_sponsorship_op.signer.account_id,
        fdata->value,
        fdata->value_len,
        0,
        0))
    FORMATTER_CHECK(
        push_to_formatter_stack(&format_revoke_sponsorship_claimable_signer_signer_key_type))
    return true;
}

static bool format_revoke_sponsorship(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    if (fdata->envelope->tx_details.tx.op_details.revoke_sponsorship_op.type ==
        REVOKE_SPONSORSHIP_SIGNER) {
        STRLCPY(fdata->value, "Revoke Sponsorship (SIGNER_KEY)", fdata->value_len);
        FORMATTER_CHECK(
            push_to_formatter_stack(&format_revoke_sponsorship_claimable_signer_account))
    } else {
        switch (fdata->envelope->tx_details.tx.op_details.revoke_sponsorship_op.ledger_key.type) {
            case ACCOUNT:
                STRLCPY(fdata->value, "Revoke Sponsorship (ACCOUNT)", fdata->value_len);
                FORMATTER_CHECK(push_to_formatter_stack(&format_revoke_sponsorship_account))
                break;
            case OFFER:
                STRLCPY(fdata->value, "Revoke Sponsorship (OFFER)", fdata->value_len);
                FORMATTER_CHECK(push_to_formatter_stack(&format_revoke_sponsorship_offer_seller_id))
                break;
            case TRUSTLINE:
                STRLCPY(fdata->value, "Revoke Sponsorship (TRUSTLINE)", fdata->value_len);
                FORMATTER_CHECK(
                    push_to_formatter_stack(&format_revoke_sponsorship_trust_line_account))
                break;
            case DATA:
                STRLCPY(fdata->value, "Revoke Sponsorship (DATA)", fdata->value_len);
                FORMATTER_CHECK(push_to_formatter_stack(&format_revoke_sponsorship_data_account))
                break;
            case CLAIMABLE_BALANCE:
                STRLCPY(fdata->value, "Revoke Sponsorship (CLAIMABLE_BALANCE)", fdata->value_len);
                FORMATTER_CHECK(
                    push_to_formatter_stack(&format_revoke_sponsorship_claimable_balance))
                break;
            case LIQUIDITY_POOL:
                STRLCPY(fdata->value, "Revoke Sponsorship (LIQUIDITY_POOL)", fdata->value_len);
                FORMATTER_CHECK(push_to_formatter_stack(&format_revoke_sponsorship_liquidity_pool))
                break;
            default:
                return false;
        }
    }
    return true;
}

static bool format_clawback_from(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "From", fdata->caption_len);
    FORMATTER_CHECK(print_muxed_account(&fdata->envelope->tx_details.tx.op_details.clawback_op.from,
                                        fdata->value,
                                        fdata->value_len,
                                        0,
                                        0))
    return format_operation_source_prepare(fdata);
}

static bool format_clawback_amount(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Clawback Balance", fdata->caption_len);
    FORMATTER_CHECK(print_amount(fdata->envelope->tx_details.tx.op_details.clawback_op.amount,
                                 &fdata->envelope->tx_details.tx.op_details.clawback_op.asset,
                                 fdata->envelope->network,
                                 fdata->value,
                                 fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_clawback_from))
    return true;
}

static bool format_clawback(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Clawback", fdata->value_len);
    FORMATTER_CHECK(push_to_formatter_stack(&format_clawback_amount))
    return true;
}

static bool format_clawback_claimable_balance_balance_id(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Balance ID", fdata->caption_len);
    FORMATTER_CHECK(print_claimable_balance_id(
        &fdata->envelope->tx_details.tx.op_details.clawback_claimable_balance_op.balance_id,
        fdata->value,
        fdata->value_len,
        0,
        0))
    return format_operation_source_prepare(fdata);
}

static bool format_clawback_claimable_balance(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Clawback Claimable Balance", fdata->value_len);
    FORMATTER_CHECK(push_to_formatter_stack(&format_clawback_claimable_balance_balance_id))
    return true;
}

static bool format_set_trust_line_set_flags(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Set Flags", fdata->caption_len);
    if (fdata->envelope->tx_details.tx.op_details.set_trust_line_flags_op.set_flags) {
        FORMATTER_CHECK(print_trust_line_flags(
            fdata->envelope->tx_details.tx.op_details.set_trust_line_flags_op.set_flags,
            fdata->value,
            fdata->value_len))
    } else {
        STRLCPY(fdata->value, "[none]", fdata->value_len);
    }
    return format_operation_source_prepare(fdata);
}

static bool format_set_trust_line_clear_flags(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Clear Flags", fdata->caption_len);
    if (fdata->envelope->tx_details.tx.op_details.set_trust_line_flags_op.clear_flags) {
        FORMATTER_CHECK(print_trust_line_flags(
            fdata->envelope->tx_details.tx.op_details.set_trust_line_flags_op.clear_flags,
            fdata->value,
            fdata->value_len))
    } else {
        STRLCPY(fdata->value, "[none]", fdata->value_len);
    }
    FORMATTER_CHECK(push_to_formatter_stack(&format_set_trust_line_set_flags))
    return true;
}

static bool format_set_trust_line_asset(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Asset", fdata->caption_len);
    FORMATTER_CHECK(
        print_asset(&fdata->envelope->tx_details.tx.op_details.set_trust_line_flags_op.asset,
                    fdata->envelope->network,
                    fdata->value,
                    fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_set_trust_line_clear_flags))
    return true;
}

static bool format_set_trust_line_trustor(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Trustor", fdata->caption_len);
    FORMATTER_CHECK(
        print_account_id(fdata->envelope->tx_details.tx.op_details.set_trust_line_flags_op.trustor,
                         fdata->value,
                         fdata->value_len,
                         0,
                         0))
    FORMATTER_CHECK(push_to_formatter_stack(&format_set_trust_line_asset))
    return true;
}

static bool format_set_trust_line_flags(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Set Trust Line Flags", fdata->value_len);
    FORMATTER_CHECK(push_to_formatter_stack(&format_set_trust_line_trustor))
    return true;
}

static bool format_liquidity_pool_deposit_max_price(formatter_data_t *fdata) {
    liquidity_pool_deposit_op_t *op =
        &fdata->envelope->tx_details.tx.op_details.liquidity_pool_deposit_op;
    STRLCPY(fdata->caption, "Max Price", fdata->caption_len);
    FORMATTER_CHECK(print_price(&op->max_price,
                                NULL,
                                NULL,
                                fdata->envelope->network,
                                fdata->value,
                                fdata->value_len))
    return format_operation_source_prepare(fdata);
}

static bool format_liquidity_pool_deposit_min_price(formatter_data_t *fdata) {
    liquidity_pool_deposit_op_t *op =
        &fdata->envelope->tx_details.tx.op_details.liquidity_pool_deposit_op;
    STRLCPY(fdata->caption, "Min Price", fdata->caption_len);
    FORMATTER_CHECK(print_price(&op->min_price,
                                NULL,
                                NULL,
                                fdata->envelope->network,
                                fdata->value,
                                fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_liquidity_pool_deposit_max_price))
    return true;
}

static bool format_liquidity_pool_deposit_max_amount_b(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Max Amount B", fdata->caption_len);
    FORMATTER_CHECK(print_amount(
        fdata->envelope->tx_details.tx.op_details.liquidity_pool_deposit_op.max_amount_b,
        NULL,
        fdata->envelope->network,
        fdata->value,
        fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_liquidity_pool_deposit_min_price))
    return true;
}

static bool format_liquidity_pool_deposit_max_amount_a(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Max Amount A", fdata->caption_len);
    FORMATTER_CHECK(print_amount(
        fdata->envelope->tx_details.tx.op_details.liquidity_pool_deposit_op.max_amount_a,
        NULL,
        fdata->envelope->network,
        fdata->value,
        fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_liquidity_pool_deposit_max_amount_b))
    return true;
}

static bool format_liquidity_pool_deposit_liquidity_pool_id(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Liquidity Pool ID", fdata->caption_len);
    FORMATTER_CHECK(print_binary(
        fdata->envelope->tx_details.tx.op_details.liquidity_pool_deposit_op.liquidity_pool_id,
        LIQUIDITY_POOL_ID_SIZE,
        fdata->value,
        fdata->value_len,
        0,
        0))
    FORMATTER_CHECK(push_to_formatter_stack(&format_liquidity_pool_deposit_max_amount_a))
    return true;
}

static bool format_liquidity_pool_deposit(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Liquidity Pool Deposit", fdata->value_len);
    FORMATTER_CHECK(push_to_formatter_stack(&format_liquidity_pool_deposit_liquidity_pool_id))
    return true;
}

static bool format_liquidity_pool_withdraw_min_amount_b(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Min Amount B", fdata->caption_len);
    FORMATTER_CHECK(print_amount(
        fdata->envelope->tx_details.tx.op_details.liquidity_pool_withdraw_op.min_amount_b,
        NULL,
        fdata->envelope->network,
        fdata->value,
        fdata->value_len))
    return format_operation_source_prepare(fdata);
}

static bool format_liquidity_pool_withdraw_min_amount_a(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Min Amount A", fdata->caption_len);
    FORMATTER_CHECK(print_amount(
        fdata->envelope->tx_details.tx.op_details.liquidity_pool_withdraw_op.min_amount_a,
        NULL,
        fdata->envelope->network,
        fdata->value,
        fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_liquidity_pool_withdraw_min_amount_b))
    return true;
}

static bool format_liquidity_pool_withdraw_amount(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Amount", fdata->caption_len);
    FORMATTER_CHECK(
        print_amount(fdata->envelope->tx_details.tx.op_details.liquidity_pool_withdraw_op.amount,
                     NULL,
                     fdata->envelope->network,
                     fdata->value,
                     fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_liquidity_pool_withdraw_min_amount_a))
    return true;
}

static bool format_liquidity_pool_withdraw_liquidity_pool_id(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Liquidity Pool ID", fdata->caption_len);
    FORMATTER_CHECK(print_binary(
        fdata->envelope->tx_details.tx.op_details.liquidity_pool_withdraw_op.liquidity_pool_id,
        LIQUIDITY_POOL_ID_SIZE,
        fdata->value,
        fdata->value_len,
        0,
        0))
    FORMATTER_CHECK(push_to_formatter_stack(&format_liquidity_pool_withdraw_amount))
    return true;
}

static bool format_liquidity_pool_withdraw(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Liquidity Pool Withdraw", fdata->value_len);
    FORMATTER_CHECK(push_to_formatter_stack(&format_liquidity_pool_withdraw_liquidity_pool_id))
    return true;
}

/**
 * If the plugin is present and the contract has data pairs, move the control to the plugin.
 */
static bool should_move_control_to_plugin(formatter_data_t *fdata) {
    if (fdata->plugin_check_presence == NULL || fdata->plugin_init_contract == NULL ||
        fdata->plugin_query_data_pair_count == NULL || fdata->plugin_query_data_pair == NULL) {
        return false;
    }

    invoke_contract_args_t invoke_contract_args;
    if (fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
        invoke_contract_args = fdata->envelope->soroban_authorization.invoke_contract_args;
    } else {
        invoke_contract_args =
            fdata->envelope->tx_details.tx.op_details.invoke_host_function_op.invoke_contract_args;
    }

    const uint8_t *contract_address = invoke_contract_args.address.address;

    // check if plugin exists
    if (!fdata->plugin_check_presence(contract_address)) {
        return false;
    }

    // init plugin
    if (fdata->plugin_init_contract(contract_address) != STELLAR_PLUGIN_RESULT_OK) {
        return false;
    }

    // get data count
    if (fdata->plugin_query_data_pair_count(contract_address, &plugin_data_pair_count) !=
        STELLAR_PLUGIN_RESULT_OK) {
        return false;
    }

    return plugin_data_pair_count != 0;
}

static bool print_scval(buffer_t buffer, char *value, uint8_t value_len) {
    uint32_t sc_type;
    FORMATTER_CHECK(parse_uint32(&buffer, &sc_type))

    switch (sc_type) {
        case SCV_BOOL: {
            bool b;
            FORMATTER_CHECK(parse_bool(&buffer, &b))
            STRLCPY(value, b ? "true" : "false", value_len);
            break;
        }
        case SCV_VOID:
            STRLCPY(value, "[void]", value_len);
            break;  // void
        case SCV_U32:
            FORMATTER_CHECK(print_uint32(buffer.ptr + buffer.offset, 0, value, value_len, true));
            break;
        case SCV_I32:
            FORMATTER_CHECK(print_int32(buffer.ptr + buffer.offset, 0, value, value_len, true));
            break;
        case SCV_U64:
            FORMATTER_CHECK(print_uint64(buffer.ptr + buffer.offset, 0, value, value_len, true));
            break;
        case SCV_I64:
            FORMATTER_CHECK(print_int64(buffer.ptr + buffer.offset, 0, value, value_len, true));
            break;
        case SCV_TIMEPOINT: {
            uint64_t timepoint;
            FORMATTER_CHECK(parse_uint64(&buffer, &timepoint));
            FORMATTER_CHECK(print_time(timepoint, value, value_len));
            break;
        }
        case SCV_DURATION:
            FORMATTER_CHECK(print_int64(buffer.ptr + buffer.offset, 0, value, value_len, true));
            break;
        case SCV_U128:
            FORMATTER_CHECK(print_uint128(buffer.ptr + buffer.offset, 0, value, value_len, true));
            break;
        case SCV_I128:
            FORMATTER_CHECK(print_int128(buffer.ptr + buffer.offset, 0, value, value_len, true));
            break;
        case SCV_U256:
            FORMATTER_CHECK(print_uint256(buffer.ptr + buffer.offset, 0, value, value_len, true));
            break;
        case SCV_I256:
            FORMATTER_CHECK(print_int256(buffer.ptr + buffer.offset, 0, value, value_len, true));
            break;
        case SCV_BYTES:
            STRLCPY(value, "[Bytes Data]", value_len);
            break;
        case SCV_STRING: {
            scv_string_t scv_string;
            FORMATTER_CHECK(parse_scv_string(&buffer, &scv_string));
            FORMATTER_CHECK(print_scv_string(&scv_string, value, value_len));
            break;
        }
        case SCV_SYMBOL: {
            scv_symbol_t scv_symbol;
            FORMATTER_CHECK(parse_scv_symbol(&buffer, &scv_symbol));
            FORMATTER_CHECK(print_scv_symbol(&scv_symbol, value, value_len));
            break;
        }
        case SCV_ADDRESS: {
            sc_address_t sc_address;
            FORMATTER_CHECK(parse_sc_address(&buffer, &sc_address));
            FORMATTER_CHECK(print_sc_address(&sc_address, value, value_len, 0, 0));
            break;
        }
        default:
            STRLCPY(value, "[unable to display]", value_len);
    }
    return true;
}

// Format sub-invocation
static bool format_next_sub_invocation(formatter_data_t *fdata) {
    uint8_t sub_invocations_count =
        fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION
            ? fdata->envelope->soroban_authorization.sub_invocations_count
            : fdata->envelope->tx_details.tx.op_details.invoke_host_function_op
                  .sub_invocations_count;
    uint8_t *sub_invocation_index =
        fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION
            ? &fdata->envelope->soroban_authorization.sub_invocation_index
            : &fdata->envelope->tx_details.tx.op_details.invoke_host_function_op
                   .sub_invocation_index;
    (*sub_invocation_index)++;
    if (*sub_invocation_index == sub_invocations_count) {
        return push_to_formatter_stack(NULL);
    } else {
        formatter_index = 0;
        return push_to_formatter_stack(format_sub_invocation_start);
    }
}

static bool format_sub_invocation_invoke_host_function_args(formatter_data_t *fdata) {
    invoke_contract_args_t invoke_contract_args;
    if (fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
        invoke_contract_args = fdata->envelope->soroban_authorization.invoke_contract_args;
    } else {
        invoke_contract_args =
            fdata->envelope->tx_details.tx.op_details.invoke_host_function_op.invoke_contract_args;
    }

    char op_caption[OPERATION_CAPTION_MAX_LENGTH] = {0};
    size_t length;
    STRLCPY(op_caption, "Arg ", OPERATION_CAPTION_MAX_LENGTH);
    length = strlen(op_caption);
    FORMATTER_CHECK(print_uint64_num(parameters_index + 1,
                                     op_caption + length,
                                     OPERATION_CAPTION_MAX_LENGTH - length))

    STRLCAT(op_caption, " of ", sizeof(op_caption));
    length = strlen(op_caption);
    FORMATTER_CHECK(print_uint64_num(invoke_contract_args.parameters_length,
                                     op_caption + length,
                                     OPERATION_CAPTION_MAX_LENGTH - length))
    STRLCPY(fdata->caption, op_caption, fdata->caption_len);

    buffer_t buffer = {.ptr = fdata->raw_data,
                       .size = fdata->raw_data_len,
                       .offset = invoke_contract_args.parameters_position};
    // Content
    for (uint8_t i = 0; i < parameters_index; i++) {
        FORMATTER_CHECK(read_scval_advance(&buffer))
    }

    FORMATTER_CHECK(print_scval(buffer, fdata->value, fdata->value_len))

    parameters_index++;
    if (parameters_index == invoke_contract_args.parameters_length) {
        return format_next_sub_invocation(fdata);
    } else {
        FORMATTER_CHECK(push_to_formatter_stack(&format_sub_invocation_invoke_host_function_args))
    }
    return true;
}

static bool format_sub_invocation_invoke_host_function_args_with_plugin(formatter_data_t *fdata) {
    invoke_contract_args_t invoke_contract_args;
    if (fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
        invoke_contract_args = fdata->envelope->soroban_authorization.invoke_contract_args;
    } else {
        invoke_contract_args =
            fdata->envelope->tx_details.tx.op_details.invoke_host_function_op.invoke_contract_args;
    }

    const uint8_t *contract_address = invoke_contract_args.address.address;

    // get data pair
    if (fdata->plugin_query_data_pair(contract_address,
                                      parameters_index,
                                      fdata->caption,
                                      fdata->caption_len,
                                      fdata->value,
                                      fdata->value_len) != STELLAR_PLUGIN_RESULT_OK) {
        return false;
    }

    parameters_index++;
    if (parameters_index == plugin_data_pair_count) {
        return format_next_sub_invocation(fdata);
    } else {
        FORMATTER_CHECK(
            push_to_formatter_stack(&format_sub_invocation_invoke_host_function_args_with_plugin))
    }
    return true;
}

static bool format_sub_invocation_invoke_host_function_func_name(formatter_data_t *fdata) {
    invoke_contract_args_t invoke_contract_args;
    if (fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
        invoke_contract_args = fdata->envelope->soroban_authorization.invoke_contract_args;
    } else {
        invoke_contract_args =
            fdata->envelope->tx_details.tx.op_details.invoke_host_function_op.invoke_contract_args;
    }
    STRLCPY(fdata->caption, "Function", fdata->caption_len);
    FORMATTER_CHECK(print_string(fdata->value,
                                 fdata->value_len,
                                 invoke_contract_args.function.name,
                                 invoke_contract_args.function.name_size))

    uint8_t data_count = should_move_control_to_plugin(fdata);
    if (data_count == 0) {
        // we should not move control to plugin
        if (invoke_contract_args.parameters_length == 0) {
            return format_next_sub_invocation(fdata);
        } else {
            parameters_index = 0;
            FORMATTER_CHECK(
                push_to_formatter_stack(&format_sub_invocation_invoke_host_function_args))
        }
    } else {
        PRINTF("we should move control to plugin\n");
        parameters_index = 0;
        FORMATTER_CHECK(
            push_to_formatter_stack(&format_sub_invocation_invoke_host_function_args_with_plugin))
    }

    return true;
}

static bool format_sub_invocation_invoke_host_function_contract_id(formatter_data_t *fdata) {
    invoke_contract_args_t invoke_contract_args;
    if (fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
        invoke_contract_args = fdata->envelope->soroban_authorization.invoke_contract_args;
    } else {
        invoke_contract_args =
            fdata->envelope->tx_details.tx.op_details.invoke_host_function_op.invoke_contract_args;
    }

    STRLCPY(fdata->caption, "Contract ID", fdata->caption_len);

    FORMATTER_CHECK(
        print_sc_address(&invoke_contract_args.address, fdata->value, fdata->value_len, 0, 0))
    push_to_formatter_stack(&format_sub_invocation_invoke_host_function_func_name);
    return true;
}

static bool format_sub_invocation_auth_function(formatter_data_t *fdata) {
    soroban_authorization_function_type_t auth_function_type =
        fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION
            ? fdata->envelope->soroban_authorization.auth_function_type
            : fdata->envelope->tx_details.tx.op_details.invoke_host_function_op.auth_function_type;
    PRINTF("auth_function_type: %d\n", auth_function_type);
    switch (auth_function_type) {
        case SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN:
            STRLCPY(fdata->caption, "Soroban", fdata->caption_len);
            STRLCPY(fdata->value, INVOKE_SMART_CONTRACT, fdata->value_len);
            FORMATTER_CHECK(
                push_to_formatter_stack(&format_sub_invocation_invoke_host_function_contract_id));
            break;
        case SOROBAN_AUTHORIZED_FUNCTION_TYPE_CREATE_CONTRACT_HOST_FN:
            STRLCPY(fdata->caption, "Soroban", fdata->caption_len);
            STRLCPY(fdata->value, "Create Smart Contract", fdata->value_len);
            FORMATTER_CHECK(format_next_sub_invocation(fdata));
            break;
        default:
            return false;
    }
    return true;
}

static bool format_sub_invocation_start(formatter_data_t *fdata) {
    uint8_t sub_invocation_index = 0;
    uint8_t sub_invocations_count = 0;
    if (fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
        sub_invocation_index = fdata->envelope->soroban_authorization.sub_invocation_index;
        sub_invocations_count = fdata->envelope->soroban_authorization.sub_invocations_count;
    } else {
        sub_invocation_index =
            fdata->envelope->tx_details.tx.op_details.invoke_host_function_op.sub_invocation_index;
        sub_invocations_count =
            fdata->envelope->tx_details.tx.op_details.invoke_host_function_op.sub_invocations_count;
    }

    STRLCPY(fdata->caption, "Nested Authorization", fdata->caption_len);

    FORMATTER_CHECK(print_uint64_num(sub_invocation_index + 1, fdata->value, fdata->value_len))
    STRLCAT(fdata->value, " of ", fdata->value_len)
    FORMATTER_CHECK(print_uint64_num(sub_invocations_count,
                                     fdata->value + strlen(fdata->value),
                                     fdata->value_len - strlen(fdata->value)))

    buffer_t buffer = {
        .ptr = fdata->raw_data,
        .size = fdata->raw_data_len,
        .offset = fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION
                      ? fdata->envelope->soroban_authorization
                            .sub_invocation_positions[sub_invocation_index]
                      : fdata->envelope->tx_details.tx.op_details.invoke_host_function_op
                            .sub_invocation_positions[sub_invocation_index]};

    // here we parse the sub-invocation, and store it in the invoke_contract_args
    FORMATTER_CHECK(parse_auth_function(
        &buffer,
        fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION
            ? &fdata->envelope->soroban_authorization.auth_function_type
            : &fdata->envelope->tx_details.tx.op_details.invoke_host_function_op.auth_function_type,
        fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION
            ? &fdata->envelope->soroban_authorization.invoke_contract_args
            : &fdata->envelope->tx_details.tx.op_details.invoke_host_function_op
                   .invoke_contract_args))

    FORMATTER_CHECK(push_to_formatter_stack(&format_sub_invocation_auth_function))
    return true;
}

// Format sub-invocation

static bool format_operation_source_for_invoke_host_function_op(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Op Source", fdata->caption_len);
    if (fdata->envelope->type == ENVELOPE_TYPE_TX &&
        fdata->envelope->tx_details.tx.source_account.type == KEY_TYPE_ED25519 &&
        fdata->envelope->tx_details.tx.op_details.source_account.type == KEY_TYPE_ED25519 &&
        memcmp(fdata->envelope->tx_details.tx.source_account.ed25519,
               fdata->signing_key,
               RAW_ED25519_PUBLIC_KEY_SIZE) == 0 &&
        memcmp(fdata->envelope->tx_details.tx.op_details.source_account.ed25519,
               fdata->signing_key,
               RAW_ED25519_PUBLIC_KEY_SIZE) == 0) {
        FORMATTER_CHECK(
            print_muxed_account(&fdata->envelope->tx_details.tx.op_details.source_account,
                                fdata->value,
                                fdata->value_len,
                                6,
                                6))
    } else {
        FORMATTER_CHECK(
            print_muxed_account(&fdata->envelope->tx_details.tx.op_details.source_account,
                                fdata->value,
                                fdata->value_len,
                                0,
                                0))
    }
    uint8_t sub_invocations_count =
        fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION
            ? fdata->envelope->soroban_authorization.sub_invocations_count
            : fdata->envelope->tx_details.tx.op_details.invoke_host_function_op
                  .sub_invocations_count;
    if (sub_invocations_count > 0) {
        formatter_index = 0;
        FORMATTER_CHECK(push_to_formatter_stack(&format_sub_invocation_start))
    } else {
        FORMATTER_CHECK(push_to_formatter_stack(NULL))
    }
    return true;
}

static bool format_operation_source_prepare_for_invoke_host_function_op(formatter_data_t *fdata) {
    if (fdata->envelope->type != ENVELOPE_TYPE_SOROBAN_AUTHORIZATION &&
        fdata->envelope->tx_details.tx.op_details.source_account_present) {
        // If the source exists, when the user clicks the next button,
        // it will jump to the page showing the source
        FORMATTER_CHECK(
            push_to_formatter_stack(&format_operation_source_for_invoke_host_function_op))
    } else {
        uint8_t sub_invocations_count =
            fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION
                ? fdata->envelope->soroban_authorization.sub_invocations_count
                : fdata->envelope->tx_details.tx.op_details.invoke_host_function_op
                      .sub_invocations_count;
        if (sub_invocations_count > 0) {
            formatter_index = 0;
            FORMATTER_CHECK(push_to_formatter_stack(&format_sub_invocation_start))
        } else {
            FORMATTER_CHECK(push_to_formatter_stack(NULL))
        }
    }
    return true;
}

static bool format_invoke_host_function_args(formatter_data_t *fdata) {
    invoke_contract_args_t invoke_contract_args;
    if (fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
        invoke_contract_args = fdata->envelope->soroban_authorization.invoke_contract_args;
    } else {
        invoke_contract_args =
            fdata->envelope->tx_details.tx.op_details.invoke_host_function_op.invoke_contract_args;
    }

    char op_caption[OPERATION_CAPTION_MAX_LENGTH] = {0};
    size_t length;
    STRLCPY(op_caption, "Arg ", OPERATION_CAPTION_MAX_LENGTH);
    length = strlen(op_caption);
    FORMATTER_CHECK(print_uint64_num(parameters_index + 1,
                                     op_caption + length,
                                     OPERATION_CAPTION_MAX_LENGTH - length))

    STRLCAT(op_caption, " of ", sizeof(op_caption));
    length = strlen(op_caption);
    FORMATTER_CHECK(print_uint64_num(invoke_contract_args.parameters_length,
                                     op_caption + length,
                                     OPERATION_CAPTION_MAX_LENGTH - length))
    STRLCPY(fdata->caption, op_caption, fdata->caption_len);

    buffer_t buffer = {.ptr = fdata->raw_data,
                       .size = fdata->raw_data_len,
                       .offset = invoke_contract_args.parameters_position};
    // Content
    for (uint8_t i = 0; i < parameters_index; i++) {
        FORMATTER_CHECK(read_scval_advance(&buffer))
    }

    FORMATTER_CHECK(print_scval(buffer, fdata->value, fdata->value_len))

    parameters_index++;
    if (parameters_index == invoke_contract_args.parameters_length) {
        return format_operation_source_prepare_for_invoke_host_function_op(fdata);
    } else {
        FORMATTER_CHECK(push_to_formatter_stack(&format_invoke_host_function_args))
    }
    return true;
}

static bool format_invoke_host_function_args_with_plugin(formatter_data_t *fdata) {
    invoke_contract_args_t invoke_contract_args;
    if (fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
        invoke_contract_args = fdata->envelope->soroban_authorization.invoke_contract_args;
    } else {
        invoke_contract_args =
            fdata->envelope->tx_details.tx.op_details.invoke_host_function_op.invoke_contract_args;
    }

    const uint8_t *contract_address = invoke_contract_args.address.address;

    // get data pair
    if (fdata->plugin_query_data_pair(contract_address,
                                      parameters_index,
                                      fdata->caption,
                                      fdata->caption_len,
                                      fdata->value,
                                      fdata->value_len) != STELLAR_PLUGIN_RESULT_OK) {
        return false;
    }

    parameters_index++;
    if (parameters_index == plugin_data_pair_count) {
        // if (fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
        //     FORMATTER_CHECK(push_to_formatter_stack(NULL))
        // } else {
        //     return format_operation_source_prepare(fdata);
        // }
        return format_operation_source_prepare_for_invoke_host_function_op(fdata);
    } else {
        FORMATTER_CHECK(push_to_formatter_stack(&format_invoke_host_function_args_with_plugin))
    }
    return true;
}

static bool format_invoke_host_function_func_name(formatter_data_t *fdata) {
    invoke_contract_args_t invoke_contract_args;
    if (fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
        invoke_contract_args = fdata->envelope->soroban_authorization.invoke_contract_args;
    } else {
        invoke_contract_args =
            fdata->envelope->tx_details.tx.op_details.invoke_host_function_op.invoke_contract_args;
    }
    STRLCPY(fdata->caption, "Function", fdata->caption_len);
    FORMATTER_CHECK(print_string(fdata->value,
                                 fdata->value_len,
                                 invoke_contract_args.function.name,
                                 invoke_contract_args.function.name_size))

    uint8_t data_count = should_move_control_to_plugin(fdata);
    if (data_count == 0) {
        // we should not move control to plugin
        if (invoke_contract_args.parameters_length == 0) {
            return format_operation_source_prepare_for_invoke_host_function_op(fdata);
        } else {
            parameters_index = 0;
            FORMATTER_CHECK(push_to_formatter_stack(&format_invoke_host_function_args))
        }
    } else {
        PRINTF("we should move control to plugin\n");
        parameters_index = 0;
        FORMATTER_CHECK(push_to_formatter_stack(&format_invoke_host_function_args_with_plugin))
    }

    return true;
}

static bool format_invoke_host_function_contract_id(formatter_data_t *fdata) {
    sc_address_t *address =
        fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION
            ? &fdata->envelope->soroban_authorization.invoke_contract_args.address
            : &fdata->envelope->tx_details.tx.op_details.invoke_host_function_op
                   .invoke_contract_args.address;

    STRLCPY(fdata->caption, "Contract ID", fdata->caption_len);

    FORMATTER_CHECK(print_sc_address(address, fdata->value, fdata->value_len, 0, 0))
    push_to_formatter_stack(&format_invoke_host_function_func_name);
    return true;
}

static bool format_invoke_host_function(formatter_data_t *fdata) {
    // avoid the host function op be overwritten by the sub-invocation
    if (fdata->envelope->tx_details.tx.op_details.invoke_host_function_op.sub_invocations_count) {
        if (!parse_transaction_operation(fdata->raw_data,
                                         fdata->raw_data_len,
                                         fdata->envelope,
                                         fdata->envelope->tx_details.tx.operation_index)) {
            return false;
        };
    }
    switch (fdata->envelope->tx_details.tx.op_details.invoke_host_function_op.host_function_type) {
        case HOST_FUNCTION_TYPE_INVOKE_CONTRACT:
            STRLCPY(fdata->caption, "Soroban", fdata->caption_len);
            STRLCPY(fdata->value, INVOKE_SMART_CONTRACT, fdata->value_len);
            FORMATTER_CHECK(push_to_formatter_stack(&format_invoke_host_function_contract_id));
            break;
        case HOST_FUNCTION_TYPE_CREATE_CONTRACT:
            STRLCPY(fdata->caption, "Soroban", fdata->caption_len);
            STRLCPY(fdata->value, "Create Smart Contract", fdata->value_len);
            // we dont need to care the sub-invocation here
            return format_operation_source_prepare(fdata);
            break;
        case HOST_FUNCTION_TYPE_UPLOAD_CONTRACT_WASM:
            STRLCPY(fdata->caption, "Soroban", fdata->caption_len);
            STRLCPY(fdata->value, "Upload Smart Contract Wasm", fdata->value_len);
            return format_operation_source_prepare(fdata);
            break;
        default:
            return false;
    }
    return true;
}

static bool format_auth_function(formatter_data_t *fdata) {
    switch (fdata->envelope->soroban_authorization.auth_function_type) {
        case SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN:
            STRLCPY(fdata->caption, "Soroban", fdata->caption_len);
            STRLCPY(fdata->value, INVOKE_SMART_CONTRACT, fdata->value_len);
            FORMATTER_CHECK(push_to_formatter_stack(&format_invoke_host_function_contract_id));
            break;
        case SOROBAN_AUTHORIZED_FUNCTION_TYPE_CREATE_CONTRACT_HOST_FN:
            STRLCPY(fdata->caption, "Soroban", fdata->caption_len);
            STRLCPY(fdata->value, "Create Smart Contract", fdata->value_len);
            // we dont need to care the sub-invocation here
            FORMATTER_CHECK(push_to_formatter_stack(NULL))
            break;
        default:
            return false;
    }
    return true;
}

static bool format_extend_footprint_ttl(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Extend Footprint TTL", fdata->value_len);
    return format_operation_source_prepare(fdata);
}

static bool format_restore_footprint(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Operation Type", fdata->caption_len);
    STRLCPY(fdata->value, "Restore Footprint", fdata->value_len);
    return format_operation_source_prepare(fdata);
}

static const format_function_t formatters[] = {&format_create_account,
                                               &format_payment,
                                               &format_path_payment_strict_receive,
                                               &format_manage_sell_offer,
                                               &format_create_passive_sell_offer,
                                               &format_set_options,
                                               &format_change_trust,
                                               &format_allow_trust,
                                               &format_account_merge,
                                               &format_inflation,
                                               &format_manage_data,
                                               &format_bump_sequence,
                                               &format_manage_buy_offer,
                                               &format_path_payment_strict_send,
                                               &format_create_claimable_balance,
                                               &format_claim_claimable_balance,
                                               &format_begin_sponsoring_future_reserves,
                                               &format_end_sponsoring_future_reserves,
                                               &format_revoke_sponsorship,
                                               &format_clawback,
                                               &format_clawback_claimable_balance,
                                               &format_set_trust_line_flags,
                                               &format_liquidity_pool_deposit,
                                               &format_liquidity_pool_withdraw,
                                               &format_invoke_host_function,
                                               &format_extend_footprint_ttl,
                                               &format_restore_footprint};

static bool format_confirm_operation(formatter_data_t *fdata) {
    if (fdata->envelope->tx_details.tx.operations_count > 1) {
        char op_caption[OPERATION_CAPTION_MAX_LENGTH] = {0};
        size_t length;
        STRLCPY(op_caption, "Operation ", OPERATION_CAPTION_MAX_LENGTH);
        length = strlen(op_caption);
        FORMATTER_CHECK(print_uint64_num(fdata->envelope->tx_details.tx.operation_index + 1,
                                         op_caption + length,
                                         OPERATION_CAPTION_MAX_LENGTH - length))
        STRLCAT(op_caption, " of ", sizeof(op_caption));
        length = strlen(op_caption);
        FORMATTER_CHECK(print_uint64_num(fdata->envelope->tx_details.tx.operations_count,
                                         op_caption + length,
                                         OPERATION_CAPTION_MAX_LENGTH - length))
        STRLCPY(fdata->caption, op_caption, fdata->caption_len);
        FORMATTER_CHECK(push_to_formatter_stack(
            ((format_function_t) PIC(formatters[fdata->envelope->tx_details.tx.op_details.type]))));
    } else {
        format_function_t func = PIC(formatters[fdata->envelope->tx_details.tx.op_details.type]);
        FORMATTER_CHECK(func(fdata));
    }
    return true;
}

static bool format_fee_bump_transaction_fee(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Max Fee", fdata->caption_len);
    asset_t asset = {.type = ASSET_TYPE_NATIVE};
    FORMATTER_CHECK(print_amount(fdata->envelope->tx_details.fee_bump_tx.fee,
                                 &asset,
                                 fdata->envelope->network,
                                 fdata->value,
                                 fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_transaction_details))
    return true;
}

static bool format_fee_bump_transaction_source(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Fee Source", fdata->caption_len);
    if (fdata->envelope->type == ENVELOPE_TYPE_TX_FEE_BUMP &&
        fdata->envelope->tx_details.fee_bump_tx.fee_source.type == KEY_TYPE_ED25519 &&
        memcmp(fdata->envelope->tx_details.fee_bump_tx.fee_source.ed25519,
               fdata->signing_key,
               RAW_ED25519_PUBLIC_KEY_SIZE) == 0) {
        FORMATTER_CHECK(print_muxed_account(&fdata->envelope->tx_details.fee_bump_tx.fee_source,
                                            fdata->value,
                                            fdata->value_len,
                                            6,
                                            6))
    } else {
        FORMATTER_CHECK(print_muxed_account(&fdata->envelope->tx_details.fee_bump_tx.fee_source,
                                            fdata->value,
                                            fdata->value_len,
                                            0,
                                            0))
    }
    FORMATTER_CHECK(push_to_formatter_stack(&format_fee_bump_transaction_fee))
    return true;
}

static bool format_fee_bump_transaction_details(formatter_data_t *fdata) {
    (void) fdata;
    STRLCPY(fdata->caption, "Fee Bump", fdata->caption_len);
    STRLCPY(fdata->value, "Transaction Details", fdata->value_len);
    FORMATTER_CHECK(push_to_formatter_stack(&format_fee_bump_transaction_source))
    return true;
}

static bool get_tx_details_formatter(formatter_data_t *fdata) {
    if (fdata->envelope->type == ENVELOPE_TYPE_TX_FEE_BUMP) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_fee_bump_transaction_details))
    }

    if (fdata->envelope->type == ENVELOPE_TYPE_TX) {
        if (fdata->envelope->tx_details.tx.memo.type != MEMO_NONE) {
            FORMATTER_CHECK(push_to_formatter_stack(&format_memo))
        } else {
            FORMATTER_CHECK(push_to_formatter_stack(&format_fee))
        }
    }

    return true;
}

static bool format_soroban_authorization_sig_exp(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Sig Exp Ledger", fdata->caption_len);
    FORMATTER_CHECK(
        print_uint64_num(fdata->envelope->soroban_authorization.signature_expiration_ledger,
                         fdata->value,
                         fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_auth_function));
    return true;
}

static bool format_soroban_authorization_nonce(formatter_data_t *fdata) {
    // avoid the root invoke_contract_args be overwritten by the sub-invocation
    if (!parse_soroban_authorization_envelope(fdata->raw_data,
                                              fdata->raw_data_len,
                                              fdata->envelope)) {
        return false;
    };
    STRLCPY(fdata->caption, "Nonce", fdata->caption_len);
    FORMATTER_CHECK(print_uint64_num(fdata->envelope->soroban_authorization.nonce,
                                     fdata->value,
                                     fdata->value_len))
    FORMATTER_CHECK(push_to_formatter_stack(&format_soroban_authorization_sig_exp))
    return true;
}

static bool format_network(formatter_data_t *fdata) {
    STRLCPY(fdata->caption, "Network", fdata->caption_len);
    STRLCPY(fdata->value, (char *) PIC(NETWORK_NAMES[fdata->envelope->network]), fdata->value_len);
    if (fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_soroban_authorization_nonce))
    } else {
        return get_tx_details_formatter(fdata);
    }
    return true;
}

static bool format_transaction_info(formatter_data_t *fdata) {
    if (fdata->envelope->network != 0) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_network))
    } else {
        return get_tx_details_formatter(fdata);
    }
    return true;
}

static bool format_soroban_authorization(formatter_data_t *fdata) {
    if (fdata->envelope->network != 0) {
        FORMATTER_CHECK(push_to_formatter_stack(&format_network))
    } else {
        FORMATTER_CHECK(push_to_formatter_stack(&format_soroban_authorization_nonce))
    }
    return true;
}

static bool format(formatter_data_t *fdata, uint8_t data_index) {
    explicit_bzero(formatter_stack, sizeof(formatter_stack));
    formatter_index = 0;
    if (fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
        return format_soroban_authorization(fdata);
    } else {
        if (data_index == 0) {
            return format_transaction_info(fdata);
        } else {
            uint8_t op_index = data_index - 1;
            if (!parse_transaction_operation(fdata->raw_data,
                                             fdata->raw_data_len,
                                             fdata->envelope,
                                             op_index)) {
                return false;
            };
            FORMATTER_CHECK(push_to_formatter_stack(&format_confirm_operation))
        }
    }
    return true;
}

static uint8_t get_data_count(formatter_data_t *fdata) {
    if (fdata->envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
        return 1;
    }
    uint8_t op_cnt = fdata->envelope->tx_details.tx.operations_count;
    return op_cnt + 1;
}

void reset_formatter(void) {
    explicit_bzero(formatter_stack, sizeof(formatter_stack));
    formatter_index = 0;
    current_data_index = 0;
}

bool get_next_data(formatter_data_t *fdata, bool forward, bool *data_exists, bool *is_op_header) {
    if (current_data_index == 0 && formatter_index == 0 && !forward) {
        return false;
    }
    explicit_bzero(fdata->caption, fdata->caption_len);
    explicit_bzero(fdata->value, fdata->value_len);
    *is_op_header = false;
    uint8_t total_data = get_data_count(fdata);
    // printf("current_data_index: %d, formatter_index: %d\n", current_data_index,
    // formatter_index);
    if (forward) {
        if (current_data_index == 0 && formatter_index == 0) {
            FORMATTER_CHECK(format(fdata, current_data_index));
            if (formatter_stack[0] == NULL) {
                return false;
            }
            FORMATTER_CHECK(formatter_stack[0](fdata));
            *data_exists = true;
        } else if (current_data_index < total_data - 1 &&
                   formatter_stack[formatter_index - 1] == NULL) {
            current_data_index++;
            FORMATTER_CHECK(format(fdata, current_data_index));
            if (formatter_stack[0] == NULL) {
                return false;
            }
            FORMATTER_CHECK(formatter_stack[0](fdata));
            *is_op_header = true;
            *data_exists = true;
        } else if (current_data_index == total_data - 1 &&
                   formatter_stack[formatter_index - 1] == NULL) {
            formatter_index++;  // we can back from the approve page
            *data_exists = false;
        } else {
            FORMATTER_CHECK(formatter_stack[formatter_index - 1](fdata));
            *data_exists = true;
        }
    } else {
        if (current_data_index == 0 && formatter_index == 2) {
            formatter_index = 0;
            *data_exists = false;
        } else if (current_data_index > 0 && formatter_index == 2) {
            current_data_index -= 1;
            FORMATTER_CHECK(format(fdata, current_data_index));
            if (formatter_stack[0] == NULL) {
                return false;
            }
            FORMATTER_CHECK(formatter_stack[0](fdata));
            *data_exists = true;
            if (current_data_index > 0) {
                *is_op_header = true;
            }
        } else {
            FORMATTER_CHECK(format(fdata, current_data_index));
            if (formatter_stack[0] == NULL) {
                return false;
            }
            FORMATTER_CHECK(formatter_stack[0](fdata));
            *data_exists = true;
            if (current_data_index > 0) {
                *is_op_header = true;
            }
        }
    }

    return true;
}
