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

#include "os.h"
#include "cx.h"

#include "./globals.h"
#include "./sw.h"
#include "./io.h"
#include "./types.h"
#include "./settings.h"
#include "./apdu/apdu_parser.h"
#include "./apdu/dispatcher.h"
#include "./swap/handle_swap_commands.h"
#include "./swap/swap_lib_calls.h"
#include "./ui/ui.h"

// The settings, stored in NVRAM.
const internal_storage_t N_storage_real;

/**
 * Handle APDU command received and send back APDU response using handlers.
 */
void app_main() {
    // Length of APDU command received in G_io_apdu_buffer
    int input_len = 0;
    // Structured APDU command
    command_t cmd;

    // Reset length of APDU response
    G_output_len = 0;
    G_io_state = READY;

    // Reset context
    explicit_bzero(&G_context, sizeof(G_context));

    for (;;) {
        BEGIN_TRY {
            TRY {
                // Reset structured APDU command
                memset(&cmd, 0, sizeof(cmd));

                // Receive command bytes in G_io_apdu_buffer
                if ((input_len = io_recv_command()) < 0) {
                    return;
                }

                // Parse APDU command from G_io_apdu_buffer
                if (!apdu_parser(&cmd, G_io_apdu_buffer, input_len)) {
                    PRINTF("=> /!\\ BAD LENGTH: %.*H\n", input_len, G_io_apdu_buffer);
                    io_send_sw(SW_WRONG_DATA_LENGTH);
                    continue;
                }

                PRINTF("=> CLA=%02X | INS=%02X | P1=%02X | P2=%02X | Lc=%02X | CData=%.*H\n",
                       cmd.cla,
                       cmd.ins,
                       cmd.p1,
                       cmd.p2,
                       cmd.lc,
                       cmd.lc,
                       cmd.data);

                // Dispatch structured APDU command to handler
                if (apdu_dispatcher(&cmd) < 0) {
                    return;
                }
            }
            CATCH(EXCEPTION_IO_RESET) {
                THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e) {
                io_send_sw(e);
            }
            FINALLY {
            }
            END_TRY;
        }
    }
}

/**
 * Exit the application and go back to the dashboard.
 */
void app_exit() {
    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(-1);
        }
        FINALLY_L(exit) {
        }
    }
    END_TRY_L(exit);
}

void standalone_app_main() {
    for (;;) {
        // Reset UI
        memset(&G_ux, 0, sizeof(G_ux));

        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

                if (!HAS_SETTING(S_INITIALIZED)) {
                    internal_storage_t storage = 0x00;
                    storage |= 0x80;
                    nvm_write((void *) &N_settings, (void *) &storage, sizeof(internal_storage_t));
                }

#ifdef HAVE_BLE
                // grab the current plane mode setting
                G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
#endif  // TARGET_NANOX

                USB_power(0);
                USB_power(1);
                ui_menu_main();

#ifdef HAVE_BLE
                BLE_power(0, NULL);
                BLE_power(1, NULL);
#endif  // HAVE_BLE
                app_main();
            }
            CATCH(EXCEPTION_IO_RESET) {
                // reset IO and UX before continuing
                CLOSE_TRY;
                continue;
            }
            CATCH_ALL {
                CLOSE_TRY;
                break;
            }
            FINALLY {
            }
        }
        END_TRY;
    }

    app_exit();
}

static void library_main_helper(struct libargs_s *args) {
    PRINTF("Inside library \n");
    switch (args->command) {
        case CHECK_ADDRESS:
            // ensure result is zero if an exception is thrown
            args->check_address->result = 0;
            args->check_address->result = handle_check_address(args->check_address);
            break;
        case SIGN_TRANSACTION:
            if (copy_transaction_parameters(args->create_transaction)) {
                // never returns
                G_called_from_swap = true;
                G.swap.response_ready = false;
                handle_swap_sign_transaction();
            }
            break;
        case GET_PRINTABLE_AMOUNT:
            handle_get_printable_amount(args->get_printable_amount);
            break;
        default:
            break;
    }
}

void library_main(struct libargs_s *args) {
    bool end = false;
    /* This loop ensures that library_main_helper and os_lib_end are called
     * within a try context, even if an exception is thrown */
    while (1) {
        BEGIN_TRY {
            TRY {
                if (!end) {
                    library_main_helper(args);
                }
                os_lib_end();
            }
            FINALLY {
                end = true;
            }
        }
        END_TRY;
    }
}

/**
 * Main loop to setup USB, Bluetooth, UI and launch ui_menu_main().
 */
__attribute__((section(".boot"))) int main(int arg0) {
    // exit critical section
    __asm volatile("cpsie i");
    // ensure exception will work as planned
    os_boot();
    if (arg0 == 0) {
        // called from dashboard as standalone Stellar App
        G_called_from_swap = false;
        standalone_app_main();
    } else {
        // Called as library from another app
        libargs_t *args = (libargs_t *) arg0;
        if (args->id == 0x100) {
            library_main(args);
        } else {
            app_exit();
        }
    }

    return 0;
}
