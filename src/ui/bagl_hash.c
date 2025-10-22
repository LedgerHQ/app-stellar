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

static void start_review_flow(void);

static action_validate_cb g_validate_callback;

// Validate/Invalidate transaction and go back to home
static void ui_action_validate_transaction(bool choice) {
    validate_transaction(choice);
    ui_menu_main();
}

// Step with icon and text
UX_STEP_NOCB(ux_hash_signing_review_step,
             pnn,
             {
                 &C_icon_eye,
                 "Review",
                 "Hash Signing",
             });
UX_STEP_NOCB(ux_hash_signing_display_hash_step,
             bnnn_paging,
             {
                 .title = "Hash",
                 .text = G.ui.detail_value,
             });
// Step with approve button
UX_STEP_CB(ux_hash_display_approve_step,
           pb,
           (*g_validate_callback)(true),
           {
               &C_icon_validate_14,
               "Sign Hash",
           });
// Step with reject button
UX_STEP_CB(ux_hash_display_reject_step,
           pb,
           (*g_validate_callback)(false),
           {
               &C_icon_crossmark,
               "Reject",
           });

UX_STEP_NOCB(ux_hash_approval_blind_signing_reminder_step,
             pbb,
             {
                 &C_icon_warning,
                 "You accepted",
                 "the risks",
             });

// FLOW to display hash signing
// #1 screen: eye icon + "Review Transaction"
// #2 screen: display hash
// #3 screen: display warning
// #4 screen: approve button
// #5 screen: reject button
UX_FLOW(ux_hash_signing_flow,
        &ux_hash_signing_review_step,
        &ux_hash_signing_display_hash_step,
        &ux_hash_approval_blind_signing_reminder_step,
        &ux_hash_display_approve_step,
        &ux_hash_display_reject_step);

// clang-format off
UX_STEP_NOCB(
    ux_hash_blind_signing_warning_step,
    pbb,
    {
      &C_icon_warning,
      "This transaction",
      "cannot be trusted",
    });
UX_STEP_NOCB(
    ux_hash_blind_signing_text1_step,
    nnnn,
    {
      "Your Ledger cannot",
      "decode this",
      "transaction. If you",
      "sign it, you could",
    });
UX_STEP_NOCB(
    ux_hash_blind_signing_text2_step,
    nnnn,
    {
      "be authorizing",
      "malicious actions",
      "that can drain your",
      "wallet.",
    });
UX_STEP_NOCB(
    ux_hash_blind_signing_link_step,
    nn,
    {
      "Learn more:",
      "ledger.com/e8",
    });
UX_STEP_CB(
    ux_hash_blind_signing_accept_step,
    pbb,
    start_review_flow(),
    {
      &C_icon_validate_14,
      "Accept risk and",
      "review transaction",
    });
UX_STEP_CB(
    ux_hash_blind_signing_reject_step,
    pb,
    ui_action_validate_transaction(false),
    {
      &C_icon_crossmark,
      "Reject",
    });
// clang-format on

UX_FLOW(ux_hash_blind_signing_flow,
        &ux_hash_blind_signing_warning_step,
        &ux_hash_blind_signing_text1_step,
        &ux_hash_blind_signing_text2_step,
        &ux_hash_blind_signing_link_step,
        &ux_hash_blind_signing_accept_step,
        &ux_hash_blind_signing_reject_step);

static void start_review_flow() {
    ux_flow_init(0, ux_hash_signing_flow, NULL);
}

int ui_display_hash() {
    if (G_context.req_type != CONFIRM_HASH || G_context.state != STATE_NONE) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }

    explicit_bzero(G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH);

    if (!format_hex(G_context.hash, HASH_SIZE, G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH)) {
        return io_send_sw(SW_DISPLAY_TRANSACTION_HASH_FAIL);
    }

    g_validate_callback = &ui_action_validate_transaction;

    ux_flow_init(0, ux_hash_blind_signing_flow, NULL);

    return 0;
}
#endif
