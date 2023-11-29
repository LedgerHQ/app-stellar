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

#include <stdbool.h>  // bool
#include <stdio.h>
#include <string.h>

#include "os.h"
#include "bolos_target.h"

#include "./transaction_formatter.h"
#include "../sw.h"
#include "../utils.h"
#include "../types.h"
#include "../globals.h"
#include "../settings.h"
#include "../common/format.h"
#include "../transaction/transaction_parser.h"

static const char *NETWORK_NAMES[3] = {"Public", "Testnet", "Unknown"};

char op_caption[OPERATION_CAPTION_MAX_LENGTH];
format_function_t formatter_stack[MAX_FORMATTERS_PER_OPERATION];
int8_t formatter_index;

static void push_to_formatter_stack(format_function_t formatter) {
    if (formatter_index + 1 >= MAX_FORMATTERS_PER_OPERATION) {
        THROW(SW_TX_FORMATTING_FAIL);
    }
    formatter_stack[formatter_index + 1] = formatter;
}

static void format_next_step(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    formatter_stack[formatter_index] = NULL;
    set_state_data(true);
}

static void format_transaction_source(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Tx Source", DETAIL_CAPTION_MAX_LENGTH);
    if (tx_ctx->envelope_type == ENVELOPE_TYPE_TX &&
        tx_ctx->tx_details.source_account.type == KEY_TYPE_ED25519 &&
        memcmp(tx_ctx->tx_details.source_account.ed25519,
               G_context.raw_public_key,
               RAW_ED25519_PUBLIC_KEY_SIZE) == 0) {
        FORMATTER_CHECK(print_muxed_account(&tx_ctx->tx_details.source_account,
                                            G.ui.detail_value,
                                            DETAIL_VALUE_MAX_LENGTH,
                                            6,
                                            6))
    } else {
        FORMATTER_CHECK(print_muxed_account(&tx_ctx->tx_details.source_account,
                                            G.ui.detail_value,
                                            DETAIL_VALUE_MAX_LENGTH,
                                            0,
                                            0))
    }
    push_to_formatter_stack(format_next_step);
}

static void format_min_seq_ledger_gap(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Min Seq Ledger Gap", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_uint(tx_ctx->tx_details.cond.min_seq_ledger_gap,
                               G.ui.detail_value,
                               DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_transaction_source);
}

static void format_min_seq_ledger_gap_prepare(tx_ctx_t *tx_ctx) {
    if (tx_ctx->tx_details.cond.min_seq_ledger_gap == 0) {
        format_transaction_source(tx_ctx);
    } else {
        format_min_seq_ledger_gap(tx_ctx);
    }
}

static void format_min_seq_age(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Min Seq Age", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_uint(tx_ctx->tx_details.cond.min_seq_age, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_min_seq_ledger_gap_prepare);
}

static void format_min_seq_age_prepare(tx_ctx_t *tx_ctx) {
    if (tx_ctx->tx_details.cond.min_seq_age == 0) {
        format_min_seq_ledger_gap_prepare(tx_ctx);
    } else {
        format_min_seq_age(tx_ctx);
    }
}

static void format_min_seq_num(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Min Seq Num", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_uint(tx_ctx->tx_details.cond.min_seq_num, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_min_seq_age_prepare);
}

static void format_min_seq_num_prepare(tx_ctx_t *tx_ctx) {
    if (!tx_ctx->tx_details.cond.min_seq_num_present || tx_ctx->tx_details.cond.min_seq_num == 0) {
        format_min_seq_age_prepare(tx_ctx);
    } else {
        format_min_seq_num(tx_ctx);
    }
}

static void format_ledger_bounds_max_ledger(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Ledger Bounds Max", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_uint(tx_ctx->tx_details.cond.ledger_bounds.max_ledger,
                               G.ui.detail_value,
                               DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_min_seq_num_prepare);
}

static void format_ledger_bounds_min_ledger(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Ledger Bounds Min", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_uint(tx_ctx->tx_details.cond.ledger_bounds.min_ledger,
                               G.ui.detail_value,
                               DETAIL_VALUE_MAX_LENGTH))
    if (tx_ctx->tx_details.cond.ledger_bounds.max_ledger != 0) {
        push_to_formatter_stack(&format_ledger_bounds_max_ledger);
    } else {
        push_to_formatter_stack(&format_min_seq_num_prepare);
    }
}

static void format_ledger_bounds(tx_ctx_t *tx_ctx) {
    if (!tx_ctx->tx_details.cond.ledger_bounds_present ||
        (tx_ctx->tx_details.cond.ledger_bounds.min_ledger == 0 &&
         tx_ctx->tx_details.cond.ledger_bounds.max_ledger == 0)) {
        format_min_seq_num_prepare(tx_ctx);
    } else if (tx_ctx->tx_details.cond.ledger_bounds.min_ledger != 0) {
        format_ledger_bounds_min_ledger(tx_ctx);
    } else {
        format_ledger_bounds_max_ledger(tx_ctx);
    }
}

static void format_time_bounds_max_time(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Valid Before (UTC)", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_time(tx_ctx->tx_details.cond.time_bounds.max_time,
                               G.ui.detail_value,
                               DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_ledger_bounds);
}

static void format_time_bounds_min_time(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Valid After (UTC)", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_time(tx_ctx->tx_details.cond.time_bounds.min_time,
                               G.ui.detail_value,
                               DETAIL_VALUE_MAX_LENGTH))

    if (tx_ctx->tx_details.cond.time_bounds.max_time != 0) {
        push_to_formatter_stack(&format_time_bounds_max_time);
    } else {
        push_to_formatter_stack(&format_ledger_bounds);
    }
}

static void format_time_bounds(tx_ctx_t *tx_ctx) {
    if (!tx_ctx->tx_details.cond.time_bounds_present ||
        (tx_ctx->tx_details.cond.time_bounds.min_time == 0 &&
         tx_ctx->tx_details.cond.time_bounds.max_time == 0)) {
        format_ledger_bounds(tx_ctx);
    } else if (tx_ctx->tx_details.cond.time_bounds.min_time != 0) {
        format_time_bounds_min_time(tx_ctx);
    } else {
        format_time_bounds_max_time(tx_ctx);
    }
}

static void format_sequence(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Sequence Num", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_uint(tx_ctx->tx_details.sequence_number, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_time_bounds);
}

static void format_fee(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Max Fee", DETAIL_CAPTION_MAX_LENGTH);
    asset_t asset = {.type = ASSET_TYPE_NATIVE};
    FORMATTER_CHECK(print_amount(tx_ctx->tx_details.fee,
                                 &asset,
                                 tx_ctx->network,
                                 G.ui.detail_value,
                                 DETAIL_VALUE_MAX_LENGTH))
#ifdef TEST
    push_to_formatter_stack(&format_sequence);
#else
    if (HAS_SETTING(S_SEQUENCE_NUMBER_ENABLED)) {
        push_to_formatter_stack(&format_sequence);
    } else {
        push_to_formatter_stack(&format_time_bounds);
    }
#endif  // TEST
}

static void format_memo(tx_ctx_t *tx_ctx) {
    memo_t *memo = &tx_ctx->tx_details.memo;
    switch (memo->type) {
        case MEMO_ID: {
            STRLCPY(G.ui.detail_caption, "Memo ID", DETAIL_CAPTION_MAX_LENGTH);
            FORMATTER_CHECK(print_uint(memo->id, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH))
            break;
        }
        case MEMO_TEXT: {
            char tmp[DETAIL_VALUE_MAX_LENGTH];
            if (is_printable_binary(memo->text.text, memo->text.text_size)) {
                STRLCPY(G.ui.detail_caption, "Memo Text", DETAIL_CAPTION_MAX_LENGTH);
                memcpy(tmp, (char *) memo->text.text, memo->text.text_size);
                tmp[memo->text.text_size] = '\0';
                STRLCPY(G.ui.detail_value, tmp, DETAIL_VALUE_MAX_LENGTH);
            } else {
                STRLCPY(G.ui.detail_caption, "Memo Text (base64)", DETAIL_CAPTION_MAX_LENGTH);
                FORMATTER_CHECK(base64_encode(memo->text.text,
                                              memo->text.text_size,
                                              tmp,
                                              DETAIL_VALUE_MAX_LENGTH))
                FORMATTER_CHECK(
                    print_summary(tmp, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH, 6, 6))
            }
            break;
        }
        case MEMO_HASH: {
            STRLCPY(G.ui.detail_caption, "Memo Hash", DETAIL_CAPTION_MAX_LENGTH);
            FORMATTER_CHECK(print_binary(memo->hash,
                                         HASH_SIZE,
                                         G.ui.detail_value,
                                         DETAIL_VALUE_MAX_LENGTH,
                                         0,
                                         0))
            break;
        }
        case MEMO_RETURN: {
            STRLCPY(G.ui.detail_caption, "Memo Return", DETAIL_CAPTION_MAX_LENGTH);
            FORMATTER_CHECK(print_binary(memo->hash,
                                         HASH_SIZE,
                                         G.ui.detail_value,
                                         DETAIL_VALUE_MAX_LENGTH,
                                         0,
                                         0))
            break;
        }
        default:
            THROW(SW_TX_FORMATTING_FAIL);
            return;
    }
    push_to_formatter_stack(&format_fee);
}

