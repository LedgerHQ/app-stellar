#pragma once

#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t

#include "bip32.h"

#include "constants.h"
#include "stellar/types.h"

/**
 * Enumeration with expected INS of APDU commands.
 */
typedef enum {
    INS_GET_PUBLIC_KEY = 0x02,            // public key of corresponding BIP32 path
    INS_SIGN_TX = 0x04,                   // sign transaction with BIP32 path
    INS_GET_APP_CONFIGURATION = 0x06,     // app configuration of the application
    INS_SIGN_HASH = 0x08,                 // sign transaction in hash mode
    INS_SIGN_SOROBAN_AUTHORATION = 0x0a,  // sign soroban authoration
} command_e;

/**
 * Enumeration with parsing state.
 */
typedef enum {
    STATE_NONE,     // No state
    STATE_PARSED,   // Transaction data parsed
    STATE_APPROVED  // Transaction data approved
} state_e;

/**
 * Enumeration with user request type.
 */
typedef enum {
    CONFIRM_ADDRESS,             // confirm address derived from public key
    CONFIRM_TRANSACTION,         // confirm transaction information
    CONFIRM_HASH,                // confirm hash information
    CONFIRM_SOROBAN_AUTHORATION  // confirm soroban authoration information
} request_type_e;

enum e_state {
    STATIC_SCREEN,
    DYNAMIC_SCREEN,
};

/**
 * Structure for global context.
 */
typedef struct {
    envelope_t envelope;
    uint8_t raw[RAW_DATA_MAX_SIZE];
    uint32_t raw_size;
    uint8_t raw_public_key[RAW_ED25519_PUBLIC_KEY_SIZE];  // BIP32 path public key
    uint8_t hash[HASH_SIZE];                              // tx hash
    uint32_t bip32_path[MAX_BIP32_PATH];                  // BIP32 path
    uint8_t bip32_path_len;                               // length of BIP32 path
    state_e state;                                        // state of the context
    request_type_e req_type;                              // user request
} global_ctx_t;

/**
 * Structure for swap.
 */
typedef struct {
    uint64_t amount;
    uint64_t fees;
    char destination[ENCODED_ED25519_PUBLIC_KEY_LENGTH];  // ed25519 address only
    char memo[20];
} swap_values_t;
