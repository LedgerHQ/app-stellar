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

#define CLA                       0xe0
#define INS_GET_PUBLIC_KEY        0x02
#define INS_SIGN_TX               0x04
#define INS_GET_APP_CONFIGURATION 0x06
#define INS_SIGN_TX_HASH          0x08
#define INS_KEEP_ALIVE            0x10
#define P1_NO_SIGNATURE           0x00
#define P1_SIGNATURE              0x01
#define P2_NO_CONFIRM             0x00
#define P2_CONFIRM                0x01
#define P1_FIRST                  0x00
#define P1_MORE                   0x80
#define P2_LAST                   0x00
#define P2_MORE                   0x80

#define MIN_APDU_SIZE 5

#define OFFSET_CLA   0
#define OFFSET_INS   1
#define OFFSET_P1    2
#define OFFSET_P2    3
#define OFFSET_LC    4
#define OFFSET_CDATA 5

/* Max transaction size */
#define MAX_RAW_TX 1120
/* For sure not more than 35 operations will fit in that */
#define MAX_OPS 35
/* Although SEP-0005 only allows 3 bip32 path elements we support more */
#define MAX_BIP32_LEN 10

/* max amount is max int64 scaled down: "922337203685.4775807" */
#define AMOUNT_MAX_SIZE 21

#define HASH_SIZE                 32
#define LIQUIDITY_POOL_ID_SIZE    32
#define ED25519_PUBLIC_KEY_LEN    32
#define ED25519_PUBLIC_STRKEY_LEN 56
#define MUXED_ACCOUNT_STRKEY_LEN  69

// ------------------------------------------------------------------------- //
//                       TRANSACTION PARSING CONSTANTS                       //
// ------------------------------------------------------------------------- //

typedef enum {
    ASSET_TYPE_NATIVE = 0,
    ASSET_TYPE_CREDIT_ALPHANUM4 = 1,
    ASSET_TYPE_CREDIT_ALPHANUM12 = 2,
    ASSET_TYPE_POOL_SHARE = 3,
} AssetType;

typedef enum {
    MEMO_NONE = 0,
    MEMO_TEXT = 1,
    MEMO_ID = 2,
    MEMO_HASH = 3,
    MEMO_RETURN = 4,
} MemoType;

typedef enum {
    ENVELOPE_TYPE_TX = 2,
    ENVELOPE_TYPE_TX_FEE_BUMP = 5,
} EnvelopeType;

#define NETWORK_TYPE_PUBLIC  0
#define NETWORK_TYPE_TEST    1
#define NETWORK_TYPE_UNKNOWN 2

typedef enum {
    XDR_OPERATION_TYPE_CREATE_ACCOUNT = 0,
    XDR_OPERATION_TYPE_PAYMENT = 1,
    XDR_OPERATION_TYPE_PATH_PAYMENT_STRICT_RECEIVE = 2,
    XDR_OPERATION_TYPE_MANAGE_SELL_OFFER = 3,
    XDR_OPERATION_TYPE_CREATE_PASSIVE_SELL_OFFER = 4,
    XDR_OPERATION_TYPE_SET_OPTIONS = 5,
    XDR_OPERATION_TYPE_CHANGE_TRUST = 6,
    XDR_OPERATION_TYPE_ALLOW_TRUST = 7,
    XDR_OPERATION_TYPE_ACCOUNT_MERGE = 8,
    XDR_OPERATION_TYPE_INFLATION = 9,
    XDR_OPERATION_TYPE_MANAGE_DATA = 10,
    XDR_OPERATION_TYPE_BUMP_SEQUENCE = 11,
    XDR_OPERATION_TYPE_MANAGE_BUY_OFFER = 12,
    XDR_OPERATION_TYPE_PATH_PAYMENT_STRICT_SEND = 13,
    XDR_OPERATION_TYPE_CREATE_CLAIMABLE_BALANCE = 14,
    XDR_OPERATION_TYPE_CLAIM_CLAIMABLE_BALANCE = 15,
    XDR_OPERATION_TYPE_BEGIN_SPONSORING_FUTURE_RESERVES = 16,
    XDR_OPERATION_TYPE_END_SPONSORING_FUTURE_RESERVES = 17,
    XDR_OPERATION_TYPE_REVOKE_SPONSORSHIP = 18,
    XDR_OPERATION_TYPE_CLAWBACK = 19,
    XDR_OPERATION_TYPE_CLAWBACK_CLAIMABLE_BALANCE = 20,
    XDR_OPERATION_TYPE_SET_TRUST_LINE_FLAGS = 21,
    XDR_OPERATION_TYPE_LIQUIDITY_POOL_DEPOSIT = 22,
    XDR_OPERATION_TYPE_LIQUIDITY_POOL_WITHDRAW = 23,
} xdr_operation_type_e;

