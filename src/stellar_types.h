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

#include <stdint.h>
#include <stdbool.h>

// ------------------------------------------------------------------------- //
//                       REQUEST PARSING CONSTANTS                           //
// ------------------------------------------------------------------------- //

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
#define AMOUNT_MAX_SIZE 22


// ------------------------------------------------------------------------- //
//                       TRANSACTION PARSING CONSTANTS                       //
// ------------------------------------------------------------------------- //

#define ASSET_TYPE_NATIVE 0
#define ASSET_TYPE_CREDIT_ALPHANUM4 1
#define ASSET_TYPE_CREDIT_ALPHANUM12 2

#define MEMO_TYPE_NONE 0
#define MEMO_TYPE_TEXT 1
#define MEMO_TYPE_ID 2
#define MEMO_TYPE_HASH 3
#define MEMO_TYPE_RETURN 4

#define NETWORK_TYPE_PUBLIC 0
#define NETWORK_TYPE_TEST 1
#define NETWORK_TYPE_UNKNOWN 2

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
#define XDR_OPERATION_TYPE_BUMP_SEQUENCE 11

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
#define OPERATION_TYPE_BUMP_SEQUENCE 15
#define OPERATION_TYPE_UNKNOWN 16

static const uint8_t NETWORK_ID_PUBLIC_HASH[64] = {0x7a, 0xc3, 0x39, 0x97, 0x54, 0x4e, 0x31, 0x75,
                                                   0xd2, 0x66, 0xbd, 0x02, 0x24, 0x39, 0xb2, 0x2c,
                                                   0xdb, 0x16, 0x50, 0x8c, 0x01, 0x16, 0x3f, 0x26,
                                                   0xe5, 0xcb, 0x2a, 0x3e, 0x10, 0x45, 0xa9, 0x79};

static const uint8_t NETWORK_ID_TEST_HASH[64] = {0xce, 0xe0, 0x30, 0x2d, 0x59, 0x84, 0x4d, 0x32,
                                                 0xbd, 0xca, 0x91, 0x5c, 0x82, 0x03, 0xdd, 0x44,
                                                 0xb3, 0x3f, 0xbb, 0x7e, 0xdc, 0x19, 0x05, 0x1e,
                                                 0xa3, 0x7a, 0xbe, 0xdf, 0x28, 0xec, 0xd4, 0x72};


static const char * NETWORK_NAMES[3] = { "Public", "Test", "Unknown" };


// ------------------------------------------------------------------------- //
//                              UTILITIES                                    //
// ------------------------------------------------------------------------- //

#ifdef TEST
#include <stdio.h>
#define THROW(code) { printf("error: %d", code); return; }
#define PRINTF(msg, arg) printf(msg, arg)
#define PIC(code) code
#define TARGET_NANOS 1
#define MEMCLEAR(dest) { memset(dest, 0, sizeof(dest)); }
#else
#define MEMCLEAR(dest) { os_memset(dest, 0, sizeof(dest)); }
#include "bolos_target.h"
#endif // TEST

// ------------------------------------------------------------------------- //
//                           DISPLAY CONSTANTS                               //
// ------------------------------------------------------------------------- //

#ifdef TARGET_BLUE
#define COLOR_BG_1 0xF9F9F9
#define COLOR_APP 0x07a2cc
#define COLOR_APP_LIGHT 0xd4eef7
#define BAGL_FONT_OPEN_SANS_LIGHT_16_22PX_AVG_WIDTH 10
#define BAGL_FONT_OPEN_SANS_REGULAR_10_13PX_AVG_WIDTH 8
#define MAX_CHAR_PER_LINE 28
#endif

// ------------------------------------------------------------------------- //
//                           TYPE DEFINITIONS                                //
// ------------------------------------------------------------------------- //


typedef struct {
    uint8_t network;
    uint8_t opType;
    uint8_t opCount;
    uint8_t opIdx;
    bool timeBounds;
#ifdef TARGET_NANOS
    char opSource[15];
    char txDetails[4][29];
    char opDetails[5][50];
#else
    char opSource[57];
    char txDetails[4][57];
    char opDetails[5][57];
#endif
} tx_content_t;

typedef struct {
    uint8_t publicKey[32];
#ifdef TARGET_NANOS
    char address[28];
#else
    char address[57];
#endif
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
#ifdef TARGET_NANOS
    uint8_t uxStep;
    uint8_t uxStepCount;
#endif
} stellar_context_t;

typedef struct {
    uint8_t fidoTransport;
    uint8_t initialized;
} stellar_nv_state_t;

#endif