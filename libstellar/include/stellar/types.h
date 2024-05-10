#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// ------------------------------------------------------------------------- //
//                       TRANSACTION PARSING CONSTANTS                       //
// ------------------------------------------------------------------------- //

#define ENCODED_ED25519_PUBLIC_KEY_LENGTH  57
#define ENCODED_CONTRACT_KEY_LENGTH        57
#define ENCODED_ED25519_PRIVATE_KEY_LENGTH 57
#define ENCODED_HASH_X_KEY_LENGTH          57
#define ENCODED_PRE_AUTH_TX_KEY_LENGTH     57
#define ENCODED_MUXED_ACCOUNT_KEY_LENGTH   70

#define RAW_ED25519_PUBLIC_KEY_SIZE  32
#define RAW_ED25519_PRIVATE_KEY_SIZE 32
#define RAW_HASH_X_KEY_SIZE          32
#define RAW_PRE_AUTH_TX_KEY_SIZE     32
#define RAW_CONTRACT_KEY_SIZE        32
#define RAW_MUXED_ACCOUNT_KEY_SIZE   40

#define VERSION_BYTE_ED25519_PUBLIC_KEY     6 << 3
#define VERSION_BYTE_ED25519_SECRET_SEED    18 << 3
#define VERSION_BYTE_PRE_AUTH_TX_KEY        19 << 3
#define VERSION_BYTE_HASH_X                 23 << 3
#define VERSION_BYTE_MUXED_ACCOUNT          12 << 3
#define VERSION_BYTE_ED25519_SIGNED_PAYLOAD 15 << 3
#define VERSION_BYTE_CONTRACT               2 << 3

#define ASSET_CODE_MAX_LENGTH         13
#define CLAIMANTS_MAX_LENGTH          10
#define PATH_PAYMENT_MAX_PATH_LENGTH  5
#define HOST_FUNCTION_ARGS_MAX_LENGTH 10

/* For sure not more than 35 operations will fit in that */
#define MAX_OPS 35

/* max amount is max int64 scaled down: "922337203685.4775807" */
#define AMOUNT_MAX_LENGTH 21

#define MAX_SUB_INVOCATIONS_SIZE 16

#define HASH_SIZE                 32
#define LIQUIDITY_POOL_ID_SIZE    32
#define CLAIMABLE_BALANCE_ID_SIZE 32

#define PUBLIC_KEY_TYPE_ED25519 0
#define MEMO_TEXT_MAX_SIZE      28
#define DATA_NAME_MAX_SIZE      64
#define DATA_VALUE_MAX_SIZE     64
#define HOME_DOMAIN_MAX_SIZE    32
#define SCV_SYMBOL_MAX_SIZE     32

#define NETWORK_TYPE_PUBLIC  0
#define NETWORK_TYPE_TEST    1
#define NETWORK_TYPE_UNKNOWN 2

typedef enum {
    ASSET_TYPE_NATIVE = 0,
    ASSET_TYPE_CREDIT_ALPHANUM4 = 1,
    ASSET_TYPE_CREDIT_ALPHANUM12 = 2,
    ASSET_TYPE_POOL_SHARE = 3,
} asset_type_t;

typedef enum {
    MEMO_NONE = 0,
    MEMO_TEXT = 1,
    MEMO_ID = 2,
    MEMO_HASH = 3,
    MEMO_RETURN = 4,
} memo_type_t;

typedef enum {
    ENVELOPE_TYPE_TX = 2,
    ENVELOPE_TYPE_TX_FEE_BUMP = 5,
    ENVELOPE_TYPE_SOROBAN_AUTHORIZATION = 9
} envelope_type_t;

