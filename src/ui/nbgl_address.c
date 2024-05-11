/*****************************************************************************
 *   Ledger Stellar App.
 *   (c) 2024 Ledger SAS.
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
#ifdef HAVE_NBGL
#include <stdbool.h>  // bool
#include <string.h>   // memset

#include "nbgl_use_case.h"

#include "display.h"
#include "globals.h"
#include "sw.h"
#include "stellar/printer.h"
#include "action/validate.h"

// Validate/Invalidate public key and go back to home
static void ui_action_validate_pubkey(bool choice) {
    validate_pubkey(choice);
    ui_menu_main();
}

static void confirmation_choice(bool confirm) {
    ui_action_validate_pubkey(confirm);
    if (confirm) {
        nbgl_useCaseStatus("ADDRESS\nVERIFIED", true, ui_menu_main);
    } else {
        nbgl_useCaseStatus("Address verification\ncancelled", false, ui_menu_main);
    }
}

int ui_display_address(void) {
    if (G_context.req_type != CONFIRM_ADDRESS || G_context.state != STATE_NONE) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }

    explicit_bzero(G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH);
    if (!print_account_id(G_context.raw_public_key,
                          G.ui.detail_value,
                          DETAIL_VALUE_MAX_LENGTH,
                          0,
                          0)) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }
    nbgl_useCaseAddressConfirmation(G.ui.detail_value, confirmation_choice);
    return 0;
}
#endif  // HAVE_NBGL
