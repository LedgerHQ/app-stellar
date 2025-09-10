/*****************************************************************************
 *   Ledger Stellar App.
 *   (c) 2024 Ledger SAS.
 *   (c) 2024 overcat.
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
#ifdef HAVE_BAGL
#include <stdbool.h>  // bool
#include <string.h>   // memset

#include "os.h"
#include "ux.h"
#include "glyphs.h"
#include "io.h"
#include "bip32.h"
#include "format.h"

#include "display.h"
#include "globals.h"
#include "sw.h"
#include "action/validate.h"
#include "stellar/printer.h"

static void ui_action_go_back_home() {
    ui_idle();
}

UX_STEP_CB(ux_error_blind_signing,
           pnn,
           ui_action_go_back_home(),
           {
               &C_icon_crossmark,
               "Blind Signing must be",
               "enabled in Settings",
           });

UX_FLOW(ux_blind_signing_error_flow, &ux_error_blind_signing);

void ui_error_blind_signing() {
    ux_flow_init(0, ux_blind_signing_error_flow, NULL);
}
#endif  // HAVE_BAGL
