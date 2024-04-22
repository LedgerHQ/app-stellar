#include <string.h>
#include <stdint.h>

#include "os.h"
#include "swap.h"

#include "handle_swap_sign_transaction.h"
#include "stellar/printer.h"

/* Format printable amount including the ticker from specified parameters.
 *
 * Must set empty printable_amount on error, printable amount otherwise */
void swap_handle_get_printable_amount(get_printable_amount_parameters_t* params) {
    uint64_t amount;
    asset_t asset = {.type = ASSET_TYPE_NATIVE};

    params->printable_amount[0] = '\0';

    if (!swap_str_to_u64(params->amount, params->amount_length, &amount)) {
        PRINTF("Amount is too big");
        goto error;
    }

    if (!print_amount(amount,
                      &asset,
                      NETWORK_TYPE_PUBLIC,
                      params->printable_amount,
                      sizeof(params->printable_amount))) {
        goto error;
    }
    return;

error:
    memset(params->printable_amount, '\0', sizeof(params->printable_amount));
}