#pragma once

#include "types.h"

/**
 * Print a public key.
 *
 * @param public_key The public key to print.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param num_chars_l The number of characters to print on the left side. Set to 0 to print the full
 * public key.
 * @param num_chars_r The number of characters to print on the right side.
 *
 * @return True if the public key was printed successfully, false otherwise.
 */
bool print_account_id(account_id_t account_id,
                      char *out,
                      size_t out_len,
                      uint8_t num_chars_l,
                      uint8_t num_chars_r);

/**
 * Print a hash x key.
 *
 * @param raw_hash_x The raw hash x key to print.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param num_chars_l The number of characters to print on the left side. Set to 0 to print the full
 * key.
 * @param num_chars_r The number of characters to print on the right side.
 *
 * @return True if the key was printed successfully, false otherwise.
 */
bool print_hash_x_key(const uint8_t raw_hash_x[static RAW_HASH_X_KEY_SIZE],
                      char *out,
                      size_t out_len,
                      uint8_t num_chars_l,
                      uint8_t num_chars_r);

/**
 * Print a pre auth x key.
 *
 * @param raw_pre_auth_tx The raw pre auth tx key to print.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param num_chars_l The number of characters to print on the left side. Set to 0 to print the full
 * key.
 * @param num_chars_r The number of characters to print on the right side.
 *
 * @return True if the key was printed successfully, false otherwise.
 */
bool print_pre_auth_x_key(const uint8_t raw_pre_auth_tx[static RAW_PRE_AUTH_TX_KEY_SIZE],
                          char *out,
                          size_t out_len,
                          uint8_t num_chars_l,
                          uint8_t num_chars_r);

/**
 * Print a muxed account.
 *
 * @param muxed_account The muxed account to print.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param num_chars_l The number of characters to print on the left side. Set to 0 to print the full
 * account.
 * @param num_chars_r The number of characters to print on the right side.
 *
 * @return True if the account was printed successfully, false otherwise.
 */
bool print_muxed_account(const muxed_account_t *muxed_account,
                         char *out,
                         size_t out_len,
                         uint8_t num_chars_l,
                         uint8_t num_chars_r);

/**
 * Print a sc address.
 *
 * @param sc_address The SC address to print.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param num_chars_l The number of characters to print on the left side. Set to 0 to print the full
 * address.
 * @param num_chars_r The number of characters to print on the right side.
 *
 * @return True if the SC address was printed successfully, false otherwise.
 */
bool print_sc_address(const sc_address_t *sc_address,
                      char *out,
                      size_t out_len,
                      uint8_t num_chars_l,
                      uint8_t num_chars_r);

/**
 * Print an Ed25519 signed payload.
 *
 * @param signed_payload The Ed25519 signed payload to print.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param num_chars_l The number of characters to print on the left side. Set to 0 to print the full
 * payload.
 * @param num_chars_r The number of characters to print on the right side.
 *
 * @return True if the signed payload was printed successfully, false otherwise.
 */
bool print_ed25519_signed_payload(const ed25519_signed_payload_t *signed_payload,
                                  char *out,
                                  size_t out_len,
                                  uint8_t num_chars_l,
                                  uint8_t num_chars_r);

/**
 * Print an asset name with the given network ID.
 *
 * @param asset The asset to print the name for.
 * @param network_id The network ID to consider when printing the asset name.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 *
 * @return True if the asset name was printed successfully, false otherwise.
 */
bool print_asset_name(const asset_t *asset, uint8_t network_id, char *out, size_t out_len);

/**
 * Print an asset with the given network ID.
 *
 * @param asset The asset to print.
 * @param network_id The network ID to consider when printing the asset.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 *
 * @return True if the asset was printed successfully, false otherwise.
 */
bool print_asset(const asset_t *asset, uint8_t network_id, char *out, size_t out_len);

/**
 * Print an amount with the given asset and network ID.
 *
 * @param amount The amount to print.
 * @param asset The asset to consider when printing the amount.
 * @param network_id The network ID to consider when printing the amount.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 *
 * @return True if the amount was printed successfully, false otherwise.
 */
bool print_amount(uint64_t amount,
                  const asset_t *asset,
                  uint8_t network_id,
                  char *out,
                  size_t out_len);

