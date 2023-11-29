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
static const char *NETWORK_NAMES[3] = {"Public", "Testnet", "Unknown"};

// Step with icon and text
UX_STEP_NOCB(ux_soroban_auth_signing_review_step,
             pnn,
             {
                 &C_icon_eye,
                 "Review",
                 "Soroban Auth",
             });
// what we're doing here is a little more complicated due to the need to reduce memory usage
UX_STEP_INIT(ux_soroban_auth_init_upper_border, NULL, NULL, { display_next_state(true); });
UX_STEP_NOCB(ux_soroban_auth_variable_display,
             bnnn_paging,
             {
                 .title = G.ui.detail_caption,
                 .text = G.ui.detail_value,
             });
UX_STEP_INIT(ux_soroban_auth_init_lower_border, NULL, NULL, { display_next_state(false); });
// Step with approve button
UX_STEP_CB(ux_soroban_auth_display_approve_step,
           pb,
           (*G.ui.validate_callback)(true),
           {
               &C_icon_validate_14,
               "Finalize",
           });
// Step with reject button
UX_STEP_CB(ux_soroban_auth_display_reject_step,
           pb,
           (*G.ui.validate_callback)(false),
           {
               &C_icon_crossmark,
               "Reject",
           });
// FLOW to display hash signing
// #1 screen: eye icon + "Soroban Auth"
// #2 screen: auth info
// #3 screen: approve button
// #4 screen: reject button
UX_FLOW(ux_soroban_auth_signing_flow,
        &ux_soroban_auth_signing_review_step,
        &ux_soroban_auth_init_upper_border,
        &ux_soroban_auth_variable_display,
        &ux_soroban_auth_init_lower_border,
        &ux_soroban_auth_display_approve_step,
        &ux_soroban_auth_display_reject_step);