typedef enum {
    OPERATION_TYPE_CREATE_ACCOUNT = 0,
    OPERATION_TYPE_PAYMENT = 1,
    OPERATION_TYPE_PATH_PAYMENT_STRICT_RECEIVE = 2,
    OPERATION_TYPE_MANAGE_SELL_OFFER = 3,
    OPERATION_TYPE_CREATE_PASSIVE_SELL_OFFER = 4,
    OPERATION_TYPE_SET_OPTIONS = 5,
    OPERATION_TYPE_CHANGE_TRUST = 6,
    OPERATION_TYPE_ALLOW_TRUST = 7,
    OPERATION_TYPE_ACCOUNT_MERGE = 8,
    OPERATION_TYPE_INFLATION = 9,
    OPERATION_TYPE_MANAGE_DATA = 10,
    OPERATION_TYPE_BUMP_SEQUENCE = 11,
    OPERATION_TYPE_MANAGE_BUY_OFFER = 12,
    OPERATION_TYPE_PATH_PAYMENT_STRICT_SEND = 13,
    OPERATION_TYPE_CREATE_CLAIMABLE_BALANCE = 14,
    OPERATION_TYPE_CLAIM_CLAIMABLE_BALANCE = 15,
    OPERATION_TYPE_BEGIN_SPONSORING_FUTURE_RESERVES = 16,
    OPERATION_TYPE_END_SPONSORING_FUTURE_RESERVES = 17,
    OPERATION_TYPE_REVOKE_SPONSORSHIP = 18,
    OPERATION_TYPE_CLAWBACK = 19,
    OPERATION_TYPE_CLAWBACK_CLAIMABLE_BALANCE = 20,
    OPERATION_TYPE_SET_TRUST_LINE_FLAGS = 21,
    OPERATION_TYPE_LIQUIDITY_POOL_DEPOSIT = 22,
    OPERATION_TYPE_LIQUIDITY_POOL_WITHDRAW = 23,
    OPERATION_INVOKE_HOST_FUNCTION = 24,
    OPERATION_EXTEND_FOOTPRINT_TTL = 25,
    OPERATION_RESTORE_FOOTPRINT = 26
} operation_type_t;

typedef const uint8_t *account_id_t;
typedef int64_t sequence_number_t;
typedef uint64_t time_point_t;
typedef int64_t duration_t;

typedef enum {
    KEY_TYPE_ED25519 = 0,
    KEY_TYPE_PRE_AUTH_TX = 1,
    KEY_TYPE_HASH_X = 2,
    KEY_TYPE_ED25519_SIGNED_PAYLOAD = 3,
    KEY_TYPE_MUXED_ED25519 = 0x100
} crypto_key_type_t;

typedef enum {
    SIGNER_KEY_TYPE_ED25519 = KEY_TYPE_ED25519,
    SIGNER_KEY_TYPE_PRE_AUTH_TX = KEY_TYPE_PRE_AUTH_TX,
    SIGNER_KEY_TYPE_HASH_X = KEY_TYPE_HASH_X,
    SIGNER_KEY_TYPE_ED25519_SIGNED_PAYLOAD = KEY_TYPE_ED25519_SIGNED_PAYLOAD
} signer_key_type_t;

typedef enum {
    // issuer has authorized account to perform transactions with its credit
    AUTHORIZED_FLAG = 1,
    // issuer has authorized account to maintain and reduce liabilities for its
    // credit
    AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG = 2,
    // issuer has specified that it may clawback its credit, and that claimable
    // balances created with its credit may also be clawed back
    TRUSTLINE_CLAWBACK_ENABLED_FLAG = 4
} trust_line_flags_t;

typedef struct {
    uint64_t id;
    const uint8_t *ed25519;
} muxed_account_med25519_t;

typedef struct {
    uint32_t type;  // crypto_key_type_t
    union {
        const uint8_t *ed25519;
        muxed_account_med25519_t med25519;
    };
} muxed_account_t;

typedef struct {
    const uint8_t *asset_code;
    account_id_t issuer;
} alpha_num4_t;

typedef struct {
    const uint8_t *asset_code;
    account_id_t issuer;
} alpha_num12_t;

typedef struct {
    uint32_t type;  // asset_type_t
    union {
        alpha_num4_t alpha_num4;
        alpha_num12_t alpha_num12;
    };
} asset_t;

typedef enum { LIQUIDITY_POOL_CONSTANT_PRODUCT = 0 } liquidity_pool_type_t;

typedef struct {
    asset_t asset_a;
    asset_t asset_b;
    int32_t fee;  // Fee is in basis points, so the actual rate is (fee/100)%
} liquidity_pool_constant_product_parameters_t;

