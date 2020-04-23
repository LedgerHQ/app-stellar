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

#if defined(TARGET_NANOS) && !defined(HAVE_UX_FLOW)

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
const ux_menu_entry_t menu_settings_hash_signing[];


void menu_settings_hash_signing_change(unsigned int enabled) {
    nvm_write((void*)&N_stellar_pstate.hashSigning, &enabled, 1);
    UX_MENU_DISPLAY(1, menu_settings, NULL);
}

void menu_settings_hash_siging_init(unsigned int ignored) {
    UNUSED(ignored);
    UX_MENU_DISPLAY(N_stellar_pstate.hashSigning, menu_settings_hash_signing, NULL);
}

const ux_menu_entry_t menu_settings_hash_signing[] = {
    {NULL, menu_settings_hash_signing_change, 0, NULL, "No", NULL, 0, 0},
    {NULL, menu_settings_hash_signing_change, 1, NULL, "Yes", NULL, 0, 0},
    UX_MENU_END
    };

const ux_menu_entry_t menu_settings[] = {
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
        UX_CALLBACK_SET_INTERVAL(1500 + bagl_label_roundtrip_duration_ms(element, 7));
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

     // Address value
    {{BAGL_LABELINE, 0x02, 16, 26, 96, 12, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000,
      BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26},
     detailValue, 0, 0, 0, NULL, NULL, NULL}

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
    print_public_key(ctx.req.pk.publicKey, detailValue, 12, 12);
    UX_DISPLAY(ui_address_nanos, ui_address_prepro);
}

// ------------------------------------------------------------------------- //
//                             APPROVE TRANSACTION                           //
// ------------------------------------------------------------------------- //

const bagl_element_t *ui_approve_tx_prepro(const bagl_element_t *element) {
    /*
     * the current formatter decides what to show this round:
     * if it prints detailCaption/detailValue then 0x03/0x04 is shown
     * if it prints opCaption then 0x02 is shown
     * if it prints neither 0x01 is shown
     */
    bool hasDetails = detailCaption[0] != '\0';
    bool hasOperation = opCaption[0] != '\0';
    switch (element->component.userid) {
    case 0x00: // Controls: always visible
        return element;
    case 0x01: // "Confirm transaction"
        if (!hasDetails && !hasOperation) {
            UX_CALLBACK_SET_INTERVAL(2000);
            return element;
        }
        return NULL;
    case 0x02: // "Operation i of n"
        if (hasOperation) {
            os_memmove(&tmp_element, element, sizeof(bagl_element_t));
            tmp_element.text = opCaption;
            UX_CALLBACK_SET_INTERVAL(MAX(2000, 1000 + bagl_label_roundtrip_duration_ms(&tmp_element, 7)));
            return &tmp_element;
        }
        return NULL;
    case 0x03: // Details caption
    case 0x04: // Details value
        if (hasDetails) {
            os_memmove(&tmp_element, element, sizeof(bagl_element_t));
            if (element->component.userid == 0x03) {
                tmp_element.text = detailCaption;
            } else {
                tmp_element.text = detailValue;
            }
            UX_CALLBACK_SET_INTERVAL(MAX(2000, 1000 + bagl_label_roundtrip_duration_ms(&tmp_element, 7)));
            return &tmp_element;
        }
    }
    return NULL;
}

/** prepare next sign hash approval screen */
const bagl_element_t *ui_tx_approve_hash_prepro(const bagl_element_t *element) {
    bool hasDetails = detailCaption[0] != '\0';
    switch (element->component.userid) {
    case 0x00: // Controls: always visible
        return element;
    case 0x01: // "Confirm transaction"
        if (!hasDetails) {
            UX_CALLBACK_SET_INTERVAL(2000);
            return element;
        }
        break;
    case 0x02: return NULL;
    case 0x03: // Details caption
    case 0x04: // Details value
        if (hasDetails) {
            os_memmove(&tmp_element, element, sizeof(bagl_element_t));
            if (element->component.userid == 0x03) {
                tmp_element.text = detailCaption;
            } else {
                tmp_element.text = detailValue;
            }
            UX_CALLBACK_SET_INTERVAL(MAX(2000, 1000 + bagl_label_roundtrip_duration_ms(&tmp_element, 7)));
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

    // "Confirm transaction"
    {{BAGL_LABELINE, 0x01, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Confirm", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x01, 0, 26, 128, 12, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "transaction", 0, 0, 0, NULL, NULL, NULL},

    // Operation caption (e.g. "Operation i of n")
    {{BAGL_LABELINE, 0x02, 0, 19, 128, 32, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    // Details caption
    {{BAGL_LABELINE, 0x03, 0, 12, 128, 32, 0, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    // Details value
    {{BAGL_LABELINE, 0x04, 16, 26, 96, 12, 0x80 | 10, 0, 0, 0xFFFFFF, 0x000000, BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER, 26},
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

format_function_t next_formatter(tx_context_t *txCtx) {
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

void ui_approve_tx_init(void) {
    ctx.req.tx.offset = 0;
    formatter = NULL;
    ui_approve_tx_next_screen(&ctx.req.tx);
    if (formatter) {
        UX_DISPLAY(ui_approve_tx_nanos, ui_approve_tx_prepro);
    }
}

void ui_approve_tx_hash_init(void) {
    MEMCLEAR(detailCaption);
    MEMCLEAR(detailValue);
    MEMCLEAR(opCaption);
    format_confirm_hash(NULL);
    UX_DISPLAY(ui_approve_tx_nanos, ui_tx_approve_hash_prepro);
}

#endif