#define PUBLIC_KEY_TYPE_ED25519 0
#define MEMO_TEXT_MAX_SIZE      28
#define DATA_NAME_MAX_SIZE      64
#define DATA_VALUE_MAX_SIZE     64
#define HOME_DOMAIN_MAX_SIZE    32

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
 * DETAIL_VALUE_MAX_SIZE value of 89 is due to the maximum length of managed data value which can be
 * 64 bytes long. Managed data values are displayed as base64 encoded strings, which are
 * 4*((len+2)/3) characters long. (An additional slot is required for the end-of-string character of
 * course)
 */
#define DETAIL_VALUE_MAX_SIZE 89

static const char *NETWORK_NAMES[3] = {"Public", "Test", "Unknown"};

// ------------------------------------------------------------------------- //
//                              UTILITIES                                    //
// ------------------------------------------------------------------------- //

#ifdef TEST
#include <stdio.h>
#include <string.h>

#define THROW(code)                \
    do {                           \
        printf("error: %d", code); \
    } while (0)

#ifdef FUZZ
#define PRINTF(...)
#else
#define PRINTF(strbuf, ...) fprintf(stderr, strbuf, __VA_ARGS__)
#endif  // FUZZ
#define PIC(code) code

#define MEMCLEAR(dest) memset(&dest, 0, sizeof(dest));
#else
#define MEMCLEAR(dest)                       \
    do {                                     \
        explicit_bzero(&dest, sizeof(dest)); \
    } while (0)
#include "bolos_target.h"
#endif  // TEST

// ------------------------------------------------------------------------- //
//                           TYPE DEFINITIONS                                //
// ------------------------------------------------------------------------- //

typedef const uint8_t *AccountID;
typedef int64_t SequenceNumber;
typedef uint64_t TimePoint;

typedef enum {
    KEY_TYPE_ED25519 = 0,
    KEY_TYPE_PRE_AUTH_TX = 1,
    KEY_TYPE_HASH_X = 2,
    KEY_TYPE_MUXED_ED25519 = 0x100
} CryptoKeyType;

typedef enum {
    SIGNER_KEY_TYPE_ED25519 = KEY_TYPE_ED25519,
    SIGNER_KEY_TYPE_PRE_AUTH_TX = KEY_TYPE_PRE_AUTH_TX,
    SIGNER_KEY_TYPE_HASH_X = KEY_TYPE_HASH_X
} SignerKeyType;

typedef enum {
    // issuer has authorized account to perform transactions with its credit
    AUTHORIZED_FLAG = 1,
    // issuer has authorized account to maintain and reduce liabilities for its
    // credit
    AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG = 2,
    // issuer has specified that it may clawback its credit, and that claimable
    // balances created with its credit may also be clawed back
    TRUSTLINE_CLAWBACK_ENABLED_FLAG = 4
} TrustLineFlags;

typedef struct {
    uint64_t id;
    const uint8_t *ed25519;
} MuxedAccountMed25519;

typedef struct {
    CryptoKeyType type;
    union {
        const uint8_t *ed25519;
        MuxedAccountMed25519 med25519;
    };
} MuxedAccount;

typedef struct {
    const char *assetCode;
    AccountID issuer;
} AlphaNum4;

typedef struct {
    const char *assetCode;
    AccountID issuer;
} AlphaNum12;

typedef struct {
    AssetType type;
    union {
        AlphaNum4 alphaNum4;
        AlphaNum12 alphaNum12;
    };
} Asset;