static bool get_next_data(char *caption, char *value, bool forward) {
    explicit_bzero(caption, sizeof(caption));
    explicit_bzero(value, sizeof(value));
    if (forward) {
        G.ui.current_data_index++;
    } else {
        G.ui.current_data_index--;
    }
    if (G_context.auth.function_type == SOROBAN_AUTHORIZED_FUNCTION_TYPE_CREATE_CONTRACT_HOST_FN) {
        switch (G.ui.current_data_index) {
            case 1:
                STRLCPY(caption, "Soroban", DETAIL_CAPTION_MAX_LENGTH);
                STRLCPY(value, "Create Contract", DETAIL_CAPTION_MAX_LENGTH);
                break;
            case 2:
                STRLCPY(caption, "Network", DETAIL_CAPTION_MAX_LENGTH);
                STRLCPY(value,
                        (char *) PIC(NETWORK_NAMES[G_context.auth.network]),
                        DETAIL_VALUE_MAX_LENGTH);
                break;
            default:
                return false;
        }
    } else {
        switch (G.ui.current_data_index) {
            case 1:
                STRLCPY(caption, "Contract ID", DETAIL_CAPTION_MAX_LENGTH);
                FORMATTER_CHECK(print_sc_address(&G_context.auth.invoke_contract_args.address,
                                                 value,
                                                 DETAIL_VALUE_MAX_LENGTH,
                                                 0,
                                                 0))
                break;
            case 2:
                STRLCPY(caption, "Function", DETAIL_CAPTION_MAX_LENGTH);
                STRLCPY(value,
                        (char *) G_context.auth.invoke_contract_args.function_name.text,
                        DETAIL_VALUE_MAX_LENGTH);
                break;
            case 3:
                STRLCPY(caption, "Network", DETAIL_CAPTION_MAX_LENGTH);
                STRLCPY(value,
                        (char *) PIC(NETWORK_NAMES[G_context.auth.network]),
                        DETAIL_VALUE_MAX_LENGTH);
                break;
            case 4:
                STRLCPY(caption, "Nonce", DETAIL_CAPTION_MAX_LENGTH);
                FORMATTER_CHECK(print_uint(G_context.auth.nonce, value, DETAIL_VALUE_MAX_LENGTH))
                break;
            case 5:
                STRLCPY(caption, "Sig Exp Ledger", DETAIL_CAPTION_MAX_LENGTH);
                FORMATTER_CHECK(
                    print_uint(G_context.auth.signature_exp_ledger, value, DETAIL_VALUE_MAX_LENGTH))
                break;
            default:
                break;
        }

        if (G.ui.current_data_index < 6) {
            return true;
        }

        switch (G_context.auth.invoke_contract_args.contract_type) {
            case SOROBAN_CONTRACT_TYPE_UNVERIFIED:
                switch (G.ui.current_data_index) {
                    case 6:
                        STRLCPY(caption, "RISK WARNING", DETAIL_CAPTION_MAX_LENGTH);
                        STRLCPY(value,
                                "Unverified contract, will not display details",
                                DETAIL_VALUE_MAX_LENGTH);
                        break;
                    case 7:
                        STRLCPY(caption, "Hash", DETAIL_CAPTION_MAX_LENGTH);
                        FORMATTER_CHECK(
                            format_hex(G_context.hash, 32, value, DETAIL_VALUE_MAX_LENGTH))
                        break;
                    default:
                        return false;
                }
                break;
            case SOROBAN_CONTRACT_TYPE_ASSET_APPROVE:
                switch (G.ui.current_data_index) {
                    case 6: {
                        STRLCPY(caption, "Spender", DETAIL_CAPTION_MAX_LENGTH);
                        FORMATTER_CHECK(print_sc_address(
                            &G_context.auth.invoke_contract_args.asset_approve.spender,
                            value,
                            DETAIL_VALUE_MAX_LENGTH,
                            0,
                            0))
                        break;
                    }
                    case 7: {
                        STRLCPY(caption, "From", DETAIL_CAPTION_MAX_LENGTH);
                        FORMATTER_CHECK(print_sc_address(
                            &G_context.auth.invoke_contract_args.asset_approve.from,
                            value,
                            DETAIL_VALUE_MAX_LENGTH,
                            0,
                            0))
                        break;
                    }
                    case 8: {
                        STRLCPY(caption, "Amount", DETAIL_CAPTION_MAX_LENGTH);
                        char tmp[DETAIL_VALUE_MAX_LENGTH];
                        explicit_bzero(tmp, DETAIL_VALUE_MAX_LENGTH);
                        FORMATTER_CHECK(
                            print_amount(G_context.auth.invoke_contract_args.asset_approve.amount,
                                         NULL,
                                         G_context.auth.network,
                                         tmp,
                                         DETAIL_VALUE_MAX_LENGTH))
                        STRLCAT(tmp, " ", DETAIL_VALUE_MAX_LENGTH);
                        STRLCAT(tmp,
                                G_context.auth.invoke_contract_args.asset_approve.asset_code,
                                DETAIL_VALUE_MAX_LENGTH);
                        STRLCPY(value, tmp, DETAIL_VALUE_MAX_LENGTH);
                        break;
                    }
                    case 9: {
                        STRLCPY(caption, "Approve Exp Ledger", DETAIL_CAPTION_MAX_LENGTH);
                        FORMATTER_CHECK(print_uint(
                            G_context.auth.invoke_contract_args.asset_approve.expiration_ledger,
                            value,
                            DETAIL_VALUE_MAX_LENGTH))
                        break;
                    }
                    default:
                        return false;
                }
                break;
            case SOROBAN_CONTRACT_TYPE_ASSET_TRANSFER:
                switch (G.ui.current_data_index) {
                    case 6: {
                        STRLCPY(caption, "Transfer", DETAIL_CAPTION_MAX_LENGTH);
                        char tmp[DETAIL_VALUE_MAX_LENGTH];
                        explicit_bzero(tmp, DETAIL_VALUE_MAX_LENGTH);
                        FORMATTER_CHECK(
                            print_amount(G_context.auth.invoke_contract_args.asset_transfer.amount,
                                         NULL,
                                         G_context.auth.network,
                                         tmp,
                                         DETAIL_VALUE_MAX_LENGTH))
                        STRLCAT(tmp, " ", DETAIL_VALUE_MAX_LENGTH);
                        STRLCAT(tmp,
                                G_context.auth.invoke_contract_args.asset_transfer.asset_code,
                                DETAIL_VALUE_MAX_LENGTH);
                        STRLCPY(value, tmp, DETAIL_VALUE_MAX_LENGTH);
                        break;
                    }
                    case 7: {
                        STRLCPY(caption, "From", DETAIL_CAPTION_MAX_LENGTH);
                        FORMATTER_CHECK(print_sc_address(
                            &G_context.auth.invoke_contract_args.asset_transfer.from,
                            value,
                            DETAIL_VALUE_MAX_LENGTH,
                            0,
                            0))
                        break;
                    }
                    case 8: {
                        STRLCPY(caption, "To", DETAIL_CAPTION_MAX_LENGTH);
                        FORMATTER_CHECK(
                            print_sc_address(&G_context.auth.invoke_contract_args.asset_transfer.to,
                                             value,
                                             DETAIL_VALUE_MAX_LENGTH,
                                             0,
                                             0))
                        break;
                    }
                    default:
                        return false;
                }
                break;
        }
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

int ui_approve_soroban_auth_init() {
    if (G_context.req_type != CONFIRM_SOROBAN_AUTHORATION || G_context.state != STATE_PARSED) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }
    PRINTF("parse_auth, network=%d\n", G_context.auth.network);

    G.ui.current_state = OUT_OF_BORDERS;
    G.ui.current_data_index = 0;
    G.ui.validate_callback = &ui_action_validate_transaction;
    ux_flow_init(0, ux_soroban_auth_signing_flow, NULL);
    return 0;
}
#endif  // HAVE_BAGL
