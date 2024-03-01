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
#include "../settings.h"
#include "../sw.h"
#include "../crypto.h"
#include "../io.h"
#include "../ui/ui.h"

int handler_sign_tx_hash(buffer_t *cdata) {
    PRINTF("handler_sign_tx_hash invoked\n");
    if (!HAS_SETTING(S_HASH_SIGNING_ENABLED)) {
        return io_send_sw(SW_TX_HASH_SIGNING_MODE_NOT_ENABLED);
    }
    explicit_bzero(&G_context, sizeof(G_context));
    G_context.req_type = CONFIRM_TRANSACTION_HASH;
    G_context.state = STATE_NONE;

    cx_ecfp_private_key_t private_key = {0};
    cx_ecfp_public_key_t public_key = {0};

    if (!buffer_read_u8(cdata, &G_context.bip32_path_len) ||
        !buffer_read_bip32_path(cdata, G_context.bip32_path, (size_t) G_context.bip32_path_len)) {
        return io_send_sw(SW_WRONG_DATA_LENGTH);
    }

    // derive private key according to BIP32 path
    if (crypto_derive_private_key(&private_key, G_context.bip32_path, G_context.bip32_path_len)) {
        return io_send_sw(SW_INTERNAL_ERROR);
    }
    // generate corresponding public key
    if (crypto_init_public_key(&private_key, &public_key, G_context.raw_public_key)) {
        return io_send_sw(SW_INTERNAL_ERROR);
    }

    // reset private key
    explicit_bzero(&private_key, sizeof(private_key));

    if (cdata->offset + HASH_SIZE != cdata->size) {
        return io_send_sw(SW_WRONG_DATA_LENGTH);
    }

    memcpy(G_context.hash, cdata->ptr + cdata->offset, HASH_SIZE);

    return ui_approve_tx_hash_init();
};
