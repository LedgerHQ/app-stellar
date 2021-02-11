#include <string.h>
#include <stdint.h>

#include "swap_lib_calls.h"
#include "stellar_api.h"

/* return 0 on error, 1 otherwise */
int handle_get_printable_amount(get_printable_amount_parameters_t* params) {
    uint64_t amount;
    Asset asset = {.type = ASSET_TYPE_NATIVE};

    params->printable_amount[0] = '\0';

    if (!swap_str_to_u64(params->amount, params->amount_length, &amount)) {
        PRINTF("Amount is too big");
        return 0;
    }

    if (print_amount(amount,
                     &asset,
                     NETWORK_TYPE_PUBLIC,
                     params->printable_amount,
                     sizeof(params->printable_amount))) {
        return 0;
    }
    return 1;
}
