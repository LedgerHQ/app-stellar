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
#define TAG_VAL_LST_PAIR_NB 2

// Globals
static char str_values[TAG_VAL_LST_PAIR_NB][DETAIL_VALUE_MAX_LENGTH];
static nbgl_pageInfoLongPress_t infoLongPress;
static nbgl_layoutTagValue_t caption_value_pairs[TAG_VAL_LST_PAIR_NB];
static nbgl_layoutTagValueList_t pairList;

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

    // Address caption/value preparation.
    caption_value_pairs[0].item = "Address";
    if (!encode_ed25519_public_key(G_context.raw_public_key,
                                   str_values[0],
                                   DETAIL_VALUE_MAX_LENGTH)) {
        io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
        return;
    }
    caption_value_pairs[0].value = str_values[0];

    // Hash caption/value preparation.
    caption_value_pairs[1].item = "Hash";
    if (!format_hex(G_context.hash, 32, str_values[1], DETAIL_VALUE_MAX_LENGTH)) {
        io_send_sw(SW_DISPLAY_TRANSACTION_HASH_FAIL);
        return;
    }
    caption_value_pairs[1].value = str_values[1];
}

static void rejectConfirmation(void) {
    ui_action_validate_transaction(false);
    nbgl_useCaseStatus("Transaction\nRejected", false, ui_menu_main);
}

static void rejectChoice(void) {
    nbgl_useCaseConfirm("Reject transaction?",
                        NULL,
                        "Yes, Reject",
                        "Go back to transaction",
                        rejectConfirmation);
}

static void reviewChoice(bool confirm) {
    if (confirm) {
        ui_action_validate_transaction(true);
        nbgl_useCaseStatus("TRANSACTION\nSIGNED", true, ui_menu_main);
    } else {
        rejectChoice();
    }
}

static void reviewStart(void) {
    nbgl_useCaseReviewStart(&C_icon_stellar_64px,
                            "Review transaction",
                            "",
                            "Reject transaction",
                            reviewWarning,
                            rejectChoice);
}

static void reviewWarning(void) {
    nbgl_useCaseReviewStart(NULL,
                            "WARNING",
                            "Hash signing",
                            "Reject transaction",
                            reviewContinue,
                            rejectChoice);
}

static void reviewContinue(void) {
    pairList.pairs = caption_value_pairs;
    pairList.nbPairs = TAG_VAL_LST_PAIR_NB;

    infoLongPress.text = "Sign transaction?";
    infoLongPress.icon = &C_icon_stellar_64px;
    infoLongPress.longPressText = "Hold to sign";
    infoLongPress.longPressToken = 0;
    infoLongPress.tuneId = TUNE_TAP_CASUAL;

    nbgl_useCaseStaticReview(&pairList, &infoLongPress, "Reject transaction", reviewChoice);
}

int ui_approve_tx_hash_init() {
    if (G_context.req_type != CONFIRM_TRANSACTION_HASH || G_context.state != STATE_NONE) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }
    preparePage();
    reviewStart();
    return 0;
}
#endif  // HAVE_NBGL
