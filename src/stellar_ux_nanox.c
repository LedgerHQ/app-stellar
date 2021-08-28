#ifdef HAVE_UX_FLOW

#include "stellar_ux.h"
#include "stellar_types.h"
#include "stellar_api.h"
#include "stellar_vars.h"
#include "stellar_format.h"

#include "glyphs.h"

#include "ux.h"
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

void settings_hash_signing_change(unsigned int enabled);
const char* settings_submenu_getter(unsigned int idx);
void settings_submenu_selector(unsigned int idx);

// ------------------------------------------------------------------------- //
//                                  MENUS                                    //
// ------------------------------------------------------------------------- //

//////////////////////////////////////////////////////////////////////////////////////
// clang-format off
UX_STEP_CB(
  settings_hash_signing_enable_step,
  pbb,
  settings_hash_signing_change(1),
  {
    &C_icon_validate_14,
    "Enable hash", 
    "signing?",
  });
UX_STEP_CB(
  settings_hash_signing_disable_step,
  pb,
  settings_hash_signing_change(0),
  {
    &C_icon_crossmark,
    "Disable", 
  });
UX_STEP_CB(
  settings_hash_signing_go_back_step,
  pb,
  ux_menulist_init(0, settings_submenu_getter, settings_submenu_selector),
  {
    &C_icon_back_x,
    "Back",
  });

UX_DEF(settings_hash_signing_flow,
  &settings_hash_signing_enable_step,
  &settings_hash_signing_disable_step,
  &settings_hash_signing_go_back_step
);
// clang-format on

void settings_hash_signing(void) {
    ux_flow_init(0, settings_hash_signing_flow, NULL);
}

void settings_hash_signing_change(unsigned int enabled) {
    nvm_write((void*) &N_stellar_pstate.hashSigning, &enabled, 1);
    ui_idle();
}

//////////////////////////////////////////////////////////////////////////////////////
// Settings menu:

const char* const settings_submenu_getter_values[] = {
    "Hash signing",
    "Back",
};

const char* settings_submenu_getter(unsigned int idx) {
    if (idx < ARRAYLEN(settings_submenu_getter_values)) {
        return settings_submenu_getter_values[idx];
    }
    return NULL;
}

void settings_submenu_selector(unsigned int idx) {
    switch (idx) {
        case 0:
            settings_hash_signing();
            break;
        default:
            ui_idle();
    }
}

//////////////////////////////////////////////////////////////////////////////////////
// clang-format off
UX_STEP_NOCB(
  idle_welcome_step,
  pbb,
  {
    &C_icon_stellar,
    "Use wallet to", 
    "view accounts",
  });
UX_STEP_CB(
  idle_settings_step,
  pb,
  ux_menulist_init(0, settings_submenu_getter, settings_submenu_selector),
  //ui_settings(),
  {
    &C_icon_coggle,
    "Settings",
  });

UX_STEP_NOCB(
  idle_version_step,
  bn,
  {
    "Version",
    APPVERSION,
  });
UX_STEP_CB(
  idle_quit_step,
  pb,
  os_sched_exit(-1),
  {
    &C_icon_dashboard_x,
    "Quit",
  });

UX_FLOW(idle_flow,
  &idle_welcome_step,
  &idle_settings_step,
  &idle_version_step,
  &idle_quit_step,
  FLOW_LOOP
);
// clang-format on

void ui_idle(void) {
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, idle_flow, NULL);
}

bagl_element_t tmp_element;

// ------------------------------------------------------------------------- //
//                             CONFIRM ADDRESS                               //
// ------------------------------------------------------------------------- //

//////////////////////////////////////////////////////////////////////////////////////
// clang-format off
UX_STEP_NOCB(
    ux_display_public_flow_0_step, 
    pnn, 
    {
      &C_icon_eye,
      "Confirm",
      "Address",
    });
UX_STEP_NOCB(
    ux_display_public_flow_1_step, 
    bnnn_paging, 
    {
      .title = "Confirm Address",
      .text = detailValue,
    });
