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
#include <limits.h>
#include <stdio.h>

#include "os_io_seproxyhal.h"

#include "stellar_types.h"
#include "stellar_api.h"
#include "stellar_vars.h"
#include "stellar_ux.h"

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

void handle_apdu(volatile unsigned int *flags, volatile unsigned int *tx) {
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

            PRINTF("New APDU:\n%.*H\n", dataLength+4, G_io_apdu_buffer);

            // reset keep-alive for u2f just short of 30sec
            ctx.u2fTimer = U2F_REQUEST_TIMEOUT;

            switch (ins) {
            case INS_GET_PUBLIC_KEY:
                handle_get_public_key(p1, p2, dataBuffer, dataLength, flags, tx);
                break;

            case INS_SIGN_TX:
                handle_sign_tx(p1, p2, dataBuffer, dataLength, flags);
                break;

            case INS_SIGN_TX_HASH:
                handle_sign_tx_hash(dataBuffer, dataLength, flags);
                break;

            case INS_GET_APP_CONFIGURATION:
                handle_get_app_configuration(tx);
                break;

            case INS_KEEP_ALIVE:
                handle_keep_alive(flags);
                break;
            default:
                THROW(0x6D00);
                break;
            }
        }
        CATCH(EXCEPTION_IO_RESET) {
            THROW(EXCEPTION_IO_RESET);
    }
        CATCH_OTHER(e) {
            switch (e & 0xF000) {
            case 0x6000:
                // Wipe the transaction context and report the exception
                sw = e;
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

void stellar_nv_state_init() {
    if (N_stellar_pstate.initialized != 0x01) {
        uint8_t initialized = 0x01;
        nvm_write((void *)&N_stellar_pstate, &initialized, 1);
    }
}

void stellar_main(void) {
    // hash sig support is not persistent

    os_memset(&ctx, 0, sizeof(ctx));

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

                handle_apdu(&flags, &tx);
            }
            CATCH(EXCEPTION_IO_RESET) {
                THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e) {
                switch (e & 0xF000) {
                case 0x6000:
                    // Wipe the transaction context and report the exception
                    sw = e;
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

void u2f_send_keep_alive() {
    ctx.u2fTimer = 0;
    G_io_apdu_buffer[0] = 0x6e;
    G_io_apdu_buffer[1] = 0x02;
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
}

unsigned char io_event(unsigned char channel) {

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

#ifndef TARGET_BLUE // S & X only
        if (G_io_apdu_media == IO_APDU_MEDIA_U2F && ctx.u2fTimer > 0) {
            ctx.u2fTimer -= 100;
            if (ctx.u2fTimer <= 0) {
                u2f_send_keep_alive();
            }
        }
#endif
        UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {
#ifdef TARGET_NANOS // S only
            if (UX_ALLOWED) {
                if (ctx.reqType == CONFIRM_TRANSACTION) {
                    ui_approve_tx_next_screen(&ctx.req.tx);
                }
                UX_REDISPLAY();
            }
#endif
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
        UX_INIT();
        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

                stellar_nv_state_init();

                // deactivate usb before activating
                USB_power(0);
                USB_power(1);

                ui_idle();

#ifdef HAVE_BLE
                BLE_power(0, NULL);
                BLE_power(1, "Nano X");
#endif // HAVE_BLE

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
