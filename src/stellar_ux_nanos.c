/*******************************************************************************
 *   Ledger Stellar App
 *   (c) 2017-2018 Ledger
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
 ********************************************************************************/
#include "bolos_target.h"

#ifdef TARGET_NANOS

#include "stellar_ux.h"
#include "stellar_types.h"
#include "stellar_api.h"
#include "stellar_vars.h"

#include "glyphs.h"

static const char * opCaptions[][6] = {
    {"Create Account", "Starting Balance", NULL, NULL, NULL},
    {"Send", "Destination", NULL, NULL, NULL},
    {"Send", "Destination", "Receive", "Via", NULL},
    {"Create Offer", "Buy", "Price", "Sell", NULL, NULL},
    {"Remove Offer", NULL, NULL, NULL, NULL},
    {"Change Offer", "Buy", "Price", "Sell", NULL},
    {"Inflation Destination", "Flags", "Thresholds", "Home Domain", "Signer"},
    {"Change Trust", "Limit", NULL, NULL, NULL},
    {"Remove Trust", NULL, NULL, NULL, NULL},
    {"Allow Trust", "Account ID", NULL, NULL, NULL},
    {"Revoke Trust", "Account ID", NULL, NULL, NULL},
    {"Merge Account", "Destination", NULL, NULL, NULL},
    {"Inflation", NULL, NULL, NULL, NULL},
    {"Set Data", "Value", NULL, NULL, NULL},
    {"Remove Data", NULL, NULL, NULL, NULL},
    {"Bump Sequence", NULL, NULL, NULL, NULL},
    {"WARNING", "Hash", NULL, NULL, NULL}
};

static const char * txCaptions[4] = {"Memo", "Fee", "Network", "Source Account"};

bagl_element_t tmp_element;

// ------------------------------------------------------------------------- //
//                                  MENUS                                    //
// ------------------------------------------------------------------------- //

const ux_menu_entry_t menu_main[];
const ux_menu_entry_t menu_settings[];
const ux_menu_entry_t menu_settings_browser[];
const ux_menu_entry_t menu_settings_multi_ops[];

// change the setting
void menu_settings_browser_change(unsigned int enabled) {
    nvm_write(&N_stellar_pstate->fidoTransport, (void *)&enabled, sizeof(uint8_t));
    USB_power_U2F(0, 0);
    USB_power_U2F(1, N_stellar_pstate->fidoTransport);
    // go back to the menu entry
    UX_MENU_DISPLAY(1, menu_settings, NULL);
}

void menu_settings_multi_ops_change(unsigned int enabled) {
    ctx.multiOpsSupport = enabled;
    UX_MENU_DISPLAY(1, menu_settings, NULL);
}

// show the currently activated entry
void menu_settings_browser_init(unsigned int ignored) {
    UNUSED(ignored);
    UX_MENU_DISPLAY(N_stellar_pstate->fidoTransport ? 1 : 0, menu_settings_browser, NULL);
}

void menu_settings_multi_ops_init(unsigned int ignored) {
    UNUSED(ignored);
    UX_MENU_DISPLAY(ctx.multiOpsSupport, menu_settings_multi_ops, NULL);
}

const ux_menu_entry_t menu_settings_browser[] = {
    {NULL, menu_settings_browser_change, 0, NULL, "No", NULL, 0, 0},
    {NULL, menu_settings_browser_change, 1, NULL, "Yes", NULL, 0, 0},
    UX_MENU_END
    };

const ux_menu_entry_t menu_settings_multi_ops[] = {
    {NULL, menu_settings_multi_ops_change, 0, NULL, "No", NULL, 0, 0},
    {NULL, menu_settings_multi_ops_change, 1, NULL, "Yes", NULL, 0, 0},
    UX_MENU_END
    };

