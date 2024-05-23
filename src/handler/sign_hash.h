#pragma once

#include "buffer.h"

/**
 * Handler for INS_SIGN_HASH command. If successfully parse BIP32 path
 * and the hash, sign the hash and send APDU response.
 * *
 * @param[in,out] cdata
 *   Command data with BIP32 path and the hash.
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int handler_sign_hash(buffer_t *cdata);