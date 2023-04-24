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
#include "utils.h"
#include "sw.h"
#include "io.h"
#include "transaction_parser.h"
#include "transaction_formatter.h"
#include "nbgl_page.h"
#include "nbgl_use_case.h"
#include "settings.h"

// Macros
#define TAG_VAL_LST_MAX_LINES_PER_PAGE     10
#define TAG_VAL_LST_MAX_PAIR_NB            TAG_VAL_LST_MAX_LINES_PER_PAGE / 2
#define TAG_VAL_LST_ITEM_MAX_CHAR_PER_LINE 31
#define TAG_VAL_LST_VAL_MAX_CHAR_PER_LINE  17
#define TAG_VAL_LST_VAL_MAX_LEN_PER_PAGE                                 \
    TAG_VAL_LST_VAL_MAX_CHAR_PER_LINE *(TAG_VAL_LST_MAX_LINES_PER_PAGE - \
                                        1)  // -1 because at least one line is used by a tag item.
#define MAX_NUMBER_OF_PAGES 40
// Enums and Structs
typedef struct {
    uint8_t pagePairNb;
    bool centered_info;
    uint8_t formatter_index;
    uint8_t data_idx;
} page_infos_t;

// Globals
static uint8_t nbPages;
static int16_t currentPage;
nbgl_layoutTagValue_t caption_value_pairs[TAG_VAL_LST_MAX_PAIR_NB];
static char str_values[TAG_VAL_LST_MAX_PAIR_NB][DETAIL_VALUE_MAX_LENGTH];
static char str_captions[TAG_VAL_LST_MAX_PAIR_NB][DETAIL_CAPTION_MAX_LENGTH];
static page_infos_t pagesInfos[MAX_NUMBER_OF_PAGES];

static void reviewContinue(void);
static void reviewStart(void);
static void rejectConfirmation(void);
static void rejectChoice(void);

// Functions definitions
static inline void INCR_AND_CHECK_PAGE_NB(void) {
    nbPages++;
    if (nbPages >= MAX_NUMBER_OF_PAGES) {
        THROW(SW_TX_FORMATTING_FAIL);
    }
}

static void prepareTxPagesInfos(void) {
    uint8_t tagLineNb = 0;
    uint8_t tagItemLineNb = 0;
    uint8_t tagValueLineNb = 0;
    uint8_t pageLineNb = 0;
    uint16_t fieldLen = 0;
    bool continue_loop = true;
    uint8_t previous_idx = 0;
    uint8_t previous_data = 0;

    // Reset globals.
    nbPages = 0;
    G.ui.current_data_index = 1;
    G_context.tx_info.offset = 0;
    formatter_index = 0;
    explicit_bzero(formatter_stack, sizeof(formatter_stack));
    explicit_bzero(pagesInfos, sizeof(pagesInfos));
    pagesInfos[0].data_idx = G.ui.current_data_index;

    // Prepare formatter stack.
    formatter_stack[0] = get_formatter(&G_context.tx_info, true);  // SET FORMATTERS STACK

    while (continue_loop) {  // Execute loop until last tx formatter is reached.
        explicit_bzero(G.ui.detail_caption, sizeof(G.ui.detail_caption));
        explicit_bzero(G.ui.detail_value, sizeof(G.ui.detail_value));
        explicit_bzero(op_caption, sizeof(op_caption));
        previous_idx = formatter_index;
        previous_data = G.ui.current_data_index;
        // Call formatter function.
        formatter_stack[formatter_index](&G_context.tx_info);
        PRINTF("Page %d - Item : %s - Value : %s\n",
               nbPages,
               G.ui.detail_caption,
               G.ui.detail_value);
        // Compute number of lines filled by tag item string.
        fieldLen = strlen(G.ui.detail_caption);
        tagItemLineNb = fieldLen / TAG_VAL_LST_ITEM_MAX_CHAR_PER_LINE;
        tagItemLineNb += (fieldLen % TAG_VAL_LST_ITEM_MAX_CHAR_PER_LINE != 0) ? 1 : 0;
        // Compute number of lines filled by tag value string.
        fieldLen = strlen(G.ui.detail_value);
        tagValueLineNb = fieldLen / TAG_VAL_LST_VAL_MAX_CHAR_PER_LINE;
        tagValueLineNb += (fieldLen % TAG_VAL_LST_VAL_MAX_CHAR_PER_LINE != 0) ? 1 : 0;
        // Add number of screen lines occupied by tag pair to total lines occupied in page.
        tagLineNb = tagValueLineNb + tagItemLineNb;
        pageLineNb += tagLineNb;
        // If there are multiple operations and a new operation is reached, create a
        // special page with only one caption/value pair to display operation number.
        if (G.ui.current_data_index > previous_data &&
            G_context.tx_info.tx_details.operations_count > 1) {
            INCR_AND_CHECK_PAGE_NB();
            pagesInfos[nbPages].pagePairNb = 1;
            pagesInfos[nbPages].formatter_index = previous_idx;
            pagesInfos[nbPages].data_idx = previous_data;
            pagesInfos[nbPages].centered_info = true;
            INCR_AND_CHECK_PAGE_NB();
            pageLineNb = 0;
            pagesInfos[nbPages].pagePairNb = 0;
            pagesInfos[nbPages].formatter_index = formatter_index + 1;
            pagesInfos[nbPages].data_idx = G.ui.current_data_index;
        }
        // Else if number of lines occupied on page > allowed max number of lines per page,
        // go to next page.
        else if (pageLineNb > TAG_VAL_LST_MAX_LINES_PER_PAGE) {
            INCR_AND_CHECK_PAGE_NB();
            pageLineNb = tagLineNb;
            pagesInfos[nbPages].pagePairNb = 1;
            pagesInfos[nbPages].formatter_index = formatter_index;
            pagesInfos[nbPages].data_idx = G.ui.current_data_index;
        } else
        // Otherwise save number of pairs on current page
        {
            pagesInfos[nbPages].pagePairNb++;
        }
        formatter_index++;
        continue_loop = (formatter_stack[formatter_index] != NULL) ? true : false;
    }

    INCR_AND_CHECK_PAGE_NB();
}

