#pragma once

#include <stdint.h>  // uint*_t

#include "os.h"
#include "cx.h"

#include "./types.h"

/**
 * Derive private key given BIP32 path.
 *
 * @param[out] private_key
 *   Pointer to private key.
 * @param[in]  bip32_path
 *   Pointer to buffer with BIP32 path.
 * @param[in]  bip32_path_len
 *   Number of path in BIP32 path.
 *
 * @return 0 on success, error number otherwise.
 *
 */
int crypto_derive_private_key(cx_ecfp_private_key_t *private_key,
                              const uint32_t *bip32_path,
                              uint8_t bip32_path_len);
/**
 * Initialize public key given private key.
 *
 * @param[in]  private_key
 *   Pointer to private key.
 * @param[out] public_key
 *   Pointer to public key.
 * @param[out] raw_public_key
 *   Pointer to raw public key.
 *
 * @return 0 on success, error number otherwise.
 *
 */
int crypto_init_public_key(cx_ecfp_private_key_t *private_key,
                           cx_ecfp_public_key_t *public_key,
                           uint8_t raw_public_key[static RAW_ED25519_PUBLIC_KEY_SIZE]);

/**
 * Sign message.
 *
 * @return 0 on success, error number otherwise.
 *
 */
int crypto_sign_message(const uint8_t *message,
                        uint8_t message_len,
                        const uint8_t *signature,
                        uint8_t signature_len);