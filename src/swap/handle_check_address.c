#include <string.h>

#include "os.h"

#include "./swap_lib_calls.h"
#include "../utils.h"
#include "../crypto.h"
#include "../common/bip32.h"

int handle_check_address(const check_address_parameters_t* params) {
    PRINTF("Params on the address %d\n", (unsigned int) params);
    PRINTF("Address to check %s\n", params->address_to_check);
    PRINTF("Inside handle_check_address\n");

    if (params->address_to_check == 0) {
        PRINTF("Address to check == 0\n");
        return 0;
    }

    uint32_t bip32_path[MAX_BIP32_PATH];
    uint8_t bip32_path_length = *params->address_parameters;
    if (!bip32_path_read(params->address_parameters + 1,
                         params->address_parameters_length - 1,
                         bip32_path,
                         bip32_path_length)) {
        PRINTF("Invalid path\n");
        return 0;
    }

    cx_ecfp_private_key_t privateKey;
    cx_ecfp_public_key_t publicKey;
    uint8_t stellar_publicKey[32];
    if (crypto_derive_private_key(&privateKey, bip32_path, bip32_path_length)) {
        explicit_bzero(&privateKey, sizeof(privateKey));
        PRINTF("derive_private_key failed\n");
        return 0;
    }

    if (crypto_init_public_key(&privateKey, &publicKey, stellar_publicKey)) {
        return 0;
    }

    explicit_bzero(&privateKey, sizeof(privateKey));

    char address[57];
    if (!encode_ed25519_public_key(stellar_publicKey, address, sizeof(address))) {
        PRINTF("public key encode failed\n");
        return 0;
    };

    if (strcmp(address, params->address_to_check) != 0) {
        PRINTF("Addresses do not match\n");
        return 0;
    }

    PRINTF("Addresses match\n");
    return 1;
}
