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

#include <stdbool.h>

#include "./dispatcher.h"
#include "../globals.h"
#include "../sw.h"
#include "../io.h"
#include "../handler/handler.h"

int apdu_dispatcher(const command_t *cmd) {
    if (cmd->cla != CLA) {
        return io_send_sw(SW_CLA_NOT_SUPPORTED);
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
        case INS_SIGN_TX_HASH:
            if (cmd->p1 != 0 || cmd->p2 != 0) {
                return io_send_sw(SW_WRONG_P1P2);
            }
            if (!cmd->data) {
                return io_send_sw(SW_WRONG_DATA_LENGTH);
            }

            buf.ptr = cmd->data;
            buf.size = cmd->lc;
            buf.offset = 0;
            return handler_sign_tx_hash(&buf);
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
        case INS_SIGN_SOROBAN_AUTHORATION:
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
            return handle_sign_soroban_authorization(&buf, !cmd->p1, (bool) (cmd->p2 & P2_MORE));
        default:
            return io_send_sw(SW_INS_NOT_SUPPORTED);
    }
}
