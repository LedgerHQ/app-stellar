#include "os.h"
#include "ux.h"
#include "os_io_seproxyhal.h"

#include "swap.h"

#include "handle_swap_sign_transaction.h"
#include "globals.h"
#include "types.h"
#include "stellar/printer.h"
#include "stellar/parser.h"

// #ifdef HAVE_NBGL
// #include "nbgl_use_case.h"
// #endif

// Save the BSS address where we will write the return value when finished
static uint8_t* G_swap_sign_return_value_address;

/* Backup up transaction parameters and wipe BSS to avoid collusion with
 * app-exchange BSS data.
 *
 * return false on error, true otherwise */
bool swap_copy_transaction_parameters(create_transaction_parameters_t* params) {
    // first copy parameters to stack, and then to global data.
    // We need this "trick" as the input data position can overlap with btc-app globals
    swap_values_t stack_data;
    memset(&stack_data, 0, sizeof(stack_data));

    if (strlen(params->destination_address) >= sizeof(stack_data.destination) ||
        strlen(params->destination_address_extra_id) >= sizeof(stack_data.memo)) {
        return false;
    }
    if (strlcpy(stack_data.destination,
                params->destination_address,
                sizeof(stack_data.destination)) >= sizeof(stack_data.destination)) {
        return false;
    }
    if (strlcpy(stack_data.memo, params->destination_address_extra_id, sizeof(stack_data.memo)) >=
        sizeof(stack_data.memo)) {
        return false;
    }
    if (!swap_str_to_u64(params->amount, params->amount_length, &stack_data.amount)) {
        return false;
    }

    if (!swap_str_to_u64(params->fee_amount, params->fee_amount_length, &stack_data.fees)) {
        return false;
    }

    // Full reset the global variables
    os_explicit_zero_BSS_segment();
    // Keep the address at which we'll reply the signing status
    G_swap_sign_return_value_address = &params->result;
    // Commit the values read from exchange to the clean global space
    memcpy(&G.swap_values, &stack_data, sizeof(stack_data));
    return true;
}

void __attribute__((noreturn)) swap_finalize_exchange_sign_transaction(bool is_success) {
    *G_swap_sign_return_value_address = is_success;
    os_lib_end();
}

bool swap_check() {
    PRINTF("swap_check invoked.\n");

    char tmp_buf[DETAIL_VALUE_MAX_LENGTH];

    // tx type
    if (G_context.envelope.type != ENVELOPE_TYPE_TX) {
        return false;
    }

    // A XLM swap consist of only one "send" operation
    if (G_context.envelope.tx_details.tx.operations_count != 1) {
        return false;
    }

    // parse the payment op
    if (!parse_transaction_operation(G_context.raw, G_context.raw_size, &G_context.envelope, 0)) {
        return false;
    }

    // op type
    if (G_context.envelope.tx_details.tx.op_details.type != OPERATION_TYPE_PAYMENT) {
        return false;
    }

    // amount
    if (G_context.envelope.tx_details.tx.op_details.payment_op.asset.type != ASSET_TYPE_NATIVE ||
        G_context.envelope.tx_details.tx.op_details.payment_op.amount !=
            (int64_t) G.swap_values.amount) {
        return false;
    }

    // destination addr
    if (!print_muxed_account(&G_context.envelope.tx_details.tx.op_details.payment_op.destination,
                             tmp_buf,
                             DETAIL_VALUE_MAX_LENGTH,
                             0,
                             0)) {
        return false;
    };

    if (strcmp(tmp_buf, G.swap_values.destination) != 0) {
        return false;
    }

    if (G_context.envelope.tx_details.tx.op_details.source_account_present) {
        return false;
    }

    // memo
    if (G_context.envelope.tx_details.tx.memo.type != MEMO_TEXT ||
        strcmp((char*) G_context.envelope.tx_details.tx.memo.text.text, G.swap_values.memo) != 0) {
        return false;
    }

    // fees
    if (G_context.envelope.network != NETWORK_TYPE_PUBLIC ||
        G_context.envelope.tx_details.tx.fee != G.swap_values.fees) {
        return false;
    }

    // we don't do any check on "TX Source" field
    // If we've reached this point without failure, we're good to go!
    return true;
}