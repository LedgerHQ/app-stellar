#include "stellar_format.h"
#include "stellar_vars.h"
#include "stellar_api.h"

#ifdef TEST
#include <bsd/string.h>
#endif

uint8_t current_data_index;

format_function_t get_formatter(tx_context_t *txCtx, bool forward) {
    switch (ctx.state) {
        case STATE_APPROVE_TX: {  // classic tx
            if (!forward) {
                if (current_data_index ==
                    0) {  // if we're already at the beginning of the buffer, return NULL
                    return NULL;
                }
                // rewind to tx beginning if we're requesting a previous operation
                txCtx->offset = 0;
                txCtx->opIdx = 0;
            }

            while (current_data_index > txCtx->opIdx) {
                if (!parse_tx_xdr(txCtx->raw, txCtx->rawLength, txCtx)) {
                    return NULL;
                }
            }
            return &format_confirm_operation;
        }
        case STATE_APPROVE_TX_HASH: {
            if (!forward) {
                return NULL;
            }
            return &format_confirm_hash_warning;
        }
        default:
            THROW(0x6123);
    }
    return NULL;
}

void ui_approve_tx_next_screen(tx_context_t *txCtx) {
    if (!formatter_stack[formatter_index]) {
        MEMCLEAR(formatter_stack);
        formatter_index = 0;
        current_data_index++;
        formatter_stack[0] = get_formatter(txCtx, true);
    }
}

void ui_approve_tx_prev_screen(tx_context_t *txCtx) {
    if (formatter_index == -1) {
        MEMCLEAR(formatter_stack);
        formatter_index = 0;
        current_data_index--;
        formatter_stack[0] = get_formatter(txCtx, false);
    }
}

void set_state_data(bool forward) {
    if (forward) {
        ui_approve_tx_next_screen(&ctx.req.tx);
    } else {
        ui_approve_tx_prev_screen(&ctx.req.tx);
    }

    // Apply last formatter to fill the screen's buffer
    if (formatter_stack[formatter_index]) {
        MEMCLEAR(detailCaption);
        MEMCLEAR(detailValue);
        MEMCLEAR(opCaption);
        formatter_stack[formatter_index](&ctx.req.tx);

        if (opCaption[0] != '\0') {
            strlcpy(detailCaption, opCaption, sizeof(detailCaption));
            detailValue[0] = ' ';
            PRINTF("caption: %s\n", detailCaption);
        } else if (detailCaption[0] != '\0' && detailValue[0] != '\0') {
            PRINTF("caption: %s\n", detailCaption);
            PRINTF("details: %s\n", detailValue);
        }
    }
}