static void preparePage(uint8_t page) {
    uint8_t i = 0;
    // Rewind transaction from the start until the page's
    // operation and formatter indexes are reached.
    // Execution time is probably not optimal but it works...
    formatter_index = 0;
    G_context.tx_info.offset = 0;
    G_context.tx_info.tx_details.operation_index = 0;
    G.ui.current_data_index = pagesInfos[page].data_idx;

    explicit_bzero(caption_value_pairs, sizeof(caption_value_pairs));
    explicit_bzero(str_values, sizeof(str_values));
    explicit_bzero(str_captions, sizeof(str_captions));

    formatter_stack[0] = get_formatter(&G_context.tx_info, true);
    // Loop which goes through the formatter functions
    // from tx start.
    for (i = 0; i < pagesInfos[page].formatter_index; i++) {
        explicit_bzero(G.ui.detail_caption, sizeof(G.ui.detail_caption));
        explicit_bzero(G.ui.detail_value, sizeof(G.ui.detail_value));
        formatter_stack[formatter_index](&G_context.tx_info);
        formatter_index++;
    }
    // Prepare current page's caption / value pairs
    // to be displayed.
    for (i = 0; i < pagesInfos[page].pagePairNb; i++) {
        explicit_bzero(G.ui.detail_caption, sizeof(G.ui.detail_caption));
        explicit_bzero(G.ui.detail_value, sizeof(G.ui.detail_value));
        explicit_bzero(op_caption, sizeof(op_caption));
        formatter_stack[formatter_index](&G_context.tx_info);
        strncpy(str_captions[i], G.ui.detail_caption, sizeof(str_captions[i]));
        strncpy(str_values[i], G.ui.detail_value, sizeof(str_values[i]));
        caption_value_pairs[i].item = str_captions[i];
        caption_value_pairs[i].value = str_values[i];

        formatter_index++;
    }
}

static bool displayTransactionPage(uint8_t page, nbgl_pageContent_t *content) {
    currentPage = page;
    if (page < nbPages) {
        preparePage(page);
        if (pagesInfos[page].centered_info) {
            content->type = CENTERED_INFO;
            content->centeredInfo.style = LARGE_CASE_INFO;
            content->centeredInfo.text1 = "Please review";
            content->centeredInfo.text2 = caption_value_pairs[0].item;
            content->centeredInfo.text3 = NULL;
            content->centeredInfo.icon = &C_icon_stellar_64px;
            content->centeredInfo.offsetY = 35;
            content->centeredInfo.onTop = false;
        } else {
            content->type = TAG_VALUE_LIST;
            content->tagValueList.nbPairs = pagesInfos[page].pagePairNb;
            content->tagValueList.pairs = (nbgl_layoutTagValue_t *) &caption_value_pairs;
            content->tagValueList.smallCaseForValue = false;
            content->tagValueList.wrapping = true;
            content->tagValueList.nbMaxLinesForValue = 0;
        }
    } else {
        content->type = INFO_LONG_PRESS, content->infoLongPress.icon = &C_icon_stellar_64px;
        content->infoLongPress.text = "Finalize transaction";
        content->infoLongPress.longPressText = "Hold to sign";
    }
    return true;
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

static void reviewContinue(void) {
    nbgl_useCaseRegularReview(currentPage,
                              nbPages + 1,
                              "Reject transaction",
                              NULL,
                              displayTransactionPage,
                              reviewChoice);
}

static void reviewStart(void) {
    nbgl_useCaseReviewStart(&C_icon_stellar_64px,
                            "Review transaction",
                            NULL,
                            "Reject transaction",
                            reviewContinue,
                            rejectChoice);
}

int ui_approve_tx_init(void) {
    if (G_context.req_type != CONFIRM_TRANSACTION || G_context.state != STATE_PARSED) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }

    currentPage = 0;
    prepareTxPagesInfos();
    reviewStart();

    return 0;
}
#endif  // HAVE_NBGL
