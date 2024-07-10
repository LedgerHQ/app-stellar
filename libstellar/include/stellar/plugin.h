#pragma once

#include "types.h"

// Interface version. Will be updated every time a breaking change in the interface is introduced.
typedef enum {
    STELLAR_PLUGIN_INTERFACE_VERSION_1 = 1,
    STELLAR_PLUGIN_INTERFACE_VERSION_LATEST = 1,
} stellar_plugin_interface_version_t;

// Codes for the different requests Stellar can send to the plugin
// The dispatch is handled by the SDK itself, the plugin code does not have to handle it
typedef enum {
    // Special request: the Stellar app is checking if we are installed on the device
    STELLAR_PLUGIN_CHECK_PRESENCE = 0x00,
    // Codes for actions the Stellar app can ask the plugin to perform
    STELLAR_PLUGIN_INIT_CONTRACT = 0x01,
    STELLAR_PLUGIN_QUERY_DATA_PAIR_COUNT = 0x02,
    STELLAR_PLUGIN_QUERY_DATA_PAIR = 0x0103,
} stellar_plugin_msg_t;

// Reply codes when responding to the Stellar application
typedef enum {
    // Successful return values
    STELLAR_PLUGIN_RESULT_OK = 0x00,

    // Unsuccessful return values
    STELLAR_PLUGIN_RESULT_ERROR = 0x01,
    STELLAR_PLUGIN_RESULT_UNAVAILABLE = 0x02,
} stellar_plugin_result_t;

// Transaction data available to the plugin READ-ONLY
typedef struct {
    const envelope_t *envelope;
    const size_t raw_size;
    const uint8_t *raw;
} stellar_plugin_shared_ro_t;

/**
 * Structure for the plugin to initialize itself for a given contract.
 */
typedef struct {
    stellar_plugin_interface_version_t interface_version;
    stellar_plugin_result_t result;

    // in
    const stellar_plugin_shared_ro_t *plugin_shared_ro;
} stellar_plugin_init_contract_t;

/**
 * Structure for the plugin to return the number of data pairs it can provide.
 */
typedef struct {
    stellar_plugin_result_t result;
    uint8_t data_pair_count;

    // in
    const stellar_plugin_shared_ro_t *plugin_shared_ro;
} stellar_plugin_query_data_pair_count_t;

/**
 * Structure for the plugin to return a specific data pair.
 */
typedef struct {
    stellar_plugin_result_t result;
    char *caption;        // to store the caption
    char *value;          // to store the value
    uint8_t caption_len;  // the max length of the caption, including the null terminator
    uint8_t value_len;    // the max length of the value, including the null terminator

    // in
    const uint8_t data_pair_index;
    const stellar_plugin_shared_ro_t *plugin_shared_ro;
} stellar_plugin_query_data_pair_t;
