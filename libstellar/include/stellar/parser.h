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
