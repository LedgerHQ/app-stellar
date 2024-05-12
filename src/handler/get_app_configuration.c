/*****************************************************************************
 *   Ledger App Stellar.
 *   (c) 2024 Ledger SAS.
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

#include <stdint.h>  // uint*_t
#include <limits.h>  // UINT8_MAX
#include <assert.h>  // _Static_assert

#include "io.h"
#include "buffer.h"

#include "get_app_configuration.h"
#include "globals.h"
#include "constants.h"
#include "sw.h"
#include "types.h"
#include "settings.h"

int handler_get_app_configuration() {
    PRINTF("handler_get_app_configuration invoked\n");

    _Static_assert(APP_VERSION_SIZE == 3, "Length of (MAJOR || MINOR || PATCH) must be 3!");
    _Static_assert(MAJOR_VERSION >= 0 && MAJOR_VERSION <= UINT8_MAX,
                   "MAJOR version must be between 0 and 255!");
    _Static_assert(MINOR_VERSION >= 0 && MINOR_VERSION <= UINT8_MAX,
                   "MINOR version must be between 0 and 255!");
    _Static_assert(PATCH_VERSION >= 0 && PATCH_VERSION <= UINT8_MAX,
                   "PATCH version must be between 0 and 255!");
    _Static_assert(RAW_TX_MAX_SIZE >= 0 && RAW_TX_MAX_SIZE <= UINT16_MAX,
                   "RAW_TX_MAX_SIZE must be between 0 and 65535!");

    uint8_t config[] = {HAS_SETTING(S_HASH_SIGNING_ENABLED),
                        MAJOR_VERSION,
                        MINOR_VERSION,
                        PATCH_VERSION,
                        RAW_TX_MAX_SIZE >> 8,
                        RAW_TX_MAX_SIZE & 0xFF};

    return io_send_response_pointer(config, sizeof(config), SW_OK);
}
