/*****************************************************************************
 *   Ledger Stellar App.
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
#ifdef HAVE_NBGL
#include <stdbool.h>  // bool
#include <string.h>   // memset
#include "format.h"

#include "nbgl_use_case.h"

#include "display.h"
#include "globals.h"
#include "sw.h"
#include "stellar/printer.h"
#include "action/validate.h"

// Macros
#define TAG_VAL_LST_PAIR_NB 1

// Globals
static char str_values[TAG_VAL_LST_PAIR_NB][DETAIL_VALUE_MAX_LENGTH];
static nbgl_layoutTagValue_t caption_value_pairs[TAG_VAL_LST_PAIR_NB];
static nbgl_layoutTagValueList_t pair_list;

// Static functions declarations
static void review_start(void);
static void review_choice(bool confirm);

// Functions definitions
static void prepare_page(void) {
    explicit_bzero(caption_value_pairs, sizeof(caption_value_pairs));
    explicit_bzero(str_values, sizeof(str_values));

    // Hash caption/value preparation.
    caption_value_pairs[0].item = "Hash";
    if (!format_hex(G_context.hash, HASH_SIZE, str_values[0], DETAIL_VALUE_MAX_LENGTH)) {
        io_send_sw(SW_DISPLAY_TRANSACTION_HASH_FAIL);
        return;
    }
    caption_value_pairs[0].value = str_values[0];

    pair_list.pairs = caption_value_pairs;
    pair_list.nbPairs = TAG_VAL_LST_PAIR_NB;
}

static void review_choice(bool confirm) {
    // Answer, display a status page and go back to main
    if (confirm) {
        nbgl_useCaseStatus("Hash signed", true, ui_idle);
    } else {
        nbgl_useCaseStatus("Hash rejected", false, ui_idle);
    }
    validate_transaction(confirm);
}

static void review_start(void) {
    nbgl_operationType_t op_type = TYPE_TRANSACTION;
    nbgl_useCaseReviewBlindSigning(op_type,
                                   &pair_list,
                                   &ICON_APP_HOME,
                                   "Review hash signing",
                                   NULL,
                                   "Sign hash?",
                                   NULL,
                                   review_choice);
}

int ui_display_hash() {
    if (G_context.req_type != CONFIRM_HASH || G_context.state != STATE_NONE) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }

    prepare_page();
    review_start();
    return 0;
}
#endif  // HAVE_NBGL
