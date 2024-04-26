#pragma once

#include "stellar/plugin.h"
#include <stdint.h>
#include <string.h>

bool plugin_check_presence(const uint8_t *contract_address);

stellar_plugin_result_t plugin_init_contract(const uint8_t *contract_address);

stellar_plugin_result_t plugin_query_data_pair_count(const uint8_t *contract_address,
                                                     uint8_t *data_pair_count);

stellar_plugin_result_t plugin_query_data_pair(const uint8_t *contract_address,
                                               uint8_t data_pair_index,
                                               char *caption,
                                               uint8_t caption_len,
                                               char *value,
                                               uint8_t value_len);