const ux_menu_entry_t menu_settings[] = {
    {NULL, menu_settings_browser_init, 0, NULL, "Browser support", NULL, 0, 0},
    {NULL, menu_settings_multi_ops_init, 0, NULL, "Enable multi-ops", NULL, 0, 0},
    {menu_main, NULL, 1, &C_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END
    };

const ux_menu_entry_t menu_about[] = {
    {NULL, NULL, 0, NULL, "Version", APPVERSION, 0, 0},
    {menu_main, NULL, 2, &C_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END
    };

const ux_menu_entry_t menu_main[] = {
    {NULL, NULL, 0, &C_icon_stellar, "Use wallet to", "view accounts", 33, 12},
    {menu_settings, NULL, 0, NULL, "Settings", NULL, 0, 0},
    {menu_about, NULL, 0, NULL, "About", NULL, 0, 0},
    {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
    UX_MENU_END
    };

void ui_idle(void) {
    UX_MENU_DISPLAY(0, menu_main, NULL);
}

// ------------------------------------------------------------------------- //
//                             CONFIRM ADDRESS                               //
// ------------------------------------------------------------------------- //

/** prepare show address screen */
unsigned int ui_address_prepro(const bagl_element_t *element) {
    if (element->component.userid == 0x02) {
        UX_CALLBACK_SET_INTERVAL(MAX(1500, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
    }
    return 1;
}

const bagl_element_t ui_address_nanos[] = {

    {{BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CROSS},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CHECK},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x01, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Confirm Address", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x02, 16, 26, 96, 12, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26},
     ctx.req.pk.address, 0, 0, 0, NULL, NULL, NULL}

};

unsigned int ui_address_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT: // CANCEL
        io_seproxyhal_touch_address_cancel(NULL);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_RIGHT: { // OK
        io_seproxyhal_touch_address_ok(NULL);
        break;
    }
    }
    return 0;
}

void ui_show_address_init(void) {
    ctx.uxStep = 0;
    ctx.uxStepCount = 2;
    UX_DISPLAY(ui_address_nanos, ui_address_prepro);
}

// ------------------------------------------------------------------------- //
//                             APPROVE TRANSACTION                           //
// ------------------------------------------------------------------------- //

/** max number of details this operation contains */
uint8_t count_op_details() {
    uint8_t i;
    for (i = 0; i < 6; i++) {
        if (((char*) PIC(opCaptions[ctx.req.tx.content.opType][i])) == NULL) {
            return i;
        }
    }
    return i;
}

/** number of steps to show this operation */
uint8_t count_steps() {
    if (ctx.req.tx.content.opIdx == ctx.req.tx.content.opCount) {
        // last operation: also show tx details
        return count_op_details() + 4 + 1;
    }
    return count_op_details() + 1;
}

/** parse the next operation from the raw transaction xdr */
void prepare_details() {
    ctx.req.tx.offset = parseTxXdr(ctx.req.tx.raw, &ctx.req.tx.content, ctx.req.tx.offset);
    ctx.uxStepCount = count_steps();
}

/** prepare next sign transaction approval screen */
const bagl_element_t *ui_tx_approval_prepro(const bagl_element_t *element) {
    switch (element->component.userid) {
    case 0x00: // Controls: always visible
        return element;
    case 0x01:
    case 0x11: // "Confirm transaction"
        if (ctx.uxStep == 0) {
            if (element->component.userid == 0x01) { // prepare details once per operation
                prepare_details();
            }
            // only show on the first operation
            if (ctx.req.tx.content.opIdx == 1) {
                UX_CALLBACK_SET_INTERVAL(1800);
                return element;
            } else {
                ctx.uxStep++;
            }
        } else {
            return NULL;
        }
    case 0x02: // Details caption
    case 0x12: // Details value
        if (ctx.uxStep > 0) {
            uint8_t opDetailsCount = count_op_details();
            // first show operation details
            while (ctx.uxStep > 0 && ctx.uxStep <= opDetailsCount) {
                uint8_t detailIdx = ctx.uxStep - 1;
                char *value = ctx.req.tx.content.opDetails[detailIdx];
                if (value[0] == '\0') {
                    ctx.uxStep++; // skip empty value
                } else {
                    os_memmove(&tmp_element, element, sizeof(bagl_element_t));
                    if (element->component.userid == 0x12) { // value
                        tmp_element.text = value;
                    } else { // caption
                        tmp_element.text = ((char*) PIC(opCaptions[ctx.req.tx.content.opType][detailIdx]));
                    }
                    UX_CALLBACK_SET_INTERVAL(MAX(1500, 1000 + bagl_label_roundtrip_duration_ms(&tmp_element, 7)));
                    return &tmp_element;
                }
            }
            // lastly show transaction level details (fee/memo/network)
            if (ctx.uxStep > opDetailsCount) {
                os_memmove(&tmp_element, element, sizeof(bagl_element_t));
                uint8_t detailIdx = ctx.uxStep - opDetailsCount - 1;
                if (element->component.userid == 0x12) { // value
                    tmp_element.text = ctx.req.tx.content.txDetails[detailIdx];
                } else { // caption
                    tmp_element.text = txCaptions[detailIdx];
                }
                UX_CALLBACK_SET_INTERVAL(MAX(1500, 1000 + bagl_label_roundtrip_duration_ms(&tmp_element, 7)));
                return &tmp_element;
            }
        }
    }
    return NULL;
}

/** prepare next sign hash approval screen */
const bagl_element_t *ui_tx_approval_hash_prepro(const bagl_element_t *element) {
    switch (element->component.userid) {
    case 0x00: // Controls: always visible
        return element;
    case 0x01:
    case 0x11: // "Confirm transaction" on the first step
        if (ctx.uxStep == 0) {
            UX_CALLBACK_SET_INTERVAL(1800);
            return element;
        }
        break;
    case 0x02: // Details caption
    case 0x12: // Details value
    {
        if (ctx.uxStep == 1) {
            os_memmove(&tmp_element, element, sizeof(bagl_element_t));
            if (element->component.userid == 0x02) {
                tmp_element.text = ((char *)PIC("WARNING"));
            } else {
                tmp_element.text = ((char *)PIC("No details"));
            }
            UX_CALLBACK_SET_INTERVAL(MAX(1500, 1000 + bagl_label_roundtrip_duration_ms(&tmp_element, 7)));
            return &tmp_element;
        }
        if (ctx.uxStep == 2) {
            os_memmove(&tmp_element, element, sizeof(bagl_element_t));
            if (element->component.userid == 0x02) {
                tmp_element.text = ((char *)PIC("Transaction Hash"));
            } else {
                print_hash_summary(ctx.req.tx.hash, ctx.req.tx.content.txDetails[0]);
                tmp_element.text = ctx.req.tx.content.txDetails[0];
            }
            UX_CALLBACK_SET_INTERVAL(MAX(1500, 1000 + bagl_label_roundtrip_duration_ms(&tmp_element, 7)));
            return &tmp_element;
        }
        break;
    }
    }
    return NULL;
}

const bagl_element_t ui_approve_tx_nanos[] = {

    // Controls
    {{BAGL_RECTANGLE, 0x00, 0, 0, 128, 32, 0, 0, BAGL_FILL, 0x000000, 0xFFFFFF, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_ICON, 0x00, 3, 12, 7, 7, 0, 0, 0, 0xFFFFFF, 0x000000, 0,  BAGL_GLYPH_ICON_CROSS},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_ICON, 0x00, 117, 13, 8, 6, 0, 0, 0, 0xFFFFFF, 0x000000, 0, BAGL_GLYPH_ICON_CHECK},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    // Step 1: "Confirm transaction"
    {{BAGL_LABELINE, 0x01, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Confirm", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x11, 0, 26, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "transaction", 0, 0, 0, NULL, NULL, NULL},

    // Details caption & value
    {{BAGL_LABELINE, 0x02, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x12, 16, 26, 96, 12, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26},
     NULL, 0, 0, 0, NULL, NULL, NULL}

};

unsigned int ui_approve_tx_nanos_button(unsigned int button_mask, unsigned int button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        io_seproxyhal_touch_tx_cancel(NULL);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_RIGHT: {
        io_seproxyhal_touch_tx_ok(NULL);
        break;
    }
    }
    return 0;
}

void ui_approve_tx_init(void) {
    ctx.uxStep = 0;
    ctx.uxStepCount = 1; // initial value to get going, updated when operation is parsed
    ctx.req.tx.offset = 0;
    UX_DISPLAY(ui_approve_tx_nanos, ui_tx_approval_prepro);
}

void ui_approve_tx_hash_init(void) {
    ctx.uxStep = 0;
    ctx.uxStepCount = 3;
    UX_DISPLAY(ui_approve_tx_nanos, ui_tx_approval_hash_prepro);
}

#endif