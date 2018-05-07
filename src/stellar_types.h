/*******************************************************************************
 *   Ledger Stellar App
 *   (c) 2017-2018 Ledger
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
 ********************************************************************************/
#ifndef _STELLAR_TYPES_H_
#define _STELLAR_TYPES_H_

#include "os.h"
#include <stdint.h>
#include <stdbool.h>


#define CLA 0xe0
#define INS_GET_PUBLIC_KEY 0x02
#define INS_SIGN_TX 0x04
#define INS_GET_APP_CONFIGURATION 0x06
#define INS_SIGN_TX_HASH 0x08
#define INS_KEEP_ALIVE 0x10
#define P1_NO_SIGNATURE 0x00
#define P1_SIGNATURE 0x01
#define P2_NO_CONFIRM 0x00
#define P2_CONFIRM 0x01
#define P1_FIRST 0x00
#define P1_MORE 0x80
#define P2_LAST 0x00
#define P2_MORE 0x80

#define OFFSET_CLA 0
#define OFFSET_INS 1
#define OFFSET_P1 2
#define OFFSET_P2 3
#define OFFSET_LC 4
#define OFFSET_CDATA 5

#define MAX_RAW_TX 1024
#define MAX_OPS 20
#define MAX_BIP32_LEN 10

#define ASSET_TYPE_NATIVE 0
#define ASSET_TYPE_CREDIT_ALPHANUM4 1
#define ASSET_TYPE_CREDIT_ALPHANUM12 2

#define MEMO_TYPE_NONE 0
#define MEMO_TYPE_TEXT 1
#define MEMO_TYPE_ID 2
#define MEMO_TYPE_HASH 3
#define MEMO_TYPE_RETURN 4

#define XDR_OPERATION_TYPE_CREATE_ACCOUNT 0
#define XDR_OPERATION_TYPE_PAYMENT 1
#define XDR_OPERATION_TYPE_PATH_PAYMENT 2
#define XDR_OPERATION_TYPE_MANAGE_OFFER 3
#define XDR_OPERATION_TYPE_CREATE_PASSIVE_OFFER 4
#define XDR_OPERATION_TYPE_SET_OPTIONS 5
#define XDR_OPERATION_TYPE_CHANGE_TRUST 6
#define XDR_OPERATION_TYPE_ALLOW_TRUST 7
#define XDR_OPERATION_TYPE_ACCOUNT_MERGE 8
#define XDR_OPERATION_TYPE_INFLATION 9
#define XDR_OPERATION_TYPE_MANAGE_DATA 10

#define OPERATION_TYPE_CREATE_ACCOUNT 0
#define OPERATION_TYPE_PAYMENT 1
#define OPERATION_TYPE_PATH_PAYMENT 2
#define OPERATION_TYPE_CREATE_OFFER 3
#define OPERATION_TYPE_REMOVE_OFFER 4
#define OPERATION_TYPE_CHANGE_OFFER 5
#define OPERATION_TYPE_SET_OPTIONS 6
#define OPERATION_TYPE_CHANGE_TRUST 7
#define OPERATION_TYPE_REMOVE_TRUST 8
#define OPERATION_TYPE_ALLOW_TRUST 9
#define OPERATION_TYPE_REVOKE_TRUST 10
#define OPERATION_TYPE_ACCOUNT_MERGE 11
#define OPERATION_TYPE_INFLATION 12
#define OPERATION_TYPE_SET_DATA 13
#define OPERATION_TYPE_REMOVE_DATA 14
#define OPERATION_TYPE_UNKNOWN 15

#ifdef TEST
#include <stdio.h>
#define THROW(code) { printf("error: %d", code); return; }
#define PRINTF(msg, arg) printf(msg, arg)
#define PIC(code) code
#define TARGET_NANOS 1
#endif // TEST


typedef struct {
    uint8_t opType;
    uint8_t opCount;
    uint8_t opIdx;
    char opSource[9];
    char txDetails[4][29];
    char opDetails[5][50];
} tx_content_t;

typedef struct {
    cx_ecfp_public_key_t publicKey;
    char address[28];
    uint8_t signature[64];
    bool returnSignature;

} pk_context_t;

typedef struct {
    uint8_t bip32Len;
    uint32_t bip32[MAX_BIP32_LEN];
    uint8_t raw[MAX_RAW_TX];
    uint32_t rawLength;
    uint8_t hash[32];
    tx_content_t content;
    uint16_t offset;
} tx_context_t;

typedef struct {
    union {
        pk_context_t pk;
        tx_context_t tx;
    } req;
    uint16_t u2fTimer;
    uint8_t multiOpsSupport;
} stellar_context_t;

typedef struct {
    uint8_t fidoTransport;
    uint8_t initialized;
} internal_storage_t;

#endif