/**
 * Print a claimable balance ID.
 *
 * @param claimable_balance_id_t The claimable balance ID to print.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param num_chars_l The number of characters to print on the left side. Set to 0 to print the full
 * ID.
 * @param num_chars_r The number of characters to print on the right side.
 *
 * @return True if the claimable balance ID was printed successfully, false otherwise.
 */
bool print_claimable_balance_id(const claimable_balance_id_t *claimable_balance_id_t,
                                char *out,
                                size_t out_len,
                                uint8_t num_chars_l,
                                uint8_t num_chars_r);

/**
 * Print account flags in a human-readable format.
 *
 * @param flags The account flags to print.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 *
 * @return True if the account flags were printed successfully, false otherwise.
 */
bool print_account_flags(uint32_t flags, char *out, size_t out_len);

/**
 * Print trust line flags in a human-readable format.
 *
 * @param flags The trust line flags to print.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 *
 * @return True if the trust line flags were printed successfully, false otherwise.
 */
bool print_trust_line_flags(uint32_t flags, char *out, size_t out_len);

/**
 * Print allow trust flags in a human-readable format.
 *
 * @param flag The allow trust flag to print.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 *
 * @return True if the allow trust flag was printed successfully, false otherwise.
 */
bool print_allow_trust_flags(uint32_t flag, char *out, size_t out_len);

/**
 * Print an unsigned 64-bit integer in a human-readable format.
 *
 * @param num The unsigned integer to print.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 *
 * @return True if the unsigned integer was printed successfully, false otherwise.
 */
bool print_uint64_num(uint64_t num, char *out, size_t out_len);

/**
 * Print a signed 64-bit integer in a human-readable format.
 *
 * @param num The signed integer to print.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 *
 * @return True if the signed integer was printed successfully, false otherwise.
 */
bool print_int64_num(int64_t num, char *out, size_t out_len);

/**
 * Check if a binary string is printable.
 *
 * @param str The binary string to check.
 * @param str_len The length of the binary string.
 *
 * @return True if the binary string is printable, false otherwise.
 */
bool is_printable_binary(const uint8_t *str, size_t str_len);

/**
 * Print a binary string in a human-readable format.
 *
 * @param in The binary input to print.
 * @param in_len The length of the binary input.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param num_chars_l The number of characters to print on the left side. Set to 0 to print the full
 * binary string.
 * @param num_chars_r The number of characters to print on the right side.
 *
 * @return True if the binary string was printed successfully, false otherwise.
 */
bool print_binary(const uint8_t *in,
                  size_t in_len,
                  char *out,
                  size_t out_len,
                  uint8_t num_chars_l,
                  uint8_t num_chars_r);

/**
 * Print a time in seconds as a human-readable string. UTC time is used.
 *
 * @param seconds The time in seconds to print.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 *
 * @return True if the time was printed successfully, false otherwise.
 */
bool print_time(uint64_t seconds, char *out, size_t out_len);

/**
 * Print a 32-bit integer with an optional separator.
 *
 * @param value The 32-bit integer value to print.
 * @param decimals The number of decimal places to add.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param add_separator A flag indicating whether to add a separator between digits.
 *
 * @return True if the integer was printed successfully, false otherwise.
 */
bool print_int32(const uint8_t *value,
                 uint8_t decimals,
                 char *out,
                 size_t out_len,
                 bool add_separator);

/**
 * Print an unsigned 32-bit integer with an optional separator.
 *
 * @param value The unsigned 32-bit integer value to print.
 * @param decimals The number of decimal places to add.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param add_separator A flag indicating whether to add a separator between digits.
 *
 * @return True if the unsigned integer was printed successfully, false otherwise.
 */
bool print_uint32(const uint8_t *value,
                  uint8_t decimals,
                  char *out,
                  size_t out_len,
                  bool add_separator);

/**
 * Print a 64-bit integer with an optional separator.
 *
 * @param value The 64-bit integer value to print.
 * @param decimals The number of decimal places to add.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param add_separator A flag indicating whether to add a separator between digits.
 *
 * @return True if the integer was printed successfully, false otherwise.
 */
bool print_int64(const uint8_t *value,
                 uint8_t decimals,
                 char *out,
                 size_t out_len,
                 bool add_separator);