static void format_transaction_details(tx_ctx_t *tx_ctx) {
    switch (tx_ctx->envelope_type) {
        case ENVELOPE_TYPE_TX_FEE_BUMP:
            STRLCPY(G.ui.detail_caption, "InnerTx", DETAIL_CAPTION_MAX_LENGTH);
            break;
        case ENVELOPE_TYPE_TX:
            STRLCPY(G.ui.detail_caption, "Transaction", DETAIL_CAPTION_MAX_LENGTH);
            break;
        default:
            THROW(SW_TX_FORMATTING_FAIL);
            return;
    }
    STRLCPY(G.ui.detail_value, "Details", DETAIL_VALUE_MAX_LENGTH);
    if (tx_ctx->tx_details.memo.type != MEMO_NONE) {
        push_to_formatter_stack(&format_memo);
    } else {
        push_to_formatter_stack(&format_fee);
    }
}

static void format_operation_source(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Op Source", DETAIL_CAPTION_MAX_LENGTH);
    if (tx_ctx->envelope_type == ENVELOPE_TYPE_TX &&
        tx_ctx->tx_details.source_account.type == KEY_TYPE_ED25519 &&
        tx_ctx->tx_details.op_details.source_account.type == KEY_TYPE_ED25519 &&
        memcmp(tx_ctx->tx_details.source_account.ed25519,
               G_context.raw_public_key,
               RAW_ED25519_PUBLIC_KEY_SIZE) == 0 &&
        memcmp(tx_ctx->tx_details.op_details.source_account.ed25519,
               G_context.raw_public_key,
               RAW_ED25519_PUBLIC_KEY_SIZE) == 0) {
        FORMATTER_CHECK(print_muxed_account(&tx_ctx->tx_details.op_details.source_account,
                                            G.ui.detail_value,
                                            DETAIL_VALUE_MAX_LENGTH,
                                            6,
                                            6))
    } else {
        FORMATTER_CHECK(print_muxed_account(&tx_ctx->tx_details.op_details.source_account,
                                            G.ui.detail_value,
                                            DETAIL_VALUE_MAX_LENGTH,
                                            0,
                                            0))
    }

    if (tx_ctx->tx_details.operation_index == tx_ctx->tx_details.operations_count) {
        // last operation
        push_to_formatter_stack(NULL);
    } else {
        // more operations
        push_to_formatter_stack(&format_next_step);
    }
}

static void format_operation_source_prepare(tx_ctx_t *tx_ctx) {
    if (tx_ctx->tx_details.op_details.source_account_present) {
        // If the source exists, when the user clicks the next button,
        // it will jump to the page showing the source
        push_to_formatter_stack(&format_operation_source);
    } else {
        // If not, jump to the signing page or show the next operation.
        if (tx_ctx->tx_details.operation_index == tx_ctx->tx_details.operations_count) {
            // last operation
            push_to_formatter_stack(NULL);
        } else {
            // more operations
            push_to_formatter_stack(&format_next_step);
        }
    }
}

static void format_bump_sequence_bump_to(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Bump To", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_int(tx_ctx->tx_details.op_details.bump_sequence_op.bump_to,
                              G.ui.detail_value,
                              DETAIL_VALUE_MAX_LENGTH))
    format_operation_source_prepare(tx_ctx);
}

static void format_bump_sequence(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Bump Sequence", DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_bump_sequence_bump_to);
}

static void format_inflation(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Inflation", DETAIL_VALUE_MAX_LENGTH);
    format_operation_source_prepare(tx_ctx);
}

static void format_account_merge_destination(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Destination", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_muxed_account(&tx_ctx->tx_details.op_details.account_merge_op.destination,
                                        G.ui.detail_value,
                                        DETAIL_VALUE_MAX_LENGTH,
                                        0,
                                        0))
    format_operation_source_prepare(tx_ctx);
}

static void format_account_merge_detail(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Merge Account", DETAIL_CAPTION_MAX_LENGTH);
    if (tx_ctx->tx_details.op_details.source_account_present) {
        FORMATTER_CHECK(print_muxed_account(&tx_ctx->tx_details.op_details.source_account,
                                            G.ui.detail_value,
                                            DETAIL_VALUE_MAX_LENGTH,
                                            0,
                                            0))
    } else {
        FORMATTER_CHECK(print_muxed_account(&tx_ctx->tx_details.source_account,
                                            G.ui.detail_value,
                                            DETAIL_VALUE_MAX_LENGTH,
                                            0,
                                            0))
    }
    push_to_formatter_stack(&format_account_merge_destination);
}

static void format_account_merge(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Account Merge", DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_account_merge_detail);
}

static void format_manage_data_value(tx_ctx_t *tx_ctx) {
    char tmp[DETAIL_VALUE_MAX_LENGTH];
    if (is_printable_binary(tx_ctx->tx_details.op_details.manage_data_op.data_value,
                            tx_ctx->tx_details.op_details.manage_data_op.data_value_size)) {
        STRLCPY(G.ui.detail_caption, "Data Value", DETAIL_CAPTION_MAX_LENGTH);
        memcpy(tmp,
               (char *) tx_ctx->tx_details.op_details.manage_data_op.data_value,
               tx_ctx->tx_details.op_details.manage_data_op.data_value_size);
        tmp[tx_ctx->tx_details.op_details.manage_data_op.data_value_size] = '\0';
        STRLCPY(G.ui.detail_value, tmp, DETAIL_VALUE_MAX_LENGTH);
    } else {
        STRLCPY(G.ui.detail_caption, "Data Value (base64)", DETAIL_CAPTION_MAX_LENGTH);
        FORMATTER_CHECK(base64_encode(tx_ctx->tx_details.op_details.manage_data_op.data_value,
                                      tx_ctx->tx_details.op_details.manage_data_op.data_value_size,
                                      tmp,
                                      sizeof(tmp)))
        FORMATTER_CHECK(print_summary(tmp, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH, 6, 6))
    }
    format_operation_source_prepare(tx_ctx);
}

static void format_manage_data(tx_ctx_t *tx_ctx) {
    if (tx_ctx->tx_details.op_details.manage_data_op.data_value_size) {
        STRLCPY(G.ui.detail_caption, "Set Data", DETAIL_CAPTION_MAX_LENGTH);
        push_to_formatter_stack(&format_manage_data_value);
    } else {
        STRLCPY(G.ui.detail_caption, "Remove Data", DETAIL_CAPTION_MAX_LENGTH);
        format_operation_source_prepare(tx_ctx);
    }
    char tmp[65];
    memcpy(tmp,
           tx_ctx->tx_details.op_details.manage_data_op.data_name,
           tx_ctx->tx_details.op_details.manage_data_op.data_name_size);
    tmp[tx_ctx->tx_details.op_details.manage_data_op.data_name_size] = '\0';
    STRLCPY(G.ui.detail_value, tmp, DETAIL_VALUE_MAX_LENGTH);
}

static void format_allow_trust_authorize(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Authorize Flag", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_allow_trust_flags(tx_ctx->tx_details.op_details.allow_trust_op.authorize,
                                            G.ui.detail_value,
                                            DETAIL_VALUE_MAX_LENGTH))
    format_operation_source_prepare(tx_ctx);
}

static void format_allow_trust_asset_code(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Asset Code", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value,
            tx_ctx->tx_details.op_details.allow_trust_op.asset_code,
            DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_allow_trust_authorize);
}

static void format_allow_trust_trustor(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Trustor", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_account_id(tx_ctx->tx_details.op_details.allow_trust_op.trustor,
                                     G.ui.detail_value,
                                     DETAIL_VALUE_MAX_LENGTH,
                                     0,
                                     0))
    push_to_formatter_stack(&format_allow_trust_asset_code);
}

static void format_allow_trust(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Allow Trust", DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_allow_trust_trustor);
}

static void format_set_option_signer_weight(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Weight", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_uint(tx_ctx->tx_details.op_details.set_options_op.signer.weight,
                               G.ui.detail_value,
                               DETAIL_VALUE_MAX_LENGTH))
    format_operation_source_prepare(tx_ctx);
}

static void format_set_option_signer_detail(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Signer Key", DETAIL_CAPTION_MAX_LENGTH);
    signer_key_t *key = &tx_ctx->tx_details.op_details.set_options_op.signer.key;

    switch (key->type) {
        case SIGNER_KEY_TYPE_ED25519: {
            FORMATTER_CHECK(
                print_account_id(key->ed25519, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH, 0, 0))
            break;
        }
        case SIGNER_KEY_TYPE_HASH_X: {
            FORMATTER_CHECK(
                print_hash_x_key(key->hash_x, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH, 0, 0))
            break;
        }

        case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
            FORMATTER_CHECK(print_pre_auth_x_key(key->pre_auth_tx,
                                                 G.ui.detail_value,
                                                 DETAIL_VALUE_MAX_LENGTH,
                                                 0,
                                                 0))
            break;
        }
        case SIGNER_KEY_TYPE_ED25519_SIGNED_PAYLOAD: {
            FORMATTER_CHECK(print_ed25519_signed_payload(&key->ed25519_signed_payload,
                                                         G.ui.detail_value,
                                                         DETAIL_VALUE_MAX_LENGTH,
                                                         12,
                                                         12))
            break;
        }
        default:
            THROW(SW_TX_FORMATTING_FAIL);
            return;
    }
    if (tx_ctx->tx_details.op_details.set_options_op.signer.weight != 0) {
        push_to_formatter_stack(&format_set_option_signer_weight);
    } else {
        format_operation_source_prepare(tx_ctx);
    }
}

