/*****************************************************************************
 *   Ledger App Stellar.
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

#include <string.h>

#include "os.h"
#include "swap.h"
#include "bip32.h"

#include "crypto.h"
#include "stellar/printer.h"

/* Check check_address_parameters_t.address_to_check against specified parameters.
 *
 * Must set params.result to 0 on error, 1 otherwise */
void swap_handle_check_address(check_address_parameters_t* params) {
    params->result = 0;
    PRINTF("Params on the address %d\n", (unsigned int) params);
    PRINTF("Address to check %s\n", params->address_to_check);
    PRINTF("Inside swap_handle_check_address\n");

    if (params->address_to_check == 0) {
        PRINTF("Address to check == 0\n");
        return;
    }

    uint32_t bip32_path[MAX_BIP32_PATH];
    uint8_t bip32_path_length = *params->address_parameters;
    if (!bip32_path_read(params->address_parameters + 1,
                         params->address_parameters_length - 1,
                         bip32_path,
                         bip32_path_length)) {
        PRINTF("Invalid path\n");
        return;
    }

    uint8_t stellar_publicKey[RAW_ED25519_PUBLIC_KEY_SIZE];

    if (crypto_derive_public_key(stellar_publicKey, bip32_path, bip32_path_length) != CX_OK) {
        PRINTF("crypto_init_public_key failed\n");
        return;
    }

    char address[57];
    if (!print_account_id(stellar_publicKey, address, sizeof(address), 0, 0)) {
        PRINTF("public key encode failed\n");
        return;
    };

    if (strcmp(address, params->address_to_check) != 0) {
        PRINTF("Addresses do not match\n");
        return;
    }

    PRINTF("Addresses match\n");
    params->result = 1;
    return;
}
