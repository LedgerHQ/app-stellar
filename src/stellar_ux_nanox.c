#include "bolos_target.h"

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
UX_FLOW_DEF_VALID(
  settings_hash_signing_enable_step,
  pbb,
  settings_hash_signing_change(1),
  {
    &C_icon_validate_14,
    "Enable hash", 
    "signing?",
  });
UX_FLOW_DEF_VALID(
  settings_hash_signing_disable_step,
  pb,
  settings_hash_signing_change(0),
  {
    &C_icon_crossmark,
    "Disable", 
  });
UX_FLOW_DEF_VALID(
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

void settings_hash_signing(void) {
     ux_flow_init(0, settings_hash_signing_flow, NULL);
}

void settings_hash_signing_change(unsigned int enabled) {
    nvm_write((void*)&N_stellar_pstate.hashSigning, &enabled, 1);
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
  switch(idx) {
    case 0:
      settings_hash_signing();
      break;
    default:
      ui_idle();
  }
}

//////////////////////////////////////////////////////////////////////////////////////
UX_FLOW_DEF_NOCB(
  idle_welcome_step,
  pbb,
  {
    &C_icon_stellar,
    "Use wallet to", 
    "view accounts",
  });
UX_FLOW_DEF_VALID(
  idle_settings_step,
  pb,
  ux_menulist_init(0, settings_submenu_getter, settings_submenu_selector),
  //ui_settings(),
  {
    &C_icon_coggle,
    "Settings",
  });

UX_FLOW_DEF_NOCB(
  idle_version_step,
  bn,
  {
    "Version",
    APPVERSION,
  });
UX_FLOW_DEF_VALID(
  idle_quit_step,
  pb,
  os_sched_exit(-1),
  {
    &C_icon_dashboard_x,
    "Quit",
  });

UX_DEF(idle_flow,
  &idle_welcome_step,
  &idle_settings_step,
  &idle_version_step,
  &idle_quit_step
);

void ui_idle(void) {
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
     ux_flow_init(0, idle_flow, NULL);
}


bagl_element_t tmp_element;


// ------------------------------------------------------------------------- //
//                             CONFIRM ADDRESS                               //
// ------------------------------------------------------------------------- //

//////////////////////////////////////////////////////////////////////////////////////
UX_FLOW_DEF_NOCB(
    ux_display_public_flow_1_step, 
    bnnn_paging, 
    {
      .title = "Confirm Address",
      .text = detailValue,
    });
UX_FLOW_DEF_VALID(
    ux_display_public_flow_2_step, 
    pb, 
    io_seproxyhal_touch_address_ok(NULL),
    {
      &C_icon_validate_14,
      "Approve",
    });
UX_FLOW_DEF_VALID(
    ux_display_public_flow_3_step, 
    pb, 
    io_seproxyhal_touch_address_cancel(NULL),
    {
      &C_icon_crossmark,
      "Reject",
    });

UX_DEF(ux_display_public_flow,
  &ux_display_public_flow_1_step,
  &ux_display_public_flow_2_step,
  &ux_display_public_flow_3_step
);

void ui_show_address_init(void) {
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    print_public_key(ctx.req.pk.publicKey, detailValue, 0, 0);
    ux_flow_init(0, ux_display_public_flow, NULL);
}

// ------------------------------------------------------------------------- //
//                             APPROVE TRANSACTION                           //
// ------------------------------------------------------------------------- //

void display_next_state(bool is_upper_border);

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

UX_FLOW_DEF_VALID(
    ux_confirm_tx_finalize_step, 
    pnn, 
    io_seproxyhal_touch_tx_ok(NULL),
    {
      &C_icon_validate_14,
      "Finalize",
      "Transaction",
    });

UX_FLOW_DEF_VALID(
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

volatile uint8_t current_data_index;

format_function_t get_formatter(tx_context_t *txCtx, bool forward) {

    switch(ctx.state){
        case STATE_APPROVE_TX:
        { // classic tx
            if(!forward){
                if(current_data_index == 0){ // if we're already at the beginning of the buffer, return NULL
                    return NULL;
                }
                // rewind to tx beginning if we're requesting a previous operation
                txCtx->offset = 0;
                txCtx->opIdx = 0;
            }

            while(current_data_index > txCtx->opIdx){
                if (!parse_tx_xdr(txCtx->raw, txCtx->rawLength, txCtx)) {
                    return NULL;
                }
            }    
            return &format_confirm_operation;
        }
        case STATE_APPROVE_TX_HASH:
        {
            if(!forward){
                return NULL;
            }
            return &format_confirm_hash_warning;
        }
        default:
            THROW(0x6123);
    }
}


void ui_approve_tx_next_screen(tx_context_t *txCtx) {
    if (!formatter_stack[formatter_index]) {
        MEMCLEAR(formatter_stack);
        formatter_index = 0;
        current_data_index ++;
        formatter_stack[0] = get_formatter(txCtx, true);
    }
}

void ui_approve_tx_prev_screen(tx_context_t *txCtx) {
    if (formatter_index == -1) {
        MEMCLEAR(formatter_stack);
        formatter_index = 0;
        current_data_index --;
        formatter_stack[0] = get_formatter(txCtx, false);
    }
}

void set_state_data(bool forward){
    if(forward){
        ui_approve_tx_next_screen(&ctx.req.tx);
    }
    else{
        ui_approve_tx_prev_screen(&ctx.req.tx);
    }
    
    // Apply last formatter to fill the screen's buffer
    if (formatter_stack[formatter_index]) {
        MEMCLEAR(detailCaption);
        MEMCLEAR(detailValue);
        MEMCLEAR(opCaption);
        formatter_stack[formatter_index](&ctx.req.tx);

        if(opCaption[0] != '\0'){
            strncpy(detailCaption, opCaption, sizeof(detailCaption));
            detailValue[0] = ' ';
            PRINTF("caption: %s %u\n", detailCaption);
        }
        else if(detailCaption[0] != '\0' && detailValue[0] != '\0'){
            PRINTF("caption: %s\n", detailCaption);
            PRINTF("details: %s\n", detailValue);
        }
    }
}

uint8_t num_data;
volatile uint8_t current_state;

#define INSIDE_BORDERS 0
#define OUT_OF_BORDERS 1

void display_next_state(bool is_upper_border){

    if(is_upper_border){ // -> from first screen
        if(current_state == OUT_OF_BORDERS){
            current_state = INSIDE_BORDERS;
            set_state_data(true);
            ux_flow_next();
        }
        else{ 
            formatter_index -= 1;
            if(current_data_index>0){ // <- from middle, more screens available
                set_state_data(false);
                if(formatter_stack[formatter_index] != NULL){
                    ux_flow_next();
                }
                else{
                    current_state = OUT_OF_BORDERS;
                    current_data_index = 0;
                    ux_flow_prev();
                }
            }
            else{ // <- from middle, no more screens available
                current_state = OUT_OF_BORDERS;
                current_data_index = 0;
                ux_flow_prev();
            }
        }
    }
    else // walking over the second border
    {
        if(current_state == OUT_OF_BORDERS){ // <- from last screen
            current_state = INSIDE_BORDERS;
            set_state_data(false);
            ux_flow_prev();
        }
        else{ 
            formatter_index += 1;
            if((num_data != 0 && current_data_index<num_data-1) || formatter_stack[formatter_index] != NULL){ // -> from middle, more screens available
                set_state_data(true);

                /*dirty hack to have coherent behavior on bnnn_paging when there are multiple screens*/
                G_ux.flow_stack[G_ux.stack_count-1].prev_index = G_ux.flow_stack[G_ux.stack_count-1].index-2;
                G_ux.flow_stack[G_ux.stack_count-1].index--;
                ux_flow_relayout();
                /*end of dirty hack*/
            }
            else{ // -> from middle, no more screens available
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
    num_data = ctx.req.tx.opCount;
    current_data_index = 0;
    current_state = OUT_OF_BORDERS;
    ux_flow_init(0, ux_confirm_flow, NULL);
}


void ui_approve_tx_hash_init(void) {
    formatter_index = 0;
    MEMCLEAR(formatter_stack);
    num_data = ctx.req.tx.opCount;
    current_data_index = 0;
    current_state = OUT_OF_BORDERS;
    ux_flow_init(0, ux_confirm_flow, NULL);
}




#endif