static void format_set_option_signer(tx_ctx_t *tx_ctx) {
    signer_t *signer = &tx_ctx->tx_details.op_details.set_options_op.signer;
    if (signer->weight) {
        STRLCPY(G.ui.detail_caption, "Add Signer", DETAIL_CAPTION_MAX_LENGTH);
    } else {
        STRLCPY(G.ui.detail_caption, "Remove Signer", DETAIL_CAPTION_MAX_LENGTH);
    }
    switch (signer->key.type) {
        case SIGNER_KEY_TYPE_ED25519: {
            STRLCPY(G.ui.detail_value, "Type Public Key", DETAIL_VALUE_MAX_LENGTH);
            break;
        }
        case SIGNER_KEY_TYPE_HASH_X: {
            STRLCPY(G.ui.detail_value, "Type Hash(x)", DETAIL_VALUE_MAX_LENGTH);
            break;
        }
        case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
            STRLCPY(G.ui.detail_value, "Type Pre-Auth", DETAIL_VALUE_MAX_LENGTH);
            break;
        }
        case SIGNER_KEY_TYPE_ED25519_SIGNED_PAYLOAD: {
            STRLCPY(G.ui.detail_value, "Type Ed25519 Signed Payload", DETAIL_VALUE_MAX_LENGTH);
            break;
        }
        default:
            THROW(SW_TX_FORMATTING_FAIL);
            return;
    }
    push_to_formatter_stack(&format_set_option_signer_detail);
}

static void format_set_option_signer_prepare(tx_ctx_t *tx_ctx) {
    if (tx_ctx->tx_details.op_details.set_options_op.signer_present) {
        push_to_formatter_stack(&format_set_option_signer);
    } else {
        format_operation_source_prepare(tx_ctx);
    }
}

static void format_set_option_home_domain(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Home Domain", DETAIL_CAPTION_MAX_LENGTH);
    if (tx_ctx->tx_details.op_details.set_options_op.home_domain_size) {
        memcpy(G.ui.detail_value,
               tx_ctx->tx_details.op_details.set_options_op.home_domain,
               tx_ctx->tx_details.op_details.set_options_op.home_domain_size);
        G.ui.detail_value[tx_ctx->tx_details.op_details.set_options_op.home_domain_size] = '\0';
    } else {
        STRLCPY(G.ui.detail_value, "[remove home domain from account]", DETAIL_VALUE_MAX_LENGTH);
    }
    format_set_option_signer_prepare(tx_ctx);
}

static void format_set_option_home_domain_prepare(tx_ctx_t *tx_ctx) {
    if (tx_ctx->tx_details.op_details.set_options_op.home_domain_present) {
        push_to_formatter_stack(&format_set_option_home_domain);
    } else {
        format_set_option_signer_prepare(tx_ctx);
    }
}

static void format_set_option_high_threshold(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "High Threshold", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_uint(tx_ctx->tx_details.op_details.set_options_op.high_threshold,
                               G.ui.detail_value,
                               DETAIL_VALUE_MAX_LENGTH))
    format_set_option_home_domain_prepare(tx_ctx);
}

static void format_set_option_high_threshold_prepare(tx_ctx_t *tx_ctx) {
    if (tx_ctx->tx_details.op_details.set_options_op.high_threshold_present) {
        push_to_formatter_stack(&format_set_option_high_threshold);
    } else {
        format_set_option_home_domain_prepare(tx_ctx);
    }
}

static void format_set_option_medium_threshold(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Medium Threshold", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_uint(tx_ctx->tx_details.op_details.set_options_op.medium_threshold,
                               G.ui.detail_value,
                               DETAIL_VALUE_MAX_LENGTH))
    format_set_option_high_threshold_prepare(tx_ctx);
}

static void format_set_option_medium_threshold_prepare(tx_ctx_t *tx_ctx) {
    if (tx_ctx->tx_details.op_details.set_options_op.medium_threshold_present) {
        push_to_formatter_stack(&format_set_option_medium_threshold);
    } else {
        format_set_option_high_threshold_prepare(tx_ctx);
    }
}

static void format_set_option_low_threshold(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Low Threshold", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_uint(tx_ctx->tx_details.op_details.set_options_op.low_threshold,
                               G.ui.detail_value,
                               DETAIL_VALUE_MAX_LENGTH))
    format_set_option_medium_threshold_prepare(tx_ctx);
}

static void format_set_option_low_threshold_prepare(tx_ctx_t *tx_ctx) {
    if (tx_ctx->tx_details.op_details.set_options_op.low_threshold_present) {
        push_to_formatter_stack(&format_set_option_low_threshold);
    } else {
        format_set_option_medium_threshold_prepare(tx_ctx);
    }
}

static void format_set_option_master_weight(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Master Weight", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_uint(tx_ctx->tx_details.op_details.set_options_op.master_weight,
                               G.ui.detail_value,
                               DETAIL_VALUE_MAX_LENGTH))
    format_set_option_low_threshold_prepare(tx_ctx);
}

static void format_set_option_master_weight_prepare(tx_ctx_t *tx_ctx) {
    if (tx_ctx->tx_details.op_details.set_options_op.master_weight_present) {
        push_to_formatter_stack(&format_set_option_master_weight);
    } else {
        format_set_option_low_threshold_prepare(tx_ctx);
    }
}

static void format_set_option_set_flags(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Set Flags", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_account_flags(tx_ctx->tx_details.op_details.set_options_op.set_flags,
                                        G.ui.detail_value,
                                        DETAIL_VALUE_MAX_LENGTH))
    format_set_option_master_weight_prepare(tx_ctx);
}

static void format_set_option_set_flags_prepare(tx_ctx_t *tx_ctx) {
    if (tx_ctx->tx_details.op_details.set_options_op.set_flags_present) {
        push_to_formatter_stack(&format_set_option_set_flags);
    } else {
        format_set_option_master_weight_prepare(tx_ctx);
    }
}

static void format_set_option_clear_flags(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Clear Flags", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_account_flags(tx_ctx->tx_details.op_details.set_options_op.clear_flags,
                                        G.ui.detail_value,
                                        DETAIL_VALUE_MAX_LENGTH))
    format_set_option_set_flags_prepare(tx_ctx);
}

static void format_set_option_clear_flags_prepare(tx_ctx_t *tx_ctx) {
    if (tx_ctx->tx_details.op_details.set_options_op.clear_flags_present) {
        push_to_formatter_stack(&format_set_option_clear_flags);
    } else {
        format_set_option_set_flags_prepare(tx_ctx);
    }
}

static void format_set_option_inflation_destination(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Inflation Dest", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_account_id(tx_ctx->tx_details.op_details.set_options_op.inflation_destination,
                         G.ui.detail_value,
                         DETAIL_VALUE_MAX_LENGTH,
                         0,
                         0))
    format_set_option_clear_flags_prepare(tx_ctx);
}

static void format_set_option_inflation_destination_prepare(tx_ctx_t *tx_ctx) {
    if (tx_ctx->tx_details.op_details.set_options_op.inflation_destination_present) {
        push_to_formatter_stack(format_set_option_inflation_destination);
    } else {
        format_set_option_clear_flags_prepare(tx_ctx);
    }
}

static void format_set_options_empty_body(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "SET OPTIONS", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "BODY IS EMPTY", DETAIL_VALUE_MAX_LENGTH);
    format_operation_source_prepare(tx_ctx);
}

static bool is_empty_set_options_body(tx_ctx_t *tx_ctx) {
    return !(tx_ctx->tx_details.op_details.set_options_op.inflation_destination_present ||
             tx_ctx->tx_details.op_details.set_options_op.clear_flags_present ||
             tx_ctx->tx_details.op_details.set_options_op.set_flags_present ||
             tx_ctx->tx_details.op_details.set_options_op.master_weight_present ||
             tx_ctx->tx_details.op_details.set_options_op.low_threshold_present ||
             tx_ctx->tx_details.op_details.set_options_op.medium_threshold_present ||
             tx_ctx->tx_details.op_details.set_options_op.high_threshold_present ||
             tx_ctx->tx_details.op_details.set_options_op.home_domain_present ||
             tx_ctx->tx_details.op_details.set_options_op.signer_present);
}

static void format_set_options(tx_ctx_t *tx_ctx) {
    // this operation is a special one among all operations, because all its fields are optional.
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Set Options", DETAIL_VALUE_MAX_LENGTH);
    if (is_empty_set_options_body(tx_ctx)) {
        push_to_formatter_stack(format_set_options_empty_body);
    } else {
        format_set_option_inflation_destination_prepare(tx_ctx);
    }
}

static void format_change_trust_limit(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Trust Limit", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_amount(tx_ctx->tx_details.op_details.change_trust_op.limit,
                                 NULL,
                                 tx_ctx->network,
                                 G.ui.detail_value,
                                 DETAIL_VALUE_MAX_LENGTH))
    format_operation_source_prepare(tx_ctx);
}

static void format_change_trust_detail_liquidity_pool_fee(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Pool Fee Rate", DETAIL_CAPTION_MAX_LENGTH);
    uint64_t fee = ((uint64_t) tx_ctx->tx_details.op_details.change_trust_op.line.liquidity_pool
                        .constant_product.fee *
                    10000000) /
                   100;
    FORMATTER_CHECK(
        print_amount(fee, NULL, tx_ctx->network, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH))
    STRLCAT(G.ui.detail_value, "%", DETAIL_VALUE_MAX_LENGTH);
    if (tx_ctx->tx_details.op_details.change_trust_op.limit &&
        tx_ctx->tx_details.op_details.change_trust_op.limit != INT64_MAX) {
        push_to_formatter_stack(&format_change_trust_limit);
    } else {
        format_operation_source_prepare(tx_ctx);
    }
}

static void format_change_trust_detail_liquidity_pool_asset_b(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Asset B", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_asset(
        &tx_ctx->tx_details.op_details.change_trust_op.line.liquidity_pool.constant_product.asset_b,
        tx_ctx->network,
        G.ui.detail_value,
        DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_change_trust_detail_liquidity_pool_fee);
}

