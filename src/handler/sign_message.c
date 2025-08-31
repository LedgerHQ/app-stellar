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

#include "io.h"
#include "buffer.h"

#include "get_public_key.h"
#include "globals.h"
#include "sw.h"
#include "crypto.h"
#include "ui/display.h"
#include "helper/send_response.h"

static unsigned char const SIGN_MAGIC_HEADER[24] = "Stellar Signed Message:\n";

int handler_sign_message(buffer_t *cdata, bool is_first_chunk, bool more) {
    if (is_first_chunk) {
        explicit_bzero(&G_context, sizeof(G_context));
    }

    if (G_context.raw_size + cdata->size > RAW_DATA_MAX_SIZE) {
        return io_send_sw(SW_DATA_TOO_LARGE);
    }

    if (is_first_chunk) {
        G_context.req_type = CONFIRM_MESSAGE;
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
        if (G_context.req_type != CONFIRM_MESSAGE) {
            return io_send_sw(SW_BAD_STATE);
        }
        memcpy(G_context.raw + G_context.raw_size, cdata->ptr, cdata->size);
        G_context.raw_size += cdata->size;
    }

    if (more) {
        return io_send_sw(SW_OK);
    }

    // Generate public key
    cx_err_t error = crypto_derive_public_key(G_context.raw_public_key,
                                              G_context.bip32_path,
                                              G_context.bip32_path_len);
    if (error != CX_OK) {
        return io_send_sw(error);
    }

    cx_sha256_t msg_hash_context;  // used to compute sha256(header + message)
    cx_sha256_init(&msg_hash_context);

    error = cx_hash_update(&msg_hash_context.header, SIGN_MAGIC_HEADER, sizeof(SIGN_MAGIC_HEADER));
    if (error != CX_OK) {
        return io_send_sw(error);
    }

    error = cx_hash_update(&msg_hash_context.header, G_context.raw, G_context.raw_size);
    if (error != CX_OK) {
        return io_send_sw(error);
    }

    error = cx_hash_final(&msg_hash_context.header, G_context.hash);
    if (error != CX_OK) {
        return io_send_sw(error);
    }

    return ui_display_message();
}
