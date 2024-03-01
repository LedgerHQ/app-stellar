#pragma once

#include <stdbool.h>  // bool
#include "../common/buffer.h"

/**
 * Handler for INS_INS_GET_APP_CONFIGURATION command. Send APDU response with version
 * of the application.
 *
 * @see MAJOR_VERSION, MINOR_VERSION and PATCH_VERSION in Makefile.
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int handler_get_app_configuration(void);

/**
 * Handler for INS_GET_PUBLIC_KEY command. If successfully parse BIP32 path,
 * derive public key and send APDU response.
 *
 * @param[in,out] cdata
 *   Command data with BIP32 path.
 * @param[in]     display
 *   Whether to display address on screen or not.
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int handler_get_public_key(buffer_t *cdata, bool display);

/**
 * Handler for INS_SIGN_TX command. If successfully parse BIP32 path
 * and transaction, sign transaction and send APDU response.
 *
 * @param[in,out] cdata
 *   Command data with BIP32 path and raw transaction serialized.
 * @param[in]     is_first_chunk
 *   Is the first data chunk
 * @param[in]       more
 *   Whether more APDU chunk to be received or not.
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int handler_sign_tx(buffer_t *cdata, bool is_first_chunk, bool more);

/**
 * Handler for INS_SIGN_TX_HASH command. If successfully parse BIP32 path
 * and transaction hash, sign transaction hash and send APDU response.
 * *
 * @param[in,out] cdata
 *   Command data with BIP32 path and transaction hash.
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int handler_sign_tx_hash(buffer_t *cdata);

/**
 * Handler for INS_SIGN_SOROBAN_AUTHORATION command. If successfully parse BIP32 path
 * and soroban authorization, sign soroban authorization and send APDU response.
 *
 * @param[in,out] cdata
 *   Command data with BIP32 path and soroban authorization serialized.
 * @param[in]     is_first_chunk
 *   Is the first data chunk
 * @param[in]       more
 *   Whether more APDU chunk to be received or not.
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int handle_sign_soroban_authorization(buffer_t *cdata, bool is_first_chunk, bool more);