/**
 * Print an unsigned 64-bit integer with an optional separator.
 *
 * @param value The unsigned 64-bit integer value to print.
 * @param decimals The number of decimal places to add.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param add_separator A flag indicating whether to add a separator between digits.
 *
 * @return True if the unsigned 64-bit integer was printed successfully, false otherwise.
 */
bool print_uint64(const uint8_t *value,
                  uint8_t decimals,
                  char *out,
                  size_t out_len,
                  bool add_separator);

/**
 * Print a 128-bit integer with an optional separator.
 *
 * @param value The 128-bit integer value to print.
 * @param decimals The number of decimal places to add.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param add_separator A flag indicating whether to add a separator between digits.
 *
 * @return True if the 128-bit integer was printed successfully, false otherwise.
 */
bool print_int128(const uint8_t *value,
                  uint8_t decimals,
                  char *out,
                  size_t out_len,
                  bool add_separator);

/**
 * Print an unsigned 128-bit integer with an optional separator.
 *
 * @param value The unsigned 128-bit integer value to print.
 * @param decimals The number of decimal places to add.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param add_separator A flag indicating whether to add a separator between digits.
 *
 * @return True if the unsigned 128-bit integer was printed successfully, false otherwise.
 */
bool print_uint128(const uint8_t *value,
                   uint8_t decimals,
                   char *out,
                   size_t out_len,
                   bool add_separator);

/**
 * Print a 256-bit integer with an optional separator.
 *
 * @param value The 256-bit integer value to print.
 * @param decimals The number of decimal places to add.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param add_separator A flag indicating whether to add a separator between digits.
 *
 * @return True if the 256-bit integer was printed successfully, false otherwise.
 */
bool print_int256(const uint8_t *value,
                  uint8_t decimals,
                  char *out,
                  size_t out_len,
                  bool add_separator);

/**
 * Print an unsigned 256-bit integer with an optional separator.
 *
 * @param value The unsigned 256-bit integer value to print.
 * @param decimals The number of decimal places to add.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param add_separator A flag indicating whether to add a separator between digits.
 *
 * @return True if the unsigned 256-bit integer was printed successfully, false otherwise.
 */
bool print_uint256(const uint8_t *value,
                   uint8_t decimals,
                   char *out,
                   size_t out_len,
                   bool add_separator);

/**
 * Print a scv symbol.
 *
 * @param scv_symbol The Stellar scv symbol to print.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 *
 * @return True if the scv symbol was printed successfully, false otherwise.
 */
bool print_scv_symbol(const scv_symbol_t *scv_symbol, char *out, size_t out_len);

/**
 * Print a scv string.
 *
 * @param scv_string The scv string to print.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 *
 * @return True if the scv string was printed successfully, false otherwise.
 */
bool print_scv_string(const scv_string_t *scv_string, char *out, size_t out_len);

/**
 * Add a separator to a number printed in the output buffer.
 *
 * @param out The output buffer where the separator will be added.
 * @param out_len The length of the output buffer.
 *
 * @return True if the separator was added successfully, false otherwise.
 */
bool add_separator_to_number(char *out, size_t out_len);

/**
 * Add a decimal point to a number printed in the output buffer.
 *
 * @param out The output buffer where the decimal point will be added.
 * @param out_len The length of the output buffer.
 * @param decimals The number of decimal places to add.
 *
 * @return True if the decimal point was added successfully, false otherwise.
 */
bool add_decimal_point(char *out, size_t out_len, uint8_t decimals);

/**
 * Print a string.
 *
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 * @param src The source buffer.
 * @param src_size The size of the source buffer, don't include the null terminator.
 *
 * @return True if the string was printed successfully, false otherwise.
 */
bool print_string(char *out, size_t out_len, const uint8_t *src, size_t src_size);

/**
 * Print a price.
 *
 * @param price The price to print.
 * @param asset_a The first asset.
 * @param asset_b The second asset.
 * @param network_id The network ID to consider when printing the price.
 * @param out The output buffer.
 * @param out_len The length of the output buffer.
 *
 * @return True if the price was printed successfully, false otherwise.
 */
bool print_price(const price_t *price,
                 const asset_t *asset_a,
                 const asset_t *asset_b,
                 uint8_t network_id,
                 char *out,
                 size_t out_len);