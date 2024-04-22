#pragma once

#include "types.h"
#include "buffer.h"

typedef struct {
    const uint8_t *raw_data;
    size_t raw_data_len;
    envelope_t *envelope;
    char *caption;
    char *value;
    uint8_t *signing_key;
    size_t value_len;
    size_t caption_len;
    bool display_sequence;
} formatter_data_t;

void reset_formatter();

bool get_next_data(formatter_data_t *fdata, bool forward, bool *data_exists, bool *is_op_header);
