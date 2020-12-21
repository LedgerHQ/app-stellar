#include <string.h>

#include "swap_lib_calls.h"
#include "os.h"
#include "stellar_api.h"

static int os_strcmp(const char* s1, const char* s2) {
    size_t size = strlen(s1) + 1;
    return memcmp(s1, s2, size);
}

int handle_check_address(check_address_parameters_t* params) {
    PRINTF("Params on the address %d\n", (unsigned int) params);
    PRINTF("Address to check %s\n", params->address_to_check);
    PRINTF("Inside handle_check_address\n");

    if (params->address_to_check == 0) {
        PRINTF("Address to check == 0\n");
        return 0;
    }

    uint32_t bip32_path[MAX_BIP32_LEN];
    int bip32_path_length =
        read_bip32(params->address_parameters, params->address_parameters_length, bip32_path);

    cx_ecfp_private_key_t privateKey;
    cx_ecfp_public_key_t publicKey;
    uint8_t stellar_publicKey[32];
    derive_private_key(&privateKey, bip32_path, bip32_path_length);
    init_public_key(&privateKey, &publicKey, stellar_publicKey);

    char address[57];
    encode_public_key(stellar_publicKey, address);

    if (os_strcmp(address, params->address_to_check) != 0) {
        PRINTF("Addresses don't match\n");
        memcpy(params->address_to_check, address, 57);
        params->address_to_check[56] = '\0';
        // memcpy(params->address_to_check, bip32_path, 12);
        // params->address_to_check[12] = bip32_path_length;
        return 0;
    }

    PRINTF("Addresses match\n");
    return 1;
}