typedef struct {
    uint32_t type;  // liquidity_pool_type_t
    union {
        liquidity_pool_constant_product_parameters_t
            constant_product;  // type == LIQUIDITY_POOL_CONSTANT_PRODUCT
    };
} liquidity_pool_parameters_t;

typedef struct {
    uint32_t type;  // asset_type_t
    union {
        alpha_num4_t alpha_num4;
        alpha_num12_t alpha_num12;
        liquidity_pool_parameters_t liquidity_pool;
    };
} change_trust_asset_t;

typedef struct {
    uint32_t type;  // asset_type_t
    union {
        alpha_num4_t alpha_num4;
        alpha_num12_t alpha_num12;
        const uint8_t *liquidity_pool_id;
    };
} trust_line_asset_t;

typedef struct {
    int32_t n;  // numerator
    int32_t d;  // denominator
} price_t;

typedef struct {
    account_id_t destination;  // account to create
    int64_t starting_balance;  // amount they end up with
} create_account_op_t;

typedef struct {
    muxed_account_t destination;  // recipient of the payment_op
    asset_t asset;                // what they end up with
    int64_t amount;               // amount they end up with
} payment_op_t;

typedef struct {
    muxed_account_t destination;  // recipient of the payment_op
    int64_t send_max;             // the maximum amount of send_asset to send (excluding fees).
                                  // The operation will fail if can't be met
    int64_t dest_amount;          // amount they end up with
    asset_t send_asset;           // asset we pay with
    asset_t dest_asset;           // what they end up with
} path_payment_strict_receive_op_t;

typedef struct {
    asset_t selling;  // A
    asset_t buying;   // B
    int64_t amount;   // amount taker gets
    price_t price;    // cost of A in terms of B
} create_passive_sell_offer_op_t;

typedef struct {
    asset_t selling;
    asset_t buying;
    int64_t amount;  // amount being sold. if set to 0, delete the offer
    price_t price;   // price of thing being sold in terms of what you are buying

    // 0=create a new offer, otherwise edit an existing offer
    int64_t offer_id;
} manage_sell_offer_op_t;

typedef struct {
    asset_t selling;
    asset_t buying;
    int64_t buy_amount;  // amount being bought. if set to 0, delete the offer
    price_t price;       // price of thing being bought in terms of what you are
                         // selling

    // 0=create a new offer, otherwise edit an existing offer
    int64_t offer_id;
} manage_buy_offer_op_t;

typedef struct {
    muxed_account_t destination;  // recipient of the payment_op
    int64_t send_amount;          // amount of send_asset to send (excluding fees)
                                  // The operation will fail if can't be met
    int64_t dest_min;             // the minimum amount of dest asset to
                                  // be received
                                  // The operation will fail if it can't be met
    asset_t send_asset;           // asset we pay with
    asset_t dest_asset;           // what they end up with
} path_payment_strict_send_op_t;

typedef struct {
    change_trust_asset_t line;
    int64_t limit;  // if limit is set to 0, deletes the trust line
} change_trust_op_t;

typedef struct {
    account_id_t trustor;
    uint32_t asset_type;
    const uint8_t *asset_code;
    // One of 0, AUTHORIZED_FLAG, or AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG.
    uint32_t authorize;
} allow_trust_op_t;

typedef struct {
    muxed_account_t destination;
} account_merge_op_t;

typedef struct {
    sequence_number_t bump_to;
} bump_sequence_op_t;

typedef struct {
    const uint8_t *ed25519;
    const uint8_t *payload;
    uint32_t payload_len;
} ed25519_signed_payload_t;

typedef struct {
    uint32_t type;  // signer_key_type_t
    union {
        const uint8_t *ed25519;
        const uint8_t *pre_auth_tx;
        const uint8_t *hash_x;
        ed25519_signed_payload_t ed25519_signed_payload;
    };
} signer_key_t;

typedef struct {
    signer_key_t key;
    uint32_t weight;  // really only need 1 byte
} signer_t;

