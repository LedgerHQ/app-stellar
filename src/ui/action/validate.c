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

#include <stdbool.h>  // bool

#include "./validate.h"
#include "../ui.h"
#include "../../send_response.h"
#include "../../sw.h"
#include "../../crypto.h"
#include "../../globals.h"

void ui_action_validate_pubkey(bool choice) {
    if (choice) {
        send_response_pubkey();
    } else {
        io_send_sw(SW_DENY);
    }

    ui_menu_main();
}

void ui_action_validate_transaction(bool choice) {
    if (choice) {
        G_context.state = STATE_APPROVED;
        uint8_t signature[SIGNATURE_SIZE];
        if (crypto_sign_message(G_context.hash, sizeof(G_context.hash), signature, SIGNATURE_SIZE) <
            0) {
            G_context.state = STATE_NONE;
            io_send_sw(SW_SIGNATURE_FAIL);
        } else {
            send_response_sig(signature, SIGNATURE_SIZE);
        }
    } else {
        G_context.state = STATE_NONE;
        io_send_sw(SW_DENY);
    }
    ui_menu_main();
};
