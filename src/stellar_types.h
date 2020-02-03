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

#define MIN_APDU_SIZE 5

#define OFFSET_CLA 0
#define OFFSET_INS 1
#define OFFSET_P1 2
#define OFFSET_P2 3
#define OFFSET_LC 4
#define OFFSET_CDATA 5

/* Max transaction size is 1.5Kb */
#define MAX_RAW_TX 1540
/* For sure not more than 35 operations will fit in that */
#define MAX_OPS 35
/* Although SEP-0005 only allows 3 bip32 path elements we support more */
#define MAX_BIP32_LEN 10

/* max amount is max int64 scaled down: "922337203685.4775807" */
#define AMOUNT_MAX_SIZE 21


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
#define XDR_OPERATION_TYPE_MANAGE_SELL_OFFER 3
#define XDR_OPERATION_TYPE_CREATE_PASSIVE_OFFER 4
#define XDR_OPERATION_TYPE_SET_OPTIONS 5
#define XDR_OPERATION_TYPE_CHANGE_TRUST 6
#define XDR_OPERATION_TYPE_ALLOW_TRUST 7
#define XDR_OPERATION_TYPE_ACCOUNT_MERGE 8
#define XDR_OPERATION_TYPE_INFLATION 9
#define XDR_OPERATION_TYPE_MANAGE_DATA 10
#define XDR_OPERATION_TYPE_BUMP_SEQUENCE 11
#define XDR_OPERATION_TYPE_MANAGE_BUY_OFFER 12

#define SIGNER_KEY_TYPE_ED25519 0
#define SIGNER_KEY_TYPE_PRE_AUTH_TX 1
#define SIGNER_KEY_TYPE_HASH_X 2

#define PUBLIC_KEY_TYPE_ED25519 0
#define MEMO_TEXT_MAX_SIZE 28
#define DATA_NAME_MAX_SIZE 64
#define DATA_VALUE_MAX_SIZE 64
#define HOME_DOMAIN_MAX_SIZE 32

// ------------------------------------------------------------------------- //
//                             DISPLAY CONSTANTS                             //
// ------------------------------------------------------------------------- //

/*
 * Longest string will be "Operation ii of nn"
 */
#define OPERATION_CAPTION_MAX_SIZE 20

/*
 * Captions don't scroll so there is no use in having more capacity than can fit on screen at once.
 */
#define DETAIL_CAPTION_MAX_SIZE 20

/*
 * DETAIL_VALUE_MAX_SIZE value of 89 is due to the maximum length of managed data value which can be 64 bytes long.
 * Managed data values are displayed as base64 encoded strings, which are 4*((len+2)/3) characters long.
 * (An additional slot is required for the end-of-string character of course)
 */
#define DETAIL_VALUE_MAX_SIZE 89

static const char* NETWORK_NAMES[3] = { "Public", "Test", "Unknown" };

// ------------------------------------------------------------------------- //
//                              UTILITIES                                    //
// ------------------------------------------------------------------------- //

#ifdef TEST
#include <stdio.h>
#define THROW(code) do { printf("error: %d", code); } while (0)
#define PRINTF(msg, arg) printf(msg, arg)
#define PIC(code) code
//#define TARGET_NANOS 1
#define TARGET_BLUE 1
#define MEMCLEAR(dest) memset(&dest, 0, sizeof(dest));
#else
#define MEMCLEAR(dest) do { os_memset(&dest, 0, sizeof(dest)); } while (0)
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
    uint8_t type;
    char code[13];
    const uint8_t *issuer;
} asset_t;

typedef struct {
    uint32_t numerator;
    uint32_t denominator;
} price_t;

typedef struct {
    const uint8_t *accountId;
    uint64_t amount;

} create_account_op_t;

typedef struct {
    const uint8_t *destination;
    uint64_t amount;
    asset_t asset;
} payment_op_t;

typedef struct {
    const uint8_t *destination;
    asset_t sourceAsset;
    uint64_t sendMax;
    asset_t destAsset;
    uint64_t destAmount;
    uint8_t pathLen;
    asset_t path[5];
} path_payment_op_t;

typedef struct {
    asset_t buying;
    asset_t selling;
    uint64_t amount;
    price_t price;
    uint64_t offerId;
    bool active;
    bool buy;
} manage_offer_op_t;

typedef struct {
    asset_t asset;
    uint64_t limit;
} change_trust_op_t;

typedef struct {
    char assetCode[13];
    const uint8_t *trustee;
    bool authorize;
} allow_trust_op_t;

typedef struct {
    const uint8_t *destination;
} account_merge_op_t;

typedef struct {
    int64_t bumpTo;
} bump_sequence_op_t;

typedef struct {
    uint8_t type;
    const uint8_t *data;
    uint32_t weight;
} signer_t;

typedef struct {
    bool inflationDestinationPresent;
    const uint8_t *inflationDestination;
    uint32_t clearFlags;
    uint32_t setFlags;
    bool masterWeightPresent;
    uint32_t masterWeight;
    bool lowThresholdPresent;
    uint32_t lowThreshold;
    bool mediumThresholdPresent;
    uint32_t mediumThreshold;
    bool highThresholdPresent;
    uint32_t highThreshold;
    uint32_t homeDomainSize;
    const uint8_t *homeDomain;
    bool signerPresent;
    signer_t signer;
} set_options_op_t;

typedef struct {
    uint8_t dataNameSize;
    const uint8_t *dataName;
    uint8_t dataValueSize;
    const uint8_t *dataValue;
} manage_data_op_t;

typedef struct {
    uint8_t type;
    bool sourcePresent;
    const uint8_t *source;
    union {
        create_account_op_t createAccount;
        payment_op_t payment;
        path_payment_op_t pathPayment;
        manage_offer_op_t manageOffer;
        change_trust_op_t changeTrust;
        allow_trust_op_t allowTrust;
        account_merge_op_t accountMerge;
        set_options_op_t setOptions;
        manage_data_op_t manageData;
        bump_sequence_op_t bumpSequence;
    } op;
} operation_details_t;

typedef struct {
    uint8_t type;
    char data[65];
} memo_t;

typedef struct {
    uint64_t minTime;
    uint64_t maxTime;
} time_bounds_t;

typedef struct {
    memo_t memo;
    uint64_t fee;
    uint8_t network;
    bool hasTimeBounds;
    time_bounds_t timeBounds;
    const uint8_t *source;
    int64_t sequenceNumber;
} tx_details_t;

typedef struct {
    uint8_t publicKey[32];
    uint8_t signature[64];
    bool returnSignature;
} pk_context_t;

typedef struct {
    uint8_t bip32Len;
    uint32_t bip32[MAX_BIP32_LEN];
    uint8_t raw[MAX_RAW_TX];
    uint32_t rawLength;
    uint8_t hash[32];
    uint16_t offset;
    operation_details_t opDetails;
    tx_details_t txDetails;
    uint8_t opCount;
    uint8_t opIdx;
} tx_context_t;

enum request_type_t {
    CONFIRM_ADDRESS,
    CONFIRM_TRANSACTION
};

enum app_state_t {
  STATE_NONE,
  STATE_PARSE_TX,
};

typedef struct {
    enum app_state_t state;
    union {
        pk_context_t pk;
        tx_context_t tx;
    } req;
    enum request_type_t reqType;
    uint16_t u2fTimer;
    uint8_t hashSigning;
} stellar_context_t;

typedef struct {
    uint8_t initialized;
} stellar_nv_state_t;

#endif