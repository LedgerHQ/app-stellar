#include "stellar_vars.h"
#include "stellar_ux.h"
#include "stellar_api.h"
#include "stellar_format.h"

void swap_check() {
    char *tmp_buf = detailValue;

    tx_context_t *txCtx = &ctx.req.tx;

    // A XLM swap consist of only one "send" operation
    if (txCtx->opCount > 1) {
        io_seproxyhal_touch_tx_cancel(NULL);
    }

    // tx type
    if (txCtx->opDetails.type != XDR_OPERATION_TYPE_PAYMENT) {
        io_seproxyhal_touch_tx_cancel(NULL);
    }

    // amount
    if (strcmp("XLM", txCtx->opDetails.op.payment.asset.code) ||
        txCtx->opDetails.op.payment.amount != swap_values.amount) {
        io_seproxyhal_touch_tx_cancel(NULL);
    }

    // destination addr
    print_public_key(txCtx->opDetails.op.payment.destination, tmp_buf, 0, 0);
    if (strcmp(tmp_buf, swap_values.destination) != 0) {
        io_seproxyhal_touch_tx_cancel(NULL);
    }

    if (txCtx->opDetails.sourcePresent) {
        io_seproxyhal_touch_tx_cancel(NULL);
    }

    // memo
    if (txCtx->txDetails.memo.type != MEMO_TYPE_TEXT ||
        strcmp(txCtx->txDetails.memo.data, swap_values.memo) != 0) {
        io_seproxyhal_touch_tx_cancel(NULL);
    }

    // fees
    if (txCtx->txDetails.network != NETWORK_TYPE_PUBLIC ||
        txCtx->txDetails.fee != swap_values.fees) {
        io_seproxyhal_touch_tx_cancel(NULL);
    }

    if (txCtx->txDetails.hasTimeBounds) {
        io_seproxyhal_touch_tx_cancel(NULL);
    }

    // // we don't do any check on "TX Source" field
    // // If we've reached this point without failure, we're good to go !
    io_seproxyhal_touch_tx_ok(NULL);
    os_sched_exit(0);
}
