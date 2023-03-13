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
#ifdef HAVE_BAGL
#include <stdbool.h>  // bool
#include <string.h>   // memset

#include "./ui.h"
#include "./action/validate.h"
#include "../globals.h"
#include "../sw.h"
#include "../utils.h"
#include "../io.h"
#include "../common/format.h"

static void display_next_state(bool is_upper_delimiter);

// Step with icon and text
UX_STEP_NOCB(ux_tx_hash_signing_review_step,
             pnn,
             {
                 &C_icon_eye,
                 "Review",
                 "Transaction",
             });
UX_STEP_NOCB(ux_tx_hash_signing_warning_step,
             pbb,
             {
                 &C_icon_warning,
                 "Hash",
                 "Signing",
             });
// what we're doing here is a little more complicated due to the need to reduce memory usage
UX_STEP_INIT(ux_tx_init_upper_border, NULL, NULL, { display_next_state(true); });
UX_STEP_NOCB(ux_tx_variable_display,
             bnnn_paging,
             {
                 .title = G.ui.detail_caption,
                 .text = G.ui.detail_value,
             });
UX_STEP_INIT(ux_tx_init_lower_border, NULL, NULL, { display_next_state(false); });
// Step with approve button
UX_STEP_CB(ux_tx_hash_display_approve_step,
           pb,
           (*G.ui.validate_callback)(true),
           {
               &C_icon_validate_14,
               "Approve",
           });
// Step with reject button
UX_STEP_CB(ux_tx_hash_display_reject_step,
           pb,
           (*G.ui.validate_callback)(false),
           {
               &C_icon_crossmark,
               "Reject",
           });
// FLOW to display hash signing
// #1 screen: eye icon + "Review Transaction"
// #1 screen: warning icon + "Hash Signing"
// #2 screen: display address
// #3 screen: display hash
// #4 screen: approve button
// #5 screen: reject button
UX_FLOW(ux_tx_hash_signing_flow,
        &ux_tx_hash_signing_review_step,
        &ux_tx_hash_signing_warning_step,
        &ux_tx_init_upper_border,
        &ux_tx_variable_display,
        &ux_tx_init_lower_border,
        &ux_tx_hash_display_approve_step,
        &ux_tx_hash_display_reject_step);

static bool get_next_data(char *caption, char *value, bool forward) {
    if (forward) {
        G.ui.current_data_index++;
    } else {
        G.ui.current_data_index--;
    }
    switch (G.ui.current_data_index) {
        case 1:
            strlcpy(caption, "Address", DETAIL_CAPTION_MAX_LENGTH);
            if (!encode_ed25519_public_key(G_context.raw_public_key,
                                           value,
                                           DETAIL_VALUE_MAX_LENGTH)) {
                return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
            }
            break;
        case 2:
            strlcpy(caption, "Hash", DETAIL_CAPTION_MAX_LENGTH);
            if (!format_hex(G_context.hash, 32, value, DETAIL_VALUE_MAX_LENGTH)) {
                return io_send_sw(SW_DISPLAY_TRANSACTION_HASH_FAIL);
            }
            break;
        default:
            return false;
    }
    return true;
}

// This is a special function you must call for bnnn_paging to work properly in an edgecase.
// It does some weird stuff with the `G_ux` global which is defined by the SDK.
// No need to dig deeper into the code, a simple copy-paste will do.
static void bnnn_paging_edgecase() {
    G_ux.flow_stack[G_ux.stack_count - 1].prev_index =
        G_ux.flow_stack[G_ux.stack_count - 1].index - 2;
    G_ux.flow_stack[G_ux.stack_count - 1].index--;
    ux_flow_relayout();
}

// Main function that handles all the business logic for our new display architecture.
static void display_next_state(bool is_upper_delimiter) {
    if (is_upper_delimiter) {  // We're called from the upper delimiter.
        if (G.ui.current_state == OUT_OF_BORDERS) {
            // Fetch new data.
            bool dynamic_data = get_next_data(G.ui.detail_caption, G.ui.detail_value, true);

            if (dynamic_data) {
                // We found some data to display so we now enter in dynamic mode.
                G.ui.current_state = INSIDE_BORDERS;
            }

            // Move to the next step, which will display the screen.
            ux_flow_next();
        } else {
            // The previous screen was NOT a static screen, so we were already in a dynamic screen.

            // Fetch new data.
            bool dynamic_data = get_next_data(G.ui.detail_caption, G.ui.detail_value, false);
            if (dynamic_data) {
                // We found some data so simply display it.
                ux_flow_next();
            } else {
                // There's no more dynamic data to display, so
                // update the current state accordingly.
                G.ui.current_state = OUT_OF_BORDERS;

                // Display the previous screen which should be a static one.
                ux_flow_prev();
            }
        }
    } else {
        // We're called from the lower delimiter.

        if (G.ui.current_state == OUT_OF_BORDERS) {
            // Fetch new data.
            bool dynamic_data = get_next_data(G.ui.detail_caption, G.ui.detail_value, false);

            if (dynamic_data) {
                // We found some data to display so enter in dynamic mode.
                G.ui.current_state = INSIDE_BORDERS;
            }

            // Display the data.
            ux_flow_prev();
        } else {
            // We're being called from a dynamic screen, so the user was already browsing the
            // array.

            // Fetch new data.
            bool dynamic_data = get_next_data(G.ui.detail_caption, G.ui.detail_value, true);
            if (dynamic_data) {
                // We found some data, so display it.
                // Similar to `ux_flow_prev()` but updates layout to account for `bnnn_paging`'s
                // weird behaviour.
                bnnn_paging_edgecase();
            } else {
                // We found no data so make sure we update the state accordingly.
                G.ui.current_state = OUT_OF_BORDERS;

                // Display the next screen
                ux_flow_next();
            }
        }
    }
}

int ui_approve_tx_hash_init() {
    if (G_context.req_type != CONFIRM_TRANSACTION_HASH || G_context.state != STATE_NONE) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }
    G.ui.current_state = OUT_OF_BORDERS;
    G.ui.current_data_index = 0;
    G.ui.validate_callback = &ui_action_validate_transaction;
    ux_flow_init(0, ux_tx_hash_signing_flow, NULL);
    return 0;
}
#endif  // HAVE_BAGL
