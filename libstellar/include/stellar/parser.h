#pragma once

#include <stdint.h>
#include <string.h>

#include "buffer.h"
#include "stellar/types.h"

bool parse_transaction_envelope(const uint8_t *data, size_t data_len, envelope_t *envelope);

bool parse_transaction_operation(const uint8_t *data,
                                 size_t data_len,
                                 envelope_t *envelope,
                                 uint8_t operation_index);

bool parse_soroban_authorization_envelope(const uint8_t *data,
                                          size_t data_len,
                                          envelope_t *envelope);

bool parse_bool(buffer_t *buffer, bool *b);

bool parse_uint64(buffer_t *buffer, uint64_t *n);

bool parse_int64(buffer_t *buffer, int64_t *n);

bool parse_scv_symbol(buffer_t *buffer, scv_symbol_t *symbol);

bool parse_scv_string(buffer_t *buffer, scv_string_t *string);

bool buffer_read32(buffer_t *buffer, uint32_t *n);

bool parse_sc_address(buffer_t *buffer, sc_address_t *sc_address);

bool read_scval_advance(buffer_t *buffer);