typedef enum { LIQUIDITY_POOL_CONSTANT_PRODUCT = 0 } LiquidityPoolType;

typedef struct {
    Asset assetA;
    Asset assetB;
    int32_t fee;  // Fee is in basis points, so the actual rate is (fee/100)%
} LiquidityPoolConstantProductParameters;

typedef struct {
    LiquidityPoolType type;
    union {
        LiquidityPoolConstantProductParameters
            constantProduct;  // type == LIQUIDITY_POOL_CONSTANT_PRODUCT
    };
} LiquidityPoolParameters;

typedef struct {
    AssetType type;
    union {
        AlphaNum4 alphaNum4;
        AlphaNum12 alphaNum12;
        LiquidityPoolParameters liquidityPool;
    };
} ChangeTrustAsset;

typedef struct {
    AssetType type;
    union {
        AlphaNum4 alphaNum4;
        AlphaNum12 alphaNum12;
        uint8_t liquidityPoolID[LIQUIDITY_POOL_ID_SIZE];
    };
} TrustLineAsset;

typedef struct {
    int32_t n;  // numerator
    int32_t d;  // denominator
} Price;

typedef struct {
    AccountID destination;    // account to create
    int64_t startingBalance;  // amount they end up with
} CreateAccountOp;

typedef struct {
    MuxedAccount destination;  // recipient of the payment
    Asset asset;               // what they end up with
    int64_t amount;            // amount they end up with
} PaymentOp;

typedef struct {
    Asset sendAsset;  // asset we pay with
    int64_t sendMax;  // the maximum amount of sendAsset to send (excluding fees).
                      // The operation will fail if can't be met

    MuxedAccount destination;  // recipient of the payment
    Asset destAsset;           // what they end up with
    int64_t destAmount;        // amount they end up with

    uint8_t pathLen;
    Asset path[5];  // additional hops it must go through to get there
} PathPaymentStrictReceiveOp;

typedef struct {
    Asset selling;   // A
    Asset buying;    // B
    int64_t amount;  // amount taker gets
    Price price;     // cost of A in terms of B
} CreatePassiveSellOfferOp;

typedef struct {
    Asset selling;
    Asset buying;
    int64_t amount;  // amount being sold. if set to 0, delete the offer
    Price price;     // price of thing being sold in terms of what you are buying

    // 0=create a new offer, otherwise edit an existing offer
    int64_t offerID;
} ManageSellOfferOp;

typedef struct {
    Asset selling;
    Asset buying;
    int64_t buyAmount;  // amount being bought. if set to 0, delete the offer
    Price price;        // price of thing being bought in terms of what you are
                        // selling

    // 0=create a new offer, otherwise edit an existing offer
    int64_t offerID;
} ManageBuyOfferOp;

typedef struct {
    Asset sendAsset;     // asset we pay with
    int64_t sendAmount;  // amount of sendAsset to send (excluding fees)
                         // The operation will fail if can't be met

    MuxedAccount destination;  // recipient of the payment
    Asset destAsset;           // what they end up with
    int64_t destMin;           // the minimum amount of dest asset to
                               // be received
                               // The operation will fail if it can't be met
    uint8_t pathLen;
    Asset path[5];  // additional hops it must go through to get there
} PathPaymentStrictSendOp;

typedef struct {
    ChangeTrustAsset line;

    uint64_t limit;  // if limit is set to 0, deletes the trust line
} ChangeTrustOp;

typedef struct {
    AccountID trustor;
    char assetCode[13];
    // One of 0, AUTHORIZED_FLAG, or AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG.
    uint32_t authorize;
} AllowTrustOp;

typedef MuxedAccount AccountMergeOp;

typedef struct {
    SequenceNumber bumpTo;
} BumpSequenceOp;

typedef struct {
    SignerKeyType type;
    const uint8_t *data;
} SignerKey;

typedef struct {
    SignerKey key;
    uint32_t weight;  // really only need 1 byte
} signer_t;

