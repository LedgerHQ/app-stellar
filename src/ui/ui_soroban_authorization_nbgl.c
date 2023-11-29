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
#ifdef HAVE_NBGL
#include <stdbool.h>  // bool
#include <string.h>   // memset

#include "ui.h"
#include "validate.h"
#include "globals.h"
#include "sw.h"
#include "utils.h"
#include "io.h"
#include "format.h"
#include "nbgl_use_case.h"

// Macros
#define TAG_VAL_LST_PAIR_NB 9

// Globals
static char str_values[TAG_VAL_LST_PAIR_NB][DETAIL_VALUE_MAX_LENGTH];
static nbgl_pageInfoLongPress_t infoLongPress;
static nbgl_layoutTagValue_t caption_value_pairs[TAG_VAL_LST_PAIR_NB];
static nbgl_layoutTagValueList_t pairList;
static const char *NETWORK_NAMES[3] = {"Public", "Testnet", "Unknown"};
static uint8_t actual_pair;

// Static functions declarations
static void reviewStart(void);
static void reviewWarning(void);
static void reviewContinue(void);
static void rejectConfirmation(void);
static void rejectChoice(void);

// Functions definitions
static void preparePage(void) {
    explicit_bzero(caption_value_pairs, sizeof(caption_value_pairs));
    explicit_bzero(str_values, sizeof(str_values));

    if (G_context.auth.function_type == SOROBAN_AUTHORIZED_FUNCTION_TYPE_CREATE_CONTRACT_HOST_FN) {
        caption_value_pairs[0].item = "Soroban";
        STRLCPY(str_values[0], "Create Contract", DETAIL_VALUE_MAX_LENGTH);
        caption_value_pairs[0].value = str_values[1];

        caption_value_pairs[1].item = "Network";
        STRLCPY(str_values[1],
                (char *) PIC(NETWORK_NAMES[G_context.auth.network]),
                DETAIL_VALUE_MAX_LENGTH);
        caption_value_pairs[1].value = str_values[1];
        actual_pair = 2;
    } else {
        caption_value_pairs[0].item = "Contract ID";
        FORMATTER_CHECK(print_sc_address(&G_context.auth.invoke_contract_args.address,
                                         str_values[0],
                                         DETAIL_VALUE_MAX_LENGTH,
                                         0,
                                         0))
        caption_value_pairs[0].value = str_values[0];

        caption_value_pairs[1].item = "Function";
        STRLCPY(str_values[1],
                (char *) G_context.auth.invoke_contract_args.function_name.text,
                DETAIL_VALUE_MAX_LENGTH);
        caption_value_pairs[1].value = str_values[1];

        caption_value_pairs[2].item = "Network";
        STRLCPY(str_values[2],
                (char *) PIC(NETWORK_NAMES[G_context.auth.network]),
                DETAIL_VALUE_MAX_LENGTH);
        caption_value_pairs[2].value = str_values[2];

        caption_value_pairs[3].item = "Nonce";
        FORMATTER_CHECK(print_uint(G_context.auth.nonce, str_values[3], DETAIL_VALUE_MAX_LENGTH))
        caption_value_pairs[3].value = str_values[3];

        caption_value_pairs[4].item = "Sig Exp Ledger";
        FORMATTER_CHECK(
            print_uint(G_context.auth.signature_exp_ledger, str_values[4], DETAIL_VALUE_MAX_LENGTH))
        caption_value_pairs[4].value = str_values[4];

        switch (G_context.auth.invoke_contract_args.contract_type) {
            case SOROBAN_CONTRACT_TYPE_UNVERIFIED:
                caption_value_pairs[5].item = "Soroban Auth Hash";
                FORMATTER_CHECK(
                    format_hex(G_context.hash, 32, str_values[5], DETAIL_VALUE_MAX_LENGTH))
                caption_value_pairs[5].value = str_values[5];
                actual_pair = 6;
                break;
            case SOROBAN_CONTRACT_TYPE_ASSET_APPROVE:
                caption_value_pairs[5].item = "Spender";
                FORMATTER_CHECK(
                    print_sc_address(&G_context.auth.invoke_contract_args.asset_approve.spender,
                                     str_values[5],
                                     DETAIL_VALUE_MAX_LENGTH,
                                     0,
                                     0))
                caption_value_pairs[5].value = str_values[5];

                caption_value_pairs[6].item = "From";
                FORMATTER_CHECK(
                    print_sc_address(&G_context.auth.invoke_contract_args.asset_approve.from,
                                     str_values[6],
                                     DETAIL_VALUE_MAX_LENGTH,
                                     0,
                                     0))
                caption_value_pairs[6].value = str_values[6];

                caption_value_pairs[7].item = "Amount";
                print_amount(G_context.auth.invoke_contract_args.asset_approve.amount,
                             NULL,
                             G_context.auth.network,
                             str_values[7],
                             DETAIL_VALUE_MAX_LENGTH);
                STRLCAT(str_values[7], " ", DETAIL_VALUE_MAX_LENGTH);
                STRLCAT(str_values[7],
                        G_context.auth.invoke_contract_args.asset_approve.asset_code,
                        DETAIL_VALUE_MAX_LENGTH);
                caption_value_pairs[7].value = str_values[7];

                caption_value_pairs[8].item = "Approve Expire Ledger";
                print_uint(G_context.auth.invoke_contract_args.asset_approve.expiration_ledger,
                           str_values[8],
                           DETAIL_VALUE_MAX_LENGTH);
                caption_value_pairs[8].value = str_values[8];

                actual_pair = 9;
                break;
            case SOROBAN_CONTRACT_TYPE_ASSET_TRANSFER:
                caption_value_pairs[5].item = "Transfer";
                FORMATTER_CHECK(
                    print_amount(G_context.auth.invoke_contract_args.asset_transfer.amount,
                                 NULL,
                                 G_context.auth.network,
                                 str_values[5],
                                 DETAIL_VALUE_MAX_LENGTH))
                STRLCAT(str_values[5], " ", DETAIL_VALUE_MAX_LENGTH);
                STRLCAT(str_values[5],
                        G_context.auth.invoke_contract_args.asset_transfer.asset_code,
                        DETAIL_VALUE_MAX_LENGTH);
                caption_value_pairs[5].value = str_values[5];

                caption_value_pairs[6].item = "From";
                FORMATTER_CHECK(
                    print_sc_address(&G_context.auth.invoke_contract_args.asset_transfer.from,
                                     str_values[6],
                                     DETAIL_VALUE_MAX_LENGTH,
                                     0,
                                     0))
                caption_value_pairs[6].value = str_values[6];

                caption_value_pairs[7].item = "To";
                FORMATTER_CHECK(
                    print_sc_address(&G_context.auth.invoke_contract_args.asset_transfer.to,
                                     str_values[7],
                                     DETAIL_VALUE_MAX_LENGTH,
                                     0,
                                     0))
                caption_value_pairs[7].value = str_values[7];
                actual_pair = 8;
                break;
        }
    }
}

