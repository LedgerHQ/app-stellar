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

#include <string.h>

#include "globals.h"
#include "stellar/printer.h"
#include "stellar/parser.h"
#include "stellar/plugin.h"

#define MAX_TOKEN_NAME_LENGTH  13
#define CLASSIC_ASSET_DECIMALS 7

typedef struct {
    uint8_t contract_address[RAW_CONTRACT_KEY_SIZE];
    char asset_name[MAX_TOKEN_NAME_LENGTH];
} soroban_token_t;

static const soroban_token_t tokens[] = {
    // CAS3J7GYLGXMF6TDJBBYYSE3HQ6BBSMLNUQ34T6TZMYMW2EVH34XOWMA
    {{
         0x25, 0xb4, 0xfc, 0xd8, 0x59, 0xae, 0xc2, 0xfa, 0x63, 0x48, 0x43,
         0x8c, 0x48, 0x9b, 0x3c, 0x3c, 0x10, 0xc9, 0x8b, 0x6d, 0x21, 0xbe,
         0x4f, 0xd3, 0xcb, 0x30, 0xcb, 0x68, 0x95, 0x3e, 0xf9, 0x77,
     },
     "XLM"},
    // CCW67TSZV3SSS2HXMBQ5JFGCKJNXKZM7UQUWUZPUTHXSTZLEO7SJMI75
    {
        {
            0xad, 0xef, 0xce, 0x59, 0xae, 0xe5, 0x29, 0x68, 0xf7, 0x60, 0x61,
            0xd4, 0x94, 0xc2, 0x52, 0x5b, 0x75, 0x65, 0x9f, 0xa4, 0x29, 0x6a,
            0x65, 0xf4, 0x99, 0xef, 0x29, 0xe5, 0x64, 0x77, 0xe4, 0x96,
        },
        "USDC",
    },
};

// get token name if by contract address
static const char *get_token_name(const uint8_t *contract_address) {
    for (uint8_t i = 0; i < sizeof(tokens) / sizeof(soroban_token_t); i++) {
        if (memcmp(tokens[i].contract_address, contract_address, RAW_CONTRACT_KEY_SIZE) == 0) {
            return tokens[i].asset_name;
        }
    }
    return NULL;
};

bool token_plugin_check_presence(const uint8_t *contract_address) {
    return get_token_name(contract_address) != NULL;
}

stellar_plugin_result_t token_plugin_query_data_pair_count(const uint8_t *contract_address,
                                                           uint8_t *data_pair_count) {
    if (get_token_name(contract_address) == NULL) {
        return STELLAR_PLUGIN_RESULT_UNAVAILABLE;
    }
    invoke_contract_args_t invoke_contract_args =
        G_context.envelope.type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION
            ? G_context.envelope.soroban_authorization.invoke_contract_args
            : G_context.envelope.tx_details.tx.op_details.invoke_host_function_op
                  .invoke_contract_args;
    char function_name[SCV_SYMBOL_MAX_SIZE + 1] = {0};
    memcpy(function_name,
           invoke_contract_args.function.name,
           invoke_contract_args.function.name_size);
    if (strcmp(function_name, "transfer") == 0) {
        (*data_pair_count) = 3;
        return STELLAR_PLUGIN_RESULT_OK;
    }

    if (strcmp(function_name, "approve") == 0) {
        (*data_pair_count) = 4;
        return STELLAR_PLUGIN_RESULT_OK;
    }

    return STELLAR_PLUGIN_RESULT_UNAVAILABLE;
}

