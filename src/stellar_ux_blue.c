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

#ifdef TARGET_BLUE

#include "stellar_ux.h"
#include "stellar_types.h"
#include "stellar_api.h"
#include "stellar_vars.h"

#include "glyphs.h"

bagl_element_t tmp_element;

volatile char titleCaption[26];
volatile char subtitleCaption[21];
volatile char displayString[33];

unsigned int io_seproxyhal_touch_settings(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_exit(const bagl_element_t *e);

static const char *opNames[16] =
    {"Create Account", "Payment", "Path Payment", "New Offer", "Remove Offer", "Change Offer",
     "Set Options", "Change Trust", "Remove Trust", "Allow Trust", "Revoke Trust",
     "Merge Account", "Inflation", "Set Data", "Remove Data", "Bump Sequence"};

const char *const detailNamesTable[][5] = {
    {"ACCOUNT ID", "START BALANCE", NULL, NULL, NULL},
    {"AMOUNT", "DESTINATION", NULL, NULL, NULL},
    {"SEND", "DESTINATION", "RECEIVE", "VIA", NULL},
    {"TYPE", "BUY", "PRICE", "SELL", NULL},
    {"OFFER ID", NULL, NULL, NULL, NULL},
    {"OFFER ID", "BUY", "PRICE", "SELL", NULL},
    {"INFL DEST", "FLAGS", "THRESHOLDS", "HOME DOMAIN", "SIGNER"},
    {"ASSET", "LIMIT", NULL, NULL, NULL},
    {"ASSET", NULL, NULL, NULL, NULL},
    {"ASSET", "ACCOUNT ID", NULL, NULL, NULL},
    {"ASSET", "ACCOUNT ID", NULL, NULL, NULL},
    {"SOURCE", "DESTINATION", NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL, NULL},
    {"NAME", "VALUE", NULL, NULL, NULL},
    {"NAME", NULL, NULL, NULL, NULL},
    {"BUMP TO", NULL, NULL, NULL, NULL}
};

// ------------------------------------------------------------------------- //
//                               IDLE SCREEN                                 //
// ------------------------------------------------------------------------- //

const bagl_element_t ui_idle_blue[] = {
    // type                               userid    x    y   w    h  str rad
    // fill      fg        bg      fid iid  txt   touchparams...       ]
    {{BAGL_RECTANGLE, 0x00, 0, 68, 320, 413, 0, 0, BAGL_FILL, COLOR_BG_1, 0x000000, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    // erase screen (only under the status bar)
    {{BAGL_RECTANGLE, 0x00, 0, 20, 320, 48, 0, 0, BAGL_FILL, COLOR_APP, COLOR_APP, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    /// TOP STATUS BAR
    {{BAGL_LABELINE, 0x00, 0, 45, 320, 30, 0, 0, BAGL_FILL, 0xFFFFFF, COLOR_APP, BAGL_FONT_OPEN_SANS_SEMIBOLD_10_13PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "STELLAR", 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 0, 19, 56, 44, 0, 0, BAGL_FILL, COLOR_APP, COLOR_APP_LIGHT,
      BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE, 0},
     BAGL_FONT_SYMBOLS_0_SETTINGS, 0, COLOR_APP, 0xFFFFFF, io_seproxyhal_touch_settings, NULL, NULL},
    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 264, 19, 56, 44, 0, 0, BAGL_FILL, COLOR_APP, COLOR_APP_LIGHT,
      BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE, 0},
     BAGL_FONT_SYMBOLS_0_DASHBOARD, 0, COLOR_APP, 0xFFFFFF, io_seproxyhal_touch_exit, NULL, NULL},

    // BADGE_STELLAR.GIF
    {{BAGL_ICON, 0x00, 135, 178, 50, 50, 0, 0, BAGL_FILL, 0, COLOR_BG_1, 0, 0},
     &C_badge_stellar, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x00, 0, 270, 320, 30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_LIGHT_16_22PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Open Stellar wallet", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x00, 0, 308, 320, 30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Connect the Ledger Blue and open your", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x00, 0, 331, 320, 30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "preferred wallet to view your accounts.", 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x00, 0, 450, 320, 14, 0, 0, 0, 0x999999, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_8_11PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Approval requests will show automatically.", 10, 0, COLOR_BG_1, NULL, NULL, NULL},
};

unsigned int ui_idle_blue_button(unsigned int button_mask, unsigned int button_mask_counter) {
    return 0;
}

void ui_idle(void) {
    UX_DISPLAY(ui_idle_blue, NULL);
}

// ------------------------------------------------------------------------- //
//                             SETTINGS SCREEN                               //
// ------------------------------------------------------------------------- //

const bagl_element_t *ui_settings_blue_toggle_browser(const bagl_element_t *e) {
    // toggle setting and request redraw of settings elements
    uint8_t setting = N_stellar_pstate->fidoTransport ? 0 : 1;
    nvm_write(N_stellar_pstate->fidoTransport, (void *)&setting, sizeof(uint8_t));
    USB_power_U2F(0, 0);
    USB_power_U2F(1, N_stellar_pstate->fidoTransport);

    // only refresh settings mutable drawn elements
    UX_REDISPLAY_IDX(8);

    // won't redisplay the bagl_none
    return 0;
}

const bagl_element_t *ui_settings_blue_toggle_multi_ops(const bagl_element_t *e) {
    // toggle setting and request redraw of settings elements
    ctx.multiOpsSupport = (ctx.multiOpsSupport == 0x00) ? 0x01 : 0x00;

    // only refresh settings mutable drawn elements
    UX_REDISPLAY_IDX(8);

    // won't redisplay the bagl_none
    return 0;
}

// don't perform any draw/color change upon finger event over settings
const bagl_element_t *ui_settings_out_over(const bagl_element_t *e) {
    return NULL;
}

unsigned int ui_settings_back_callback(const bagl_element_t *e) {
    ui_idle();
    return 0;
}

const bagl_element_t ui_settings_blue[] = {
    {{BAGL_RECTANGLE, 0x00, 0, 68, 320, 413, 0, 0, BAGL_FILL, COLOR_BG_1, 0x000000, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    // erase screen (only under the status bar)
    {{BAGL_RECTANGLE, 0x00, 0, 20, 320, 48, 0, 0, BAGL_FILL, COLOR_APP, COLOR_APP, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    /// TOP STATUS BAR
    {{BAGL_LABELINE, 0x00, 0, 45, 320, 30, 0, 0, BAGL_FILL, 0xFFFFFF, COLOR_APP, BAGL_FONT_OPEN_SANS_SEMIBOLD_10_13PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "SETTINGS", 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 0, 19, 50, 44, 0, 0, BAGL_FILL, COLOR_APP, COLOR_APP_LIGHT,
      BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE, 0},
     BAGL_FONT_SYMBOLS_0_LEFT, 0, COLOR_APP, 0xFFFFFF, ui_settings_back_callback, NULL, NULL},
    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 264, 19, 56, 44, 0, 0, BAGL_FILL, COLOR_APP, COLOR_APP_LIGHT,
      BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE, 0},
     BAGL_FONT_SYMBOLS_0_DASHBOARD, 0, COLOR_APP, 0xFFFFFF, io_seproxyhal_touch_exit, NULL, NULL},


    {{BAGL_LABELINE, 0x00, 30, 105, 160, 30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     "Enable multi-ops", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x00, 30, 126, 260, 30, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_8_11PX, 0},
     "Enable multi-operation support", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_NONE | BAGL_FLAG_TOUCHABLE, 0x00, 0, 78, 320, 68, 0, 0, BAGL_FILL, 0xFFFFFF, 0x000000, 0, 0},
     NULL, 0, 0xEEEEEE, 0x000000, ui_settings_blue_toggle_multi_ops, ui_settings_out_over, ui_settings_out_over},

    {{BAGL_ICON, 0x01, 258, 98, 32, 18, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

#ifdef HAVE_U2F
    {{BAGL_LABELINE, 0x00, 30, 173, 160, 30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     "Browser support", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x00, 30, 194, 260, 30, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_8_11PX, 0},
     "Enable integrated browser support", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_NONE | BAGL_FLAG_TOUCHABLE, 0x00, 0, 146, 320, 68, 0, 0, BAGL_FILL, 0xFFFFFF, 0x000000, 0, 0},
     NULL, 0, 0xEEEEEE, 0x000000, ui_settings_blue_toggle_browser, ui_settings_out_over, ui_settings_out_over},

    {{BAGL_ICON, 0x02, 258, 166, 32, 18, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
#endif // HAVE_U2F
};

const bagl_element_t *ui_settings_blue_prepro(const bagl_element_t *e) {
    // none elements are skipped
    if ((e->component.type & (~BAGL_FLAG_TOUCHABLE)) == BAGL_NONE) {
        return NULL;
    }
    // swap icon buffer to be displayed depending on if corresponding setting is
    // enabled or not.
    if (e->component.userid) {
        os_memmove(&tmp_element, e, sizeof(bagl_element_t));
        switch (e->component.userid) {
        case 0x01:
            // swap icon content
            if (ctx.multiOpsSupport) {
                tmp_element.text = &C_icon_toggle_set;
            } else {
                tmp_element.text = &C_icon_toggle_reset;
            }
            break;
        case 0x02:
            // swap icon content
            if (N_stellar_pstate->fidoTransport) {
                tmp_element.text = &C_icon_toggle_set;
            } else {
                tmp_element.text = &C_icon_toggle_reset;
            }
            break;
        }
        return &tmp_element;
    }
    return 1;
}

unsigned int ui_settings_blue_button(unsigned int button_mask, unsigned int button_mask_counter) {
    return 0;
}

// ------------------------------------------------------------------------- //
//                          CONFIRM ADDRESS SCREEN                           //
// ------------------------------------------------------------------------- //

const bagl_element_t ui_address_blue[] = {
    {{BAGL_RECTANGLE, 0x00, 0, 68, 320, 413, 0, 0, BAGL_FILL, COLOR_BG_1, 0x000000, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    // erase screen (only under the status bar)
    {{BAGL_RECTANGLE, 0x00, 0, 20, 320, 48, 0, 0, BAGL_FILL, COLOR_APP, COLOR_APP, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    /// TOP STATUS BAR
    {{BAGL_LABELINE, 0x00, 0, 45, 320, 30, 0, 0, BAGL_FILL, 0xFFFFFF, COLOR_APP, BAGL_FONT_OPEN_SANS_SEMIBOLD_10_13PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "CONFIRM ADDRESS", 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x00, 30, 106, 320, 30, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0},
     "ADDRESS", 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x10, 30, 136, 260, 30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     displayString, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x11, 30, 159, 260, 30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     displayString, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 40, 414, 115, 36, 0, 18, BAGL_FILL, 0xCCCCCC, COLOR_BG_1,
      BAGL_FONT_OPEN_SANS_REGULAR_11_14PX | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE, 0},
     "REJECT", 0, 0xB7B7B7, COLOR_BG_1, io_seproxyhal_touch_address_cancel, NULL, NULL},
    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 165, 414, 115, 36, 0, 18, BAGL_FILL, 0x41ccb4, COLOR_BG_1,
      BAGL_FONT_OPEN_SANS_REGULAR_11_14PX | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE, 0},
     "CONFIRM", 0, 0x3ab7a2, COLOR_BG_1, io_seproxyhal_touch_address_ok, NULL, NULL},
};

unsigned int ui_address_blue_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        unsigned int length = strlen(ctx.req.pk.address);
        if (length >= (element->component.userid & 0xF) * MAX_CHAR_PER_LINE) {
            os_memset(displayString, 0, MAX_CHAR_PER_LINE + 1);
            os_memmove(
                displayString,
                ctx.req.pk.address + (element->component.userid & 0xF) * MAX_CHAR_PER_LINE,
                MIN(length - (element->component.userid & 0xF) * MAX_CHAR_PER_LINE, MAX_CHAR_PER_LINE));
            return 1;
        }
        // nothing to draw for this line
        return 0;
    }
    return 1;
}
unsigned int ui_address_blue_button(unsigned int button_mask, unsigned int button_mask_counter) {
    return 0;
}

void ui_show_address_init(void) {
    UX_DISPLAY(ui_address_blue, ui_address_blue_prepro);
}

// ------------------------------------------------------------------------- //
//                            VALUE DETAIL SCREEN                            //
// ------------------------------------------------------------------------- //

char *detailsTitle;
char *detailsContent;

typedef void (*callback_t)(void);
callback_t ui_details_back_callback;

const bagl_element_t * ui_details_blue_back_callback(const bagl_element_t *element) {
    ui_details_back_callback();
    return 0;
}
void ui_approval_blue_init(void);

const bagl_element_t ui_details_blue[] = {
    // erase screen (only under the status bar)
    {{BAGL_RECTANGLE, 0x00, 0, 68, 320, 413, 0, 0, BAGL_FILL, COLOR_BG_1, 0x000000, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_RECTANGLE, 0x00, 0, 20, 320, 48, 0, 0, BAGL_FILL, COLOR_APP, COLOR_APP, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    /// TOP STATUS BAR
    {{BAGL_LABELINE, 0x01, 0, 45, 320, 30, 0, 0, BAGL_FILL, 0xFFFFFF, COLOR_APP,
     BAGL_FONT_OPEN_SANS_SEMIBOLD_10_13PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     displayString, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 0, 19, 50, 44, 0, 0,
     BAGL_FILL, COLOR_APP, COLOR_APP_LIGHT, BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE, 0},
     BAGL_FONT_SYMBOLS_0_LEFT, 0, COLOR_APP, 0xFFFFFF, ui_details_blue_back_callback, NULL, NULL},

    {{BAGL_LABELINE, 0x00, 30, 106, 320, 30, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0},
     "VALUE", 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x10, 30, 136, 320, 30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     displayString, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x11, 30, 159, 320, 30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     displayString, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x12, 30, 182, 320, 30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     displayString, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x00, 0, 450, 320, 14, 0, 0, 0, 0x999999, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_8_11PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "Review the whole value before continuing.", 10, 0, COLOR_BG_1, NULL, NULL, NULL}
};

unsigned int ui_details_blue_prepro(const bagl_element_t *element) {
    if (element->component.userid == 1) {
        strcpy(displayString, (const char *)PIC(detailsTitle));
    } else if (element->component.userid > 0) {
        unsigned int length = strlen(detailsContent);
        unsigned int offset = (element->component.userid & 0xF) * MAX_CHAR_PER_LINE;
        if (length >= offset) {
            os_memset(displayString, 0, MAX_CHAR_PER_LINE + 1);
            os_memmove(displayString, detailsContent + offset, MIN(length - offset, MAX_CHAR_PER_LINE));
            return 1;
        }
        // nothing to draw for this line
        return 0;
    }
    return 1;
}

unsigned int ui_details_blue_button(unsigned int button_mask, unsigned int button_mask_counter) {
    return 0;
}

// ------------------------------------------------------------------------- //
//                       TRANSACTION APPROVAL SCREEN                         //
// ------------------------------------------------------------------------- //

const char *detailNames[6];
const char *detailValues[6];

uint8_t currentScreen;
uint16_t offsets[MAX_OPS];

void prepare_details() {
    os_memset(detailNames, 0, sizeof(detailNames));
    os_memset(detailValues, 0, sizeof(detailValues));
    if (currentScreen > 0 && offsets[currentScreen] == 0) { // transaction details (memo/fee/etc)
        strcpy(titleCaption, ((char *)PIC("Transaction level details")));
        subtitleCaption[0] = '\0';
        detailNames[0] = ((char *)PIC("MEMO"));
        detailNames[1] = ((char *)PIC("FEE"));
        detailNames[2] = ((char *)PIC("NETWORK"));
        detailNames[3] = ((char *)PIC("SOURCE"));
        detailValues[0] = ctx.req.tx.content.txDetails[0];
        detailValues[1] = ctx.req.tx.content.txDetails[1];
        detailValues[2] = ctx.req.tx.content.txDetails[2];
        detailValues[3] = ctx.req.tx.content.txDetails[3];
        if (ctx.req.tx.content.timeBounds) {
            detailNames[4] = ((char *)PIC("TIME BOUNDS"));
            detailValues[4] = ((char *)PIC("ON"));
        }
    } else { // operation details
        offsets[currentScreen+1] = parse_tx_xdr(ctx.req.tx.raw, &ctx.req.tx.content, offsets[currentScreen]);
        strcpy(titleCaption, ((char *)PIC("Operation ")));
        if (ctx.req.tx.content.opCount > 1) {
            print_uint(currentScreen+1, titleCaption+strlen(titleCaption));
            strcpy(titleCaption+strlen(titleCaption), ((char *)PIC(" of ")));
            print_uint(ctx.req.tx.content.opCount, titleCaption+strlen(titleCaption));
        }
        strcpy(subtitleCaption, ((char *)PIC(opNames[ctx.req.tx.content.opType])));
        uint8_t i, j;
        for (i = 0, j = 0; i < 5; i++) {
            if (ctx.req.tx.content.opDetails[i][0] != '\0') {
                detailNames[j] = ((char *)PIC(detailNamesTable[ctx.req.tx.content.opType][i]));
                detailValues[j] = ctx.req.tx.content.opDetails[i];
                j++;
            }
        }
        if (ctx.req.tx.content.opSource[0] != '\0') {
            detailNames[j] = ((char *)PIC("SOURCE"));
            detailValues[j] = ctx.req.tx.content.opSource;
        }
    }
}

const bagl_element_t *ui_approval_common_show_details(const bagl_element_t *e) {
    uint8_t detailIdx = e->component.userid & 0xF;
    char *detailVal = detailValues[detailIdx];
    if (detailVal != NULL && strlen(detailVal) * BAGL_FONT_OPEN_SANS_REGULAR_10_13PX_AVG_WIDTH >= 160) {
        // display details screen
        detailsTitle = detailNames[detailIdx];
        detailsContent = detailVal;
        ui_details_back_callback = ui_approval_blue_init;
        UX_DISPLAY(ui_details_blue, ui_details_blue_prepro);
    }
    return NULL;
}

const bagl_element_t *ui_approval_prev(const bagl_element_t *e) {
    currentScreen--;
    prepare_details();
    ui_approval_blue_init();
    return NULL;
}

const bagl_element_t *ui_approval_next(const bagl_element_t *e) {
    currentScreen++;
    prepare_details();
    ui_approval_blue_init();
    return NULL;
}

const bagl_element_t *ui_menu_item_out_over(const bagl_element_t *e) {
    // the selection rectangle is after the none|touchable
    e = (const bagl_element_t *)(((unsigned int)e) + sizeof(bagl_element_t));
    return e;
}

const bagl_element_t ui_approval_blue[] = {
    {{BAGL_RECTANGLE, 0x00, 0, 68, 320, 413, 0, 0, BAGL_FILL, COLOR_BG_1, 0x000000, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    // erase screen (only under the status bar)
    {{BAGL_RECTANGLE, 0x00, 0, 20, 320, 48, 0, 0, BAGL_FILL, COLOR_APP, COLOR_APP, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    /// TOP STATUS BAR
    {{BAGL_LABELINE, 0x60, 0, 45, 320, 30, 0, 0, BAGL_FILL, 0xFFFFFF, COLOR_APP, BAGL_FONT_OPEN_SANS_SEMIBOLD_10_13PX | BAGL_FONT_ALIGNMENT_CENTER, 0},
     "CONFIRM TRANSACTION", 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x01, 0, 19, 50, 44, 0, 0,
     BAGL_FILL, COLOR_APP, COLOR_APP_LIGHT, BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE, 0},
     BAGL_FONT_SYMBOLS_0_LEFT, 0, COLOR_APP, 0xFFFFFF, ui_approval_prev, NULL, NULL},

    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x02, 270, 19, 50, 44, 0, 0,
     BAGL_FILL, COLOR_APP, COLOR_APP_LIGHT, BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE, 0},
     BAGL_FONT_SYMBOLS_0_MINIRIGHT, 0, COLOR_APP, 0xFFFFFF, ui_approval_next, NULL, NULL},

    // BADGE_TRANSACTION.GIF
    {{BAGL_ICON, 0x40, 30, 88, 50, 50, 0, 0, BAGL_FILL, 0, COLOR_BG_1, 0, 0}, &C_badge_transaction, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x50, 100, 107, 320, 30, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     titleCaption, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x00, 100, 128, 320, 30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0},
    subtitleCaption, 0, 0, 0, NULL, NULL, NULL},

    // Details 1
    {{BAGL_LABELINE, 0x70, 30, 179, 120, 20, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x10, 130, 179, 160, 20, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_RIGHT, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x20, 284, 179, 6, 16, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_RIGHT, 0},
     BAGL_FONT_SYMBOLS_0_MINIRIGHT, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_NONE | BAGL_FLAG_TOUCHABLE, 0x80, 0, 158, 320, 34, 0, 9, BAGL_FILL, 0xFFFFFF, 0x000000, 0, 0},
     NULL, 0, 0xEEEEEE, 0x000000, ui_approval_common_show_details, ui_menu_item_out_over, ui_menu_item_out_over},
    {{BAGL_RECTANGLE, 0x20, 0, 158, 5, 34, 0, 0, BAGL_FILL, COLOR_BG_1, COLOR_BG_1, 0, 0},
     NULL, 0, 0x41CCB4, 0, NULL, NULL, NULL},

    {{BAGL_RECTANGLE, 0x30, 30, 192, 260, 1, 1, 0, 0, 0xEEEEEE, COLOR_BG_1, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},

    // Details 2
    {{BAGL_LABELINE, 0x71, 30, 214, 100, 23, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x11, 130, 214, 160, 23, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_RIGHT, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x21, 284, 214, 6, 16, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_RIGHT, 0},
     BAGL_FONT_SYMBOLS_0_MINIRIGHT, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_NONE | BAGL_FLAG_TOUCHABLE, 0x81, 0, 193, 320, 34, 0, 9, BAGL_FILL, 0xFFFFFF, 0x000000, 0, 0},
     NULL, 0, 0xEEEEEE, 0x000000, ui_approval_common_show_details, ui_menu_item_out_over, ui_menu_item_out_over},
    {{BAGL_RECTANGLE, 0x21, 0, 193, 5, 34, 0, 0, BAGL_FILL, COLOR_BG_1, COLOR_BG_1, 0, 0},
     NULL, 0, 0x41CCB4, 0, NULL, NULL, NULL},

    {{BAGL_RECTANGLE, 0x31, 30, 227, 260, 1, 1, 0, 0, 0xEEEEEE, COLOR_BG_1, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},

    // Details 3
    {{BAGL_LABELINE, 0x72, 30, 249, 120, 20, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x12, 130, 249, 160, 20, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_RIGHT, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x22, 284, 249, 6, 16, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_RIGHT, 0},
     BAGL_FONT_SYMBOLS_0_MINIRIGHT, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_NONE | BAGL_FLAG_TOUCHABLE, 0x82, 0, 228, 320, 34, 0, 9, BAGL_FILL, 0xFFFFFF, 0x000000, 0, 0},
     NULL, 0, 0xEEEEEE, 0x000000, ui_approval_common_show_details, ui_menu_item_out_over, ui_menu_item_out_over},
    {{BAGL_RECTANGLE, 0x22, 0, 228, 5, 34, 0, 0, BAGL_FILL, COLOR_BG_1, COLOR_BG_1, 0, 0},
     NULL, 0, 0x41CCB4, 0, NULL, NULL, NULL},

    {{BAGL_RECTANGLE, 0x32, 30, 262, 260, 1, 1, 0, 0, 0xEEEEEE, COLOR_BG_1, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},

    // Details 4
    {{BAGL_LABELINE, 0x73, 30, 284, 100, 23, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x13, 130, 284, 160, 23, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_RIGHT, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x23, 284, 284, 6, 16, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_RIGHT, 0},
     BAGL_FONT_SYMBOLS_0_MINIRIGHT, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_NONE | BAGL_FLAG_TOUCHABLE, 0x83, 0, 263, 320, 34, 0, 9, BAGL_FILL, 0xFFFFFF, 0x000000, 0, 0},
     NULL, 0, 0xEEEEEE, 0x000000, ui_approval_common_show_details, ui_menu_item_out_over, ui_menu_item_out_over},
    {{BAGL_RECTANGLE, 0x23, 0, 263, 5, 34, 0, 0, BAGL_FILL, COLOR_BG_1, COLOR_BG_1, 0, 0},
     NULL, 0, 0x41CCB4, 0, NULL, NULL, NULL},

    {{BAGL_RECTANGLE, 0x33, 30, 297, 260, 1, 1, 0, 0, 0xEEEEEE, COLOR_BG_1, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},

    // Details 5
    {{BAGL_LABELINE, 0x74, 30, 319, 100, 23, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x14, 130, 319, 160, 23, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_RIGHT, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x24, 284, 319, 6, 16, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_RIGHT, 0},
     BAGL_FONT_SYMBOLS_0_MINIRIGHT, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_NONE | BAGL_FLAG_TOUCHABLE, 0x84, 0, 298, 320, 34, 0, 9, BAGL_FILL, 0xFFFFFF, 0x000000, 0, 0},
     NULL, 0, 0xEEEEEE, 0x000000, ui_approval_common_show_details, ui_menu_item_out_over, ui_menu_item_out_over},
    {{BAGL_RECTANGLE, 0x24, 0, 298, 5, 34, 0, 0, BAGL_FILL, COLOR_BG_1, COLOR_BG_1, 0, 0},
     NULL, 0, 0x41CCB4, 0, NULL, NULL, NULL},

    {{BAGL_RECTANGLE, 0x34, 30, 332, 260, 1, 1, 0, 0, 0xEEEEEE, COLOR_BG_1, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},

    // Details 6
    {{BAGL_LABELINE, 0x75, 30, 354, 100, 23, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x15, 130, 354, 160, 23, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_RIGHT, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x25, 284, 354, 6, 16, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_RIGHT, 0},
     BAGL_FONT_SYMBOLS_0_MINIRIGHT, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_NONE | BAGL_FLAG_TOUCHABLE, 0x85, 0, 333, 320, 34, 0, 9, BAGL_FILL, 0xFFFFFF, 0x000000, 0, 0},
     NULL, 0, 0xEEEEEE, 0x000000, ui_approval_common_show_details, ui_menu_item_out_over, ui_menu_item_out_over},
    {{BAGL_RECTANGLE, 0x25, 0, 333, 5, 34, 0, 0, BAGL_FILL, COLOR_BG_1, COLOR_BG_1, 0, 0},
     NULL, 0, 0x41CCB4, 0, NULL, NULL, NULL},

    {{BAGL_RECTANGLE, 0x35, 30, 367, 260, 1, 1, 0, 0, 0xEEEEEE, COLOR_BG_1, 0, 0}, NULL, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 40, 414, 115, 36, 0, 18, BAGL_FILL, 0xCCCCCC, COLOR_BG_1,
      BAGL_FONT_OPEN_SANS_REGULAR_11_14PX | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE, 0},
      "REJECT", 0, 0xB7B7B7, COLOR_BG_1, io_seproxyhal_touch_tx_cancel, NULL, NULL},
    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 165, 414, 115, 36, 0, 18, BAGL_FILL, 0x41ccb4, COLOR_BG_1,
      BAGL_FONT_OPEN_SANS_REGULAR_11_14PX | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE, 0},
      "CONFIRM", 0, 0x3ab7a2, COLOR_BG_1, io_seproxyhal_touch_tx_ok, NULL, NULL},
};

const bagl_element_t *ui_approval_blue_prepro(const bagl_element_t *element) {
    // none elements are skipped
    if ((element->component.type & (~BAGL_FLAG_TOUCHABLE)) == BAGL_NONE) {
        return 0;
    } else {
        uint8_t type = element->component.userid & 0xF0;
        uint8_t index = element->component.userid & 0xF;
        switch (type) {
        case 0x00: {
            if (element->component.userid == 0x01) {
                return currentScreen > 0;
            }
            if (element->component.userid == 0x02) {
                return currentScreen < ctx.req.tx.content.opCount;
            }
            return 1;
        }
        case 0x40:
        case 0x60:
        case 0x50:
            return 1;

        // detail label
        case 0x70:
            if (!detailNames[index]) {
                return NULL;
            }
            if (!detailValues[index]) {
                return NULL;
            }
            os_memmove(&tmp_element, element, sizeof(bagl_element_t));
            tmp_element.text = detailNames[index];
            return &tmp_element;

        // detail value
        case 0x10:
            if (!detailNames[index]) {
                return NULL;
            }
            if (!detailValues[index]) {
                return NULL;
            }
            os_memmove(&tmp_element, element, sizeof(bagl_element_t));
            tmp_element.text = detailValues[index];

            // x -= 18 when overflow is detected
            if (strlen(detailValues[index]) * BAGL_FONT_OPEN_SANS_REGULAR_10_13PX_AVG_WIDTH >= 160) {
                tmp_element.component.x -= 18;
            }
            return &tmp_element;

        // right arrow and left selection rectangle
        case 0x20:
            if (!detailNames[index]) {
                return NULL;
            }
            if (strlen(detailValues[index]) * BAGL_FONT_OPEN_SANS_REGULAR_10_13PX_AVG_WIDTH < 160) {
                return NULL;
            }
            break;
        // horizontal delimiter
        case 0x30:
            if (index >= 5 || !detailNames[index+1]) {
                return NULL;
            }
        }
    }
    return element;
}

unsigned int ui_approval_blue_button(unsigned int button_mask, unsigned int button_mask_counter) {
    return 0;
}

void ui_approval_blue_init(void) {
    UX_DISPLAY(ui_approval_blue, ui_approval_blue_prepro);
}

void ui_approve_tx_hash_init(void) {
    currentScreen = 0;
    os_memset(detailNames, 0, sizeof(detailNames));
    os_memset(detailValues, 0, sizeof(detailValues));
    detailNames[0] = ((char *)PIC("Hash"));;
    char hashSummary[65];
    print_hash(ctx.req.tx.hash, hashSummary);
    detailValues[0] = hashSummary;
    strcpy(titleCaption, "WARNING");
    strcpy(subtitleCaption, "No details available");
    ui_approval_blue_init();
}

void ui_approve_tx_init(void) {
    currentScreen = 0;
    offsets[0] = 0;
    prepare_details();
    ui_approval_blue_init();
}

unsigned int io_seproxyhal_touch_settings(const bagl_element_t *e) {
    UX_DISPLAY(ui_settings_blue, ui_settings_blue_prepro);
    return 0; // do not redraw button, screen has switched
}

unsigned int io_seproxyhal_touch_exit(const bagl_element_t *e) {
    // Go back to the dashboard
    os_sched_exit(0);
    return 0; // do not redraw the widget
}

#endif