static void format_change_trust_detail_liquidity_pool_asset_a(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Asset A", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_asset(
        &tx_ctx->tx_details.op_details.change_trust_op.line.liquidity_pool.constant_product.asset_a,
        tx_ctx->network,
        G.ui.detail_value,
        DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_change_trust_detail_liquidity_pool_asset_b);
}

static void format_change_trust(tx_ctx_t *tx_ctx) {
    if (tx_ctx->tx_details.op_details.change_trust_op.limit) {
        STRLCPY(G.ui.detail_caption, "Change Trust", DETAIL_CAPTION_MAX_LENGTH);
    } else {
        STRLCPY(G.ui.detail_caption, "Remove Trust", DETAIL_CAPTION_MAX_LENGTH);
    }
    uint8_t asset_type = tx_ctx->tx_details.op_details.change_trust_op.line.type;
    switch (asset_type) {
        case ASSET_TYPE_CREDIT_ALPHANUM4:
        case ASSET_TYPE_CREDIT_ALPHANUM12:
            FORMATTER_CHECK(
                print_asset((asset_t *) &tx_ctx->tx_details.op_details.change_trust_op.line,
                            tx_ctx->network,
                            G.ui.detail_value,
                            DETAIL_VALUE_MAX_LENGTH))
            if (tx_ctx->tx_details.op_details.change_trust_op.limit &&
                tx_ctx->tx_details.op_details.change_trust_op.limit != INT64_MAX) {
                push_to_formatter_stack(&format_change_trust_limit);
            } else {
                format_operation_source_prepare(tx_ctx);
            }
            break;
        case ASSET_TYPE_POOL_SHARE:
            STRLCPY(G.ui.detail_value, "Liquidity Pool Asset", DETAIL_VALUE_MAX_LENGTH);
            push_to_formatter_stack(&format_change_trust_detail_liquidity_pool_asset_a);
            break;
        default:
            THROW(SW_TX_FORMATTING_FAIL);
            return;
    }
}

static void format_manage_sell_offer_price(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Price", DETAIL_CAPTION_MAX_LENGTH);
    uint64_t price =
        ((uint64_t) tx_ctx->tx_details.op_details.manage_sell_offer_op.price.n * 10000000) /
        tx_ctx->tx_details.op_details.manage_sell_offer_op.price.d;
    FORMATTER_CHECK(
        print_amount(price, NULL, tx_ctx->network, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH))
    STRLCAT(G.ui.detail_value, " ", DETAIL_VALUE_MAX_LENGTH);
    char tmp_asset_code[13] = {0};
    FORMATTER_CHECK(print_asset_name(&tx_ctx->tx_details.op_details.manage_sell_offer_op.buying,
                                     tx_ctx->network,
                                     tmp_asset_code,
                                     sizeof(tmp_asset_code)))
    STRLCAT(G.ui.detail_value, tmp_asset_code, DETAIL_VALUE_MAX_LENGTH);
    STRLCAT(G.ui.detail_value, "/", DETAIL_VALUE_MAX_LENGTH);
    FORMATTER_CHECK(print_asset_name(&tx_ctx->tx_details.op_details.manage_sell_offer_op.selling,
                                     tx_ctx->network,
                                     tmp_asset_code,
                                     sizeof(tmp_asset_code)))
    STRLCAT(G.ui.detail_value, tmp_asset_code, DETAIL_VALUE_MAX_LENGTH);
    format_operation_source_prepare(tx_ctx);
}

static void format_manage_sell_offer_sell(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Sell", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_amount(tx_ctx->tx_details.op_details.manage_sell_offer_op.amount,
                                 &tx_ctx->tx_details.op_details.manage_sell_offer_op.selling,
                                 tx_ctx->network,
                                 G.ui.detail_value,
                                 DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_manage_sell_offer_price);
}

static void format_manage_sell_offer_buy(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Buy", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_asset(&tx_ctx->tx_details.op_details.manage_sell_offer_op.buying,
                                tx_ctx->network,
                                G.ui.detail_value,
                                DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_manage_sell_offer_sell);
}

static void format_manage_sell_offer(tx_ctx_t *tx_ctx) {
    if (!tx_ctx->tx_details.op_details.manage_sell_offer_op.amount) {
        STRLCPY(G.ui.detail_caption, "Remove Offer", DETAIL_CAPTION_MAX_LENGTH);
        FORMATTER_CHECK(print_uint(tx_ctx->tx_details.op_details.manage_sell_offer_op.offer_id,
                                   G.ui.detail_value,
                                   DETAIL_VALUE_MAX_LENGTH))
        format_operation_source_prepare(tx_ctx);
    } else {
        if (tx_ctx->tx_details.op_details.manage_sell_offer_op.offer_id) {
            STRLCPY(G.ui.detail_caption, "Change Offer", DETAIL_CAPTION_MAX_LENGTH);
            FORMATTER_CHECK(print_uint(tx_ctx->tx_details.op_details.manage_sell_offer_op.offer_id,
                                       G.ui.detail_value,
                                       DETAIL_VALUE_MAX_LENGTH))
        } else {
            STRLCPY(G.ui.detail_caption, "Create Offer", DETAIL_CAPTION_MAX_LENGTH);
            STRLCPY(G.ui.detail_value, "Type Active", DETAIL_VALUE_MAX_LENGTH);
        }
        push_to_formatter_stack(&format_manage_sell_offer_buy);
    }
}

static void format_manage_buy_offer_price(tx_ctx_t *tx_ctx) {
    manage_buy_offer_op_t *op = &tx_ctx->tx_details.op_details.manage_buy_offer_op;

    STRLCPY(G.ui.detail_caption, "Price", DETAIL_CAPTION_MAX_LENGTH);
    uint64_t price = ((uint64_t) op->price.n * 10000000) / op->price.d;
    FORMATTER_CHECK(
        print_amount(price, NULL, tx_ctx->network, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH))
    STRLCAT(G.ui.detail_value, " ", DETAIL_VALUE_MAX_LENGTH);
    char tmp_asset_code[13] = {0};
    FORMATTER_CHECK(print_asset_name(&tx_ctx->tx_details.op_details.manage_buy_offer_op.selling,
                                     tx_ctx->network,
                                     tmp_asset_code,
                                     sizeof(tmp_asset_code)))
    STRLCAT(G.ui.detail_value, tmp_asset_code, DETAIL_VALUE_MAX_LENGTH);
    STRLCAT(G.ui.detail_value, "/", DETAIL_VALUE_MAX_LENGTH);
    FORMATTER_CHECK(print_asset_name(&tx_ctx->tx_details.op_details.manage_buy_offer_op.buying,
                                     tx_ctx->network,
                                     tmp_asset_code,
                                     sizeof(tmp_asset_code)))
    STRLCAT(G.ui.detail_value, tmp_asset_code, DETAIL_VALUE_MAX_LENGTH);
    format_operation_source_prepare(tx_ctx);
}

static void format_manage_buy_offer_buy(tx_ctx_t *tx_ctx) {
    manage_buy_offer_op_t *op = &tx_ctx->tx_details.op_details.manage_buy_offer_op;

    STRLCPY(G.ui.detail_caption, "Buy", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_amount(op->buy_amount,
                                 &op->buying,
                                 tx_ctx->network,
                                 G.ui.detail_value,
                                 DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_manage_buy_offer_price);
}

static void format_manage_buy_offer_sell(tx_ctx_t *tx_ctx) {
    manage_buy_offer_op_t *op = &tx_ctx->tx_details.op_details.manage_buy_offer_op;

    STRLCPY(G.ui.detail_caption, "Sell", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_asset(&op->selling, tx_ctx->network, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_manage_buy_offer_buy);
}

static void format_manage_buy_offer(tx_ctx_t *tx_ctx) {
    manage_buy_offer_op_t *op = &tx_ctx->tx_details.op_details.manage_buy_offer_op;

    if (op->buy_amount == 0) {
        STRLCPY(G.ui.detail_caption, "Remove Offer", DETAIL_CAPTION_MAX_LENGTH);
        FORMATTER_CHECK(print_uint(op->offer_id, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH))
        format_operation_source_prepare(tx_ctx);
    } else {
        if (op->offer_id) {
            STRLCPY(G.ui.detail_caption, "Change Offer", DETAIL_CAPTION_MAX_LENGTH);
            FORMATTER_CHECK(print_uint(op->offer_id, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH))
        } else {
            STRLCPY(G.ui.detail_caption, "Create Offer", DETAIL_CAPTION_MAX_LENGTH);
            STRLCPY(G.ui.detail_value, "Type Active", DETAIL_VALUE_MAX_LENGTH);
        }
        push_to_formatter_stack(&format_manage_buy_offer_sell);
    }
}

static void format_create_passive_sell_offer_price(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Price", DETAIL_CAPTION_MAX_LENGTH);

    create_passive_sell_offer_op_t *op =
        &tx_ctx->tx_details.op_details.create_passive_sell_offer_op;
    uint64_t price = ((uint64_t) op->price.n * 10000000) / op->price.d;
    FORMATTER_CHECK(
        print_amount(price, NULL, tx_ctx->network, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH))
    STRLCAT(G.ui.detail_value, " ", DETAIL_VALUE_MAX_LENGTH);
    char tmp_asset_code[13] = {0};
    FORMATTER_CHECK(
        print_asset_name(&tx_ctx->tx_details.op_details.create_passive_sell_offer_op.buying,
                         tx_ctx->network,
                         tmp_asset_code,
                         sizeof(tmp_asset_code)))
    STRLCAT(G.ui.detail_value, tmp_asset_code, DETAIL_VALUE_MAX_LENGTH);
    STRLCAT(G.ui.detail_value, "/", DETAIL_VALUE_MAX_LENGTH);
    FORMATTER_CHECK(
        print_asset_name(&tx_ctx->tx_details.op_details.create_passive_sell_offer_op.selling,
                         tx_ctx->network,
                         tmp_asset_code,
                         sizeof(tmp_asset_code)))
    STRLCAT(G.ui.detail_value, tmp_asset_code, DETAIL_VALUE_MAX_LENGTH);
    format_operation_source_prepare(tx_ctx);
}

static void format_create_passive_sell_offer_sell(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Sell", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_amount(tx_ctx->tx_details.op_details.create_passive_sell_offer_op.amount,
                     &tx_ctx->tx_details.op_details.create_passive_sell_offer_op.selling,
                     tx_ctx->network,
                     G.ui.detail_value,
                     DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_create_passive_sell_offer_price);
}

static void format_create_passive_sell_offer_buy(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Buy", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_asset(&tx_ctx->tx_details.op_details.create_passive_sell_offer_op.buying,
                                tx_ctx->network,
                                G.ui.detail_value,
                                DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_create_passive_sell_offer_sell);
}

