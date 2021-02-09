#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "stellar_api.h"
#include "stellar_format.h"

stellar_context_t ctx;

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    memset(&ctx, 0, sizeof(ctx));
    if (Size > sizeof(ctx.req.tx.raw)) {
        return 0;
    }
    memcpy(&ctx.req.tx.raw, Data, Size);
    ctx.req.tx.rawLength = Size;
    if (!parse_tx_xdr(Data, Size, &ctx.req.tx)) {
        return 0;
    }

    ctx.state = STATE_APPROVE_TX;

    set_state_data(true);
    while (formatter_stack[formatter_index] != NULL) {
        printf("%s: %s\n", detailCaption, detailValue);
        formatter_index++;

        if (formatter_stack[formatter_index] != NULL) {
            set_state_data(true);
        }
    }
    return 0;
}
