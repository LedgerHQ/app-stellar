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

#include <stdbool.h>  // bool
#include <string.h>   // memset

#include "./ui.h"
#include "./action/validate.h"
#include "../globals.h"
#include "../utils.h"
#include "../sw.h"
#include "../io.h"
#include "../transaction/transaction_parser.h"
#include "../transaction/transaction_formatter.h"

static uint8_t num_data;

static void display_next_state(bool is_upper_border);
// clang-format off
UX_STEP_NOCB(
    ux_confirm_tx_init_flow_step,
    pnn,
    {
      &C_icon_eye,
      "Review",
      "Transaction",
    });

UX_STEP_INIT(
    ux_init_upper_border,
    NULL,
    NULL,
    {
        display_next_state(true);
    });
UX_STEP_NOCB(
    ux_variable_display,
    bnnn_paging,
    {
      .title = G_ui_detail_caption,
      .text = G_ui_detail_value,
    });
UX_STEP_INIT(
    ux_init_lower_border,
    NULL,
    NULL,
    {
        display_next_state(false);
    });

UX_STEP_CB(
    ux_confirm_tx_finalize_step,
    pnn,
    G_ui_validate_callback(true),
    {
      &C_icon_validate_14,
      "Finalize",
      "Transaction",
    });

UX_STEP_CB(
    ux_reject_tx_flow_step,
    pb,
    G_ui_validate_callback(false),
    {
      &C_icon_crossmark,
      "Cancel",
    });

UX_FLOW(ux_confirm_flow,
  &ux_confirm_tx_init_flow_step,

  &ux_init_upper_border,
  &ux_variable_display,
  &ux_init_lower_border,

  &ux_confirm_tx_finalize_step,
  &ux_reject_tx_flow_step
);


static void display_next_state(bool is_upper_border) {
    PRINTF(
        "display_next_state invoked. is_upper_border = %d, G_ui_current_state = %d, formatter_index = "
        "%d, G_ui_current_data_index = %d\n",
        is_upper_border,
        G_ui_current_state,
        formatter_index,
        G_ui_current_data_index);
    if (is_upper_border) {  // -> from first screen
        if (G_ui_current_state == OUT_OF_BORDERS) {
            G_ui_current_state = INSIDE_BORDERS;
            set_state_data(true);
            ux_flow_next();
        } else {
            formatter_index -= 1;
            if (G_ui_current_data_index > 0) {  // <- from middle, more screens available
                set_state_data(false);
                if (formatter_stack[formatter_index] != NULL) {
                    ux_flow_next();
                } else {
                    G_ui_current_state = OUT_OF_BORDERS;
                    G_ui_current_data_index = 0;
                    ux_flow_prev();
                }
            } else {  // <- from middle, no more screens available
                G_ui_current_state = OUT_OF_BORDERS;
                G_ui_current_data_index = 0;
                ux_flow_prev();
            }
        }
    } else  // walking over the second border
    {
        if (G_ui_current_state == OUT_OF_BORDERS) {  // <- from last screen
            G_ui_current_state = INSIDE_BORDERS;
            set_state_data(false);
            ux_flow_prev();
        } else {
            if ((num_data != 0 && G_ui_current_data_index < num_data - 1) ||
                formatter_stack[formatter_index + 1] !=
                    NULL) {  // -> from middle, more screens available
                formatter_index += 1;
                set_state_data(true);
                /*dirty hack to have coherent behavior on bnnn_paging when there are multiple
                 * screens*/
                G_ux.flow_stack[G_ux.stack_count - 1].prev_index =
                    G_ux.flow_stack[G_ux.stack_count - 1].index - 2;
                G_ux.flow_stack[G_ux.stack_count - 1].index--;
                ux_flow_relayout();
                /*end of dirty hack*/
            } else {  // -> from middle, no more screens available
                G_ui_current_state = OUT_OF_BORDERS;
                ux_flow_next();
            }
        }
    }
}


int ui_approve_tx_init(void) {
    if (G_context.req_type != CONFIRM_TRANSACTION || G_context.state != STATE_PARSED) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }
    G_ui_current_data_index = 0;
    G_ui_current_state = OUT_OF_BORDERS;
    G_context.tx_info.offset = 0;
    formatter_index = 0;

    explicit_bzero(formatter_stack, sizeof(formatter_stack));
    num_data = G_context.tx_info.tx_details.operations_count;
    G_ui_validate_callback = &ui_action_validate_transaction;
    ux_flow_init(0, ux_confirm_flow, NULL);
    return 0;
}