static void format_create_passive_sell_offer(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Create Passive Sell Offer", DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_create_passive_sell_offer_buy);
}

static void format_path_payment_strict_receive_receive(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Receive", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_amount(tx_ctx->tx_details.op_details.path_payment_strict_receive_op.dest_amount,
                     &tx_ctx->tx_details.op_details.path_payment_strict_receive_op.dest_asset,
                     tx_ctx->network,
                     G.ui.detail_value,
                     DETAIL_VALUE_MAX_LENGTH))
    format_operation_source_prepare(tx_ctx);
}

static void format_path_payment_strict_receive_destination(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Destination", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_muxed_account(
        &tx_ctx->tx_details.op_details.path_payment_strict_receive_op.destination,
        G.ui.detail_value,
        DETAIL_VALUE_MAX_LENGTH,
        0,
        0))
    push_to_formatter_stack(&format_path_payment_strict_receive_receive);
}

static void format_path_payment_strict_receive(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Send Max", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_amount(tx_ctx->tx_details.op_details.path_payment_strict_receive_op.send_max,
                     &tx_ctx->tx_details.op_details.path_payment_strict_receive_op.send_asset,
                     tx_ctx->network,
                     G.ui.detail_value,
                     DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_path_payment_strict_receive_destination);
}

static void format_path_payment_strict_send_receive(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Receive Min", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_amount(tx_ctx->tx_details.op_details.path_payment_strict_send_op.dest_min,
                     &tx_ctx->tx_details.op_details.path_payment_strict_send_op.dest_asset,
                     tx_ctx->network,
                     G.ui.detail_value,
                     DETAIL_VALUE_MAX_LENGTH))
    format_operation_source_prepare(tx_ctx);
}

static void format_path_payment_strict_send_destination(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Destination", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_muxed_account(&tx_ctx->tx_details.op_details.path_payment_strict_send_op.destination,
                            G.ui.detail_value,
                            DETAIL_VALUE_MAX_LENGTH,
                            0,
                            0))
    push_to_formatter_stack(&format_path_payment_strict_send_receive);
}

static void format_path_payment_strict_send(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Send", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_amount(tx_ctx->tx_details.op_details.path_payment_strict_send_op.send_amount,
                     &tx_ctx->tx_details.op_details.path_payment_strict_send_op.send_asset,
                     tx_ctx->network,
                     G.ui.detail_value,
                     DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_path_payment_strict_send_destination);
}

static void format_payment_destination(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Destination", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_muxed_account(&tx_ctx->tx_details.op_details.payment_op.destination,
                                        G.ui.detail_value,
                                        DETAIL_VALUE_MAX_LENGTH,
                                        0,
                                        0))
    format_operation_source_prepare(tx_ctx);
}

static void format_payment(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Send", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_amount(tx_ctx->tx_details.op_details.payment_op.amount,
                                 &tx_ctx->tx_details.op_details.payment_op.asset,
                                 tx_ctx->network,
                                 G.ui.detail_value,
                                 DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_payment_destination);
}

static void format_create_account_amount(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Starting Balance", DETAIL_CAPTION_MAX_LENGTH);
    asset_t asset = {.type = ASSET_TYPE_NATIVE};
    FORMATTER_CHECK(print_amount(tx_ctx->tx_details.op_details.create_account_op.starting_balance,
                                 &asset,
                                 tx_ctx->network,
                                 G.ui.detail_value,
                                 DETAIL_VALUE_MAX_LENGTH))
    format_operation_source_prepare(tx_ctx);
}

static void format_create_account_destination(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Destination", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_account_id(tx_ctx->tx_details.op_details.create_account_op.destination,
                                     G.ui.detail_value,
                                     DETAIL_VALUE_MAX_LENGTH,
                                     0,
                                     0))
    push_to_formatter_stack(&format_create_account_amount);
}

static void format_create_account(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Create Account", DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_create_account_destination);
}

void format_create_claimable_balance_warning(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    // TODO: The claimant can be very complicated. I haven't figured out how to
    // display it for the time being, so let's display an WARNING here first.
    STRLCPY(G.ui.detail_caption, "WARNING", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value,
            "Currently does not support displaying claimant details",
            DETAIL_VALUE_MAX_LENGTH);
    format_operation_source_prepare(tx_ctx);
}

static void format_create_claimable_balance_balance(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Balance", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_amount(tx_ctx->tx_details.op_details.create_claimable_balance_op.amount,
                                 &tx_ctx->tx_details.op_details.create_claimable_balance_op.asset,
                                 tx_ctx->network,
                                 G.ui.detail_value,
                                 DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_create_claimable_balance_warning);
}

static void format_create_claimable_balance(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Create Claimable Balance", DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_create_claimable_balance_balance);
}

static void format_claim_claimable_balance_balance_id(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Balance ID", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_claimable_balance_id(
        &tx_ctx->tx_details.op_details.claim_claimable_balance_op.balance_id,
        G.ui.detail_value,
        DETAIL_VALUE_MAX_LENGTH,
        12,
        12))
    format_operation_source_prepare(tx_ctx);
}

static void format_claim_claimable_balance(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Claim Claimable Balance", DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_claim_claimable_balance_balance_id);
}

static void format_claim_claimable_balance_sponsored_id(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Sponsored ID", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_account_id(
        tx_ctx->tx_details.op_details.begin_sponsoring_future_reserves_op.sponsored_id,
        G.ui.detail_value,
        DETAIL_VALUE_MAX_LENGTH,
        0,
        0))
    format_operation_source_prepare(tx_ctx);
}

static void format_begin_sponsoring_future_reserves(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Begin Sponsoring Future Reserves", DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_claim_claimable_balance_sponsored_id);
}

static void format_end_sponsoring_future_reserves(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "End Sponsoring Future Reserves", DETAIL_VALUE_MAX_LENGTH);
    format_operation_source_prepare(tx_ctx);
}

static void format_revoke_sponsorship_account(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Account ID", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_account_id(
        tx_ctx->tx_details.op_details.revoke_sponsorship_op.ledger_key.account.account_id,
        G.ui.detail_value,
        DETAIL_VALUE_MAX_LENGTH,
        0,
        0))
    format_operation_source_prepare(tx_ctx);
}

static void format_revoke_sponsorship_trust_line_asset(tx_ctx_t *tx_ctx) {
    if (tx_ctx->tx_details.op_details.revoke_sponsorship_op.ledger_key.trust_line.asset.type ==
        ASSET_TYPE_POOL_SHARE) {
        STRLCPY(G.ui.detail_caption, "Liquidity Pool ID", DETAIL_CAPTION_MAX_LENGTH);
        FORMATTER_CHECK(print_binary(tx_ctx->tx_details.op_details.revoke_sponsorship_op.ledger_key
                                         .trust_line.asset.liquidity_pool_id,
                                     LIQUIDITY_POOL_ID_SIZE,
                                     G.ui.detail_value,
                                     DETAIL_VALUE_MAX_LENGTH,
                                     0,
                                     0))
    } else {
        STRLCPY(G.ui.detail_caption, "Asset", DETAIL_CAPTION_MAX_LENGTH);
        FORMATTER_CHECK(print_asset((asset_t *) &tx_ctx->tx_details.op_details.revoke_sponsorship_op
                                        .ledger_key.trust_line.asset,
                                    tx_ctx->network,
                                    G.ui.detail_value,
                                    DETAIL_VALUE_MAX_LENGTH))
    }
    format_operation_source_prepare(tx_ctx);
}

static void format_revoke_sponsorship_trust_line_account(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Account ID", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_account_id(
        tx_ctx->tx_details.op_details.revoke_sponsorship_op.ledger_key.trust_line.account_id,
        G.ui.detail_value,
        DETAIL_VALUE_MAX_LENGTH,
        0,
        0))
    push_to_formatter_stack(&format_revoke_sponsorship_trust_line_asset);
}
static void format_revoke_sponsorship_offer_offer_id(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Offer ID", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_uint(tx_ctx->tx_details.op_details.revoke_sponsorship_op.ledger_key.offer.offer_id,
                   G.ui.detail_value,
                   DETAIL_VALUE_MAX_LENGTH))

    format_operation_source_prepare(tx_ctx);
}

static void format_revoke_sponsorship_offer_seller_id(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Seller ID", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_account_id(
        tx_ctx->tx_details.op_details.revoke_sponsorship_op.ledger_key.offer.seller_id,
        G.ui.detail_value,
        DETAIL_VALUE_MAX_LENGTH,
        0,
        0))
    push_to_formatter_stack(&format_revoke_sponsorship_offer_offer_id);
}

