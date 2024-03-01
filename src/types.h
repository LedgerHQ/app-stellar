#pragma once

#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t

#include "./common/bip32.h"
#include "./transaction/transaction_types.h"

/**
 * Instruction class of the Stellar application.
 */
#define CLA 0xE0

/**
 * Maximum length of MAJOR_VERSION || MINOR_VERSION || PATCH_VERSION.
 */
#define APP_VERSION_SIZE 3

/**
 * Length of hash_signing_enabled
 */
#define APP_CONFIGURATION_SIZE 1

/*
 * Captions don't scroll so there is no use in having more capacity than can fit on screen at once.
 */
#define DETAIL_CAPTION_MAX_LENGTH 20

/*
 * DETAIL_VALUE_MAX_LENGTH value of 89 is due to the maximum length of managed data value which can
 * be 64 bytes long. Managed data values are displayed as base64 encoded strings, which are
 * 4*((len+2)/3) characters long. (An additional slot is required for the end-of-string character of
 * course)
 */
#define DETAIL_VALUE_MAX_LENGTH 89

/**
 * Maximum transaction size (bytes).
 */
#ifdef TARGET_NANOS
#define RAW_TX_MAX_SIZE 1120
#else
#define RAW_TX_MAX_SIZE 5120
#endif

/**
 * signature length (bytes).
 */
#define SIGNATURE_SIZE 64

/**
 * Callback to reuse action with approve/reject in step FLOW.
 */
typedef void (*ui_action_validate_cb)(bool);

/**
 * Enumeration for the status of IO.
 */
typedef enum {
    READY,     // ready for new event
    RECEIVED,  // data received
    WAITING    // waiting
} io_state_e;

/**
 * Enumeration with expected INS of APDU commands.
 */
typedef enum {
    INS_GET_PUBLIC_KEY = 0x02,            // public key of corresponding BIP32 path
    INS_SIGN_TX = 0x04,                   // sign transaction with BIP32 path
    INS_GET_APP_CONFIGURATION = 0x06,     // app configuration of the application
    INS_SIGN_TX_HASH = 0x08,              // sign transaction in hash mode
    INS_SIGN_SOROBAN_AUTHORATION = 0x0a,  // sign soroban authoration
} command_e;

/**
 * Structure with fields of APDU command.
 */
typedef struct {
    uint8_t cla;    // Instruction class
    command_e ins;  // Instruction code
    uint8_t p1;     // Instruction parameter 1
    uint8_t p2;     // Instruction parameter 2
    uint8_t lc;     // Lenght of command data
    uint8_t *data;  // Command data
} command_t;

/**
 * Enumeration with user request type.
 */
typedef enum {
    CONFIRM_ADDRESS,             // confirm address derived from public key
    CONFIRM_TRANSACTION,         // confirm transaction information
    CONFIRM_TRANSACTION_HASH,    // confirm transaction hash information
    CONFIRM_SOROBAN_AUTHORATION  // confirm soroban authoration information
} request_type_e;

/**
 * Enumeration with parsing state.
 */
typedef enum {
    STATE_NONE,     // No state
    STATE_PARSED,   // Transaction data parsed
    STATE_APPROVED  // Transaction data approved
} state_e;

/**
 * Structure for transaction context.
 *
 */
typedef struct {
    uint8_t raw[RAW_TX_MAX_SIZE];
    uint32_t raw_size;
    uint16_t offset;
    uint8_t network;
    envelope_type_t envelope_type;
    fee_bump_transaction_details_t fee_bump_tx_details;
    transaction_details_t tx_details;
} tx_ctx_t;

typedef struct {
    uint8_t raw[RAW_TX_MAX_SIZE];
    uint32_t raw_size;
    uint8_t network;
    uint64_t nonce;
    uint32_t signature_exp_ledger;
    soroban_authorization_function_type_t function_type;
    invoke_contract_args_t invoke_contract_args;  // only exist when function_type is
                                                  // SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN
} auth_ctx_t;

/**
 * Structure for global context.
 */
typedef struct {
    union {
        tx_ctx_t tx_info;  // tx
        auth_ctx_t auth;   // soroban auth
    };

    uint8_t hash[HASH_SIZE];                              // tx hash
    uint32_t bip32_path[MAX_BIP32_PATH];                  // BIP32 path
    uint8_t raw_public_key[RAW_ED25519_PUBLIC_KEY_SIZE];  // BIP32 path public key
    uint8_t bip32_path_len;                               // length of BIP32 path
    state_e state;                                        // state of the context
    request_type_e req_type;                              // user request
} global_ctx_t;

typedef struct {
    uint64_t amount;
    uint64_t fees;
    char destination[ENCODED_ED25519_PUBLIC_KEY_LENGTH];  // ed25519 address only
    char memo[20];
} swap_values_t;
