#pragma once

#include "types.h"
#include "buffer.h"
#include "plugin.h"

/**
 * Function pointer to check if a plugin is present for a given contract.
 */
typedef bool (*plugin_check_presence_t)(const uint8_t *contract_address);

/**
 * Function pointer to initialize a plugin for a given contract.
 */
typedef stellar_plugin_result_t (*plugin_init_contract_t)(const uint8_t *contract_address);

/**
 * Function pointer to query the number of data pairs from a plugin.
 */
typedef stellar_plugin_result_t (*plugin_query_data_pair_count_t)(const uint8_t *contract_address,
                                                                  uint8_t *data_pair_count);

/**
 * Function pointer to query a data pair from a plugin.
 */
typedef stellar_plugin_result_t (*plugin_query_data_pair_t)(const uint8_t *contract_address,
                                                            uint8_t data_pair_index,
                                                            char *caption,
                                                            uint8_t caption_len,
                                                            char *value,
                                                            uint8_t value_len);
/**
 * Structure for formatter data.
 */
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

/**
 * Reset the formatter state.
 */
void reset_formatter(void);

/**
 * Get the next data to display.
 *
 * @param fdata The formatter data.
 * @param forward True if the data should be fetched forward, false if backward.
 * @param data_exists True if data exists, false otherwise.
 * @param is_op_header True if the data is an operation header, false otherwise.
 *
 * @return True if the data was fetched successfully, false otherwise.
 */
bool get_next_data(formatter_data_t *fdata, bool forward, bool *data_exists, bool *is_op_header);