static void format_revoke_sponsorship_data_data_name(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Data Name", DETAIL_CAPTION_MAX_LENGTH);

    _Static_assert(DATA_NAME_MAX_SIZE + 1 < DETAIL_VALUE_MAX_LENGTH,
                   "DATA_NAME_MAX_SIZE must be smaller than DETAIL_VALUE_MAX_LENGTH");

    memcpy(G.ui.detail_value,
           tx_ctx->tx_details.op_details.revoke_sponsorship_op.ledger_key.data.data_name,
           tx_ctx->tx_details.op_details.revoke_sponsorship_op.ledger_key.data.data_name_size);
    G.ui.detail_value[tx_ctx->tx_details.op_details.revoke_sponsorship_op.ledger_key.data
                          .data_name_size] = '\0';
    format_operation_source_prepare(tx_ctx);
}

static void format_revoke_sponsorship_data_account(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Account ID", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_account_id(
        tx_ctx->tx_details.op_details.revoke_sponsorship_op.ledger_key.data.account_id,
        G.ui.detail_value,
        DETAIL_VALUE_MAX_LENGTH,
        0,
        0))
    push_to_formatter_stack(&format_revoke_sponsorship_data_data_name);
}

static void format_revoke_sponsorship_claimable_balance(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Balance ID", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_claimable_balance_id(&tx_ctx->tx_details.op_details.revoke_sponsorship_op
                                                    .ledger_key.claimable_balance.balance_id,
                                               G.ui.detail_value,
                                               DETAIL_VALUE_MAX_LENGTH,
                                               0,
                                               0))
    format_operation_source_prepare(tx_ctx);
}

static void format_revoke_sponsorship_liquidity_pool(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Liquidity Pool ID", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_binary(tx_ctx->tx_details.op_details.revoke_sponsorship_op.ledger_key
                                     .liquidity_pool.liquidity_pool_id,
                                 LIQUIDITY_POOL_ID_SIZE,
                                 G.ui.detail_value,
                                 DETAIL_VALUE_MAX_LENGTH,
                                 0,
                                 0))
    format_operation_source_prepare(tx_ctx);
}

static void format_revoke_sponsorship_claimable_signer_signer_key_detail(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Signer Key", DETAIL_CAPTION_MAX_LENGTH);
    signer_key_t *key = &tx_ctx->tx_details.op_details.revoke_sponsorship_op.signer.signer_key;

    switch (key->type) {
        case SIGNER_KEY_TYPE_ED25519: {
            FORMATTER_CHECK(
                print_account_id(key->ed25519, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH, 0, 0))
            break;
        }
        case SIGNER_KEY_TYPE_HASH_X: {
            FORMATTER_CHECK(
                print_hash_x_key(key->hash_x, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH, 0, 0))
            break;
        }
        case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
            FORMATTER_CHECK(print_pre_auth_x_key(key->pre_auth_tx,
                                                 G.ui.detail_value,
                                                 DETAIL_VALUE_MAX_LENGTH,
                                                 0,
                                                 0))
            break;
        }
        case SIGNER_KEY_TYPE_ED25519_SIGNED_PAYLOAD: {
            FORMATTER_CHECK(print_ed25519_signed_payload(&key->ed25519_signed_payload,
                                                         G.ui.detail_value,
                                                         DETAIL_VALUE_MAX_LENGTH,
                                                         12,
                                                         12))
            break;
        }
        default:
            THROW(SW_TX_FORMATTING_FAIL);
            return;
    }
    format_operation_source_prepare(tx_ctx);
}

static void format_revoke_sponsorship_claimable_signer_signer_key_type(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Signer Key Type", DETAIL_CAPTION_MAX_LENGTH);
    switch (tx_ctx->tx_details.op_details.revoke_sponsorship_op.signer.signer_key.type) {
        case SIGNER_KEY_TYPE_ED25519: {
            STRLCPY(G.ui.detail_value, "Public Key", DETAIL_VALUE_MAX_LENGTH);
            break;
        }
        case SIGNER_KEY_TYPE_HASH_X: {
            STRLCPY(G.ui.detail_value, "Hash(x)", DETAIL_VALUE_MAX_LENGTH);
            break;
        }
        case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
            STRLCPY(G.ui.detail_value, "Pre-Auth", DETAIL_VALUE_MAX_LENGTH);
            break;
        }
        case SIGNER_KEY_TYPE_ED25519_SIGNED_PAYLOAD: {
            STRLCPY(G.ui.detail_value, "Ed25519 Signed Payload", DETAIL_VALUE_MAX_LENGTH);
            break;
        }
        default:
            THROW(SW_TX_FORMATTING_FAIL);
            return;
    }

    push_to_formatter_stack(&format_revoke_sponsorship_claimable_signer_signer_key_detail);
}

static void format_revoke_sponsorship_claimable_signer_account(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Account ID", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_account_id(tx_ctx->tx_details.op_details.revoke_sponsorship_op.signer.account_id,
                         G.ui.detail_value,
                         DETAIL_VALUE_MAX_LENGTH,
                         0,
                         0))
    push_to_formatter_stack(&format_revoke_sponsorship_claimable_signer_signer_key_type);
}

static void format_revoke_sponsorship(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    if (tx_ctx->tx_details.op_details.revoke_sponsorship_op.type == REVOKE_SPONSORSHIP_SIGNER) {
        STRLCPY(G.ui.detail_value, "Revoke Sponsorship (SIGNER_KEY)", DETAIL_VALUE_MAX_LENGTH);
        push_to_formatter_stack(&format_revoke_sponsorship_claimable_signer_account);
    } else {
        switch (tx_ctx->tx_details.op_details.revoke_sponsorship_op.ledger_key.type) {
            case ACCOUNT:
                STRLCPY(G.ui.detail_value, "Revoke Sponsorship (ACCOUNT)", DETAIL_VALUE_MAX_LENGTH);
                push_to_formatter_stack(&format_revoke_sponsorship_account);
                break;
            case OFFER:
                STRLCPY(G.ui.detail_value, "Revoke Sponsorship (OFFER)", DETAIL_VALUE_MAX_LENGTH);
                push_to_formatter_stack(&format_revoke_sponsorship_offer_seller_id);
                break;
            case TRUSTLINE:
                STRLCPY(G.ui.detail_value,
                        "Revoke Sponsorship (TRUSTLINE)",
                        DETAIL_VALUE_MAX_LENGTH);
                push_to_formatter_stack(&format_revoke_sponsorship_trust_line_account);
                break;
            case DATA:
                STRLCPY(G.ui.detail_value, "Revoke Sponsorship (DATA)", DETAIL_VALUE_MAX_LENGTH);
                push_to_formatter_stack(&format_revoke_sponsorship_data_account);
                break;
            case CLAIMABLE_BALANCE:
                STRLCPY(G.ui.detail_value,
                        "Revoke Sponsorship (CLAIMABLE_BALANCE)",
                        DETAIL_VALUE_MAX_LENGTH);
                push_to_formatter_stack(&format_revoke_sponsorship_claimable_balance);
                break;
            case LIQUIDITY_POOL:
                STRLCPY(G.ui.detail_value,
                        "Revoke Sponsorship (LIQUIDITY_POOL)",
                        DETAIL_VALUE_MAX_LENGTH);
                push_to_formatter_stack(&format_revoke_sponsorship_liquidity_pool);
                break;
            default:
                THROW(SW_TX_FORMATTING_FAIL);
                break;
        }
    }
}

static void format_clawback_from(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "From", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_muxed_account(&tx_ctx->tx_details.op_details.clawback_op.from,
                                        G.ui.detail_value,
                                        DETAIL_VALUE_MAX_LENGTH,
                                        0,
                                        0))
    format_operation_source_prepare(tx_ctx);
}

static void format_clawback_amount(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Clawback Balance", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_amount(tx_ctx->tx_details.op_details.clawback_op.amount,
                                 &tx_ctx->tx_details.op_details.clawback_op.asset,
                                 tx_ctx->network,
                                 G.ui.detail_value,
                                 DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_clawback_from);
}

static void format_clawback(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Clawback", DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_clawback_amount);
}

static void format_clawback_claimable_balance_balance_id(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Balance ID", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_claimable_balance_id(
        &tx_ctx->tx_details.op_details.clawback_claimable_balance_op.balance_id,
        G.ui.detail_value,
        DETAIL_VALUE_MAX_LENGTH,
        0,
        0))
    format_operation_source_prepare(tx_ctx);
}

static void format_clawback_claimable_balance(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Clawback Claimable Balance", DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_clawback_claimable_balance_balance_id);
}

static void format_set_trust_line_set_flags(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Set Flags", DETAIL_CAPTION_MAX_LENGTH);
    if (tx_ctx->tx_details.op_details.set_trust_line_flags_op.set_flags) {
        FORMATTER_CHECK(
            print_trust_line_flags(tx_ctx->tx_details.op_details.set_trust_line_flags_op.set_flags,
                                   G.ui.detail_value,
                                   DETAIL_VALUE_MAX_LENGTH))
    } else {
        STRLCPY(G.ui.detail_value, "[none]", DETAIL_VALUE_MAX_LENGTH);
    }
    format_operation_source_prepare(tx_ctx);
}

static void format_set_trust_line_clear_flags(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Clear Flags", DETAIL_CAPTION_MAX_LENGTH);
    if (tx_ctx->tx_details.op_details.set_trust_line_flags_op.clear_flags) {
        FORMATTER_CHECK(print_trust_line_flags(
            tx_ctx->tx_details.op_details.set_trust_line_flags_op.clear_flags,
            G.ui.detail_value,
            DETAIL_VALUE_MAX_LENGTH))
    } else {
        STRLCPY(G.ui.detail_value, "[none]", DETAIL_VALUE_MAX_LENGTH);
    }
    push_to_formatter_stack(&format_set_trust_line_set_flags);
}

