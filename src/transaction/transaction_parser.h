#pragma once
#include "../types.h"

bool parse_tx_xdr(const uint8_t *data, size_t size, tx_ctx_t *tx_ctx);
