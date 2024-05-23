/*****************************************************************************
 *   Ledger App Stellar.
 *   (c) 2024 Ledger SAS.
 *   (c) 2024 overcat.
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