static void format_set_trust_line_asset(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Asset", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_asset(&tx_ctx->tx_details.op_details.set_trust_line_flags_op.asset,
                                tx_ctx->network,
                                G.ui.detail_value,
                                DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_set_trust_line_clear_flags);
}

static void format_set_trust_line_trustor(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Trustor", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_account_id(tx_ctx->tx_details.op_details.set_trust_line_flags_op.trustor,
                                     G.ui.detail_value,
                                     DETAIL_VALUE_MAX_LENGTH,
                                     0,
                                     0))
    push_to_formatter_stack(&format_set_trust_line_asset);
}

static void format_set_trust_line_flags(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Set Trust Line Flags", DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_set_trust_line_trustor);
}

static void format_liquidity_pool_deposit_max_price(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Max Price", DETAIL_CAPTION_MAX_LENGTH);
    uint64_t price =
        ((uint64_t) tx_ctx->tx_details.op_details.liquidity_pool_deposit_op.max_price.n *
         10000000) /
        tx_ctx->tx_details.op_details.liquidity_pool_deposit_op.max_price.d;
    FORMATTER_CHECK(
        print_amount(price, NULL, tx_ctx->network, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH))
    format_operation_source_prepare(tx_ctx);
}

static void format_liquidity_pool_deposit_min_price(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Min Price", DETAIL_CAPTION_MAX_LENGTH);
    uint64_t price =
        ((uint64_t) tx_ctx->tx_details.op_details.liquidity_pool_deposit_op.min_price.n *
         10000000) /
        tx_ctx->tx_details.op_details.liquidity_pool_deposit_op.min_price.d;
    FORMATTER_CHECK(
        print_amount(price, NULL, tx_ctx->network, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_liquidity_pool_deposit_max_price);
}

static void format_liquidity_pool_deposit_max_amount_b(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Max Amount B", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_amount(tx_ctx->tx_details.op_details.liquidity_pool_deposit_op.max_amount_b,
                     NULL,
                     tx_ctx->network,
                     G.ui.detail_value,
                     DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_liquidity_pool_deposit_min_price);
}

static void format_liquidity_pool_deposit_max_amount_a(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Max Amount A", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_amount(tx_ctx->tx_details.op_details.liquidity_pool_deposit_op.max_amount_a,
                     NULL,
                     tx_ctx->network,
                     G.ui.detail_value,
                     DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_liquidity_pool_deposit_max_amount_b);
}

static void format_liquidity_pool_deposit_liquidity_pool_id(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Liquidity Pool ID", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_binary(tx_ctx->tx_details.op_details.liquidity_pool_deposit_op.liquidity_pool_id,
                     LIQUIDITY_POOL_ID_SIZE,
                     G.ui.detail_value,
                     DETAIL_VALUE_MAX_LENGTH,
                     0,
                     0))
    push_to_formatter_stack(&format_liquidity_pool_deposit_max_amount_a);
}

static void format_liquidity_pool_deposit(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Liquidity Pool Deposit", DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_liquidity_pool_deposit_liquidity_pool_id);
}

static void format_liquidity_pool_withdraw_min_amount_b(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Min Amount B", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_amount(tx_ctx->tx_details.op_details.liquidity_pool_withdraw_op.min_amount_b,
                     NULL,
                     tx_ctx->network,
                     G.ui.detail_value,
                     DETAIL_VALUE_MAX_LENGTH))
    format_operation_source_prepare(tx_ctx);
}

static void format_liquidity_pool_withdraw_min_amount_a(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Min Amount A", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_amount(tx_ctx->tx_details.op_details.liquidity_pool_withdraw_op.min_amount_a,
                     NULL,
                     tx_ctx->network,
                     G.ui.detail_value,
                     DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_liquidity_pool_withdraw_min_amount_b);
}

static void format_liquidity_pool_withdraw_amount(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Amount", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_amount(tx_ctx->tx_details.op_details.liquidity_pool_withdraw_op.amount,
                                 NULL,
                                 tx_ctx->network,
                                 G.ui.detail_value,
                                 DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_liquidity_pool_withdraw_min_amount_a);
}

static void format_liquidity_pool_withdraw_liquidity_pool_id(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Liquidity Pool ID", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(
        print_binary(tx_ctx->tx_details.op_details.liquidity_pool_withdraw_op.liquidity_pool_id,
                     LIQUIDITY_POOL_ID_SIZE,
                     G.ui.detail_value,
                     DETAIL_VALUE_MAX_LENGTH,
                     0,
                     0))
    push_to_formatter_stack(&format_liquidity_pool_withdraw_amount);
}

static void format_liquidity_pool_withdraw(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "Operation Type", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Liquidity Pool Withdraw", DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_liquidity_pool_withdraw_liquidity_pool_id);
}

static void format_invoke_host_function_asset_approve_expiration_ledger(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Expiration Ledger", DETAIL_CAPTION_MAX_LENGTH);
    FORMATTER_CHECK(print_uint(tx_ctx->tx_details.op_details.invoke_host_function_op
                                   .invoke_contract_args.asset_approve.expiration_ledger,
                               G.ui.detail_value,
                               DETAIL_VALUE_MAX_LENGTH))
    format_operation_source_prepare(tx_ctx);
}

static void format_invoke_host_function_asset_approve_amount(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Amount", DETAIL_CAPTION_MAX_LENGTH);
    char tmp[DETAIL_VALUE_MAX_LENGTH];
    explicit_bzero(tmp, DETAIL_VALUE_MAX_LENGTH);
    FORMATTER_CHECK(print_amount(tx_ctx->tx_details.op_details.invoke_host_function_op
                                     .invoke_contract_args.asset_approve.amount,
                                 NULL,
                                 tx_ctx->network,
                                 tmp,
                                 DETAIL_VALUE_MAX_LENGTH))
    STRLCAT(tmp, " ", DETAIL_VALUE_MAX_LENGTH);
    STRLCAT(tmp,
            tx_ctx->tx_details.op_details.invoke_host_function_op.invoke_contract_args.asset_approve
                .asset_code,
            DETAIL_VALUE_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, tmp, DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_invoke_host_function_asset_approve_expiration_ledger);
}

static void format_invoke_host_function_asset_approve_spender(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Spender", DETAIL_CAPTION_MAX_LENGTH);

    FORMATTER_CHECK(print_sc_address(&tx_ctx->tx_details.op_details.invoke_host_function_op
                                          .invoke_contract_args.asset_approve.spender,
                                     G.ui.detail_value,
                                     DETAIL_VALUE_MAX_LENGTH,
                                     0,
                                     0))
    push_to_formatter_stack(&format_invoke_host_function_asset_approve_amount);
}

static void format_invoke_host_function_asset_approve_from(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "From", DETAIL_CAPTION_MAX_LENGTH);

    FORMATTER_CHECK(print_sc_address(&tx_ctx->tx_details.op_details.invoke_host_function_op
                                          .invoke_contract_args.asset_approve.from,
                                     G.ui.detail_value,
                                     DETAIL_VALUE_MAX_LENGTH,
                                     0,
                                     0))
    push_to_formatter_stack(&format_invoke_host_function_asset_approve_spender);
}

static void format_invoke_host_function_asset_transfer_to(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "To", DETAIL_CAPTION_MAX_LENGTH);

    FORMATTER_CHECK(print_sc_address(&tx_ctx->tx_details.op_details.invoke_host_function_op
                                          .invoke_contract_args.asset_transfer.to,
                                     G.ui.detail_value,
                                     DETAIL_VALUE_MAX_LENGTH,
                                     0,
                                     0))
    format_operation_source_prepare(tx_ctx);
}

static void format_invoke_host_function_asset_transfer_from(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "From", DETAIL_CAPTION_MAX_LENGTH);

    FORMATTER_CHECK(print_sc_address(&tx_ctx->tx_details.op_details.invoke_host_function_op
                                          .invoke_contract_args.asset_transfer.from,
                                     G.ui.detail_value,
                                     DETAIL_VALUE_MAX_LENGTH,
                                     0,
                                     0))
    push_to_formatter_stack(&format_invoke_host_function_asset_transfer_to);
}

static void format_invoke_host_function_asset_transfer_amount(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Transfer", DETAIL_CAPTION_MAX_LENGTH);
    char tmp[DETAIL_VALUE_MAX_LENGTH];
    explicit_bzero(tmp, DETAIL_VALUE_MAX_LENGTH);
    FORMATTER_CHECK(print_amount(tx_ctx->tx_details.op_details.invoke_host_function_op
                                     .invoke_contract_args.asset_transfer.amount,
                                 NULL,
                                 tx_ctx->network,
                                 tmp,
                                 DETAIL_VALUE_MAX_LENGTH))
    STRLCAT(tmp, " ", DETAIL_VALUE_MAX_LENGTH);
    STRLCAT(tmp,
            tx_ctx->tx_details.op_details.invoke_host_function_op.invoke_contract_args
                .asset_transfer.asset_code,
            DETAIL_VALUE_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, tmp, DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_invoke_host_function_asset_transfer_from);
}

static void format_invoke_host_function_unverified_contract_warning(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "RISK WARNING", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value,
            "Unverified contract, will not display details",
            DETAIL_VALUE_MAX_LENGTH);
    format_operation_source_prepare(tx_ctx);
}

