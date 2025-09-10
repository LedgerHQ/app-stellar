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
#include "settings.h"
#include "plugin.h"
#include "action/validate.h"
#include "stellar/printer.h"
#include "stellar/formatter.h"
#include "stellar/printer.h"

static action_validate_cb g_validate_callback;
static bool data_exists;
static formatter_data_t formatter_data;

// Validate/Invalidate transaction and go back to home
static void ui_action_validate_transaction(bool choice) {
    validate_transaction(choice);
    ui_idle();
}

static void bnnn_paging_edgecase() {
    G_ux.flow_stack[G_ux.stack_count - 1].prev_index =
        G_ux.flow_stack[G_ux.stack_count - 1].index - 2;
    G_ux.flow_stack[G_ux.stack_count - 1].index--;
    ux_flow_relayout();
}

// Main function that handles all the business logic for our new display architecture.
static void display_next_state(bool is_upper_delimiter) {
    bool is_op_header;
    if (is_upper_delimiter) {  // We're called from the upper delimiter.
        if (G.ui.current_state == STATIC_SCREEN) {
            // Fetch new data.
            if (!get_next_data(&formatter_data, true, &data_exists, &is_op_header)) {
                THROW(SW_FORMATTING_FAIL);
            };
            if (data_exists) {
                // We found some data to display so we now enter in dynamic mode.
                G.ui.current_state = DYNAMIC_SCREEN;
            }

            // Move to the next step, which will display the screen.
            ux_flow_next();
        } else {
            // The previous screen was NOT a static screen, so we were already in a dynamic screen.

            // Fetch new data.
            if (!get_next_data(&formatter_data, false, &data_exists, &is_op_header)) {
                THROW(SW_FORMATTING_FAIL);
            };
            if (data_exists) {
                // We found some data so simply display it.
                ux_flow_next();
            } else {
                // There's no more dynamic data to display, so
                // update the current state accordingly.
                G.ui.current_state = STATIC_SCREEN;

                // Display the previous screen which should be a static one.
                ux_flow_prev();
            }
        }
    } else {
        // We're called from the lower delimiter.

        if (G.ui.current_state == STATIC_SCREEN) {
            // Fetch new data.
            if (!get_next_data(&formatter_data, false, &data_exists, &is_op_header)) {
                THROW(SW_FORMATTING_FAIL);
            };
            if (data_exists) {
                // We found some data to display so enter in dynamic mode.
                G.ui.current_state = DYNAMIC_SCREEN;
            }

            // Display the data.
            ux_flow_prev();
        } else {
            // We're being called from a dynamic screen, so the user was already browsing the array.

            // Fetch new data.
            if (!get_next_data(&formatter_data, true, &data_exists, &is_op_header)) {
                THROW(SW_FORMATTING_FAIL);
            };
            if (data_exists) {
                // We found some data, so display it.
                // Similar to `ux_flow_prev()` but updates layout to account for `bnnn_paging`'s
                // weird behaviour.
                bnnn_paging_edgecase();
            } else {
                // We found no data so make sure we update the state accordingly.
                G.ui.current_state = STATIC_SCREEN;

                // Display the next screen
                ux_flow_next();
            }
        }
    }
}

// Step with icon and text
UX_STEP_NOCB(ux_tx_review_step,
             pnn,
             {
                 &C_icon_eye,
                 "Review",
                 "Transaction",
             });
UX_STEP_NOCB(ux_auth_review_step,
             pnn,
             {
                 &C_icon_eye,
                 "Review",
                 "Soroban Auth",
             });
UX_STEP_INIT(ux_tx_upper_delimiter, NULL, NULL, {
    // This function will be detailed later on.
    display_next_state(true);
});
UX_STEP_NOCB(ux_tx_generic,
             bnnn_paging,
             {
                 .title = G.ui.detail_caption,
                 .text = G.ui.detail_value,
             });
// Note we're using UX_STEP_INIT because this step won't display anything.
UX_STEP_INIT(ux_tx_lower_delimiter, NULL, NULL, {
    // This function will be detailed later on.
    display_next_state(false);
});

// Step for blind signing warning at the beginning of the flow
UX_STEP_NOCB(ux_tx_and_auth_blind_signing_reminder_step,
             pbb,
             {
                 &C_icon_warning,
                 "Blind",
                 "signing",
             });
