#pragma once

#include <stdint.h>  // uint*_t

#include "os.h"
#include "cx.h"

#include "stellar/types.h"

/**
 * Derive public key given BIP32 path.
 *
 * @param[out] raw_public_key
 *   Pointer to raw public key.
 * @param[in]  bip32_path
 *  Pointer to buffer with BIP32 path.
 * @param[in]  bip32_path_len
 * Length of BIP32 path.
 *
 * @return CX_OK on success, error number otherwise.
 *
 */
cx_err_t crypto_derive_public_key(uint8_t raw_public_key[static RAW_ED25519_PUBLIC_KEY_SIZE],
                                  const uint32_t *bip32_path,
                                  uint8_t bip32_path_len);

/**
 * Sign message.
 *
 * @param[in]  message
 *  Pointer to message.
 * @param[in]  message_len
 * Length of message.
 * @param[in]  signature
 * Pointer to signature.
 * @param[in]  signature_len
 * Length of signature.
 * @param[in]  bip32_path
 * Pointer to buffer with BIP32 path.
 *
 * @return CX_OK on success, error number otherwise.
 *
 */
cx_err_t crypto_sign_message(const uint8_t *message,
                             uint8_t message_len,
                             const uint8_t *signature,
                             uint8_t signature_len,
                             const uint32_t *bip32_path,
                             uint8_t bip32_path_len);
