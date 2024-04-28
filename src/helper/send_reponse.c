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

#include "send_response.h"
#include "constants.h"
#include "globals.h"
#include "sw.h"

int helper_send_response_pubkey() {
    return io_send_response_pointer(G_context.raw_public_key, RAW_ED25519_PUBLIC_KEY_SIZE, SW_OK);
}

int helper_send_response_sig(const uint8_t *signature) {
    return io_send_response_pointer(signature, SIGNATURE_SIZE, SW_OK);
}