typedef struct {
    bool inflationDestinationPresent;
    AccountID inflationDestination;
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
    bool homeDomainPresent;
    uint32_t homeDomainSize;
    const uint8_t *homeDomain;
    bool signerPresent;
    signer_t signer;
} SetOptionsOp;

typedef struct {
    uint8_t dataNameSize;
    const uint8_t *dataName;
    uint8_t dataValueSize;
    const uint8_t *dataValue;
} ManageDataOp;

typedef enum {
    CLAIM_PREDICATE_UNCONDITIONAL = 0,
    CLAIM_PREDICATE_AND = 1,
    CLAIM_PREDICATE_OR = 2,
    CLAIM_PREDICATE_NOT = 3,
    CLAIM_PREDICATE_BEFORE_ABSOLUTE_TIME = 4,
    CLAIM_PREDICATE_BEFORE_RELATIVE_TIME = 5
} ClaimPredicateType;

typedef enum {
    CLAIMANT_TYPE_V0 = 0,
} ClaimantType;

typedef struct {
    ClaimantType type;
    union {
        struct {
            AccountID destination;  // The account that can use this condition
        } v0;
    };

} Claimant;

typedef struct {
    Asset asset;
    int64_t amount;
    uint8_t claimantLen;
    Claimant claimants[10];
} CreateClaimableBalanceOp;

typedef enum {
    CLAIMABLE_BALANCE_ID_TYPE_V0 = 0,
} ClaimableBalanceIDType;

typedef struct {
    ClaimableBalanceIDType type;
    uint8_t v0[32];
} ClaimableBalanceID;

typedef struct {
    ClaimableBalanceID balanceID;
} ClaimClaimableBalanceOp;

typedef struct {
    AccountID sponsoredID;
} BeginSponsoringFutureReservesOp;

typedef enum {
    ACCOUNT = 0,
    TRUSTLINE = 1,
    OFFER = 2,
    DATA = 3,
    CLAIMABLE_BALANCE = 4,
    LIQUIDITY_POOL = 5
} LedgerEntryType;

typedef struct {
    LedgerEntryType type;
    union {
        struct {
            AccountID accountID;
        } account;  // type == ACCOUNT

        struct {
            AccountID accountID;
            TrustLineAsset asset;
        } trustLine;  // type == TRUSTLINE

        struct {
            AccountID sellerID;
            int64_t offerID;
        } offer;  // type == OFFER

        struct {
            AccountID accountID;
            uint8_t dataNameSize;
            const uint8_t *dataName;
        } data;  // type == DATA

        struct {
            ClaimableBalanceID balanceId;
        } claimableBalance;  // type == CLAIMABLE_BALANCE

        struct {
            uint8_t liquidityPoolID[LIQUIDITY_POOL_ID_SIZE];
        } liquidityPool;  // type == LIQUIDITY_POOL
    };

} LedgerKey;

typedef enum {
    REVOKE_SPONSORSHIP_LEDGER_ENTRY = 0,
    REVOKE_SPONSORSHIP_SIGNER = 1
} RevokeSponsorshipType;

typedef struct {
    RevokeSponsorshipType type;
    union {
        LedgerKey ledgerKey;
        struct {
            AccountID accountID;
            SignerKey signerKey;
        } signer;
    };

} RevokeSponsorshipOp;

typedef struct {
    Asset asset;
    MuxedAccount from;
    int64_t amount;
} ClawbackOp;

typedef struct {
    ClaimableBalanceID balanceID;
} ClawbackClaimableBalanceOp;

typedef struct {
    AccountID trustor;
    Asset asset;
    uint32_t clearFlags;  // which flags to clear
    uint32_t setFlags;    // which flags to set
} SetTrustLineFlagsOp;

typedef struct {
    uint8_t liquidityPoolID[LIQUIDITY_POOL_ID_SIZE];
    int64_t maxAmountA;  // maximum amount of first asset to deposit
    int64_t maxAmountB;  // maximum amount of second asset to deposit
    Price minPrice;      // minimum depositA/depositB
    Price maxPrice;      // maximum depositA/depositB
} LiquidityPoolDepositOp;