static void format_invoke_host_function_func_name(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Function", DETAIL_CAPTION_MAX_LENGTH);

    memcpy(G.ui.detail_value,
           tx_ctx->tx_details.op_details.invoke_host_function_op.invoke_contract_args.function_name
               .text,
           tx_ctx->tx_details.op_details.invoke_host_function_op.invoke_contract_args.function_name
               .text_size);
    G.ui.detail_value[tx_ctx->tx_details.op_details.invoke_host_function_op.invoke_contract_args
                          .function_name.text_size] = '\0';

    switch (
        tx_ctx->tx_details.op_details.invoke_host_function_op.invoke_contract_args.contract_type) {
        case SOROBAN_CONTRACT_TYPE_UNVERIFIED:
            push_to_formatter_stack(&format_invoke_host_function_unverified_contract_warning);
            break;
        case SOROBAN_CONTRACT_TYPE_ASSET_APPROVE:
            push_to_formatter_stack(&format_invoke_host_function_asset_approve_from);
            break;
        case SOROBAN_CONTRACT_TYPE_ASSET_TRANSFER:
            push_to_formatter_stack(&format_invoke_host_function_asset_transfer_amount);
            break;
        default:
            THROW(SW_TX_FORMATTING_FAIL);
            return;
    }
}

static void format_invoke_host_function_contract_id(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Contract ID", DETAIL_CAPTION_MAX_LENGTH);

    FORMATTER_CHECK(print_sc_address(
        &tx_ctx->tx_details.op_details.invoke_host_function_op.invoke_contract_args.address,
        G.ui.detail_value,
        DETAIL_VALUE_MAX_LENGTH,
        0,
        0))
    push_to_formatter_stack(&format_invoke_host_function_func_name);
}

static void format_invoke_host_function(tx_ctx_t *tx_ctx) {
    switch (tx_ctx->tx_details.op_details.invoke_host_function_op.host_function_type) {
        case HOST_FUNCTION_TYPE_INVOKE_CONTRACT:
            STRLCPY(G.ui.detail_caption, "Soroban", DETAIL_CAPTION_MAX_LENGTH);
            STRLCPY(G.ui.detail_value, "Invoke Smart Contract", DETAIL_VALUE_MAX_LENGTH);
            push_to_formatter_stack(&format_invoke_host_function_contract_id);
            break;
        case HOST_FUNCTION_TYPE_CREATE_CONTRACT:
            STRLCPY(G.ui.detail_caption, "Soroban", DETAIL_CAPTION_MAX_LENGTH);
            STRLCPY(G.ui.detail_value, "Create Smart Contract", DETAIL_VALUE_MAX_LENGTH);
            format_operation_source_prepare(tx_ctx);
            break;
        case HOST_FUNCTION_TYPE_UPLOAD_CONTRACT_WASM:
            STRLCPY(G.ui.detail_caption, "Soroban", DETAIL_CAPTION_MAX_LENGTH);
            STRLCPY(G.ui.detail_value, "Upload Smart Contract Wasm", DETAIL_VALUE_MAX_LENGTH);
            format_operation_source_prepare(tx_ctx);
            break;
        default:
            THROW(SW_TX_FORMATTING_FAIL);
            return;
    }
}

static void format_extend_footprint_ttl(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Soroban", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Extend Footprint TTL", DETAIL_VALUE_MAX_LENGTH);
    format_operation_source_prepare(tx_ctx);
}

static void format_restore_footprint(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Soroban", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Restore Footprint", DETAIL_VALUE_MAX_LENGTH);
    format_operation_source_prepare(tx_ctx);
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

void format_confirm_operation(tx_ctx_t *tx_ctx) {
    if (tx_ctx->tx_details.operations_count > 1) {
        size_t length;
        STRLCPY(op_caption, "Operation ", OPERATION_CAPTION_MAX_LENGTH);
        length = strlen(op_caption);
        FORMATTER_CHECK(print_uint(tx_ctx->tx_details.operation_index,
                                   op_caption + length,
                                   OPERATION_CAPTION_MAX_LENGTH - length))
        STRLCAT(op_caption, " of ", sizeof(op_caption));
        length = strlen(op_caption);
        FORMATTER_CHECK(print_uint(tx_ctx->tx_details.operations_count,
                                   op_caption + length,
                                   OPERATION_CAPTION_MAX_LENGTH - length))
        push_to_formatter_stack(
            ((format_function_t) PIC(formatters[tx_ctx->tx_details.op_details.type])));
    } else {
        ((format_function_t) PIC(formatters[tx_ctx->tx_details.op_details.type]))(tx_ctx);
    }
}

static void format_fee_bump_transaction_fee(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Max Fee", DETAIL_CAPTION_MAX_LENGTH);
    asset_t asset = {.type = ASSET_TYPE_NATIVE};
    FORMATTER_CHECK(print_amount(tx_ctx->fee_bump_tx_details.fee,
                                 &asset,
                                 tx_ctx->network,
                                 G.ui.detail_value,
                                 DETAIL_VALUE_MAX_LENGTH))
    push_to_formatter_stack(&format_transaction_details);
}

static void format_fee_bump_transaction_source(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Fee Source", DETAIL_CAPTION_MAX_LENGTH);
    if (tx_ctx->envelope_type == ENVELOPE_TYPE_TX_FEE_BUMP &&
        tx_ctx->fee_bump_tx_details.fee_source.type == KEY_TYPE_ED25519 &&
        memcmp(tx_ctx->fee_bump_tx_details.fee_source.ed25519,
               G_context.raw_public_key,
               RAW_ED25519_PUBLIC_KEY_SIZE) == 0) {
        FORMATTER_CHECK(print_muxed_account(&tx_ctx->fee_bump_tx_details.fee_source,
                                            G.ui.detail_value,
                                            DETAIL_VALUE_MAX_LENGTH,
                                            6,
                                            6))
    } else {
        FORMATTER_CHECK(print_muxed_account(&tx_ctx->fee_bump_tx_details.fee_source,
                                            G.ui.detail_value,
                                            DETAIL_VALUE_MAX_LENGTH,
                                            0,
                                            0))
    }
    push_to_formatter_stack(&format_fee_bump_transaction_fee);
}

static void format_fee_bump_transaction_details(tx_ctx_t *tx_ctx) {
    (void) tx_ctx;
    STRLCPY(G.ui.detail_caption, "Fee Bump", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value, "Transaction Details", DETAIL_VALUE_MAX_LENGTH);
    push_to_formatter_stack(&format_fee_bump_transaction_source);
}

static format_function_t get_tx_details_formatter(tx_ctx_t *tx_ctx) {
    if (tx_ctx->envelope_type == ENVELOPE_TYPE_TX_FEE_BUMP) {
        return &format_fee_bump_transaction_details;
    }

    if (tx_ctx->envelope_type == ENVELOPE_TYPE_TX) {
        if (tx_ctx->tx_details.memo.type != MEMO_NONE) {
            return &format_memo;
        } else {
            return &format_fee;
        }
    }

    THROW(SW_TX_FORMATTING_FAIL);
    return NULL;
}

static void format_network(tx_ctx_t *tx_ctx) {
    STRLCPY(G.ui.detail_caption, "Network", DETAIL_CAPTION_MAX_LENGTH);
    STRLCPY(G.ui.detail_value,
            (char *) PIC(NETWORK_NAMES[tx_ctx->network]),
            DETAIL_VALUE_MAX_LENGTH);
    format_function_t formatter = get_tx_details_formatter(tx_ctx);
    push_to_formatter_stack(formatter);
}

static format_function_t get_tx_formatter(tx_ctx_t *tx_ctx) {
    if (tx_ctx->network != 0) {
        return &format_network;
    } else {
        return get_tx_details_formatter(tx_ctx);
    }
}

format_function_t get_formatter(tx_ctx_t *tx_ctx, bool forward) {
    if (!forward) {
        if (G.ui.current_data_index ==
            0) {  // if we're already at the beginning of the buffer, return NULL
            return NULL;
        }
        // rewind to tx beginning if we're requesting a previous operation
        tx_ctx->offset = 0;
        tx_ctx->tx_details.operation_index = 0;
    }

    if (G.ui.current_data_index == 1) {
        return get_tx_formatter(tx_ctx);
    }

    // 1 == data_count_before_ops
    while (G.ui.current_data_index - 1 > tx_ctx->tx_details.operation_index) {
        if (!parse_tx_xdr(tx_ctx->raw, tx_ctx->raw_size, tx_ctx)) {
            return NULL;
        }
    }
    return &format_confirm_operation;
}

void ui_approve_tx_next_screen(tx_ctx_t *tx_ctx) {
    if (!formatter_stack[formatter_index]) {
        explicit_bzero(formatter_stack, sizeof(formatter_stack));
        formatter_index = 0;
        G.ui.current_data_index++;
        formatter_stack[0] = get_formatter(tx_ctx, true);
    }
}

void ui_approve_tx_prev_screen(tx_ctx_t *tx_ctx) {
    if (formatter_index == -1) {
        explicit_bzero(formatter_stack, sizeof(formatter_stack));
        formatter_index = 0;
        G.ui.current_data_index--;
        formatter_stack[0] = get_formatter(tx_ctx, false);
    }
}

void set_state_data(bool forward) {
    PRINTF("set_state_data invoked, forward = %d\n", forward);
    if (forward) {
        ui_approve_tx_next_screen(&G_context.tx_info);
    } else {
        ui_approve_tx_prev_screen(&G_context.tx_info);
    }

    // Apply last formatter to fill the screen's buffer
    if (formatter_stack[formatter_index]) {
        explicit_bzero(G.ui.detail_caption, sizeof(G.ui.detail_caption));
        explicit_bzero(G.ui.detail_value, sizeof(G.ui.detail_value));
        explicit_bzero(op_caption, sizeof(op_caption));
        formatter_stack[formatter_index](&G_context.tx_info);

        if (op_caption[0] != '\0') {
            STRLCPY(G.ui.detail_caption, op_caption, sizeof(G.ui.detail_caption));
            G.ui.detail_value[0] = ' ';
        }
    }
}
