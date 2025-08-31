#pragma once

#include "buffer.h"

/**
 * Handler for INS_SIGN_MESSAGE command. If successfully parse BIP32 path
 * and the message, sign the message and send APDU response.
 * *
 * @param[in,out] cdata
 *   Command data with BIP32 path and the message.
 * @param[in]     is_first_chunk
 *   Is the first data chunk
 * @param[in]     more
 *   Whether more APDU chunk to be received or not.
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int handler_sign_message(buffer_t *cdata, bool is_first_chunk, bool more);
