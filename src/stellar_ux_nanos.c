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
#include "stellar_format.h"

#include "glyphs.h"

bagl_element_t tmp_element;

// ------------------------------------------------------------------------- //
//                                  MENUS                                    //
// ------------------------------------------------------------------------- //

const ux_menu_entry_t menu_main[];
const ux_menu_entry_t menu_settings[];
const ux_menu_entry_t menu_settings_browser[];
const ux_menu_entry_t menu_settings_hash_signing[];

// change the setting
void menu_settings_browser_change(unsigned int enabled) {
    nvm_write(&N_stellar_pstate->fidoTransport, (void *)&enabled, sizeof(uint8_t));
    USB_power_U2F(0, 0);
    USB_power_U2F(1, N_stellar_pstate->fidoTransport);
    // go back to the menu entry
    UX_MENU_DISPLAY(1, menu_settings, NULL);
}

void menu_settings_hash_signing_change(unsigned int enabled) {
    ctx.hashSigning = enabled;
    UX_MENU_DISPLAY(1, menu_settings, NULL);
}

// show the currently activated entry
void menu_settings_browser_init(unsigned int ignored) {
    UNUSED(ignored);
    UX_MENU_DISPLAY(N_stellar_pstate->fidoTransport ? 1 : 0, menu_settings_browser, NULL);
}

void menu_settings_hash_siging_init(unsigned int ignored) {
    UNUSED(ignored);
    UX_MENU_DISPLAY(ctx.hashSigning, menu_settings_hash_signing, NULL);
}

const ux_menu_entry_t menu_settings_browser[] = {
    {NULL, menu_settings_browser_change, 0, NULL, "No", NULL, 0, 0},
    {NULL, menu_settings_browser_change, 1, NULL, "Yes", NULL, 0, 0},
    UX_MENU_END
    };

const ux_menu_entry_t menu_settings_hash_signing[] = {
    {NULL, menu_settings_hash_signing_change, 0, NULL, "No", NULL, 0, 0},
    {NULL, menu_settings_hash_signing_change, 1, NULL, "Yes", NULL, 0, 0},
    UX_MENU_END
    };

const ux_menu_entry_t menu_settings[] = {
    {NULL, menu_settings_browser_init, 0, NULL, "Browser support", NULL, 0, 0},
    {NULL, menu_settings_hash_siging_init, 0, NULL, "Hash signing", NULL, 0, 0},
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
const bagl_element_t *ui_address_prepro(const bagl_element_t *element) {
    if (element->component.userid == 0x02) {
        UX_CALLBACK_SET_INTERVAL(MAX(1500, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
    }
    return element;
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
    UX_DISPLAY(ui_address_nanos, ui_address_prepro);
}

// ------------------------------------------------------------------------- //
//                             APPROVE TRANSACTION                           //
// ------------------------------------------------------------------------- //

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

const bagl_element_t *ui_approve_tx_prepro(const bagl_element_t *element) {
    switch (element->component.userid) {
    case 0x00: // Controls: always visible
        return element;
    case 0x01:
    case 0x11: // "Confirm transaction"
        if (detailCaption[0] == '\0' && opCaption[0] == '\0') {
            UX_CALLBACK_SET_INTERVAL(1750);
            return element;
        }
        return NULL;
    case 0x02: // "Operation i of n"
        if (opCaption[0] != '\0') {
            os_memmove(&tmp_element, element, sizeof(bagl_element_t));
            tmp_element.text = opCaption;
            UX_CALLBACK_SET_INTERVAL(MAX(1750, 1000 + bagl_label_roundtrip_duration_ms(&tmp_element, 7)));
            return &tmp_element;
        }
        return NULL;
    case 0x03: // Details caption
    case 0x13: // Details value
        if (detailCaption[0] != '\0') {
            os_memmove(&tmp_element, element, sizeof(bagl_element_t));
            if (element->component.userid == 0x03) {
                tmp_element.text = detailCaption;
            } else {
                tmp_element.text = detailValue;
            }
            UX_CALLBACK_SET_INTERVAL(MAX(1750, 1000 + bagl_label_roundtrip_duration_ms(&tmp_element, 7)));
            return &tmp_element;
        }
    }
    return NULL;
}

/** prepare next sign hash approval screen */
const bagl_element_t *ui_tx_approve_hash_prepro(const bagl_element_t *element) {
    switch (element->component.userid) {
    case 0x00: // Controls: always visible
        return element;
    case 0x01:
    case 0x11: // "Confirm transaction"
        if (detailCaption[0] == '\0') {
            UX_CALLBACK_SET_INTERVAL(1750);
            return element;
        }
        break;
    case 0x02:
        return NULL;
    case 0x03: // Details caption
    case 0x13: // Details value
        if (detailCaption[0] != '\0') {
            os_memmove(&tmp_element, element, sizeof(bagl_element_t));
            if (element->component.userid == 0x03) {
                tmp_element.text = detailCaption;
            } else {
                tmp_element.text = detailValue;
            }
            UX_CALLBACK_SET_INTERVAL(MAX(1750, 1000 + bagl_label_roundtrip_duration_ms(&tmp_element, 7)));
            return &tmp_element;
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

    {{BAGL_LABELINE, 0x01, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Confirm", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x11, 0, 26, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "transaction", 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x02, 0, 19, 128, 32, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    // Details caption & value
    {{BAGL_LABELINE, 0x03, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x13, 16, 26, 96, 12, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26},
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
    ctx.req.tx.offset = 0;
    formatter = NULL;
    ui_approve_tx_next_screen(&ctx.req.tx);
    UX_DISPLAY(ui_approve_tx_nanos, ui_approve_tx_prepro);
}

void ui_approve_tx_hash_init(void) {
    formatter = format_confirm_hash;
    ui_approve_tx_next_screen(NULL);
    UX_DISPLAY(ui_approve_tx_nanos, ui_tx_approve_hash_prepro);
}

#endif
