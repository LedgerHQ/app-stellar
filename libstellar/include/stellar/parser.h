#pragma once

#include <stdint.h>
#include <string.h>

#include "buffer.h"
#include "types.h"

/**
 * Parse a transaction envelope from a byte array.
 *
 * @param data The byte array to parse.
 * @param data_len The length of the byte array.
 * @param envelope The envelope to fill with the parsed data.
 *
 * @return True if the parsing was successful, false otherwise.
 */
bool parse_transaction_envelope(const uint8_t *data, size_t data_len, envelope_t *envelope);

/**
 * Parse a transaction operation from a byte array.
 *
 * @param data The byte array to parse.
 * @param data_len The length of the byte array.
 * @param envelope The envelope to fill with the parsed data.
 *
 * @return True if the parsing was successful, false otherwise.
 */
bool parse_transaction_operation(const uint8_t *data,
                                 size_t data_len,
                                 envelope_t *envelope,
                                 uint8_t operation_index);

/**
 * Parse a Soroban authorization envelope from a byte array.
 *
 * @param data The byte array to parse.
 * @param data_len The length of the byte array.
 * @param envelope The envelope to fill with the parsed data.
 *
 * @return True if the parsing was successful, false otherwise.
 */
bool parse_soroban_authorization_envelope(const uint8_t *data,
                                          size_t data_len,
                                          envelope_t *envelope);

/**
 * Parse a boolean from a buffer.
 *
 * @param buffer The buffer to parse.
 * @param b The boolean to fill with the parsed data.
 *
 * @return True if the parsing was successful, false otherwise.
 */
bool parse_bool(buffer_t *buffer, bool *b);

/**
 * Parse a uint64 from a buffer.
 *
 * @param buffer The buffer to parse.
 * @param n The uint64 to fill with the parsed data.
 *
 * @return True if the parsing was successful, false otherwise.
 */
bool parse_uint64(buffer_t *buffer, uint64_t *n);

/**
 * Parse a int64 from a buffer.
 *
 * @param buffer The buffer to parse.
 * @param n The int64 to fill with the parsed data.
 *
 * @return True if the parsing was successful, false otherwise.
 */
bool parse_int64(buffer_t *buffer, int64_t *n);

/**
 * Parse a uint32 from a buffer.
 *
 * @param buffer The buffer to parse.
 * @param n The uint32 to fill with the parsed data.
 *
 * @return True if the parsing was successful, false otherwise.
 */
bool parse_uint32(buffer_t *buffer, uint32_t *n);

/**
 * Parse a int32 from a buffer.
 *
 * @param buffer The buffer to parse.
 * @param n The int32 to fill with the parsed data.
 *
 * @return True if the parsing was successful, false otherwise.
 */
bool parse_int32(buffer_t *buffer, int32_t *n);

/**
 * Parse a scv_symbol from a buffer.
 *
 * @param buffer The buffer to parse.
 * @param symbol The scv_symbol to fill with the parsed data.
 *
 * @return True if the parsing was successful, false otherwise.
 */
bool parse_scv_symbol(buffer_t *buffer, scv_symbol_t *symbol);

/**
 * Parse a scv_string from a buffer.
 *
 * @param buffer The buffer to parse.
 * @param string The scv_string to fill with the parsed data.
 *
 * @return True if the parsing was successful, false otherwise.
 */
bool parse_scv_string(buffer_t *buffer, scv_string_t *string);

/**
 * Parse a sc_address from a buffer.
 *
 * @param buffer The buffer to parse.
 * @param sc_address The sc_address to fill with the parsed data.
 *
 * @return True if the parsing was successful, false otherwise.
 */
bool parse_sc_address(buffer_t *buffer, sc_address_t *sc_address);

/**
 * Read a sc_value from a buffer, advancing the buffer.
 *
 * @param buffer The buffer to read from.
 *
 * @return True if the reading was successful, false otherwise.
 */
bool read_scval_advance(buffer_t *buffer);

/**
 * Parse a auth function from a buffer.
 *
 * @param buffer The buffer to parse.
 * @param type The authorization function type to fill with the parsed data.
 * @param args The invoke contract arguments to fill with the parsed data.
 *
 * @return True if the parsing was successful, false otherwise.
 */
bool parse_auth_function(buffer_t *buffer, uint32_t *type, invoke_contract_args_t *args);
