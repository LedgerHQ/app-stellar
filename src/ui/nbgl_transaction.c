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

#include "os.h"
#include "nbgl_use_case.h"

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
#include "stellar/parser.h"

// The amount of data may be very large, in order to avoid insufficient memory, we load a certain
// amount of data at most once.
#define MAX_DATA_IN_SINGLE_FORMAT 32

// Globals
static char str_values[MAX_DATA_IN_SINGLE_FORMAT][DETAIL_VALUE_MAX_LENGTH];
static char str_captions[MAX_DATA_IN_SINGLE_FORMAT][DETAIL_CAPTION_MAX_LENGTH];
static nbgl_contentTagValue_t pairs[MAX_DATA_IN_SINGLE_FORMAT];
static nbgl_contentTagValueList_t pairs_list;
static uint32_t displayed_data = 0;
static formatter_data_t formatter_data;

static uint32_t more_data_to_send(void);
static void review_prepare(void);
static void review_start(void);
static void review_continue(bool ask_more);
static void review_choice(bool confirm);

static uint32_t more_data_to_send() {
    reset_formatter();
    // the index of the current data
    uint32_t current_data_index = 0;
    // the number of data pairs filled in this round
    uint32_t filled_count = 0;

    while (true) {
        bool data_exists = true;
        bool is_op_header = false;

        if (!get_next_data(&formatter_data, true, &data_exists, &is_op_header)) {
            THROW(SW_FORMATTING_FAIL);
        }

        // if there is no more data to display, we break
        if (!data_exists) {
            break;
        }

        // if we are checking the tx data, and it has more than one operation, and it is the op
        // header (Operation i of n), we break
        if (filled_count != 0 && G_context.envelope.type != ENVELOPE_TYPE_SOROBAN_AUTHORIZATION &&
            G_context.envelope.tx_details.tx.operations_count > 1 && is_op_header) {
            break;
        }

        // if the current data index is less than the displayed data index, we skip it
        if (current_data_index < displayed_data) {
            current_data_index++;
            continue;
        }

        // if we have already full filled the data, we break
        if (filled_count >= MAX_DATA_IN_SINGLE_FORMAT) {
            break;
        }

        strlcpy(str_captions[filled_count],
                G.ui.detail_caption,
                sizeof(str_captions[filled_count]));
        strlcpy(str_values[filled_count], G.ui.detail_value, sizeof(str_values[filled_count]));

        filled_count++;
        current_data_index++;
    }

    displayed_data += filled_count;
    return filled_count;
}

static void review_choice(bool confirm) {
    // display a status page and go back to main
    if (G_context.envelope.type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
        if (confirm) {
            nbgl_useCaseStatus("Soroban Auth signed", true, ui_idle);
        } else {
            nbgl_useCaseStatus("Soroban Auth rejected", false, ui_idle);
        }
    } else {
        nbgl_useCaseReviewStatus(
            confirm ? STATUS_TYPE_TRANSACTION_SIGNED : STATUS_TYPE_TRANSACTION_REJECTED,
            ui_idle);
    }
    validate_transaction(confirm);
}

static void review_continue(bool ask_more) {
    if (ask_more) {
        uint32_t pair_cnt = more_data_to_send();

        if (pair_cnt != 0) {
            for (uint32_t i = 0; i < pair_cnt; i++) {
                pairs[i].item = str_captions[i];
                pairs[i].value = str_values[i];
            }
            pairs_list.nbPairs = pair_cnt;
            pairs_list.pairs = pairs;
            nbgl_useCaseReviewStreamingContinue(&pairs_list, review_continue);
        } else {
            if (formatter_data.envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
                nbgl_useCaseReviewStreamingFinish("Sign Soroban Auth?", review_choice);
            } else {
                nbgl_useCaseReviewStreamingFinish("Sign transaction?", review_choice);
            }
        }
    } else {
        review_choice(false);
    }
}

static void review_start(void) {
    nbgl_operationType_t op_type = TYPE_TRANSACTION;
    if (G_context.unverified_contracts) {
        op_type |= BLIND_OPERATION;
    }

    if (formatter_data.envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
        if (G_context.unverified_contracts) {
            nbgl_useCaseReviewStreamingBlindSigningStart(op_type,
                                                         &C_icon_stellar_64px,
                                                         "Review Soroban Auth",
                                                         NULL,
                                                         review_continue);
        } else {
            nbgl_useCaseReviewStreamingStart(op_type,
                                             &C_icon_stellar_64px,
                                             "Review Soroban Auth",
                                             NULL,
                                             review_continue);
        }
    } else {
        if (G_context.unverified_contracts) {
            nbgl_useCaseReviewStreamingBlindSigningStart(op_type,
                                                         &C_icon_stellar_64px,
                                                         "Review transaction",
                                                         NULL,
                                                         review_continue);
        } else {
            nbgl_useCaseReviewStreamingStart(op_type,
                                             &C_icon_stellar_64px,
                                             "Review transaction",
                                             NULL,
                                             review_continue);
        }
    }
}

static void review_prepare() {
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

    // init formatter_data
    memcpy(&formatter_data, &fdata, sizeof(formatter_data_t));
    explicit_bzero(&pairs, sizeof(pairs));
    explicit_bzero(&pairs_list, sizeof(pairs_list));
    displayed_data = 0;
}

int ui_display_transaction(void) {
    if (G_context.req_type != CONFIRM_TRANSACTION || G_context.state != STATE_PARSED) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }
    review_prepare();
    review_start();
    return 0;
}

int ui_display_auth() {
    if (G_context.req_type != CONFIRM_SOROBAN_AUTHORIZATION || G_context.state != STATE_PARSED) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }
    review_prepare();
    review_start();
    return 0;
}
#endif  // HAVE_NBGL
