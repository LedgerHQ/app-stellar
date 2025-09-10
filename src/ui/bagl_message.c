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
#ifdef HAVE_BAGL
#include <stdbool.h>  // bool
#include <string.h>   // memset

#include "os.h"
#include "ux.h"
#include "glyphs.h"
#include "io.h"
#include "bip32.h"
#include "stellar/printer.h"

#include "display.h"
#include "globals.h"
#include "sw.h"
#include "action/validate.h"

static action_validate_cb g_validate_callback;
static uint16_t g_message_offset;

// Forward declarations of the flows
extern const ux_flow_step_t *const ux_message_display_final_flow[];
extern const ux_flow_step_t *const ux_message_display_continue_flow[];

// Prepare the next chunk of message content
static void prepare_next_message_chunk(void) {
    size_t remaining = G_context.raw_size - g_message_offset;

    size_t safe_chunk_size = calculate_safe_chunk_size(G_context.raw + g_message_offset,
                                                       remaining,
                                                       DETAIL_VALUE_MAX_LENGTH);

    explicit_bzero(G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH);
    if (!print_raw_bytes(G_context.raw + g_message_offset,
                         safe_chunk_size,
                         G.ui.detail_value,
                         DETAIL_VALUE_MAX_LENGTH)) {
        THROW(SW_FORMATTING_FAIL);
    }
}

// Navigate to next message chunk or approval
static void ui_action_message_next(bool choice) {
    UNUSED(choice);

    // Calculate how many bytes were processed in the current chunk
    size_t remaining = G_context.raw_size - g_message_offset;
    size_t current_chunk_size = calculate_safe_chunk_size(G_context.raw + g_message_offset,
                                                          remaining,
                                                          DETAIL_VALUE_MAX_LENGTH);

    g_message_offset += current_chunk_size;

    if (g_message_offset < G_context.raw_size) {
        // Prepare next chunk and display it
        prepare_next_message_chunk();

        // Check if this will be the last chunk
        size_t next_remaining = G_context.raw_size - g_message_offset;
        size_t next_chunk_size = calculate_safe_chunk_size(G_context.raw + g_message_offset,
                                                           next_remaining,
                                                           DETAIL_VALUE_MAX_LENGTH);

        if (g_message_offset + next_chunk_size >= G_context.raw_size) {
            // Last chunk - use final flow
            ux_flow_init(0, ux_message_display_final_flow, NULL);
        } else {
            // More chunks - stay in continuation flow
            ux_flow_init(0, ux_message_display_continue_flow, NULL);
        }
    } else {
        // Should not happen, but handle gracefully
        ux_flow_next();
    }
}

// Validate/Invalidate message and go back to home
static void ui_action_validate_message(bool choice) {
    validate_message(choice);
    ui_idle();
}

UX_STEP_NOCB(ux_message_display_confirm_step, pnn, {&C_icon_eye, "Review", "Message"});

UX_STEP_NOCB(ux_message_display_content_step,
             bnnn_paging,
             {
                 .title = "Message",
                 .text = G.ui.detail_value,
             });

// Step for continuing message display (for long messages)
UX_STEP_CB(ux_message_display_next_step,
           nnn,
           ui_action_message_next(true),
           {"Double-press to", "continue message", "press right to skip"});

UX_STEP_CB(ux_message_display_approve_step,
           pb,
           (*g_validate_callback)(true),
           {
               &C_icon_validate_14,
               "Sign Message",
           });

UX_STEP_CB(ux_message_display_reject_step,
           pb,
           (*g_validate_callback)(false),
           {
               &C_icon_crossmark,
               "Reject",
           });

// FLOW to display message content that continues (with sign/reject options):
UX_FLOW(ux_message_display_continue_flow,
        &ux_message_display_content_step,
        &ux_message_display_next_step,
        &ux_message_display_approve_step,
        &ux_message_display_reject_step);

// FLOW to display final message content:
UX_FLOW(ux_message_display_final_flow,
        &ux_message_display_content_step,
        &ux_message_display_approve_step,
        &ux_message_display_reject_step);

// FLOW to display first message content:
UX_FLOW(ux_message_display_content_flow,
        &ux_message_display_confirm_step,
        &ux_message_display_content_step,
        &ux_message_display_next_step,
        &ux_message_display_approve_step,
        &ux_message_display_reject_step);

int ui_display_message() {
    if (G_context.req_type != CONFIRM_MESSAGE || G_context.state != STATE_NONE) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }

    explicit_bzero(G.ui.detail_value, DETAIL_VALUE_MAX_LENGTH);
    g_validate_callback = &ui_action_validate_message;
    g_message_offset = 0;

    // Display the message content (raw bytes with printable chars and \xHH for non-printable)
    // G_context.raw_size contains the exact number of bytes, no null terminator assumed
    prepare_next_message_chunk();

    // Choose flow based on whether message can fit in one chunk
    size_t first_chunk_size =
        calculate_safe_chunk_size(G_context.raw, G_context.raw_size, DETAIL_VALUE_MAX_LENGTH);

    if (first_chunk_size >= G_context.raw_size) {
        // Short message - entire message fits in one chunk
        ux_flow_init(0, ux_message_display_final_flow, NULL);
    } else {
        // Long message - multiple chunks needed
        ux_flow_init(0, ux_message_display_content_flow, NULL);
    }

    return 0;
}
#endif  // HAVE_BAGL