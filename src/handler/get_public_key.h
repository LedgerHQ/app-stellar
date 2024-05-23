#pragma once

#include <stdbool.h>  // bool

#include "buffer.h"

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
