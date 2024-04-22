/*****************************************************************************
 *   Ledger App Stellar.
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

#ifdef HAVE_BAGL

#include <stdbool.h>  // bool
#include <string.h>   // memset

#include "os.h"
#include "ux.h"
#include "glyphs.h"
#include "io.h"
#include "bip32.h"
#include "format.h"

#include "display.h"
#include "constants.h"
#include "globals.h"
#include "sw.h"
#include "action/validate.h"
#include "stellar/printer.h"
#include "stellar/formatter.h"
#include "stellar/printer.h"

static action_validate_cb g_validate_callback;

// Validate/Invalidate transaction and go back to home
static void ui_action_validate_transaction(bool choice) {
    validate_transaction(choice);
    ui_menu_main();
}

// Step with icon and text
UX_STEP_NOCB(ux_tx_hash_signing_review_step,
             pnn,
             {
                 &C_icon_eye,
                 "Review",
                 "Hash Signing",
             });
UX_STEP_NOCB(ux_tx_hash_signing_warning_step,
             pbb,
             {
                 &C_icon_warning,
                 "Dangerous",
                 "Operation",
             });
UX_STEP_NOCB(ux_tx_hash_signing_display_hash_step,
             bnnn_paging,
             {
                 .title = "Hash",
                 .text = G.ui.detail_value,
             });
// Step with approve button
UX_STEP_CB(ux_tx_hash_display_approve_step,
           pb,
           (*g_validate_callback)(true),
           {
               &C_icon_validate_14,
               "Approve",
           });
// Step with reject button
UX_STEP_CB(ux_tx_hash_display_reject_step,
           pb,
           (*g_validate_callback)(false),
           {
               &C_icon_crossmark,
               "Reject",
           });

// FLOW to display hash signing
// #1 screen: eye icon + "Review Transaction"
// #2 screen: warning icon + "Hash Signing"
// #3 screen: display hash
// #4 screen: approve button
// #5 screen: reject button
UX_FLOW(ux_tx_hash_signing_flow,
        &ux_tx_hash_signing_review_step,
        &ux_tx_hash_signing_warning_step,
        &ux_tx_hash_signing_display_hash_step,
        &ux_tx_hash_display_approve_step,
        &ux_tx_hash_display_reject_step);

int ui_display_hash() {
    if (G_context.req_type != CONFIRM_HASH || G_context.state != STATE_NONE) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }
    memset(G.ui.detail_value, 0, 89);

    if (!format_hex(G_context.hash, 32, G.ui.detail_value, 89)) {
        return io_send_sw(SW_DISPLAY_TRANSACTION_HASH_FAIL);
    }

    g_validate_callback = &ui_action_validate_transaction;

    ux_flow_init(0, ux_tx_hash_signing_flow, NULL);
    return 0;
}
#endif