stellar_plugin_result_t token_plugin_query_data_pair(const uint8_t *contract_address,
                                                     uint8_t data_pair_index,
                                                     char *caption,
                                                     uint8_t caption_len,
                                                     char *value,
                                                     uint8_t value_len) {
    const char *token_name = get_token_name(contract_address);
    if (token_name == NULL) {
        return STELLAR_PLUGIN_RESULT_UNAVAILABLE;
    }

    invoke_contract_args_t invoke_contract_args =
        G_context.envelope.type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION
            ? G_context.envelope.soroban_authorization.invoke_contract_args
            : G_context.envelope.tx_details.tx.op_details.invoke_host_function_op
                  .invoke_contract_args;
    char function_name[SCV_SYMBOL_MAX_SIZE + 1] = {0};
    memcpy(function_name,
           invoke_contract_args.function.name,
           invoke_contract_args.function.name_size);
    buffer_t buffer = {.ptr = G_context.raw,
                       .size = G_context.raw_size,
                       .offset = invoke_contract_args.parameters_position};
    if (strcmp(function_name, "transfer") == 0) {
        switch (data_pair_index) {
            case 0: {
                strlcpy(caption, "Transfer", caption_len);
                if (!read_scval_advance(&buffer) || !read_scval_advance(&buffer)) {
                    return STELLAR_PLUGIN_RESULT_ERROR;
                }

                uint32_t sc_type;
                if (!parse_uint32(&buffer, &sc_type) || sc_type != SCV_I128 ||
                    !print_int128(buffer.ptr + buffer.offset,
                                  CLASSIC_ASSET_DECIMALS,
                                  value,
                                  value_len,
                                  true)) {
                    return STELLAR_PLUGIN_RESULT_ERROR;
                }
                strlcat(value, " ", value_len);
                strlcat(value, token_name, value_len);
                break;
            }
            case 1: {
                strlcpy(caption, "From", caption_len);
                sc_address_t from;
                uint32_t sc_type;

                if (!parse_uint32(&buffer, &sc_type) || sc_type != SCV_ADDRESS ||
                    !parse_sc_address(&buffer, &from) ||
                    !print_sc_address(&from, value, value_len, 0, 0)) {
                    return STELLAR_PLUGIN_RESULT_ERROR;
                }

                break;
            }
            case 2:
                strlcpy(caption, "To", caption_len);
                if (!read_scval_advance(&buffer)) {
                    return STELLAR_PLUGIN_RESULT_ERROR;
                }

                sc_address_t to;
                uint32_t sc_type;

                if (!parse_uint32(&buffer, &sc_type) || sc_type != SCV_ADDRESS ||
                    !parse_sc_address(&buffer, &to) ||
                    !print_sc_address(&to, value, value_len, 0, 0)) {
                    return STELLAR_PLUGIN_RESULT_ERROR;
                }
                break;
            default:
                return STELLAR_PLUGIN_RESULT_ERROR;
        }
    }

    if (strcmp(function_name, "approve") == 0) {
        switch (data_pair_index) {
            case 0: {
                strlcpy(caption, "From", caption_len);
                sc_address_t from;
                uint32_t sc_type;

                if (!parse_uint32(&buffer, &sc_type) || sc_type != SCV_ADDRESS ||
                    !parse_sc_address(&buffer, &from) ||
                    !print_sc_address(&from, value, value_len, 0, 0)) {
                    return STELLAR_PLUGIN_RESULT_ERROR;
                }
                break;
            }
            case 1: {
                strlcpy(caption, "Spender", caption_len);
                if (!read_scval_advance(&buffer)) {
                    return STELLAR_PLUGIN_RESULT_ERROR;
                }

                sc_address_t to;
                uint32_t sc_type;

                if (!parse_uint32(&buffer, &sc_type) || sc_type != SCV_ADDRESS ||
                    !parse_sc_address(&buffer, &to) ||
                    !print_sc_address(&to, value, value_len, 0, 0)) {
                    return STELLAR_PLUGIN_RESULT_ERROR;
                }
                break;
            }
            case 2: {
                strlcpy(caption, "Amount", caption_len);
                if (!read_scval_advance(&buffer) || !read_scval_advance(&buffer)) {
                    return STELLAR_PLUGIN_RESULT_ERROR;
                }

                uint32_t sc_type;
                if (!parse_uint32(&buffer, &sc_type) || sc_type != SCV_I128 ||
                    !print_int128(buffer.ptr + buffer.offset,
                                  CLASSIC_ASSET_DECIMALS,
                                  value,
                                  value_len,
                                  true)) {
                    return STELLAR_PLUGIN_RESULT_ERROR;
                }

                strlcat(value, " ", value_len);
                strlcat(value, token_name, value_len);
                break;
            }
            case 3: {
                strlcpy(caption, "Live Until Ledger", caption_len);
                if (!read_scval_advance(&buffer) || !read_scval_advance(&buffer) ||
                    !read_scval_advance(&buffer)) {
                    return STELLAR_PLUGIN_RESULT_ERROR;
                }

                uint32_t sc_type;
                if (!parse_uint32(&buffer, &sc_type) || sc_type != SCV_U32 ||
                    !print_uint32(buffer.ptr + buffer.offset, 0, value, value_len, false)) {
                    return STELLAR_PLUGIN_RESULT_ERROR;
                }

                break;
            }
            default:
                return STELLAR_PLUGIN_RESULT_ERROR;
        }
        return STELLAR_PLUGIN_RESULT_OK;
    }

    return STELLAR_PLUGIN_RESULT_OK;
}