static void rejectConfirmation(void) {
    ui_action_validate_transaction(false);
    nbgl_useCaseStatus("Soroban Auth\nRejected", false, ui_menu_main);
}

static void rejectChoice(void) {
    nbgl_useCaseConfirm("Reject Soroban Auth?",
                        NULL,
                        "Yes, Reject",
                        "Go back to Soroban Auth",
                        rejectConfirmation);
}

static void reviewChoice(bool confirm) {
    if (confirm) {
        ui_action_validate_transaction(true);
        nbgl_useCaseStatus("SOROBAN AUTH\nSIGNED", true, ui_menu_main);
    } else {
        rejectChoice();
    }
}

static void reviewStart(void) {
    if (G_context.auth.invoke_contract_args.contract_type == SOROBAN_CONTRACT_TYPE_UNVERIFIED) {
        nbgl_useCaseReviewStart(&C_icon_stellar_64px,
                                "Review Soroban Auth",
                                "",
                                "Reject Soroban Auth",
                                reviewWarning,
                                rejectChoice);
    } else {
        nbgl_useCaseReviewStart(&C_icon_stellar_64px,
                                "Review Soroban Auth",
                                "",
                                "Reject Soroban Auth",
                                reviewContinue,
                                rejectChoice);
    }
}

static void reviewWarning(void) {
    nbgl_useCaseReviewStart(NULL,
                            "WARNING",
                            "Unverified contract,\nwill not display details",
                            "Reject transaction",
                            reviewContinue,
                            rejectChoice);
}

static void reviewContinue(void) {
    pairList.pairs = caption_value_pairs;
    pairList.nbPairs = actual_pair;

    infoLongPress.text = "Sign Soroban Auth?";
    infoLongPress.icon = &C_icon_stellar_64px;
    infoLongPress.longPressText = "Hold to sign";
    infoLongPress.longPressToken = 0;
    infoLongPress.tuneId = TUNE_TAP_CASUAL;

    nbgl_useCaseStaticReview(&pairList, &infoLongPress, "Reject Soroban Auth", reviewChoice);
}

int ui_approve_soroban_auth_init() {
    if (G_context.req_type != CONFIRM_SOROBAN_AUTHORATION || G_context.state != STATE_PARSED) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }
    preparePage();
    reviewStart();
    return 0;
}
#endif  // HAVE_NBGL
