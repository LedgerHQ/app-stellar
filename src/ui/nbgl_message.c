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

#include "nbgl_use_case.h"

#include "os.h"
#include "display.h"
#include "globals.h"
#include "sw.h"
#include "action/validate.h"
#include "stellar/printer.h"

#define MESSAGE_DISPLAY_MAX_SIZE 1024  // Buffer size

// Shared buffer for message display - used for both standard and streaming review
static char g_message_buffer[MESSAGE_DISPLAY_MAX_SIZE];
static uint16_t g_message_offset;

// Forward declarations
static void review_choice(bool confirm);
static void streaming_continue(bool confirm);
static void streaming_start(bool confirm);

// Prepare a chunk of message for streaming display
// Note: G_context.raw contains raw bytes without null terminator
static void prepare_message_chunk(void) {
    size_t remaining = G_context.raw_size - g_message_offset;

    size_t safe_chunk_size = calculate_safe_chunk_size(G_context.raw + g_message_offset,
                                                       remaining,
                                                       MESSAGE_DISPLAY_MAX_SIZE);

    explicit_bzero(g_message_buffer, MESSAGE_DISPLAY_MAX_SIZE);
    if (!print_raw_bytes(G_context.raw + g_message_offset,
                         safe_chunk_size,
                         g_message_buffer,
                         MESSAGE_DISPLAY_MAX_SIZE)) {
        THROW(SW_FORMATTING_FAIL);
    }
}

// Streaming continue callback
static void streaming_continue(bool confirm) {
    if (confirm) {
        // Move to next chunk of raw data - advance by actual processed bytes
        size_t remaining = G_context.raw_size - g_message_offset;
        size_t current_chunk_size = calculate_safe_chunk_size(G_context.raw + g_message_offset,
                                                              remaining,
                                                              MESSAGE_DISPLAY_MAX_SIZE);
        g_message_offset += current_chunk_size;
        if (g_message_offset < G_context.raw_size) {
            prepare_message_chunk();

            static nbgl_layoutTagValue_t pairs[1];
            pairs[0].item = "Message";
            pairs[0].value = g_message_buffer;

            nbgl_layoutTagValueList_t tagValueList = {.pairs = pairs,
                                                      .nbPairs = 1,
                                                      .wrapping = true};

            nbgl_useCaseReviewStreamingContinue(&tagValueList, streaming_continue);
        } else {
            nbgl_useCaseReviewStreamingFinish("Sign Stellar message", review_choice);
        }
    } else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_MESSAGE_REJECTED, ui_idle);
        validate_message(false);
    }
}

// Streaming start callback
static void streaming_start(bool confirm) {
    if (confirm) {
        prepare_message_chunk();

        static nbgl_layoutTagValue_t pairs[1];
        pairs[0].item = "Message";
        pairs[0].value = g_message_buffer;

        nbgl_layoutTagValueList_t tagValueList = {.pairs = pairs, .nbPairs = 1, .wrapping = true};

        // Check if there will be more content after this first chunk
        size_t first_chunk_size =
            calculate_safe_chunk_size(G_context.raw, G_context.raw_size, MESSAGE_DISPLAY_MAX_SIZE);

        if (first_chunk_size < G_context.raw_size) {
            // More chunks to follow
            nbgl_useCaseReviewStreamingContinue(&tagValueList, streaming_continue);
        } else {
            // Last chunk
            nbgl_useCaseReviewStreamingFinish("Sign Stellar message", review_choice);
        }
    } else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_MESSAGE_REJECTED, ui_idle);
        validate_message(false);
    }
}

static void review_choice(bool confirm) {
    if (confirm) {
        nbgl_useCaseReviewStatus(STATUS_TYPE_MESSAGE_SIGNED, ui_idle);
    } else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_MESSAGE_REJECTED, ui_idle);
    }
    validate_message(confirm);
}

int ui_display_message(void) {
    if (G_context.req_type != CONFIRM_MESSAGE || G_context.state != STATE_NONE) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }

    // Check if entire message can fit in one display buffer
    size_t actual_formatted_size = 0;
    for (size_t i = 0; i < G_context.raw_size; i++) {
        uint8_t byte = G_context.raw[i];
        if (byte >= 0x20 && byte <= 0x7E) {
            actual_formatted_size += 1;  // Printable character
        } else {
            actual_formatted_size += 4;  // Non-printable character (\xHH)
        }
    }

    if (actual_formatted_size <= MESSAGE_DISPLAY_MAX_SIZE - 1) {
        // Short message - use simple standard review
        explicit_bzero(g_message_buffer, sizeof(g_message_buffer));
        if (!print_raw_bytes(G_context.raw,
                             G_context.raw_size,
                             g_message_buffer,
                             sizeof(g_message_buffer) - 1)) {
            THROW(SW_FORMATTING_FAIL);
        }

        static nbgl_layoutTagValue_t pairs[1];
        pairs[0].item = "Message";
        pairs[0].value = g_message_buffer;

        nbgl_layoutTagValueList_t tagValueList = {
            .pairs = pairs,
            .nbPairs = 1,
            .wrapping = true,
            .nbMaxLinesForValue = 0,  // Let NBGL handle automatic pagination
            .token = 0,
            .callback = NULL};

        // Use standard review - NBGL will automatically handle content
        nbgl_useCaseReview(TYPE_MESSAGE,
                           &tagValueList,
                           &C_icon_stellar_64px,
                           "Sign Stellar message",
                           NULL,
                           "Sign message?",
                           review_choice);
    } else {
        // Long message - use streaming review for complete display
        g_message_offset = 0;
        nbgl_useCaseReviewStreamingStart(TYPE_MESSAGE,
                                         &C_icon_stellar_64px,
                                         "Sign Stellar message",
                                         NULL,
                                         streaming_start);
    }

    return 0;
}
#endif  // HAVE_NBGL