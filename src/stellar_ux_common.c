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
#include "stellar_vars.h"

unsigned int io_seproxyhal_respond(unsigned short sw, uint32_t tx) {
    G_io_apdu_buffer[tx++] = sw >> 8;
    G_io_apdu_buffer[tx++] = sw;

    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);

    // Display back the original UX
    ui_idle();
    return 0;  // do not redraw the widgets
}

unsigned int io_seproxyhal_touch_address_ok(const bagl_element_t *e) {
    (void) e;
    return io_seproxyhal_respond(0x9000, ctx.req.pk.tx);
}

unsigned int io_seproxyhal_touch_address_cancel(const bagl_element_t *e) {
    (void) e;
    return io_seproxyhal_respond(0x6985, 0);
}

unsigned int io_seproxyhal_touch_tx_ok(const bagl_element_t *e) {
    (void) e;
    ctx.state = STATE_NONE;
    return io_seproxyhal_respond(0x9000, ctx.req.tx.tx);
}

unsigned int io_seproxyhal_touch_tx_cancel(const bagl_element_t *e) {
    (void) e;
    explicit_bzero(G_io_apdu_buffer, sizeof(G_io_apdu_buffer));
    return io_seproxyhal_respond(0x6985, 0);
}
