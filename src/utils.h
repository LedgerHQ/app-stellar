#pragma once

#include "./types.h"

#define STRLCPY(dst, src, size)               \
    {                                         \
        size_t len = strlcpy(dst, src, size); \
        if (len >= size) {                    \
            THROW(SW_TX_FORMATTING_FAIL);     \
        }                                     \
    }

#define FORMATTER_CHECK(x)                      \
    {                                           \
        if (!(x)) THROW(SW_TX_FORMATTING_FAIL); \
    }

#define STRLCAT(dst, src, size)               \
    {                                         \
        size_t len = strlcat(dst, src, size); \
        if (len >= size) {                    \
            THROW(SW_TX_FORMATTING_FAIL);     \
        }                                     \
    }

bool encode_ed25519_public_key(const uint8_t raw_public_key[static RAW_ED25519_PUBLIC_KEY_SIZE],
                               char *out,
                               size_t out_len);

bool encode_hash_x_key(const uint8_t raw_hash_x[static RAW_HASH_X_KEY_SIZE],
                       char *out,
                       size_t out_len);

bool encode_pre_auth_x_key(const uint8_t raw_pre_auth_tx[static RAW_PRE_AUTH_TX_KEY_SIZE],
                           char *out,
                           size_t out_len);

bool encode_muxed_account(const muxed_account_t *raw_muxed_account, char *out, size_t out_len);

bool encode_ed25519_signed_payload(const ed25519_signed_payload_t *signed_payload,
                                   char *out,
                                   size_t out_len);

bool print_claimable_balance_id(const claimable_balance_id_t *claimable_balance_id_t,
                                char *out,
                                size_t out_len,
                                uint8_t num_chars_l,
                                uint8_t num_chars_r);

bool print_binary(const uint8_t *in,
                  size_t in_len,
                  char *out,
                  size_t out_len,
                  uint8_t num_chars_l,
                  uint8_t num_chars_r);

bool print_time(uint64_t seconds, char *out, size_t out_len);

bool print_asset_name(const asset_t *asset, uint8_t network_id, char *out, size_t out_len);

bool print_asset(const asset_t *asset, uint8_t network_id, char *out, size_t out_len);

bool print_account_flags(uint32_t flags, char *out, size_t out_len);

bool print_trust_line_flags(uint32_t flags, char *out, size_t out_len);

bool print_allow_trust_flags(uint32_t flag, char *out, size_t out_len);

bool print_amount(uint64_t amount,
                  const asset_t *asset,
                  uint8_t network_id,
                  char *out,
                  size_t out_len);

bool print_account_id(account_id_t account_id,
                      char *out,
                      size_t out_len,
                      uint8_t num_chars_l,
                      uint8_t num_chars_r);

bool print_hash_x_key(const uint8_t raw_hash_x[static RAW_HASH_X_KEY_SIZE],
                      char *out,
                      size_t out_len,
                      uint8_t num_chars_l,
                      uint8_t num_chars_r);

bool print_pre_auth_x_key(const uint8_t raw_pre_auth_tx[static RAW_PRE_AUTH_TX_KEY_SIZE],
                          char *out,
                          size_t out_len,
                          uint8_t num_chars_l,
                          uint8_t num_chars_r);

bool print_ed25519_signed_payload(const ed25519_signed_payload_t *signed_payload,
                                  char *out,
                                  size_t out_len,
                                  uint8_t num_chars_l,
                                  uint8_t num_chars_r);

bool print_sc_address(const sc_address_t *sc_address,
                      char *out,
                      size_t out_len,
                      uint8_t num_chars_l,
                      uint8_t num_chars_r);

bool print_muxed_account(const muxed_account_t *muxed_account,
                         char *out,
                         size_t out_len,
                         uint8_t num_chars_l,
                         uint8_t num_chars_r);

bool print_summary(const char *in,
                   char *out,
                   size_t out_len,
                   uint8_t num_chars_l,
                   uint8_t num_chars_r);

bool print_uint(uint64_t num, char *out, size_t out_len);

bool print_int(int64_t num, char *out, size_t out_len);

bool base64_encode(const uint8_t *data, size_t in_len, char *out, size_t out_len);

bool is_printable_binary(const uint8_t *str, size_t str_len);
