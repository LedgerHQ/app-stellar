#include <string.h>
#include <stdint.h>

#include "swap_lib_calls.h"
#include "stellar_api.h"

/* return 0 on error, 1 otherwise */
int handle_get_printable_amount(get_printable_amount_parameters_t* params) {
    params->printable_amount[0] = 0;
    if (params->amount_length > 8) {
        PRINTF("Amount is too big");
        return 0;
    }

    unsigned char amount_buffer[8];
    memset(amount_buffer, 0, sizeof(amount_buffer));
    memcpy(amount_buffer + sizeof(amount_buffer) - params->amount_length,
           params->amount,
           params->amount_length);
    buffer_t buffer = {.ptr = amount_buffer, .offset = 0, .size = 8};

    uint64_t amount;
    buffer_read64(&buffer, &amount);
    print_amount(amount, "XLM", params->printable_amount);

    return 1;
}
