/*****************************************************************************
 *   Ledger Stellar App.
 *   (c) 2022 Ledger SAS.
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

#include "./handler.h"
#include "../globals.h"
#include "../types.h"
#include "../sw.h"
#include "../send_response.h"
#include "../crypto.h"
#include "../ui/ui.h"
#include "../swap/swap_lib_calls.h"
#include "../swap/handle_swap_commands.h"
#include "../transaction/transaction_parser.h"

int handler_sign_tx(buffer_t *cdata, bool is_first_chunk, bool more) {
    if (is_first_chunk) {
        explicit_bzero(&G_context, sizeof(G_context));
    }

    if (G_context.tx_info.raw_size + cdata->size > RAW_TX_MAX_SIZE) {
        return io_send_sw(SW_WRONG_TX_LENGTH);
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
        memcpy(G_context.tx_info.raw, cdata->ptr + cdata->offset, data_length);
        G_context.tx_info.raw_size += data_length;
    } else {
        if (G_context.req_type != CONFIRM_TRANSACTION) {
            return io_send_sw(SW_BAD_STATE);
        }
        memcpy(G_context.tx_info.raw + G_context.tx_info.raw_size, cdata->ptr, cdata->size);
        G_context.tx_info.raw_size += cdata->size;
    }

    PRINTF("data size: %d\n", G_context.tx_info.raw_size);

    if (more) {
        return io_send_sw(SW_OK);
    }

    if (!parse_tx_xdr(G_context.tx_info.raw, G_context.tx_info.raw_size, &G_context.tx_info)) {
        THROW(SW_TX_PARSING_FAIL);
    }

    G_context.state = STATE_PARSED;
    PRINTF("tx parsed.\n");

    // We have been called from the Exchange app that has already vaidated the TX in the UI
    if (G_called_from_swap) {
        if (G.swap.response_ready) {
            // Safety against trying to make the app sign multiple TX
            // This panic quit is a failsafe that should never trigger, as the app is supposed to
            // exit after the first send when started in swap mode
            os_sched_exit(-1);
        } else {
            // We will quit the app after this transaction, whether it succeeds or fails
            G.swap.response_ready = true;
        }
        if (!swap_check()) {
            return io_send_sw(SW_SWAP_CHECKING_FAIL);
        }
        uint8_t signature[SIGNATURE_SIZE];

        if (cx_hash_sha256(G_context.tx_info.raw,
                           G_context.tx_info.raw_size,
                           G_context.hash,
                           HASH_SIZE) != HASH_SIZE) {
            THROW(SW_TX_HASH_FAIL);
        }

        if (crypto_sign_message(G_context.hash,
                                sizeof(G_context.hash),
                                signature,
                                SIGNATURE_SIZE) != 0) {
            G_context.state = STATE_NONE;
            return io_send_sw(SW_SIGNATURE_FAIL);
        } else {
            return send_response_sig(signature, SIGNATURE_SIZE);
        }

    } else {
        // Normal (not-swap) mode, derive the public_key and display the validation UI

        cx_ecfp_private_key_t private_key = {0};
        cx_ecfp_public_key_t public_key = {0};

        // derive private key according to BIP32 path
        int error =
            crypto_derive_private_key(&private_key, G_context.bip32_path, G_context.bip32_path_len);
        if (error != 0) {
            explicit_bzero(&private_key, sizeof(private_key));
            return io_send_sw(SW_INTERNAL_ERROR);
        }
        // generate corresponding public key
        crypto_init_public_key(&private_key, &public_key, G_context.raw_public_key);
        // reset private key
        explicit_bzero(&private_key, sizeof(private_key));

        if (cx_hash_sha256(G_context.tx_info.raw,
                           G_context.tx_info.raw_size,
                           G_context.hash,
                           HASH_SIZE) != HASH_SIZE) {
            THROW(SW_TX_HASH_FAIL);
        }

        return ui_approve_tx_init();
    }
};
