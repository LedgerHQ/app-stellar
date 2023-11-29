#pragma once

#include <stdint.h>

typedef uint8_t internal_storage_t;

/**
 * The settings, stored in NVRAM. Initializer is ignored by ledger.
 */
extern const internal_storage_t N_storage_real;

#define N_settings (*(volatile internal_storage_t *) PIC(&N_storage_real))

// flip a bit k = 0 to 7 for u8
#define _FLIP_BIT(n, k) (((n) ^ (1 << (k))))

// toggle a setting item
#define SETTING_TOGGLE(_set)                                                                   \
    do {                                                                                       \
        internal_storage_t _temp_settings = _FLIP_BIT(N_settings, _set);                       \
        nvm_write((void *) &N_settings, (void *) &_temp_settings, sizeof(internal_storage_t)); \
    } while (0)

// check a setting item
#define HAS_SETTING(k) ((N_settings & (1 << (k))) >> (k))

#define S_HASH_SIGNING_ENABLED     0
#define S_CUSTOM_CONTRACTS_ENABLED 1
#define S_SEQUENCE_NUMBER_ENABLED  2

#define S_INITIALIZED 7
