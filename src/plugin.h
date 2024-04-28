#pragma once

#include "stellar/plugin.h"
#include <stdint.h>
#include <string.h>

/**
 * Check if the plugin is present for a given contract.
 *
 * @param contract_address The address of the contract to check.
 * @return True if the plugin is present, false otherwise.
 */
bool plugin_check_presence(const uint8_t *contract_address);

/**
 * Initialize the plugin for a given contract.
 *
 * @param contract_address The address of the contract to initialize.
 * @return The result of the operation.
 */
stellar_plugin_result_t plugin_init_contract(const uint8_t *contract_address);

/**
 * Query the plugin for the number of data pairs it can provide.
 *
 * @param contract_address The address of the contract to query.
 * @param data_pair_count The number of data pairs the plugin can provide.
 * @return The result of the operation.
 */
stellar_plugin_result_t plugin_query_data_pair_count(const uint8_t *contract_address,
                                                     uint8_t *data_pair_count);

/**
 * Query the plugin for a specific data pair.
 *
 * @param contract_address The address of the contract to query.
 * @param data_pair_index The index of the data pair to query.
 * @param caption The caption of the data pair.
 * @param caption_len The length of the caption buffer.
 * @param value The value of the data pair.
 * @param value_len The length of the value buffer.
 * @return The result of the operation.
 */
stellar_plugin_result_t plugin_query_data_pair(const uint8_t *contract_address,
                                               uint8_t data_pair_index,
                                               char *caption,
                                               uint8_t caption_len,
                                               char *value,
                                               uint8_t value_len);