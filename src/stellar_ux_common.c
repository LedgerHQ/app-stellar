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
#include "stellar_ux.h"

#include "os_io_seproxyhal.h"
#include "stellar_api.h"

unsigned int io_seproxyhal_respond(unsigned short sw, uint32_t tx) {
    G_io_apdu_buffer[tx++] = sw >> 8;
    G_io_apdu_buffer[tx++] = sw;

    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);

    /* temporary workaround for the 'freeze' bug: EXCEPTION_IO_RESET is catched
     * by main(), which resets seproxhal and the UI */
    if (1) {
        THROW(EXCEPTION_IO_RESET);
    }

    // Display back the original UX
    ui_idle();
    return 0; // do not redraw the widgets
}

unsigned int io_seproxyhal_touch_address_ok(const bagl_element_t *e) {
    uint32_t tx = set_result_get_public_key();
    return io_seproxyhal_respond(0x9000, tx);
}

unsigned int io_seproxyhal_touch_address_cancel(const bagl_element_t *e) {
    return io_seproxyhal_respond(0x6985, 0);
}

unsigned int io_seproxyhal_touch_tx_ok(const bagl_element_t *e) {
    uint32_t tx = set_result_sign_tx();
    return io_seproxyhal_respond(0x9000, tx);
}

unsigned int io_seproxyhal_touch_tx_cancel(const bagl_element_t *e) {
    return io_seproxyhal_respond(0x6985, 0);
}