typedef struct {
    bool inflation_destination_present;
    account_id_t inflation_destination;
    bool clear_flags_present;
    uint32_t clear_flags;
    bool set_flags_present;
    uint32_t set_flags;
    bool master_weight_present;
    uint32_t master_weight;
    bool low_threshold_present;
    uint32_t low_threshold;
    bool medium_threshold_present;
    uint32_t medium_threshold;
    bool high_threshold_present;
    uint32_t high_threshold;
    bool home_domain_present;
    size_t home_domain_size;
    const uint8_t *home_domain;
    bool signer_present;
    signer_t signer;
} set_options_op_t;

typedef struct {
    size_t data_name_size;
    const uint8_t *data_name;
    size_t data_value_size;
    const uint8_t *data_value;
} manage_data_op_t;

typedef enum {
    CLAIM_PREDICATE_UNCONDITIONAL = 0,
    CLAIM_PREDICATE_AND = 1,
    CLAIM_PREDICATE_OR = 2,
    CLAIM_PREDICATE_NOT = 3,
    CLAIM_PREDICATE_BEFORE_ABSOLUTE_TIME = 4,
    CLAIM_PREDICATE_BEFORE_RELATIVE_TIME = 5
} claim_predicate_type_t;

typedef enum {
    CLAIMANT_TYPE_V0 = 0,
} claimant_type_t;

typedef struct {
    uint32_t type;  // claimant_type_t
    union {
        struct {
            account_id_t destination;  // The account that can use this condition
        } v0;
    };

} claimant_t;

typedef struct {
    asset_t asset;
    int64_t amount;
    uint8_t claimant_len;
    claimant_t claimants[CLAIMANTS_MAX_LENGTH];
} create_claimable_balance_op_t;

typedef enum {
    CLAIMABLE_BALANCE_ID_TYPE_V0 = 0,
} claimable_balance_id_type_t;

typedef struct {
    uint32_t type;  // claimable_balance_id_type_t
    const uint8_t *v0;
} claimable_balance_id_t;

typedef struct {
    claimable_balance_id_t balance_id;
} claim_claimable_balance_op_t;

typedef struct {
    account_id_t sponsored_id;
} begin_sponsoring_future_reserves_op_t;

typedef enum {
    ACCOUNT = 0,
    TRUSTLINE = 1,
    OFFER = 2,
    DATA = 3,
    CLAIMABLE_BALANCE = 4,
    LIQUIDITY_POOL = 5,
    CONTRACT_DATA = 6,
    CONTRACT_CODE = 7,
    CONFIG_SETTING = 8,
    TTL = 9
} ledger_entry_type_t;

typedef struct {
    uint32_t type;  // ledger_entry_type_t
    union {
        struct {
            account_id_t account_id;
        } account;  // type == ACCOUNT

        struct {
            account_id_t account_id;
            trust_line_asset_t asset;
        } trust_line;  // type == TRUSTLINE

        struct {
            account_id_t seller_id;
            int64_t offer_id;
        } offer;  // type == OFFER

        struct {
            account_id_t account_id;
            size_t data_name_size;
            const uint8_t *data_name;
        } data;  // type == DATA

        struct {
            claimable_balance_id_t balance_id;
        } claimable_balance;  // type == CLAIMABLE_BALANCE

        struct {
            const uint8_t *liquidity_pool_id;
        } liquidity_pool;  // type == LIQUIDITY_POOL
    };
    // TODO: other fields?

} ledger_key_t;

typedef enum {
    REVOKE_SPONSORSHIP_LEDGER_ENTRY = 0,
    REVOKE_SPONSORSHIP_SIGNER = 1
} revoke_sponsorship_type_t;

typedef struct {
    uint32_t type;  // revoke_sponsorship_type_t
    union {
        ledger_key_t ledger_key;
        struct {
            account_id_t account_id;
            signer_key_t signer_key;
        } signer;
    };

} revoke_sponsorship_op_t;

typedef struct {
    asset_t asset;
    muxed_account_t from;
    int64_t amount;
} clawback_op_t;

typedef struct {
    claimable_balance_id_t balance_id;
} clawback_claimable_balance_op_t;

typedef struct {
    account_id_t trustor;
    asset_t asset;
    uint32_t clear_flags;  // which flags to clear
    uint32_t set_flags;    // which flags to set
} set_trust_line_flags_op_t;

