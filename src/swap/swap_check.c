#include <string.h>

#include "os.h"

#include "../globals.h"
#include "../utils.h"

bool swap_check() {
    PRINTF("swap_check invoked.\n");
    char tmp_buf[sizeof(G.ui.detail_value)];

    // tx type
    if (G_context.tx_info.envelope_type != ENVELOPE_TYPE_TX) {
        return false;
    }

    // A XLM swap consist of only one "send" operation
    if (G_context.tx_info.tx_details.operations_count != 1) {
        return false;
    }

    // op type
    if (G_context.tx_info.tx_details.op_details.type != OPERATION_TYPE_PAYMENT) {
        return false;
    }

    // amount
    if (G_context.tx_info.tx_details.op_details.payment_op.asset.type != ASSET_TYPE_NATIVE ||
        G_context.tx_info.tx_details.op_details.payment_op.amount !=
            (int64_t) G.swap.values.amount) {
        return false;
    }

    // destination addr
    if (!print_muxed_account(&G_context.tx_info.tx_details.op_details.payment_op.destination,
                             tmp_buf,
                             DETAIL_VALUE_MAX_LENGTH,
                             0,
                             0)) {
        return false;
    };

    if (strcmp(tmp_buf, G.swap.values.destination) != 0) {
        return false;
    }

    if (G_context.tx_info.tx_details.op_details.source_account_present) {
        return false;
    }

    // memo
    if (G_context.tx_info.tx_details.memo.type != MEMO_TEXT ||
        strcmp((char *) G_context.tx_info.tx_details.memo.text.text, G.swap.values.memo) != 0) {
        return false;
    }

    // fees
    if (G_context.tx_info.network != NETWORK_TYPE_PUBLIC ||
        G_context.tx_info.tx_details.fee != G.swap.values.fees) {
        return false;
    }

    // we don't do any check on "TX Source" field
    // If we've reached this point without failure, we're good to go!
    return true;
}
