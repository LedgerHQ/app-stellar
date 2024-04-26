#pragma once

#include "types.h"
#include "buffer.h"
#include "plugin.h"

typedef bool (*plugin_check_presence_t)(const uint8_t *contract_address);

typedef stellar_plugin_result_t (*plugin_init_contract_t)(const uint8_t *contract_address);

typedef stellar_plugin_result_t (*plugin_query_data_pair_count_t)(const uint8_t *contract_address,
                                                                  uint8_t *data_pair_count);

typedef stellar_plugin_result_t (*plugin_query_data_pair_t)(const uint8_t *contract_address,
                                                            uint8_t data_pair_index,
                                                            char *caption,
                                                            uint8_t caption_len,
                                                            char *value,
                                                            uint8_t value_len);

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
    plugin_check_presence_t plugin_check_presence;
    plugin_init_contract_t plugin_init_contract;
    plugin_query_data_pair_count_t plugin_query_data_pair_count;
    plugin_query_data_pair_t plugin_query_data_pair;
} formatter_data_t;

void reset_formatter();

bool get_next_data(formatter_data_t *fdata, bool forward, bool *data_exists, bool *is_op_header);
