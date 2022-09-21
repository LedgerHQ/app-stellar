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

#include "./send_response.h"
#include "./globals.h"
#include "./sw.h"
#include "./common/buffer.h"

int send_response_pubkey() {
    return io_send_response(&(const buffer_t){.ptr = G_context.raw_public_key,
                                              .size = RAW_ED25519_PUBLIC_KEY_SIZE,
                                              .offset = 0},
                            SW_OK);
}

int send_response_sig(const uint8_t *signature, uint8_t signature_len) {
    return io_send_response(&(const buffer_t){.ptr = signature, .size = signature_len, .offset = 0},
                            SW_OK);
}
