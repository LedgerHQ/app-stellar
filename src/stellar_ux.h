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

#ifndef STELLAR_UX_COMMON_H
#define STELLAR_UX_COMMON_H

#include "os_io_seproxyhal.h"
#include "stellar_types.h"

// ------------------------------------------------------------------------- //
//                     Implemented by stellar_ux_common.c                    //
// ------------------------------------------------------------------------- //

unsigned int io_seproxyhal_respond(unsigned short sw, uint32_t tx);
unsigned int io_seproxyhal_touch_tx_ok(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_tx_cancel(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_address_ok(const bagl_element_t *e);
unsigned int io_seproxyhal_touch_address_cancel(const bagl_element_t *e);

// ------------------------------------------------------------------------- //
//        Implemented by both stellar_ux_nanos.c and stellar_ux_blue.c       //
// ------------------------------------------------------------------------- //

void ui_show_address_init(void);
void ui_approve_tx_init(void);
void display_next_screen(void);
void ui_approve_tx_next_screen(tx_context_t *txCtx);
void ui_approve_tx_hash_init(void);
void ui_idle(void);

#endif
