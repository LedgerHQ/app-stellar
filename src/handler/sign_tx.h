#pragma once

#include <stdbool.h>  // bool

#include "buffer.h"

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