typedef struct {
    const uint8_t *liquidity_pool_id;
    int64_t max_amount_a;  // maximum amount of first asset to deposit
    int64_t max_amount_b;  // maximum amount of second asset to deposit
    price_t min_price;     // minimum depositA/depositB
    price_t max_price;     // maximum depositA/depositB
} liquidity_pool_deposit_op_t;

typedef struct {
    const uint8_t *liquidity_pool_id;
    int64_t amount;        // amount of pool shares to withdraw
    int64_t min_amount_a;  // minimum amount of first asset to withdraw
    int64_t min_amount_b;  // minimum amount of second asset to withdraw
} liquidity_pool_withdraw_op_t;

// ************************* Soroban ************************* //

#define CONTRACT_ID_PREIMAGE_FROM_ADDRESS 0
#define CONTRACT_ID_PREIMAGE_FROM_ASSET   1
#define CONTRACT_EXECUTABLE_WASM          0
#define CONTRACT_EXECUTABLE_STELLAR_ASSET 1

typedef enum SCValType {
    SCV_BOOL = 0,
    SCV_VOID = 1,
    SCV_ERROR = 2,
    SCV_U32 = 3,
    SCV_I32 = 4,
    SCV_U64 = 5,
    SCV_I64 = 6,
    SCV_TIMEPOINT = 7,
    SCV_DURATION = 8,
    SCV_U128 = 9,
    SCV_I128 = 10,
    SCV_U256 = 11,
    SCV_I256 = 12,
    SCV_BYTES = 13,
    SCV_STRING = 14,
    SCV_SYMBOL = 15,
    SCV_VEC = 16,
    SCV_MAP = 17,
    SCV_ADDRESS = 18,
    SCV_CONTRACT_INSTANCE = 19,
    SCV_LEDGER_KEY_CONTRACT_INSTANCE = 20,
    SCV_LEDGER_KEY_NONCE = 21
} sc_val_type_t;

typedef struct {
    size_t size;  // the max size of the symbol is SCV_SYMBOL_MAX_SIZE
    const uint8_t *symbol;
} scv_symbol_t;

typedef struct {
    size_t size;  // dont include the null terminator
    const uint8_t *string;
} scv_string_t;

typedef enum { SC_ADDRESS_TYPE_ACCOUNT = 0, SC_ADDRESS_TYPE_CONTRACT = 1 } sc_address_type_t;

typedef struct {
    uint32_t type;           // sc_address_type_t
    const uint8_t *address;  // account id or contract id, 32
} sc_address_t;

typedef enum {
    SOROBAN_CREDENTIALS_SOURCE_ACCOUNT = 0,
    SOROBAN_CREDENTIALS_ADDRESS = 1
} soroban_credentials_type_t;

typedef struct {
    sc_address_t address;
    struct {
        size_t name_size;
        const uint8_t *name;
    } function;
    size_t parameters_position;
    uint8_t parameters_length;
} invoke_contract_args_t;

typedef enum {
    HOST_FUNCTION_TYPE_INVOKE_CONTRACT = 0,
    HOST_FUNCTION_TYPE_CREATE_CONTRACT = 1,
    HOST_FUNCTION_TYPE_UPLOAD_CONTRACT_WASM = 2
} host_function_type_t;

typedef enum {
    SOROBAN_AUTHORIZED_FUNCTION_TYPE_CONTRACT_FN = 0,
    SOROBAN_AUTHORIZED_FUNCTION_TYPE_CREATE_CONTRACT_HOST_FN = 1
} soroban_authorization_function_type_t;

typedef struct {
    uint32_t host_function_type;  // host_function_type_t
    invoke_contract_args_t
        invoke_contract_args;  // exists if host_function_type == HOST_FUNCTION_TYPE_INVOKE_CONTRACT
    uint32_t auth_function_type;  // soroban_authorization_function_type_t
    size_t sub_invocation_positions[MAX_SUB_INVOCATIONS_SIZE];
    uint8_t sub_invocations_count;
    uint8_t sub_invocation_index;
} invoke_host_function_op_t;

