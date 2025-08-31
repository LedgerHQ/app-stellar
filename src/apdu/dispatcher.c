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

#include <stdbool.h>

#include "buffer.h"
#include "io.h"
#include "ledger_assert.h"
#include "swap.h"

#include "dispatcher.h"
#include "sw.h"
#include "types.h"
#include "handler/get_app_configuration.h"
#include "handler/get_public_key.h"
#include "handler/sign_tx.h"
#include "handler/sign_hash.h"
#include "handler/sign_auth.h"
#include "handler/sign_message.h"

int apdu_dispatcher(const command_t *cmd) {
    LEDGER_ASSERT(cmd != NULL, "NULL cmd");

    if (cmd->cla != CLA) {
        return io_send_sw(SW_CLA_NOT_SUPPORTED);
    }

    if (G_called_from_swap) {
        if (cmd->ins != INS_GET_PUBLIC_KEY && cmd->ins != INS_SIGN_TX) {
            PRINTF("Only GET_PUBLIC_KEY and SIGN_TX can be called during swap\n");
            return io_send_sw(SW_INS_NOT_SUPPORTED);
        }
    }

    buffer_t buf = {0};

    switch (cmd->ins) {
        case INS_GET_APP_CONFIGURATION:
            if (cmd->p1 != 0 || cmd->p2 != 0) {
                return io_send_sw(SW_WRONG_P1P2);
            }
            return handler_get_app_configuration();
        case INS_GET_PUBLIC_KEY:
            if (cmd->p1 != 0 || cmd->p2 > 1) {
                return io_send_sw(SW_WRONG_P1P2);
            }

            if (!cmd->data) {
                return io_send_sw(SW_WRONG_DATA_LENGTH);
            }

            buf.ptr = cmd->data;
            buf.size = cmd->lc;
            buf.offset = 0;

            return handler_get_public_key(&buf, (bool) cmd->p2);
        case INS_SIGN_HASH:
            if (cmd->p1 != P1_FIRST || cmd->p2 != P2_LAST) {
                return io_send_sw(SW_WRONG_P1P2);
            }

            if (!cmd->data) {
                return io_send_sw(SW_WRONG_DATA_LENGTH);
            }

            buf.ptr = cmd->data;
            buf.size = cmd->lc;
            buf.offset = 0;
            return handler_sign_hash(&buf);
        case INS_SIGN_TX:
            if ((cmd->p1 != P1_FIRST && cmd->p1 != P1_MORE) ||
                (cmd->p2 != P2_LAST && cmd->p2 != P2_MORE)) {
                return io_send_sw(SW_WRONG_P1P2);
            }

            if (!cmd->data) {
                return io_send_sw(SW_WRONG_DATA_LENGTH);
            }

            buf.ptr = cmd->data;
            buf.size = cmd->lc;
            buf.offset = 0;

            return handler_sign_tx(&buf, !cmd->p1, (bool) (cmd->p2 & P2_MORE));
        case INS_SIGN_SOROBAN_AUTHORIZATION:
            if ((cmd->p1 != P1_FIRST && cmd->p1 != P1_MORE) ||
                (cmd->p2 != P2_LAST && cmd->p2 != P2_MORE)) {
                return io_send_sw(SW_WRONG_P1P2);
            }

            if (!cmd->data) {
                return io_send_sw(SW_WRONG_DATA_LENGTH);
            }

            buf.ptr = cmd->data;
            buf.size = cmd->lc;
            buf.offset = 0;

            return handler_sign_auth(&buf, !cmd->p1, (bool) (cmd->p2 & P2_MORE));

        case INS_SIGN_MESSAGE:
            if ((cmd->p1 != P1_FIRST && cmd->p1 != P1_MORE) ||
                (cmd->p2 != P2_LAST && cmd->p2 != P2_MORE)) {
                return io_send_sw(SW_WRONG_P1P2);
            }

            if (!cmd->data) {
                return io_send_sw(SW_WRONG_DATA_LENGTH);
            }

            buf.ptr = cmd->data;
            buf.size = cmd->lc;
            buf.offset = 0;

            return handler_sign_message(&buf, !cmd->p1, (bool) (cmd->p2 & P2_MORE));

        default:
            return io_send_sw(SW_INS_NOT_SUPPORTED);
    }
}