UX_STEP_CB(
    ux_display_public_flow_2_step, 
    pb, 
    io_seproxyhal_touch_address_ok(NULL),
    {
      &C_icon_validate_14,
      "Approve",
    });
UX_STEP_CB(
    ux_display_public_flow_3_step, 
    pb, 
    io_seproxyhal_touch_address_cancel(NULL),
    {
      &C_icon_crossmark,
      "Reject",
    });

UX_DEF(ux_display_public_flow,
  &ux_display_public_flow_0_step,
  &ux_display_public_flow_1_step,
  &ux_display_public_flow_2_step,
  &ux_display_public_flow_3_step
);
// clang-format on

void ui_show_address_init(void) {
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }
    print_public_key(ctx.req.pk.publicKey, detailValue, 0, 0);
    ux_flow_init(0, ux_display_public_flow, NULL);
}

// ------------------------------------------------------------------------- //
//                             APPROVE TRANSACTION                           //
// ------------------------------------------------------------------------- //

void display_next_state(bool is_upper_border);

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
      .title = detailCaption,
      .text = detailValue,
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
    io_seproxyhal_touch_tx_ok(NULL),
    {
      &C_icon_validate_14,
      "Finalize",
      "Transaction",
    });

UX_STEP_CB(
    ux_reject_tx_flow_step, 
    pb, 
    io_seproxyhal_touch_tx_cancel(NULL),
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
// clang-format on

uint8_t num_data;
volatile uint8_t current_state;

#define INSIDE_BORDERS 0
#define OUT_OF_BORDERS 1

void display_next_state(bool is_upper_border) {
    if (is_upper_border) {  // -> from first screen
        if (current_state == OUT_OF_BORDERS) {
            current_state = INSIDE_BORDERS;
            set_state_data(true);
            ux_flow_next();
        } else {
            formatter_index -= 1;
            if (current_data_index > 0) {  // <- from middle, more screens available
                set_state_data(false);
                if (formatter_stack[formatter_index] != NULL) {
                    ux_flow_next();
                } else {
                    current_state = OUT_OF_BORDERS;
                    current_data_index = 0;
                    ux_flow_prev();
                }
            } else {  // <- from middle, no more screens available
                current_state = OUT_OF_BORDERS;
                current_data_index = 0;
                ux_flow_prev();
            }
        }
    } else  // walking over the second border
    {
        if (current_state == OUT_OF_BORDERS) {  // <- from last screen
            current_state = INSIDE_BORDERS;
            set_state_data(false);
            ux_flow_prev();
        } else {
            formatter_index += 1;
            if ((num_data != 0 && current_data_index < num_data - 1) ||
                formatter_stack[formatter_index] !=
                    NULL) {  // -> from middle, more screens available
                set_state_data(true);

                /*dirty hack to have coherent behavior on bnnn_paging when there are multiple
                 * screens*/
                G_ux.flow_stack[G_ux.stack_count - 1].prev_index =
                    G_ux.flow_stack[G_ux.stack_count - 1].index - 2;
                G_ux.flow_stack[G_ux.stack_count - 1].index--;
                ux_flow_relayout();
                /*end of dirty hack*/
            } else {  // -> from middle, no more screens available
                current_state = OUT_OF_BORDERS;
                ux_flow_next();
            }
        }
    }
}

void ui_approve_tx_init(void) {
    ctx.req.tx.offset = 0;
    formatter_index = 0;
    MEMCLEAR(formatter_stack);
    num_data = ctx.req.tx.txDetails.opCount;
    current_data_index = 0;
    current_state = OUT_OF_BORDERS;
    ux_flow_init(0, ux_confirm_flow, NULL);
}

void ui_approve_tx_hash_init(void) {
    formatter_index = 0;
    MEMCLEAR(formatter_stack);
    num_data = ctx.req.tx.txDetails.opCount;
    current_data_index = 0;
    current_state = OUT_OF_BORDERS;
    ux_flow_init(0, ux_confirm_flow, NULL);
}

#endif
