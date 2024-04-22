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

    cx_ecfp_private_key_t privateKey;
    cx_ecfp_public_key_t publicKey;
    uint8_t stellar_publicKey[RAW_ED25519_PUBLIC_KEY_SIZE];

    if (crypto_derive_private_key(&privateKey, bip32_path, bip32_path_length) != CX_OK) {
        explicit_bzero(&privateKey, sizeof(privateKey));
        PRINTF("derive_private_key failed\n");
        return;
    }

    if (crypto_init_public_key(&privateKey, &publicKey, stellar_publicKey) != CX_OK) {
        explicit_bzero(&privateKey, sizeof(privateKey));
        PRINTF("crypto_init_public_key failed\n");
        return;
    }

    explicit_bzero(&privateKey, sizeof(privateKey));

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
