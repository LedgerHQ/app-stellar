#pragma once

#include "os.h"
#include "macros.h"

/**
 * Helper to send APDU response with public key.
 *
 * response = G_context.raw_public_key (RAW_ED25519_PUBLIC_KEY_SIZE)
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int helper_send_response_pubkey(void);

/**
 * Helper to send APDU response with signature.
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int helper_send_response_sig(const uint8_t *signature);
