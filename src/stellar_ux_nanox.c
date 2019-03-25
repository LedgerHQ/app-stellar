#include "bolos_target.h"

#ifdef TARGET_NANOX

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
  ux_menulist_init(settings_submenu_getter, settings_submenu_selector),
  {
    &C_icon_back,
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
    ctx.hashSigning = enabled;
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
  ux_menulist_init(settings_submenu_getter, settings_submenu_selector),
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
    &C_icon_dashboard,
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
    print_public_key(ctx.req.pk.publicKey, detailValue, 12, 12);
    ux_flow_init(0, ux_display_public_flow, NULL);
}

// ------------------------------------------------------------------------- //
//                             APPROVE TRANSACTION                           //
// ------------------------------------------------------------------------- //


UX_FLOW_DEF_VALID(
    ux_confirm_tx_init_flow_step, 
    pnn, 
    display_next_screen(),
    {
      &C_icon_eye,
      "Review",
      "Transaction",
    });

UX_FLOW_DEF_VALID(
    ux_confirm_tx_operation_caption_flow_step, 
    bnnn_paging, 
    display_next_screen(),
    {
      .text = opCaption,
    });
UX_FLOW_DEF_VALID(
    ux_confirm_tx_details_flow_step, 
    bnnn_paging, 
    display_next_screen(),
    {
      .title = detailCaption,
      .text = detailValue,
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



UX_DEF(ux_confirm_tx_init_flow,
  &ux_confirm_tx_init_flow_step
);

UX_DEF(ux_confirm_tx_operation_caption_flow,
  &ux_confirm_tx_operation_caption_flow_step
);

UX_DEF(ux_confirm_tx_details_flow,
  &ux_confirm_tx_details_flow_step
);

UX_DEF(ux_confirm_tx_finalize_flow,
  &ux_confirm_tx_finalize_step,
  &ux_reject_tx_flow_step
);


format_function_t next_formatter(tx_context_t *txCtx) {
    BEGIN_TRY {
        TRY {
            parse_tx_xdr(txCtx->raw, txCtx);
        } CATCH_OTHER(sw) {
            io_seproxyhal_respond(sw, 0);
            return NULL;
        } FINALLY {
        }
    } END_TRY;
    if (txCtx->opIdx == 1) {
        return &format_confirm_transaction;
    } else {
        return &format_confirm_operation;
    }
}

void ui_approve_tx_next_screen(tx_context_t *txCtx) {
    if (!formatter) {
        formatter = next_formatter(txCtx);
    }
    if (formatter) {
        MEMCLEAR(detailCaption);
        MEMCLEAR(detailValue);
        MEMCLEAR(opCaption);
        formatter(txCtx);
    }
}

void display_next_screen(){

    if(!ctx.hashSigning){ // classic tx
        ui_approve_tx_next_screen(&ctx.req.tx);
        if (formatter) {
            if(G_ux.stack_count == 0) {
                ux_stack_push();
            }
            bool hasDetails = detailCaption[0] != '\0';
            bool hasOperation = opCaption[0] != '\0';

            PRINTF("detailCaption:%.*H\n", 20, detailCaption);
            PRINTF("opCaption:%.*H\n", 20, opCaption);

            if(!hasDetails && !hasOperation){
                ux_flow_init(0, ux_confirm_tx_init_flow, NULL);
            }
            else if(hasOperation){
                ux_flow_init(0, ux_confirm_tx_operation_caption_flow, NULL);
            }
            else if(hasDetails){
                ux_flow_init(0, ux_confirm_tx_details_flow, NULL);
            }
            else{
                THROW(0x6111);
            }
            
        }
        else{
            ux_flow_init(0, ux_confirm_tx_finalize_flow, NULL);
        }
    }
    else { // tx hash
        format_confirm_hash(NULL);
        if (formatter) {
            if(G_ux.stack_count == 0) {
                ux_stack_push();
            }
            bool hasDetails = detailCaption[0] != '\0';

            if(!hasDetails){
                ux_flow_init(0, ux_confirm_tx_init_flow, NULL);
            }
            else{
                ux_flow_init(0, ux_confirm_tx_details_flow, NULL);
            }
        }
        else{
            ux_flow_init(0, ux_confirm_tx_finalize_flow, NULL);
        }
    }
    
}

void ui_approve_tx_init(void) {
    ctx.req.tx.offset = 0;
    formatter = NULL;
    
    display_next_screen();
}


void ui_approve_tx_hash_init(void) {
    MEMCLEAR(detailCaption);
    MEMCLEAR(detailValue);
    MEMCLEAR(opCaption);

    display_next_screen();
}




#endif