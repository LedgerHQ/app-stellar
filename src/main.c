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

#include "os.h"
#include "cx.h"
#include <stdbool.h>
#include <limits.h>

#include "os_io_seproxyhal.h"
#include "string.h"

#include "glyphs.h"

#include "base32.h"
#include "xdr_parser.h"
#include "stlr_utils.h"

#ifdef HAVE_U2F

#include "u2f_service.h"
#include "u2f_transport.h"

volatile unsigned char u2fMessageBuffer[U2F_MAX_MESSAGE_SIZE];

extern void USB_power_U2F(unsigned char enabled, unsigned char fido);
extern bool fidoActivated;

#endif

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

uint32_t set_result_get_publicKey(void);

#define MAX_BIP32_LEN 10

#define CLA 0xE0
#define INS_GET_PUBLIC_KEY 0x02
#define INS_SIGN_TX 0x04
#define INS_GET_APP_CONFIGURATION 0x06
#define INS_SIGN_TX_HASH 0x08
#define INS_KEEP_ALIVE 0x10
#define P1_NO_SIGNATURE 0x00
#define P1_SIGNATURE 0x01
#define P2_NO_CONFIRM 0x00
#define P2_CONFIRM 0x01
#define P1_FIRST 0x00
#define P1_MORE 0x80
#define P2_LAST 0x00
#define P2_MORE 0x80

#define OFFSET_CLA 0
#define OFFSET_INS 1
#define OFFSET_P1 2
#define OFFSET_P2 3
#define OFFSET_LC 4
#define OFFSET_CDATA 5

#define MAX_UI_STEPS 10

#define MAX_RAW_TX 1024

typedef struct {
    cx_ecfp_public_key_t publicKey;
    char address[57];
    uint8_t signature[64];
    bool returnSignature;

} pk_context_t;

typedef struct {
    uint8_t bip32Len;
    uint32_t bip32[MAX_BIP32_LEN];
    uint8_t raw[MAX_RAW_TX];
    uint32_t rawLength;
    uint8_t hash[32];
    tx_content_t content;
    uint16_t offset;
} tx_context_t;

typedef struct {
    union {
        pk_context_t pk;
        tx_context_t tx;
    } req;
    uint16_t u2fTimer;
} stellar_context_t;

stellar_context_t ctx;

volatile uint8_t multiOpsSupport;

bagl_element_t tmp_element;

#if defined(TARGET_BLUE)
volatile char operationCaption[15];
volatile char displayString[33];
volatile char subtitleCaption[16];
#endif

#ifdef HAVE_U2F

volatile uint8_t fidoTransport;
volatile u2f_service_t u2fService;

#endif