typedef struct {
    uint32_t extend_to;
} extend_footprint_ttl_op_t;

typedef struct {
    void *placeholder;
} restore_footprint_op_t;

typedef struct {
    uint64_t nonce;
    uint32_t signature_expiration_ledger;
    uint32_t auth_function_type;  // soroban_authorization_function_type_t
    invoke_contract_args_t invoke_contract_args;
    size_t sub_invocation_positions[MAX_SUB_INVOCATIONS_SIZE];
    uint8_t sub_invocations_count;
    uint8_t sub_invocation_index;
} soroban_authorization_t;

// ************************* Soroban ************************* //
typedef struct {
    muxed_account_t source_account;
    uint32_t type;  // envelope_type_t
    bool source_account_present;
    union {
        create_account_op_t create_account_op;
        payment_op_t payment_op;
        path_payment_strict_receive_op_t path_payment_strict_receive_op;
        manage_sell_offer_op_t manage_sell_offer_op;
        create_passive_sell_offer_op_t create_passive_sell_offer_op;
        set_options_op_t set_options_op;
        change_trust_op_t change_trust_op;
        allow_trust_op_t allow_trust_op;
        account_merge_op_t account_merge_op;
        manage_data_op_t manage_data_op;
        bump_sequence_op_t bump_sequence_op;
        manage_buy_offer_op_t manage_buy_offer_op;
        path_payment_strict_send_op_t path_payment_strict_send_op;
        create_claimable_balance_op_t create_claimable_balance_op;
        claim_claimable_balance_op_t claim_claimable_balance_op;
        begin_sponsoring_future_reserves_op_t begin_sponsoring_future_reserves_op;
        revoke_sponsorship_op_t revoke_sponsorship_op;
        clawback_op_t clawback_op;
        clawback_claimable_balance_op_t clawback_claimable_balance_op;
        set_trust_line_flags_op_t set_trust_line_flags_op;
        liquidity_pool_deposit_op_t liquidity_pool_deposit_op;
        liquidity_pool_withdraw_op_t liquidity_pool_withdraw_op;
        invoke_host_function_op_t invoke_host_function_op;
        extend_footprint_ttl_op_t extend_footprint_ttl_op;
        restore_footprint_op_t restore_footprint_op;
    };
} operation_t;

typedef struct {
    uint32_t type;  // memo_type_t
    union {
        uint64_t id;
        struct {
            size_t text_size;
            const uint8_t *text;
        } text;
        const uint8_t *hash;
        const uint8_t *return_hash;
    };
} memo_t;

typedef struct {
    time_point_t min_time;
    time_point_t max_time;  // 0 here means no max_time
} time_bounds_t;

typedef struct {
    uint32_t min_ledger;
    uint32_t max_ledger;
} ledger_bounds_t;

typedef enum { PRECOND_NONE = 0, PRECOND_TIME = 1, PRECOND_V2 = 2 } precondition_type_t;

typedef struct {
    time_bounds_t time_bounds;
    ledger_bounds_t ledger_bounds;
    sequence_number_t min_seq_num;
    duration_t min_seq_age;
    uint32_t min_seq_ledger_gap;
    bool time_bounds_present;
    bool ledger_bounds_present;
    bool min_seq_num_present;
} preconditions_t;

typedef struct {
    muxed_account_t source_account;     // account used to run the transaction
    sequence_number_t sequence_number;  // sequence number to consume in the account
    preconditions_t cond;               // validity conditions
    memo_t memo;
    operation_t op_details;
    uint32_t fee;  // the fee the source_account will pay
    size_t operation_position;
    uint8_t operations_count;
    uint8_t operation_index;
} transaction_details_t;

typedef struct {
    muxed_account_t fee_source;
    int64_t fee;
} fee_bump_transaction_details_t;

typedef struct {
    transaction_details_t tx;
    fee_bump_transaction_details_t fee_bump_tx;
} tx_details_t;

typedef struct {
    union {
        tx_details_t tx_details;
        soroban_authorization_t soroban_authorization;
    };
    uint32_t type;  // envelope_type_t
    uint8_t network;
} envelope_t;