typedef struct {
    uint8_t liquidityPoolID[LIQUIDITY_POOL_ID_SIZE];
    int64_t amount;      // amount of pool shares to withdraw
    int64_t minAmountA;  // minimum amount of first asset to withdraw
    int64_t minAmountB;  // minimum amount of second asset to withdraw
} LiquidityPoolWithdrawOp;

typedef struct {
    bool sourceAccountPresent;
    MuxedAccount sourceAccount;
    uint8_t type;
    union {
        CreateAccountOp createAccount;
        PaymentOp payment;
        PathPaymentStrictReceiveOp pathPaymentStrictReceiveOp;
        ManageSellOfferOp manageSellOfferOp;
        CreatePassiveSellOfferOp createPassiveSellOfferOp;
        SetOptionsOp setOptionsOp;
        ChangeTrustOp changeTrustOp;
        AllowTrustOp allowTrustOp;
        MuxedAccount destination;
        ManageDataOp manageDataOp;
        BumpSequenceOp bumpSequenceOp;
        ManageBuyOfferOp manageBuyOfferOp;
        PathPaymentStrictSendOp pathPaymentStrictSendOp;
        CreateClaimableBalanceOp createClaimableBalanceOp;
        ClaimClaimableBalanceOp claimClaimableBalanceOp;
        BeginSponsoringFutureReservesOp beginSponsoringFutureReservesOp;
        RevokeSponsorshipOp revokeSponsorshipOp;
        ClawbackOp clawbackOp;
        ClawbackClaimableBalanceOp clawbackClaimableBalanceOp;
        SetTrustLineFlagsOp setTrustLineFlagsOp;
        LiquidityPoolDepositOp liquidityPoolDepositOp;
        LiquidityPoolWithdrawOp liquidityPoolWithdrawOp;
    };
} Operation;

typedef struct {
    MemoType type;
    union {
        uint64_t id;
        const char *text;
        const uint8_t *hash;
    };
} Memo;

typedef struct {
    TimePoint minTime;
    TimePoint maxTime;  // 0 here means no maxTime
} TimeBounds;

typedef struct {
    MuxedAccount sourceAccount;     // account used to run the transaction
    uint32_t fee;                   // the fee the sourceAccount will pay
    SequenceNumber sequenceNumber;  // sequence number to consume in the account
    bool hasTimeBounds;
    TimeBounds timeBounds;  // validity range (inclusive) for the last ledger close time
    Memo memo;
    Operation opDetails;
    uint8_t opCount;
    uint8_t opIdx;
} TransactionDetails;

typedef struct {
    MuxedAccount feeSource;
    int64_t fee;
} FeeBumpTransactionDetails;

typedef struct {
    uint8_t publicKey[ED25519_PUBLIC_KEY_LEN];
    uint8_t signature[64];
    bool returnSignature;
    uint32_t tx;
} pk_context_t;

typedef struct {
    uint8_t bip32Len;
    uint32_t bip32[MAX_BIP32_LEN];
    uint8_t publicKey[ED25519_PUBLIC_KEY_LEN];
    uint8_t raw[MAX_RAW_TX];
    uint32_t rawLength;
    uint8_t hash[HASH_SIZE];
    uint16_t offset;
    uint32_t tx;
    uint8_t network;
    EnvelopeType envelopeType;
    FeeBumpTransactionDetails feeBumpTxDetails;
    TransactionDetails txDetails;
} tx_context_t;

enum request_type_t { CONFIRM_ADDRESS, CONFIRM_TRANSACTION };

enum app_state_t { STATE_NONE, STATE_PARSE_TX, STATE_APPROVE_TX, STATE_APPROVE_TX_HASH };

typedef struct {
    enum app_state_t state;
    union {
        pk_context_t pk;
        tx_context_t tx;
    } req;
    enum request_type_t reqType;
    int16_t u2fTimer;
} stellar_context_t;

typedef struct {
    uint8_t initialized;
    uint8_t hashSigning;
} stellar_nv_state_t;

typedef struct {
    uint64_t amount;
    uint64_t fees;
    char destination[ED25519_PUBLIC_STRKEY_LEN + 1];  // ed25519 address only
    char memo[20];
} swap_values_t;

#endif
