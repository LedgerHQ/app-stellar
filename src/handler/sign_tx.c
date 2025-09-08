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

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memset, explicit_bzero

#include "os.h"
#include "cx.h"
#include "buffer.h"
#include "swap.h"

#include "sign_tx.h"
#include "sw.h"
#include "globals.h"
#include "plugin.h"
#include "ui/display.h"
#include "crypto.h"
#include "helper/send_response.h"
#include "swap/handle_swap_sign_transaction.h"
#include "stellar/parser.h"
#include "stellar/formatter.h"

static bool check_include_custom_contract();

int handler_sign_tx(buffer_t *cdata, bool is_first_chunk, bool more) {
    if (is_first_chunk) {
        explicit_bzero(&G_context, sizeof(G_context));
    }

    if (G_context.raw_size + cdata->size > RAW_DATA_MAX_SIZE) {
        return io_send_sw(SW_DATA_TOO_LARGE);
    }

    if (is_first_chunk) {
        G_context.req_type = CONFIRM_TRANSACTION;
        G_context.state = STATE_NONE;

        if (!buffer_read_u8(cdata, &G_context.bip32_path_len) ||
            !buffer_read_bip32_path(cdata,
                                    G_context.bip32_path,
                                    (size_t) G_context.bip32_path_len)) {
            return io_send_sw(SW_WRONG_DATA_LENGTH);
        }
        size_t data_length = cdata->size - cdata->offset;
        memcpy(G_context.raw, cdata->ptr + cdata->offset, data_length);
        G_context.raw_size += data_length;
    } else {
        if (G_context.req_type != CONFIRM_TRANSACTION) {
            return io_send_sw(SW_BAD_STATE);
        }
        memcpy(G_context.raw + G_context.raw_size, cdata->ptr, cdata->size);
        G_context.raw_size += cdata->size;
    }

    PRINTF("data size: %d\n", G_context.raw_size);

    if (more) {
        return io_send_sw(SW_OK);
    }

    if (!parse_transaction_envelope(G_context.raw, G_context.raw_size, &G_context.envelope)) {
        return io_send_sw(SW_DATA_PARSING_FAIL);
    }

    G_context.state = STATE_PARSED;
    PRINTF("tx parsed.\n");

    // We have been called from the Exchange app that has already validated the TX in the UI
    if (G_called_from_swap) {
        if (G_swap_response_ready) {
            // Safety against trying to make the app sign multiple TX
            // This panic quit is a failsafe that should never trigger, as the app is supposed to
            // exit after the first send when started in swap mode
            os_sched_exit(-1);
        } else {
            // We will quit the app after this transaction, whether it succeeds or fails
            G_swap_response_ready = true;
        }

        if (!swap_check()) {
            return io_send_sw(SW_SWAP_CHECKING_FAIL);
        }
        uint8_t signature[SIGNATURE_SIZE];

        if (cx_hash_sha256(G_context.raw, G_context.raw_size, G_context.hash, HASH_SIZE) !=
            HASH_SIZE) {
            return io_send_sw(SW_DATA_HASH_FAIL);
        }

        if (crypto_sign_message(G_context.hash,
                                sizeof(G_context.hash),
                                signature,
                                SIGNATURE_SIZE,
                                G_context.bip32_path,
                                G_context.bip32_path_len) != CX_OK) {
            G_context.state = STATE_NONE;
            return io_send_sw(SW_SIGNATURE_FAIL);
        } else {
            return helper_send_response_sig(signature);
        }

    } else {
        // Normal (not-swap) mode, derive the public_key and display the validation UI
        cx_err_t error = crypto_derive_public_key(G_context.raw_public_key,
                                                  G_context.bip32_path,
                                                  G_context.bip32_path_len);

        if (error != CX_OK) {
            return io_send_sw(error);
        }

        if (cx_hash_sha256(G_context.raw, G_context.raw_size, G_context.hash, HASH_SIZE) !=
            HASH_SIZE) {
            return io_send_sw(SW_DATA_HASH_FAIL);
        }

        G_context.unverified_contracts = check_include_custom_contract();
        return ui_display_transaction();
    }
};

static bool check_include_custom_contract() {
    // check if the transaction contains a unverified contract
    for (uint8_t i = 0; i < G_context.envelope.tx_details.tx.operations_count; i++) {
        if (!parse_transaction_operation(G_context.raw,
                                         G_context.raw_size,
                                         &G_context.envelope,
                                         i)) {
            // should not happen
            return io_send_sw(SW_DATA_PARSING_FAIL);
        }
        if (G_context.envelope.tx_details.tx.op_details.type == OPERATION_INVOKE_HOST_FUNCTION &&
            G_context.envelope.tx_details.tx.op_details.invoke_host_function_op
                    .host_function_type == HOST_FUNCTION_TYPE_INVOKE_CONTRACT) {
            const uint8_t *contract_address =
                G_context.envelope.tx_details.tx.op_details.invoke_host_function_op
                    .invoke_contract_args.address.address;
            if (!plugin_check_presence(contract_address)) {
                return true;
            }

            if (plugin_init_contract(contract_address) != STELLAR_PLUGIN_RESULT_OK) {
                return true;
            }

            uint8_t data_pair_count_tmp = 0;
            if (plugin_query_data_pair_count(contract_address, &data_pair_count_tmp) !=
                STELLAR_PLUGIN_RESULT_OK) {
                return true;
            }
        }
    }
    return false;
}
