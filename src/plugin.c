/*****************************************************************************
 *   Ledger App Stellar.
 *   (c) 2024 Ledger SAS.
 *   (c) 2024 overcat.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include "os.h"

#include "plugin.h"
#include "globals.h"
#include "soroban_token.h"

#define MAX_APP_NAME_LENGTH 20

typedef struct {
    uint8_t contract_address[RAW_CONTRACT_KEY_SIZE];
    char app_name[MAX_APP_NAME_LENGTH];
} plugin_info_t;

static const plugin_info_t plugins[] = {
    {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     "StellarTest"},
};

// get app name if by contract address
static const char *get_app_name(const uint8_t *contract_address) {
    for (uint8_t i = 0; i < sizeof(plugins) / sizeof(plugin_info_t); i++) {
        if (memcmp(plugins[i].contract_address, contract_address, RAW_CONTRACT_KEY_SIZE) == 0) {
            return plugins[i].app_name;
        }
    }
    return NULL;
};

bool plugin_check_presence(const uint8_t *contract_address) {
    // Build-in token plugin
    if (token_plugin_check_presence(contract_address)) {
        return true;
    }

    const char *app_name = get_app_name(contract_address);
    if (app_name == NULL) {
        return false;
    }

    uint32_t params[2];
    params[0] = (uint32_t) app_name;
    params[1] = STELLAR_PLUGIN_CHECK_PRESENCE;
    BEGIN_TRY {
        TRY {
            os_lib_call(params);
            PRINTF("plugin is present\n");
            return true;
        }
        CATCH_OTHER(e) {
            UNUSED(e);
            return false;
        }
        FINALLY {
        }
    }
    END_TRY;
    return false;
};

stellar_plugin_result_t plugin_init_contract(const uint8_t *contract_address) {
    // Build-in token plugin
    if (token_plugin_check_presence(contract_address)) {
        return STELLAR_PLUGIN_RESULT_OK;
    }

    const char *app_name = get_app_name(contract_address);

    stellar_plugin_shared_ro_t plugin_shared_ro = {
        .envelope = &G_context.envelope,
        .raw_size = G_context.raw_size,
        .raw = G_context.raw,
    };

    stellar_plugin_init_contract_t init_contract_param = {
        .interface_version = STELLAR_PLUGIN_INTERFACE_VERSION_LATEST,
        .result = STELLAR_PLUGIN_RESULT_UNAVAILABLE,
        .plugin_shared_ro = &plugin_shared_ro,
    };

    uint32_t params[3];
    params[0] = (uint32_t) app_name;
    params[1] = STELLAR_PLUGIN_INIT_CONTRACT;
    params[2] = (uint32_t) &init_contract_param;
    os_lib_call(params);
    return init_contract_param.result;
};

stellar_plugin_result_t plugin_query_data_pair_count(const uint8_t *contract_address,
                                                     uint8_t *data_pair_count) {
    // Build-in token plugin
    if (token_plugin_check_presence(contract_address)) {
        return token_plugin_query_data_pair_count(contract_address, data_pair_count);
    }

    const char *app_name = get_app_name(contract_address);

    stellar_plugin_shared_ro_t plugin_shared_ro = {
        .envelope = &G_context.envelope,
        .raw_size = G_context.raw_size,
        .raw = G_context.raw,
    };

    stellar_plugin_query_data_pair_count_t query_data_pair_count_param = {
        .result = STELLAR_PLUGIN_RESULT_UNAVAILABLE,
        .data_pair_count = 0,
        .plugin_shared_ro = &plugin_shared_ro,
    };

    uint32_t params[3];
    params[0] = (uint32_t) app_name;
    params[1] = STELLAR_PLUGIN_QUERY_DATA_PAIR_COUNT;
    params[2] = (uint32_t) &query_data_pair_count_param;
    os_lib_call(params);
    *data_pair_count = query_data_pair_count_param.data_pair_count;
    return query_data_pair_count_param.result;
};

stellar_plugin_result_t plugin_query_data_pair(const uint8_t *contract_address,
                                               uint8_t data_pair_index,
                                               char *caption,
                                               uint8_t caption_len,
                                               char *value,
                                               uint8_t value_len) {
    // Build-in token plugin
    if (token_plugin_check_presence(contract_address)) {
        return token_plugin_query_data_pair(contract_address,
                                            data_pair_index,
                                            caption,
                                            caption_len,
                                            value,
                                            value_len);
    }

    const char *app_name = get_app_name(contract_address);

    stellar_plugin_shared_ro_t plugin_shared_ro = {
        .envelope = &G_context.envelope,
        .raw_size = G_context.raw_size,
        .raw = G_context.raw,
    };

    stellar_plugin_query_data_pair_t query_data_pair_param = {
        .result = STELLAR_PLUGIN_RESULT_UNAVAILABLE,
        .caption = caption,
        .value = value,
        .caption_len = caption_len,
        .value_len = value_len,
        .plugin_shared_ro = &plugin_shared_ro,
        .data_pair_index = data_pair_index,
    };

    uint32_t params[3];
    params[0] = (uint32_t) app_name;
    params[1] = STELLAR_PLUGIN_QUERY_DATA_PAIR;
    params[2] = (uint32_t) &query_data_pair_param;
    os_lib_call(params);
    return query_data_pair_param.result;
};
