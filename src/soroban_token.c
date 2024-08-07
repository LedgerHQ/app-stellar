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

// This list is generated based on the data from scripts/gen_token_list.py
// Due to storage space limitations, on the NANO S we only support XLM and USDC.
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
#ifndef TARGET_NANOS
    // CBVDRT5474OBUEXF5MJB3UGQ5CG7CKGCAH5M4RV5NBCDJUBZ5OXHJLOU
    {{
         0x6a, 0x38, 0xcf, 0xbc, 0xff, 0x1c, 0x1a, 0x12, 0xe5, 0xeb, 0x12,
         0x1d, 0xd0, 0xd0, 0xe8, 0x8d, 0xf1, 0x28, 0xc2, 0x01, 0xfa, 0xce,
         0x46, 0xbd, 0x68, 0x44, 0x34, 0xd0, 0x39, 0xeb, 0xae, 0x74,
     },
     "EURC"},
    // CAIRIR3ITE2KNBWHRIAOBBZ2AHIKU5BVTKFTW5IYCOAENR4L5T2THGN6
    {{
         0x11, 0x14, 0x47, 0x68, 0x99, 0x34, 0xa6, 0x86, 0xc7, 0x8a, 0x00,
         0xe0, 0x87, 0x3a, 0x01, 0xd0, 0xaa, 0x74, 0x35, 0x9a, 0x8b, 0x3b,
         0x75, 0x18, 0x13, 0x80, 0x46, 0xc7, 0x8b, 0xec, 0xf5, 0x33,
     },
     "BTC"},
    // CBHIQPUXLFLC5O44ZJVUTCL5LMZFLVGU5DEIGSYKBSAPFMOGTKOQEPFM
    {{
         0x4e, 0x88, 0x3e, 0x97, 0x59, 0x56, 0x2e, 0xbb, 0x9c, 0xca, 0x6b,
         0x49, 0x89, 0x7d, 0x5b, 0x32, 0x55, 0xd4, 0xd4, 0xe8, 0xc8, 0x83,
         0x4b, 0x0a, 0x0c, 0x80, 0xf2, 0xb1, 0xc6, 0x9a, 0x9d, 0x02,
     },
     "BTCLN"},
    // CD2GNQFE3GRUY5LRE5WT26OQIUBUQ5YKRVDRQ3FWCW7A7NZAPPFIHFAY
    {{
         0xf4, 0x66, 0xc0, 0xa4, 0xd9, 0xa3, 0x4c, 0x75, 0x71, 0x27, 0x6d,
         0x3d, 0x79, 0xd0, 0x45, 0x03, 0x48, 0x77, 0x0a, 0x8d, 0x47, 0x18,
         0x6c, 0xb6, 0x15, 0xbe, 0x0f, 0xb7, 0x20, 0x7b, 0xca, 0x83,
     },
     "TZS"},
    // CAUIKL3IYGMERDRUN6YSCLWVAKIFG5Q4YJHUKM4S4NJZQIA3BAS6OJPK
    {{
         0x28, 0x85, 0x2f, 0x68, 0xc1, 0x98, 0x48, 0x8e, 0x34, 0x6f, 0xb1,
         0x21, 0x2e, 0xd5, 0x02, 0x90, 0x53, 0x76, 0x1c, 0xc2, 0x4f, 0x45,
         0x33, 0x92, 0xe3, 0x53, 0x98, 0x20, 0x1b, 0x08, 0x25, 0xe7,
     },
     "AQUA"},
    // CDCKFBZYF2AQCSM3JOF2ZM27O3Y6AJAI4OTCQKAFNZ3FHBYUTFOKICIY
    {{
         0xc4, 0xa2, 0x87, 0x38, 0x2e, 0x81, 0x01, 0x49, 0x9b, 0x4b, 0x8b,
         0xac, 0xb3, 0x5f, 0x76, 0xf1, 0xe0, 0x24, 0x08, 0xe3, 0xa6, 0x28,
         0x28, 0x05, 0x6e, 0x76, 0x53, 0x87, 0x14, 0x99, 0x5c, 0xa4,
     },
     "XTAR"},
    // CAPIOPSODD5QP4SJNIS4ASUWML4LH7ZEKTAPBJYZSMKXCATEKDZFKLHK
    {{
         0x1e, 0x87, 0x3e, 0x4e, 0x18, 0xfb, 0x07, 0xf2, 0x49, 0x6a, 0x25,
         0xc0, 0x4a, 0x96, 0x62, 0xf8, 0xb3, 0xff, 0x24, 0x54, 0xc0, 0xf0,
         0xa7, 0x19, 0x93, 0x15, 0x71, 0x02, 0x64, 0x50, 0xf2, 0x55,
     },
     "NUNA"},
    // CBF4E5GSTVSITE5Q2ENOTEUQJPBZAU3SBDVLQMSQ7GLBRTSYGUAT722K
    {{
         0x4b, 0xc2, 0x74, 0xd2, 0x9d, 0x64, 0x89, 0x93, 0xb0, 0xd1, 0x1a,
         0xe9, 0x92, 0x90, 0x4b, 0xc3, 0x90, 0x53, 0x72, 0x08, 0xea, 0xb8,
         0x32, 0x50, 0xf9, 0x96, 0x18, 0xce, 0x58, 0x35, 0x01, 0x3f,
     },
     "BRL"},
    // CBH4M45TQBLDPXOK6L7VYKMEJWFITBOL64BN3WDAIIDT4LNUTWTTOCKF
    {{
         0x4f, 0xc6, 0x73, 0xb3, 0x80, 0x56, 0x37, 0xdd, 0xca, 0xf2, 0xff,
         0x5c, 0x29, 0x84, 0x4d, 0x8a, 0x89, 0x85, 0xcb, 0xf7, 0x02, 0xdd,
         0xd8, 0x60, 0x42, 0x07, 0x3e, 0x2d, 0xb4, 0x9d, 0xa7, 0x37,
     },
     "ETH"},
    // CDOFW7HNKLUZRLFZST4EW7V3AV4JI5IHMT6BPXXSY2IEFZ4NE5TWU2P4
    {{
         0xdc, 0x5b, 0x7c, 0xed, 0x52, 0xe9, 0x98, 0xac, 0xb9, 0x94, 0xf8,
         0x4b, 0x7e, 0xbb, 0x05, 0x78, 0x94, 0x75, 0x07, 0x64, 0xfc, 0x17,
         0xde, 0xf2, 0xc6, 0x90, 0x42, 0xe7, 0x8d, 0x27, 0x67, 0x6a,
     },
     "yUSDC"},
    // CBHBD77PWZ3AXPQVYVDBHDKEMVNOR26UZUZHWCB6QC7J5SETQPRUQAS4
    {{
         0x4e, 0x11, 0xff, 0xef, 0xb6, 0x76, 0x0b, 0xbe, 0x15, 0xc5, 0x46,
         0x13, 0x8d, 0x44, 0x65, 0x5a, 0xe8, 0xeb, 0xd4, 0xcd, 0x32, 0x7b,
         0x08, 0x3e, 0x80, 0xbe, 0x9e, 0xc8, 0x93, 0x83, 0xe3, 0x48,
     },
     "SSLX"},
    // CCD6H4LBTHAPY3NGEE6TLLRUSPJGX4K5XI2J6E4MUNDB5TNXEKC23H5B
    {{
         0x87, 0xe3, 0xf1, 0x61, 0x99, 0xc0, 0xfc, 0x6d, 0xa6, 0x21, 0x3d,
         0x35, 0xae, 0x34, 0x93, 0xd2, 0x6b, 0xf1, 0x5d, 0xba, 0x34, 0x9f,
         0x13, 0x8c, 0xa3, 0x46, 0x1e, 0xcd, 0xb7, 0x22, 0x85, 0xad,
     },
     "ARS"},
    // CCXY3CNHSU2DPUOZFKNNH67IVRMBRCATX4SABDSLBY5LAJI66LRLHTJQ
    {{
         0xaf, 0x8d, 0x89, 0xa7, 0x95, 0x34, 0x37, 0xd1, 0xd9, 0x2a, 0x9a,
         0xd3, 0xfb, 0xe8, 0xac, 0x58, 0x18, 0x88, 0x13, 0xbf, 0x24, 0x00,
         0x8e, 0x4b, 0x0e, 0x3a, 0xb0, 0x25, 0x1e, 0xf2, 0xe2, 0xb3,
     },
     "TFT"},
    // CCRPYMVKZLWGZHEDZ23FOE22E3T3HOCNP5Y2EFZFVRUVIXU5NJ7UNGV2
    {{
         0xa2, 0xfc, 0x32, 0xaa, 0xca, 0xec, 0x6c, 0x9c, 0x83, 0xce, 0xb6,
         0x57, 0x13, 0x5a, 0x26, 0xe7, 0xb3, 0xb8, 0x4d, 0x7f, 0x71, 0xa2,
         0x17, 0x25, 0xac, 0x69, 0x54, 0x5e, 0x9d, 0x6a, 0x7f, 0x46,
     },
     "ARST"},
    // CAAV3AE3VKD2P4TY7LWTQMMJHIJ4WOCZ5ANCIJPC3NRSERKVXNHBU2W7
    {{
         0x01, 0x5d, 0x80, 0x9b, 0xaa, 0x87, 0xa7, 0xf2, 0x78, 0xfa, 0xed,
         0x38, 0x31, 0x89, 0x3a, 0x13, 0xcb, 0x38, 0x59, 0xe8, 0x1a, 0x24,
         0x25, 0xe2, 0xdb, 0x63, 0x22, 0x45, 0x55, 0xbb, 0x4e, 0x1a,
     },
     "XRP"},
    // CDYEOOVL6WV4JRY45CXQKOBJFFAPOM5KNQCCDNM333L6RM2L4RO3LKYG
    {{
         0xf0, 0x47, 0x3a, 0xab, 0xf5, 0xab, 0xc4, 0xc7, 0x1c, 0xe8, 0xaf,
         0x05, 0x38, 0x29, 0x29, 0x40, 0xf7, 0x33, 0xaa, 0x6c, 0x04, 0x21,
         0xb5, 0x9b, 0xde, 0xd7, 0xe8, 0xb3, 0x4b, 0xe4, 0x5d, 0xb5,
     },
     "yETH"},
    // CDCSVQEMYBNK7URICI77CENLP23OIM4AAXUIJ32Z6V3ZT3QXCP2HJXD7
    {{
         0xc5, 0x2a, 0xc0, 0x8c, 0xc0, 0x5a, 0xaf, 0xd2, 0x28, 0x12, 0x3f,
         0xf1, 0x11, 0xab, 0x7e, 0xb6, 0xe4, 0x33, 0x80, 0x05, 0xe8, 0x84,
         0xef, 0x59, 0xf5, 0x77, 0x99, 0xee, 0x17, 0x13, 0xf4, 0x74,
     },
     "NGNT"},
    // CB2XLDU74PIXO5DENULX53IIC3DMKGN2UM5IBGMSSI634IAQJ7O3Z3UQ
    {{
         0x75, 0x75, 0x8e, 0x9f, 0xe3, 0xd1, 0x77, 0x74, 0x64, 0x6d, 0x17,
         0x7e, 0xed, 0x08, 0x16, 0xc6, 0xc5, 0x19, 0xba, 0xa3, 0x3a, 0x80,
         0x99, 0x92, 0x92, 0x3d, 0xbe, 0x20, 0x10, 0x4f, 0xdd, 0xbc,
     },
     "RIO"},
    // CBZVSNVB55ANF24QVJL2K5QCLOAB6XITGTGXYEAF6NPTXYKEJUYQOHFC
    {{
         0x73, 0x59, 0x36, 0xa1, 0xef, 0x40, 0xd2, 0xeb, 0x90, 0xaa, 0x57,
         0xa5, 0x76, 0x02, 0x5b, 0x80, 0x1f, 0x5d, 0x13, 0x34, 0xcd, 0x7c,
         0x10, 0x05, 0xf3, 0x5f, 0x3b, 0xe1, 0x44, 0x4d, 0x31, 0x07,
     },
     "yXLM"},
    // CDZNTXPXR2I7VFDBYBADK2DE2SSGQ3DTXT5MJDCTYUMNVFK3CWG5NVC7
    {{
         0xf2, 0xd9, 0xdd, 0xf7, 0x8e, 0x91, 0xfa, 0x94, 0x61, 0xc0, 0x40,
         0x35, 0x68, 0x64, 0xd4, 0xa4, 0x68, 0x6c, 0x73, 0xbc, 0xfa, 0xc4,
         0x8c, 0x53, 0xc5, 0x18, 0xda, 0x95, 0x5b, 0x15, 0x8d, 0xd6,
     },
     "ETH"},
    // CCJVS6IVXAAXWCMFVK6QLWHZHR4RTVRSEZRQ53GOAEDN3VY2BLPVY72J
    {{
         0x93, 0x59, 0x79, 0x15, 0xb8, 0x01, 0x7b, 0x09, 0x85, 0xaa, 0xbd,
         0x05, 0xd8, 0xf9, 0x3c, 0x79, 0x19, 0xd6, 0x32, 0x26, 0x63, 0x0e,
         0xec, 0xce, 0x01, 0x06, 0xdd, 0xd7, 0x1a, 0x0a, 0xdf, 0x5c,
     },
     "SCOP"},
    // CBXE6V454EUYWVQCI4TCSOG4CSNPQ2BLYOTKAKXYFHO3KNVX4CXYCY2T
    {{
         0x6e, 0x4f, 0x57, 0x9d, 0xe1, 0x29, 0x8b, 0x56, 0x02, 0x47, 0x26,
         0x29, 0x38, 0xdc, 0x14, 0x9a, 0xf8, 0x68, 0x2b, 0xc3, 0xa6, 0xa0,
         0x2a, 0xf8, 0x29, 0xdd, 0xb5, 0x36, 0xb7, 0xe0, 0xaf, 0x81,
     },
     "LSP"},
    // CBDRPADR3KIBJNUBNRTTO4P7NO5RVPMYKRJB5YCZUZ6B66RKYK324UJY
    {{
         0x47, 0x17, 0x80, 0x71, 0xda, 0x90, 0x14, 0xb6, 0x81, 0x6c, 0x67,
         0x37, 0x71, 0xff, 0x6b, 0xbb, 0x1a, 0xbd, 0x98, 0x54, 0x52, 0x1e,
         0xe0, 0x59, 0xa6, 0x7c, 0x1f, 0x7a, 0x2a, 0xc2, 0xb7, 0xae,
     },
     "CLPX"},
    // CCG27OZ5AV4WUXS6XTECWAXEY5UOMEFI2CWFA3LHZGBTLYZWTJF3MJYQ
    {{
         0x8d, 0xaf, 0xbb, 0x3d, 0x05, 0x79, 0x6a, 0x5e, 0x5e, 0xbc, 0xc8,
         0x2b, 0x02, 0xe4, 0xc7, 0x68, 0xe6, 0x10, 0xa8, 0xd0, 0xac, 0x50,
         0x6d, 0x67, 0xc9, 0x83, 0x35, 0xe3, 0x36, 0x9a, 0x4b, 0xb6,
     },
     "AFR"},
    // CAO7DDJNGMOYQPRYDY5JVZ5YEK4UQBSMGLAEWRCUOTRMDSBMGWSAATDZ
    {{
         0x1d, 0xf1, 0x8d, 0x2d, 0x33, 0x1d, 0x88, 0x3e, 0x38, 0x1e, 0x3a,
         0x9a, 0xe7, 0xb8, 0x22, 0xb9, 0x48, 0x06, 0x4c, 0x32, 0xc0, 0x4b,
         0x44, 0x54, 0x74, 0xe2, 0xc1, 0xc8, 0x2c, 0x35, 0xa4, 0x00,
     },
     "BTC"},
    // CB2XMFB6BDIHFOSFB5IXHDOYV3SI3IXMNIZLPDZHC7ENDCXSBEBZAO2Y
    {{
         0x75, 0x76, 0x14, 0x3e, 0x08, 0xd0, 0x72, 0xba, 0x45, 0x0f, 0x51,
         0x73, 0x8d, 0xd8, 0xae, 0xe4, 0x8d, 0xa2, 0xec, 0x6a, 0x32, 0xb7,
         0x8f, 0x27, 0x17, 0xc8, 0xd1, 0x8a, 0xf2, 0x09, 0x03, 0x90,
     },
     "yBTC"},
    // CDGLDM5N34GBRCALDBKVVV4ACVS2TWZLUSMILM32FPIMPHLUUIIRTCNF
    {{
         0xcc, 0xb1, 0xb3, 0xad, 0xdf, 0x0c, 0x18, 0x88, 0x0b, 0x18, 0x55,
         0x5a, 0xd7, 0x80, 0x15, 0x65, 0xa9, 0xdb, 0x2b, 0xa4, 0x98, 0x85,
         0xb3, 0x7a, 0x2b, 0xd0, 0xc7, 0x9d, 0x74, 0xa2, 0x11, 0x19,
     },
     "FRED"},
    // CCKCKCPHYVXQD4NECBFJTFSCU2AMSJGCNG4O6K4JVRE2BLPR7WNDBQIQ
    {{
         0x94, 0x25, 0x09, 0xe7, 0xc5, 0x6f, 0x01, 0xf1, 0xa4, 0x10, 0x4a,
         0x99, 0x96, 0x42, 0xa6, 0x80, 0xc9, 0x24, 0xc2, 0x69, 0xb8, 0xef,
         0x2b, 0x89, 0xac, 0x49, 0xa0, 0xad, 0xf1, 0xfd, 0x9a, 0x30,
     },
     "SHX"},
#endif
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