// Step for blind signing warning at the signing step
UX_STEP_CB(ux_tx_and_auth_blind_signing_approve_step,
           pbb,
           (*g_validate_callback)(true),
           {
               &C_icon_validate_14,
               "Accept risk",
               "and sign",
           });

// Step with approve button
UX_STEP_CB(ux_tx_approve_step,
           pbb,
           (*g_validate_callback)(true),
           {
               &C_icon_validate_14,
               "Sign",
               "Transaction",
           });
UX_STEP_CB(ux_auth_approve_step,
           pbb,
           (*g_validate_callback)(true),
           {
               &C_icon_validate_14,
               "Sign",
               "Soroban Auth",
           });
// Step with reject button
UX_STEP_CB(ux_tx_reject_step,
           pb,
           (*g_validate_callback)(false),
           {
               &C_icon_crossmark,
               "Reject",
           });
// FLOW to display transaction information:
// https://developers.ledger.com/docs/device-app/develop/ui/flows/advanced-display-management
UX_FLOW(ux_tx_flow,
        &ux_tx_review_step,
        &ux_tx_upper_delimiter,
        &ux_tx_generic,
        &ux_tx_lower_delimiter,
        &ux_tx_approve_step,
        &ux_tx_reject_step);

UX_FLOW(ux_auth_flow,
        &ux_auth_review_step,
        &ux_tx_upper_delimiter,
        &ux_tx_generic,
        &ux_tx_lower_delimiter,
        &ux_auth_approve_step,
        &ux_tx_reject_step);

UX_FLOW(ux_tx_flow_with_reminder,
        &ux_tx_and_auth_blind_signing_reminder_step,
        &ux_tx_review_step,
        &ux_tx_upper_delimiter,
        &ux_tx_generic,
        &ux_tx_lower_delimiter,
        &ux_tx_and_auth_blind_signing_approve_step,
        &ux_tx_reject_step);

UX_FLOW(ux_auth_flow_with_reminder,
        &ux_tx_and_auth_blind_signing_reminder_step,
        &ux_auth_review_step,
        &ux_tx_upper_delimiter,
        &ux_tx_generic,
        &ux_tx_lower_delimiter,
        &ux_tx_and_auth_blind_signing_approve_step,
        &ux_tx_reject_step);

void prepare_display() {
    formatter_data_t fdata = {
        .raw_data = G_context.raw,
        .raw_data_len = G_context.raw_size,
        .envelope = &G_context.envelope,
        .caption = G.ui.detail_caption,
        .value = G.ui.detail_value,
        .signing_key = G_context.raw_public_key,
        .caption_len = DETAIL_CAPTION_MAX_LENGTH,
        .value_len = DETAIL_VALUE_MAX_LENGTH,
        .display_sequence = HAS_SETTING(S_SEQUENCE_NUMBER_ENABLED),
        .plugin_check_presence = &plugin_check_presence,
        .plugin_init_contract = &plugin_init_contract,
        .plugin_query_data_pair_count = &plugin_query_data_pair_count,
        .plugin_query_data_pair = &plugin_query_data_pair,
    };
    reset_formatter();

    // init formatter_data
    memcpy(&formatter_data, &fdata, sizeof(formatter_data_t));
    // PRINTF("formatter_data.raw_size: %d\n", formatter_data.buffer->size);

    g_validate_callback = &ui_action_validate_transaction;
}

int ui_display_transaction() {
    if (G_context.req_type != CONFIRM_TRANSACTION || G_context.state != STATE_PARSED) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }
    prepare_display();
    if (G_context.unverified_contracts) {
        ux_flow_init(0, ux_tx_flow_with_reminder, NULL);
    } else {
        ux_flow_init(0, ux_tx_flow, NULL);
    }
    return 0;
}

int ui_display_auth() {
    if (G_context.req_type != CONFIRM_SOROBAN_AUTHORIZATION || G_context.state != STATE_PARSED) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }
    prepare_display();
    if (G_context.unverified_contracts) {
        ux_flow_init(0, ux_auth_flow_with_reminder, NULL);
    } else {
        ux_flow_init(0, ux_auth_flow, NULL);
    }
    return 0;
}
#endif