unsigned int io_seproxyhal_touch_settings(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_exit(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_tx_ok(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_tx_cancel(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_address_ok(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_address_cancel(const bagl_element_t *e);
void ui_idle(void);
ux_state_t ux;
// display stepped screens
uint8_t ux_step;
uint8_t ux_step_count;

typedef struct internalStorage_t {
    uint8_t fidoTransport;
    uint8_t initialized;
} internalStorage_t;

WIDE internalStorage_t N_storage_real;
#define N_storage (*(WIDE internalStorage_t *)PIC(&N_storage_real))

#ifdef HAVE_U2F

void u2f_proxy_response(u2f_service_t *service, unsigned int tx) {
    os_memset(service->messageBuffer, 0, 5);
    os_memmove(service->messageBuffer + 5, G_io_apdu_buffer, tx);
    service->messageBuffer[tx + 5] = 0x90;
    service->messageBuffer[tx + 6] = 0x00;
    u2f_send_fragmented_response(service, U2F_CMD_MSG, service->messageBuffer, tx + 7, true);
}

#endif

const bagl_element_t *ui_menu_item_out_over(const bagl_element_t *e) {
    // the selection rectangle is after the none|touchable
    e = (const bagl_element_t *)(((unsigned int)e) + sizeof(bagl_element_t));
    return e;
}

#if defined(TARGET_BLUE)

#define BAGL_FONT_OPEN_SANS_LIGHT_16_22PX_AVG_WIDTH 10
#define BAGL_FONT_OPEN_SANS_REGULAR_10_13PX_AVG_WIDTH 8
#define MAX_CHAR_PER_LINE 28

#define COLOR_BG_1 0xF9F9F9
#define COLOR_APP 0x27a2db
#define COLOR_APP_LIGHT 0x93d1ed

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
#endif // #if TARGET_BLUE

#if defined(TARGET_NANOS)

const ux_menu_entry_t menu_main[];
const ux_menu_entry_t menu_settings[];
const ux_menu_entry_t menu_settings_browser[];
const ux_menu_entry_t menu_settings_multi_ops[];

// change the setting
void menu_settings_browser_change(unsigned int enabled) {
    fidoTransport = enabled;
    nvm_write(&N_storage.fidoTransport, (void *)&fidoTransport, sizeof(uint8_t));
    USB_power_U2F(0, 0);
    USB_power_U2F(1, N_storage.fidoTransport);
    // go back to the menu entry
    UX_MENU_DISPLAY(1, menu_settings, NULL);
}

void menu_settings_multi_ops_change(unsigned int enabled) {
    multiOpsSupport = enabled;
    UX_MENU_DISPLAY(1, menu_settings, NULL);
}

// show the currently activated entry
void menu_settings_browser_init(unsigned int ignored) {
    UNUSED(ignored);
    UX_MENU_DISPLAY(N_storage.fidoTransport ? 1 : 0, menu_settings_browser, NULL);
}

void menu_settings_multi_ops_init(unsigned int ignored) {
    UNUSED(ignored);
    UX_MENU_DISPLAY(multiOpsSupport ? 1 : 0, menu_settings_multi_ops, NULL);
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

#endif // #if TARGET_NANOS

#if defined(TARGET_BLUE)

const bagl_element_t *ui_settings_blue_toggle_browser(const bagl_element_t *e) {
    // toggle setting and request redraw of settings elements
    uint8_t setting = N_storage.fidoTransport ? 0 : 1;
    nvm_write(&N_storage.fidoTransport, (void *)&setting, sizeof(uint8_t));
    USB_power_U2F(0, 0);
    USB_power_U2F(1, N_storage.fidoTransport);

    // only refresh settings mutable drawn elements
    UX_REDISPLAY_IDX(8);

    // won't redisplay the bagl_none
    return 0;
}

const bagl_element_t *ui_settings_blue_toggle_multi_ops(const bagl_element_t *e) {
    // toggle setting and request redraw of settings elements
    multiOpsSupport = (multiOpsSupport == 0) ? 1 : 0;

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

#ifdef HAVE_U2F
    {{BAGL_LABELINE, 0x00, 30, 105, 160, 30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     "Enable multi-ops", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x00, 30, 126, 260, 30, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_8_11PX, 0},
     "Enable multi-operation support", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_NONE | BAGL_FLAG_TOUCHABLE, 0x00, 0, 78, 320, 68, 0, 0, BAGL_FILL, 0xFFFFFF, 0x000000, 0, 0},
     NULL, 0, 0xEEEEEE, 0x000000, ui_settings_blue_toggle_multi_ops, ui_settings_out_over, ui_settings_out_over},

    {{BAGL_ICON, 0x01, 258, 98, 32, 18, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
#endif // HAVE_U2F

    {{BAGL_LABELINE, 0x00, 30, 173, 160, 30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX, 0},
     "Browser support", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x00, 30, 194, 260, 30, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_8_11PX, 0},
     "Enable integrated browser support", 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_NONE | BAGL_FLAG_TOUCHABLE, 0x00, 0, 146, 320, 68, 0, 0, BAGL_FILL, 0xFFFFFF, 0x000000, 0, 0},
     NULL, 0, 0xEEEEEE, 0x000000, ui_settings_blue_toggle_browser, ui_settings_out_over, ui_settings_out_over},

    {{BAGL_ICON, 0x02, 258, 166, 32, 18, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

};

const bagl_element_t *ui_settings_blue_prepro(const bagl_element_t *e) {
    // none elements are skipped
    if ((e->component.type & (~BAGL_FLAG_TOUCHABLE)) == BAGL_NONE) {
        return 0;
    }
    // swap icon buffer to be displayed depending on if corresponding setting is
    // enabled or not.
    if (e->component.userid) {
        os_memmove(&tmp_element, e, sizeof(bagl_element_t));
        switch (e->component.userid) {
        case 0x01:
            // swap icon content
            if (multiOpsSupport) {
                tmp_element.text = &C_icon_toggle_set;
            } else {
                tmp_element.text = &C_icon_toggle_reset;
            }
            break;
        case 0x02:
            // swap icon content
            if (N_storage.fidoTransport) {
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

#endif // #if defined(TARGET_NANOS)

#if defined(TARGET_BLUE)
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
#endif // #if defined(TARGET_BLUE)

#if defined(TARGET_NANOS)
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

unsigned int ui_address_prepro(const bagl_element_t *element) {
    if (element->component.userid == 0x02) {
        UX_CALLBACK_SET_INTERVAL(MAX(1500, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
    }
    return 1;
}

unsigned int ui_address_nanos_button(unsigned int button_mask, unsigned int button_mask_counter);
#endif // #if defined(TARGET_NANOS)

#if defined(TARGET_BLUE)

char *ui_details_title;
char *ui_details_content;
typedef void (*callback_t)(void);
callback_t ui_details_back_callback;

const bagl_element_t * ui_details_blue_back_callback(const bagl_element_t *element) {
    ui_details_back_callback();
    return 0;
}

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
        strcpy(displayString, (const char *)PIC(ui_details_title));
    } else if (element->component.userid > 0) {
        unsigned int length = strlen(ui_details_content);
        unsigned int offset = (element->component.userid & 0xF) * MAX_CHAR_PER_LINE;
        if (length >= offset) {
            os_memset(displayString, 0, MAX_CHAR_PER_LINE + 1);
            os_memmove(displayString, ui_details_content + offset, MIN(length - offset, MAX_CHAR_PER_LINE));
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

void ui_approval_blue_init(void);

static const char *op_names[16] =
    {"Create Account", "Payment", "Path Payment", "New Offer", "Remove Offer", "Change Offer",
     "Set Options", "Change Trust", "Remove Trust", "Allow Trust", "Revoke Trust",
     "Merge Account", "Inflation", "Set Data", "Remove Data", "Unknown"};

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
    {"DESTINATION", NULL, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL, NULL},
    {"NAME", "VALUE", NULL, NULL, NULL},
    {"NAME", NULL, NULL, NULL, NULL}
};

const char *detailNames[7];
const char *detailValues[7];
uint8_t currentScreen;
uint16_t offsets[10];

const void initDetails() {
    os_memset(detailNames, 0, sizeof(detailNames));
    os_memset(detailValues, 0, sizeof(detailValues));
    if (ctx.req.tx.rawLength > 0) { // parse raw tx
        if (currentScreen > 0 && offsets[currentScreen] == 0) { // transaction details (memo/fee/etc)
            strcpy(operationCaption, ((char *)PIC("Extra details")));
            detailNames[0] = ((char *)PIC("MEMO"));
            detailNames[1] = ((char *)PIC("FEE"));
            detailNames[2] = ((char *)PIC("NETWORK"));
            detailValues[0] = ctx.req.tx.content.txDetails[0];
            detailValues[1] = ctx.req.tx.content.txDetails[1];
            detailValues[2] = ctx.req.tx.content.txDetails[2];
        } else { // operation details
            os_memset(&ctx.req.tx.content.opDetails, 0, sizeof(ctx.req.tx.content.opDetails));
            offsets[currentScreen+1] = parseTxXdr(ctx.req.tx.raw, &ctx.req.tx.content, offsets[currentScreen]);
            strcpy(operationCaption, ((char *)PIC(op_names[ctx.req.tx.content.opType])));
            uint8_t i, j;
            for (i = 0, j = 0; i < 5; i++) {
                if (ctx.req.tx.content.opDetails[i][0] != '\0') {
                    detailNames[j] = ((char *)PIC(detailNamesTable[ctx.req.tx.content.opType][i]));
                    detailValues[j] = ctx.req.tx.content.opDetails[i];
                    j++;
                }
            }
        }
//        strcpy(subtitleCaption, ctx.req.tx.content.txDetails[3]);
    }
}



const bagl_element_t *ui_approval_common_show_details(const bagl_element_t *e) {
    uint8_t detailIdx = e->component.userid & 0xF;
    char * detailVal = detailValues[detailIdx];
    if (detailVal != NULL && strlen(detailVal) * BAGL_FONT_OPEN_SANS_REGULAR_10_13PX_AVG_WIDTH >= 160) {
        // display details screen
        ui_details_title = detailNames[detailIdx];
        ui_details_content = detailVal;
        ui_details_back_callback = ui_approval_blue_init;
        UX_DISPLAY(ui_details_blue, ui_details_blue_prepro);
    }
    return NULL;
}

const bagl_element_t *ui_approval_prev(const bagl_element_t *e) {
    currentScreen--;
    initDetails();
    ui_approval_blue_init();
    return NULL;
}

const bagl_element_t *ui_approval_next(const bagl_element_t *e) {
    currentScreen++;
    initDetails();
    ui_approval_blue_init();
    return NULL;
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
     operationCaption, 0, 0, 0, NULL, NULL, NULL},

    {{BAGL_LABELINE, 0x00, 100, 128, 320, 30, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0},
     subtitleCaption, 0, 0, 0, NULL, NULL, NULL},

    // Fee
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

    {{BAGL_RECTANGLE, 0x30, 30, 192, 260, 1, 1, 0, 0, 0xEEEEEE, COLOR_BG_1, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    // Memo
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

    {{BAGL_RECTANGLE, 0x31, 30, 227, 260, 1, 1, 0, 0, 0xEEEEEE, COLOR_BG_1, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    // Details 1
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

    {{BAGL_RECTANGLE, 0x32, 30, 262, 260, 1, 1, 0, 0, 0xEEEEEE, COLOR_BG_1, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    // Details 2
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

    {{BAGL_RECTANGLE, 0x33, 30, 297, 260, 1, 1, 0, 0, 0xEEEEEE, COLOR_BG_1, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    // Details 3
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

    {{BAGL_RECTANGLE, 0x34, 30, 332, 260, 1, 1, 0, 0, 0xEEEEEE, COLOR_BG_1, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    // Details 4
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

    {{BAGL_RECTANGLE, 0x35, 30, 367, 260, 1, 1, 0, 0, 0xEEEEEE, COLOR_BG_1, 0, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},

    // Details 5
    {{BAGL_LABELINE, 0x76, 30, 389, 100, 23, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x16, 130, 389, 160, 23, 0, 0, BAGL_FILL, 0x000000, COLOR_BG_1, BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_RIGHT, 0},
     NULL, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_LABELINE, 0x26, 284, 389, 6, 16, 0, 0, BAGL_FILL, 0x999999, COLOR_BG_1, BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_RIGHT, 0},
     BAGL_FONT_SYMBOLS_0_MINIRIGHT, 0, 0, 0, NULL, NULL, NULL},
    {{BAGL_NONE | BAGL_FLAG_TOUCHABLE, 0x86, 0, 368, 320, 34, 0, 9, BAGL_FILL, 0xFFFFFF, 0x000000, 0, 0},
     NULL, 0, 0xEEEEEE, 0x000000, ui_approval_common_show_details, ui_menu_item_out_over, ui_menu_item_out_over},
    {{BAGL_RECTANGLE, 0x26, 0, 368, 5, 34, 0, 0, BAGL_FILL, COLOR_BG_1, COLOR_BG_1, 0, 0},
     NULL, 0, 0x41CCB4, 0, NULL, NULL, NULL},

    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 40, 414, 115, 36, 0, 18, BAGL_FILL, 0xCCCCCC, COLOR_BG_1,
      BAGL_FONT_OPEN_SANS_REGULAR_11_14PX | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE, 0},
      "REJECT", 0, 0xB7B7B7, COLOR_BG_1, io_seproxyhal_touch_tx_cancel, NULL, NULL},
    {{BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, 0x00, 165, 414, 115, 36, 0, 18, BAGL_FILL, 0x41ccb4, COLOR_BG_1,
      BAGL_FONT_OPEN_SANS_REGULAR_11_14PX | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE, 0},
      "CONFIRM", 0, 0x3ab7a2, COLOR_BG_1, io_seproxyhal_touch_tx_ok, NULL, NULL},
};

const bagl_element_t *ui_approval_blue_prepro(const bagl_element_t *element) {
    if (element->component.userid == 0x00) {
        return 1;
    }
    if (element->component.userid == 0x01) {
        return currentScreen > 0;
    }
    if (element->component.userid == 0x02) {
        return currentScreen < ctx.req.tx.content.opCount;
    }

    // none elements are skipped
    if ((element->component.type & (~BAGL_FLAG_TOUCHABLE)) == BAGL_NONE) {
        return 0;
    } else {
        switch (element->component.userid & 0xF0) {
        // icon
        case 0x40:
        // TITLE
        case 0x60:
        // SUBLINE
        case 0x50:
            return 1;

        // details label
        case 0x70:
            if (!detailNames[element->component.userid & 0xF]) {
                return NULL;
            }
            if (!detailValues[element->component.userid & 0xF]) {
                return NULL;
            }
            os_memmove(&tmp_element, element, sizeof(bagl_element_t));
            tmp_element.text = detailNames[element->component.userid & 0xF];
            return &tmp_element;

        // detail value
        case 0x10:
            if (!detailNames[element->component.userid & 0xF]) {
                return NULL;
            }
            if (!detailValues[element->component.userid & 0xF]) {
                return NULL;
            }
            os_memmove(&tmp_element, element, sizeof(bagl_element_t));
            tmp_element.text = detailValues[element->component.userid & 0xF];

            // x -= 18 when overflow is detected
            if (strlen(tmp_element.text) * BAGL_FONT_OPEN_SANS_REGULAR_10_13PX_AVG_WIDTH >= 160) {
                tmp_element.component.x -= 18;
            }
            return &tmp_element;

        // right arrow and left selection rectangle
        case 0x20:
            if (!detailNames[element->component.userid & 0xF]) {
                return NULL;
            }
            if (strlen(detailValues[element->component.userid & 0xF]) * BAGL_FONT_OPEN_SANS_REGULAR_10_13PX_AVG_WIDTH < 160) {
                return NULL;
            }

        // horizontal delimiter
        case 0x30:
            return detailNames[element->component.userid & 0xF] ? element : NULL;
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

void ui_approve_tx_blue_init(void) {
    currentScreen = 0;
    offsets[0] = 0;
    initDetails();
    ui_approval_blue_init();
}

void ui_approve_tx_hash_blue_init(void) {
    os_memset(detailNames, 0, sizeof(detailNames));
    os_memset(detailValues, 0, sizeof(detailValues));
    detailNames[0] = ((char *)PIC("Hash"));
    detailValues[0] = ctx.req.tx.content.txDetails[0];
    strcpy(operationCaption, "WARNING");
    strcpy(subtitleCaption, "No details available");
    ui_approval_blue_init();
}

#endif // #if defined(TARGET_BLUE)

#if defined(TARGET_NANOS)

static const char * op_captions[][6] = {
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
    {"Merge Account", NULL, NULL, NULL, NULL},
    {"Operation", NULL, NULL, NULL, NULL},
    {"Set Data", "Value", NULL, NULL, NULL},
    {"Remove Data", NULL, NULL, NULL, NULL},
    {"WARNING", "Hash", NULL, NULL, NULL}
};

static const char * tx_captions[4] = {"Memo", "Fee", "Network", "Source Account"};

const uint8_t countOpDetails() {
    uint8_t i;
    for (i = 0; i < 5; i++) {
        if (((char*) PIC(op_captions[ctx.req.tx.content.opType][i])) == NULL) {
            return i;
        }
    }
    return i;
}

const uint8_t countSteps() {
    if (ctx.req.tx.content.opIdx == ctx.req.tx.content.opCount) {
        // last operation: also show tx details
        return countOpDetails() + 4 + 1;
    }
    return countOpDetails() + 1;
}

const void prepareDetails() {
    if (ctx.req.tx.rawLength > 0) { // parse raw tx
        os_memset(&ctx.req.tx.content.opDetails, 0, sizeof(ctx.req.tx.content.opDetails));
        ctx.req.tx.offset = parseTxXdr(ctx.req.tx.raw, &ctx.req.tx.content, ctx.req.tx.offset);
        ux_step_count = countSteps();
    }
}


const bagl_element_t *ui_tx_approval_prepro(const bagl_element_t *element) {
    switch (element->component.userid) {
    case 0x00: // Controls: always visible
        return element;
    case 0x01:
    case 0x11: // "Confirm transaction"
        if (ux_step == 0) {
            if (element->component.userid == 0x01) { // prepare details once per operation
                prepareDetails();
            }
            if (ctx.req.tx.content.opIdx == 1) {
                UX_CALLBACK_SET_INTERVAL(1800);
                return element;
            } else {
                ux_step++; // skip and fall through
            }
        } else {
            return NULL;
        }
    case 0x02: // Details caption
    case 0x12: // Details value
        if (ux_step > 0) {
            uint8_t opDetailsCount = countOpDetails();
            while (ux_step > 0 && ux_step <= opDetailsCount) {
                uint8_t detailIdx = ux_step - 1;
                char *value = ctx.req.tx.content.opDetails[detailIdx];
                if (value[0] == '\0') {
                    ux_step++; // skip empty value
                } else {
                    os_memmove(&tmp_element, element, sizeof(bagl_element_t));
                    if (element->component.userid == 0x12) {
                        tmp_element.text = value;
                    } else {
                        tmp_element.text = ((char*) PIC(op_captions[ctx.req.tx.content.opType][detailIdx]));
                    }
                    UX_CALLBACK_SET_INTERVAL(MAX(1500, 1000 + bagl_label_roundtrip_duration_ms(&tmp_element, 7)));
                    return &tmp_element;
                }
            }
            if (ux_step > opDetailsCount) {
                os_memmove(&tmp_element, element, sizeof(bagl_element_t));
                uint8_t detailIdx = ux_step - opDetailsCount - 1;
                if (element->component.userid == 0x12) {
                    tmp_element.text = ctx.req.tx.content.txDetails[detailIdx];
                } else {
                    tmp_element.text = tx_captions[detailIdx];
                }
                UX_CALLBACK_SET_INTERVAL(MAX(1500, 1000 + bagl_label_roundtrip_duration_ms(&tmp_element, 7)));
                return &tmp_element;
            }
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
#endif // #if defined(TARGET_NANOS)

void ui_idle(void) {
#if defined(TARGET_BLUE)
    UX_DISPLAY(ui_idle_blue, NULL);
#elif defined(TARGET_NANOS)
    UX_MENU_DISPLAY(0, menu_main, NULL);
#endif // #if TARGET_ID
}

#if defined(TARGET_BLUE)
unsigned int io_seproxyhal_touch_settings(const bagl_element_t *e) {
    UX_DISPLAY(ui_settings_blue, ui_settings_blue_prepro);
    return 0; // do not redraw button, screen has switched
}
#endif // #if defined(TARGET_BLUE)


unsigned int io_seproxyhal_touch_exit(const bagl_element_t *e) {
    // Go back to the dashboard
    os_sched_exit(0);
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_address_ok(const bagl_element_t *e) {
    uint32_t tx = set_result_get_publicKey();
    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;
#ifdef HAVE_U2F
    if (fidoActivated) {
        u2f_proxy_response((u2f_service_t *)&u2fService, tx);
    } else {
        // Send back the response, do not restart the event loop
        io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
    }
#else  // HAVE_U2F
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
#endif // HAVE_U2F
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_address_cancel(const bagl_element_t *e) {
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
#ifdef HAVE_U2F
    if (fidoActivated) {
        u2f_proxy_response((u2f_service_t *)&u2fService, 2);
    } else {
        // Send back the response, do not restart the event loop
        io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    }
#else  // HAVE_U2F
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
#endif // HAVE_U2F
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

#if defined(TARGET_NANOS)
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
#endif // #if defined(TARGET_NANOS)

unsigned int io_seproxyhal_touch_tx_ok(const bagl_element_t *e) {
    uint32_t tx = 0;

    // initialize private key
    uint8_t privateKeyData[32];
    cx_ecfp_private_key_t privateKey;
    os_perso_derive_node_bip32(CX_CURVE_Ed25519, ctx.req.tx.bip32, ctx.req.tx.bip32Len, privateKeyData, NULL);
    cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyData, 32, &privateKey);
    os_memset(privateKeyData, 0, sizeof(privateKeyData));

    // sign hash
#if CX_APILEVEL >= 8
    tx = cx_eddsa_sign(&privateKey, CX_LAST, CX_SHA512, ctx.req.tx.hash, 32, NULL, 0, G_io_apdu_buffer, 64, NULL);
#else    
    tx = cx_eddsa_sign(&privateKey, NULL, CX_LAST, CX_SHA512, ctx.req.tx.hash, 32, G_io_apdu_buffer);
#endif    
    os_memset(&privateKey, 0, sizeof(privateKey));

    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;

#ifdef HAVE_U2F
    if (fidoActivated) {
        u2f_proxy_response((u2f_service_t *)&u2fService, tx);
    } else {
        // Send back the response, do not restart the event loop
        io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
    }
#else  // HAVE_U2F
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
#endif // HAVE_U2F
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

unsigned int io_seproxyhal_touch_tx_cancel(const bagl_element_t *e) {
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
#ifdef HAVE_U2F
    if (fidoActivated) {
        u2f_proxy_response((u2f_service_t *)&u2fService, 2);
    } else {
        // Send back the response, do not restart the event loop
        io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    }
#else  // HAVE_U2F
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
#endif // HAVE_U2F
    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widget
}

#if defined(TARGET_NANOS)
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

#endif // #if defined(TARGET_NANOS)

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
    case CHANNEL_KEYBOARD:
        break;
    case CHANNEL_SPI:
        if (tx_len) {
            io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

            if (channel & IO_RESET_AFTER_REPLIED) {
                reset();
            }
            return 0;
        } else {
            return io_seproxyhal_spi_recv(G_io_apdu_buffer, sizeof(G_io_apdu_buffer), 0);
        }
    default:
        THROW(INVALID_PARAMETER);
    }
    return 0;
}


uint32_t set_result_get_publicKey() {
    uint32_t tx = 0;

    uint8_t publicKey[32];
    // copy public key little endian to big endian
    uint8_t i;
    for (i = 0; i < 32; i++) {
        publicKey[i] = ctx.req.pk.publicKey.W[64 - i];
    }
    if ((ctx.req.pk.publicKey.W[32] & 1) != 0) {
        publicKey[31] |= 0x80;
    }

    os_memmove(G_io_apdu_buffer + tx, publicKey, 32);

    tx += 32;

    if (ctx.req.pk.returnSignature) {
        os_memmove(G_io_apdu_buffer + tx, ctx.req.pk.signature, 64);
        tx += 64;
    }

    return tx;
}

uint8_t read_bip32(uint8_t *dataBuffer, uint32_t *bip32) {
    uint8_t bip32Len = dataBuffer[0];
    dataBuffer += 1;
    if (bip32Len < 0x01 || bip32Len > MAX_BIP32_LEN) {
        THROW(0x6a80);
    }
    uint8_t i;
    for (i = 0; i < bip32Len; i++) {
        bip32[i] = (dataBuffer[0] << 24) | (dataBuffer[1] << 16) | (dataBuffer[2] << 8) | (dataBuffer[3]);
        dataBuffer += 4;
    }
    return bip32Len;
}

void handleGetPublicKey(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {

    if ((p1 != P1_SIGNATURE) && (p1 != P1_NO_SIGNATURE)) {
        THROW(0x6B00);
    }
    if ((p2 != P2_CONFIRM) && (p2 != P2_NO_CONFIRM)) {
        THROW(0x6B00);
    }
    ctx.req.pk.returnSignature = (p1 == P1_SIGNATURE);

    uint32_t bip32[MAX_BIP32_LEN];
    uint8_t bip32Len = read_bip32(dataBuffer, bip32);
    dataBuffer += 1 + bip32Len * 4;
    dataLength -= 1 + bip32Len * 4;

    uint16_t msgLength;
    uint8_t msg[32];
    if (ctx.req.pk.returnSignature) {
        msgLength = dataLength;
        if (msgLength > 32) {
            THROW(0x6a80);
        }
        os_memmove(msg, dataBuffer, msgLength);
    }

    uint8_t privateKeyData[32];
    cx_ecfp_private_key_t privateKey;
    os_perso_derive_node_bip32(CX_CURVE_Ed25519, bip32, bip32Len, privateKeyData, NULL);
    cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyData, 32, &privateKey);
    os_memset(privateKeyData, 0, sizeof(privateKeyData));
    cx_ecfp_generate_pair(CX_CURVE_Ed25519, &ctx.req.pk.publicKey, &privateKey, 1);
    if (ctx.req.pk.returnSignature) {
#if CX_APILEVEL >= 8
        cx_eddsa_sign(&privateKey, CX_LAST, CX_SHA512, msg, msgLength, NULL, 0, ctx.req.pk.signature, 64, NULL);
#else        
        cx_eddsa_sign(&privateKey, NULL, CX_LAST, CX_SHA512, msg, msgLength, ctx.req.pk.signature);
#endif        
    }
    os_memset(&privateKey, 0, sizeof(privateKey));

    if (p2 & P2_CONFIRM) {
        uint8_t publicKey[32];
        // copy public key little endian to big endian
        uint8_t i;
        for (i = 0; i < 32; i++) {
            publicKey[i] = ctx.req.pk.publicKey.W[64 - i];
        }
        if ((ctx.req.pk.publicKey.W[32] & 1) != 0) {
            publicKey[31] |= 0x80;
        }
        print_public_key(publicKey, ctx.req.pk.address);
#if defined(TARGET_BLUE)
        UX_DISPLAY(ui_address_blue, ui_address_blue_prepro);
#elif defined(TARGET_NANOS)
        ux_step = 0;
        ux_step_count = 2;
        UX_DISPLAY(ui_address_nanos, ui_address_prepro);
#endif // #if TARGET
        *flags |= IO_ASYNCH_REPLY;
    } else {
        *tx = set_result_get_publicKey();
        THROW(0x9000);
    }
}

void handleGetAppConfiguration(volatile unsigned int *tx) {
    G_io_apdu_buffer[0] = multiOpsSupport ? 0x01 : 0x00;
    G_io_apdu_buffer[1] = LEDGER_MAJOR_VERSION;
    G_io_apdu_buffer[2] = LEDGER_MINOR_VERSION;
    G_io_apdu_buffer[3] = LEDGER_PATCH_VERSION;
    *tx = 4;
    THROW(0x9000);
}

void handleSignTx(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {

    if ((p1 != P1_FIRST) && (p1 != P1_MORE)) {
        THROW(0x6B00);
    }
    if ((p2 != P2_LAST) && (p2 != P2_MORE)) {
        THROW(0x6B00);
    }

    if (p1 == P1_FIRST) {
        // read the bip32 path
        ctx.req.tx.bip32Len = read_bip32(dataBuffer, ctx.req.tx.bip32);
        dataBuffer += 1 + ctx.req.tx.bip32Len * 4;
        dataLength -= 1 + ctx.req.tx.bip32Len * 4;

        // read raw tx data
        ctx.req.tx.rawLength = dataLength;
        os_memmove(ctx.req.tx.raw, dataBuffer, dataLength);
    } else {
        // read more raw tx data
        uint32_t offset = ctx.req.tx.rawLength;
        ctx.req.tx.rawLength += dataLength;
        if (ctx.req.tx.rawLength > MAX_RAW_TX) {
            THROW(0x6700);
        }
        os_memmove(ctx.req.tx.raw+offset, dataBuffer, dataLength);
    }

    if (p2 == P2_MORE) {
        THROW(0x9000);
    }

    os_memset(&ctx.req.tx.content, 0, sizeof(ctx.req.tx.content));

    // hash transaction
#if CX_APILEVEL >= 8
    cx_hash_sha256(ctx.req.tx.raw, ctx.req.tx.rawLength, ctx.req.tx.hash, 32);
#else
    cx_hash_sha256(ctx.req.tx.raw, ctx.req.tx.rawLength, ctx.req.tx.hash);
#endif

    // show details
#if defined(TARGET_BLUE)
    ui_approve_tx_blue_init();
#elif defined(TARGET_NANOS)
    ux_step = 0;
    ux_step_count = 1;
    ctx.req.tx.offset = 0;
    UX_DISPLAY(ui_approve_tx_nanos, ui_tx_approval_prepro);
#endif

    *flags |= IO_ASYNCH_REPLY;
}

void handleSignTxHash(uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {

    if (!multiOpsSupport) {
        THROW(0x6c66);
    }

    os_memset(&ctx.req.tx, 0, sizeof(ctx.req.tx));

    // read the bip32 path
    ctx.req.tx.bip32Len = read_bip32(dataBuffer, ctx.req.tx.bip32);
    dataBuffer += 1 + ctx.req.tx.bip32Len * 4;
    dataLength -= 1 + ctx.req.tx.bip32Len * 4;

    // read the tx hash
    os_memmove(ctx.req.tx.hash, dataBuffer, dataLength);

    // prepare for display
    os_memset(&ctx.req.tx.content, 0, sizeof(ctx.req.tx.content));
    ctx.req.tx.content.opType = OPERATION_TYPE_UNKNOWN;
    ctx.req.tx.rawLength = 0;

#if defined(TARGET_BLUE)
    print_hash_summary(ctx.req.tx.hash, (char *)ctx.req.tx.content.txDetails[0]);
    ui_approve_tx_hash_blue_init();
#elif defined(TARGET_NANOS)
    strcpy((char *)ctx.req.tx.content.opDetails[0], "No details");
    print_hash_summary(ctx.req.tx.hash, (char *)ctx.req.tx.content.opDetails[1]);
    ux_step = 0;
    ux_step_count = 3;
    UX_DISPLAY(ui_approve_tx_nanos, ui_tx_approval_prepro);
#endif // #if TARGET

    *flags |= IO_ASYNCH_REPLY;
}

void handleKeepAlive(volatile unsigned int *flags) {
    *flags |= IO_ASYNCH_REPLY;
}

void handleApdu(volatile unsigned int *flags, volatile unsigned int *tx) {
    unsigned short sw = 0;

    BEGIN_TRY {
        TRY {
            if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
                THROW(0x6e00);
            }

            uint8_t ins = G_io_apdu_buffer[OFFSET_INS];
            uint8_t p1 = G_io_apdu_buffer[OFFSET_P1];
            uint8_t p2 = G_io_apdu_buffer[OFFSET_P2];
            uint8_t dataLength = G_io_apdu_buffer[OFFSET_LC];
            uint8_t *dataBuffer = G_io_apdu_buffer + OFFSET_CDATA;

#ifdef HAVE_U2F
            // reset keep-alive for u2f just short of 30sec
            ctx.u2fTimer = 28000;
#endif // HAVE_U2F

            switch (ins) {
            case INS_GET_PUBLIC_KEY:
                handleGetPublicKey(p1, p2, dataBuffer, dataLength, flags, tx);
                break;

            case INS_SIGN_TX:
                handleSignTx(p1, p2, dataBuffer, dataLength, flags, tx);
                break;

            case INS_SIGN_TX_HASH:
                handleSignTxHash(dataBuffer, dataLength, flags, tx);
                break;

            case INS_GET_APP_CONFIGURATION:
                handleGetAppConfiguration(tx);
                break;

            case INS_KEEP_ALIVE:
                handleKeepAlive(flags);
                break;
            default:
                THROW(0x6D00);
                break;
            }
        }
        CATCH_OTHER(e) {
            switch (e & 0xF000) {
            case 0x6000:
                // Wipe the transaction context and report the exception
                sw = e;
                os_memset(&ctx.req.tx.content, 0, sizeof(ctx.req.tx.content));
                break;
            case 0x9000:
                // All is well
                sw = e;
                break;
            default:
                // Internal error
                sw = 0x6800 | (e & 0x7FF);
                break;
            }
            // Unexpected exception => report
            G_io_apdu_buffer[*tx] = sw >> 8;
            G_io_apdu_buffer[*tx + 1] = sw;
            *tx += 2;
        }
        FINALLY {
        }
    }
    END_TRY;
}

void stellar_main(void) {
    // multi-ops support is not persistent
    multiOpsSupport = 0;
    ctx.u2fTimer = 0;

    volatile unsigned int rx = 0;
    volatile unsigned int tx = 0;
    volatile unsigned int flags = 0;

    // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
    // goal is to retrieve APDU.
    // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
    // sure the io_event is called with a
    // switch event, before the apdu is replied to the bootloader. This avoid
    // APDU injection faults.
    for (;;) {
        volatile unsigned short sw = 0;

        BEGIN_TRY {
            TRY {
                rx = tx;
                tx = 0;

                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    THROW(0x6982);
                }

                handleApdu(&flags, &tx);
            }
            CATCH_OTHER(e) {
                switch (e & 0xF000) {
                case 0x6000:
                    // Wipe the transaction context and report the exception
                    sw = e;
                    os_memset(&ctx.req.tx.content, 0, sizeof(ctx.req.tx.content));
                    break;
                case 0x9000:
                    // All is well
                    sw = e;
                    break;
                default:
                    // Internal error
                    sw = 0x6800 | (e & 0x7FF);
                    break;
                }
                // Unexpected exception => report
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY {
            }
        }
        END_TRY;
    }

    // return_to_dashboard:
    return;
}

// override point, but nothing more to do
void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *)element);
}

#ifdef HAVE_U2F
void sendKeepAlive() {
    ctx.u2fTimer = 0;
    G_io_apdu_buffer[0] = 0x6e;
    G_io_apdu_buffer[1] = 0x02;
    u2f_proxy_response((u2f_service_t *)&u2fService, 2);
}
#endif // HAVE_U2F

unsigned char io_event(unsigned char channel) {
    // nothing done with the event, throw an error on the transport layer if
    // needed

    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
    case SEPROXYHAL_TAG_FINGER_EVENT:
        UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
        break;

    case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
        UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
        break;

    case SEPROXYHAL_TAG_STATUS_EVENT:
        if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID &&
            !(U4BE(G_io_seproxyhal_spi_buffer, 3) &
              SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
            THROW(EXCEPTION_IO_RESET);
        }
    // no break is intentional
    default:
        UX_DEFAULT_EVENT();
        break;

    case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
        UX_DISPLAYED_EVENT({});
        break;

    case SEPROXYHAL_TAG_TICKER_EVENT:

        #ifdef HAVE_U2F
        if (fidoActivated && ctx.u2fTimer > 0) {
            ctx.u2fTimer -= 100;
            if (ctx.u2fTimer <= 0) {
                sendKeepAlive();
            }
        }
        #endif // HAVE_U2F

        UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
            if (UX_ALLOWED) {
                if (ux_step_count) {
                    // prepare next screen
                    ux_step = (ux_step + 1) % ux_step_count;
                    // redisplay screen
                    UX_REDISPLAY();
                }
            }
        });
        break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}

void app_exit(void) {
    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(-1);
        }
        FINALLY_L(exit) {
        }
    }
    END_TRY_L(exit);
}


__attribute__((section(".boot"))) int main(void) {
    // exit critical section
    __asm volatile("cpsie i");

    // ensure exception will work as planned
    os_boot();

    for (;;) {
        os_memset(&ctx.req.tx.content, 0, sizeof(ctx.req.tx.content));

        UX_INIT();
        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

                if (N_storage.initialized != 0x01) {
                    internalStorage_t storage;
                    storage.fidoTransport = 0x01;
                    storage.initialized = 0x01;
                    nvm_write(&N_storage, (void *)&storage, sizeof(internalStorage_t));
                }

#ifdef HAVE_U2F
                os_memset((unsigned char *)&u2fService, 0, sizeof(u2fService));
                u2fService.inputBuffer = G_io_apdu_buffer;
                u2fService.outputBuffer = G_io_apdu_buffer;
                u2fService.messageBuffer = (uint8_t *)u2fMessageBuffer;
                u2fService.messageBufferSize = U2F_MAX_MESSAGE_SIZE;
                u2f_initialize_service((u2f_service_t *)&u2fService);

                USB_power_U2F(1, N_storage.fidoTransport);
#else
                USB_power_U2F(1, 0);
#endif

                ui_idle();

#if defined(TARGET_BLUE)
                UX_SET_STATUS_BAR_COLOR(0xFFFFFF, COLOR_APP);
#endif

                stellar_main();
            }
            CATCH(EXCEPTION_IO_RESET) {
                continue;
            }
            CATCH_ALL {
                break;
            }
            FINALLY {
            }
        }
        END_TRY;
    }
    app_exit();

    return 0;
}
