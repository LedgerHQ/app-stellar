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

#include <stdint.h>   // uint*_t
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memset, explicit_bzero

#include "os.h"
#include "cx.h"
#include "io.h"
#include "buffer.h"

#include "get_public_key.h"
#include "globals.h"
#include "types.h"
#include "sw.h"
#include "crypto.h"
#include "settings.h"
#include "ui/display.h"
#include "helper/send_response.h"

int handler_sign_hash(buffer_t *cdata) {
    if (!HAS_SETTING(S_HASH_SIGNING_ENABLED)) {
        return io_send_sw(SW_HASH_SIGNING_MODE_NOT_ENABLED);
    }

    explicit_bzero(&G_context, sizeof(G_context));
    G_context.req_type = CONFIRM_HASH;
    G_context.state = STATE_NONE;

    if (!buffer_read_u8(cdata, &G_context.bip32_path_len) ||
        !buffer_read_bip32_path(cdata, G_context.bip32_path, (size_t) G_context.bip32_path_len)) {
        return io_send_sw(SW_WRONG_DATA_LENGTH);
    }

    if (cdata->offset + HASH_SIZE != cdata->size) {
        return io_send_sw(SW_WRONG_DATA_LENGTH);
    }

    memcpy(G_context.hash, cdata->ptr + cdata->offset, HASH_SIZE);

    // Generate public key
    cx_err_t error = crypto_derive_public_key(G_context.raw_public_key,
                                              G_context.bip32_path,
                                              G_context.bip32_path_len);
    if (error != CX_OK) {
        return io_send_sw(error);
    }

    return ui_display_hash();
}
