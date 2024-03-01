#pragma once

/**
 * Status word for fail of transaction formatting.
 */
#define SW_TX_FORMATTING_FAIL 0x6125

/**
 * Status word for denied by user.
 */
#define SW_DENY 0x6985

/**
 * Status word for either wrong Lc or minimum APDU lenght is incorrect.
 */
#define SW_WRONG_DATA_LENGTH 0x6A87

/**
 * Status word for incorrect P1 or P2.
 */
#define SW_WRONG_P1P2 0x6B00

/**
 * Unknown stellar operation
 */
#define SW_UNKNOWN_OP 0x6C24

/**
 * Unknown stellar operation
 */
#define SW_UNKNOWN_ENVELOPE_TYPE 0x6C25

/**
 * Status word for hash signing model not enabled.
 */
#define SW_TX_HASH_SIGNING_MODE_NOT_ENABLED 0x6C66

/**
 * Status word for custom contract model not enabled.
 */
#define SW_CUSTOM_CONTRACT_MODE_NOT_ENABLED 0x6C67

/**
 * Status word for unknown command with this INS.
 */
#define SW_INS_NOT_SUPPORTED 0x6D00

/**
 * Status word for instruction class is different than CLA.
 */
#define SW_CLA_NOT_SUPPORTED 0x6E00

/**
 * Status word for wrong response length (buffer too small or too big).
 */
#define SW_WRONG_RESPONSE_LENGTH 0xB000

/**
 * Status word for fail to display address.
 */
#define SW_DISPLAY_ADDRESS_FAIL 0xB002

/**
 * Status word for fail to display transaction hash.
 */
#define SW_DISPLAY_TRANSACTION_HASH_FAIL 0xB003

/**
 * Status word for wrong transaction length.
 */
#define SW_WRONG_TX_LENGTH 0xB004

/**
 * Status word for fail of transaction parsing.
 */
#define SW_TX_PARSING_FAIL 0xB005

/**
 * Status word for fail of transaction hash.
 */
#define SW_TX_HASH_FAIL 0xB006

/**
 * Status word for bad state.
 */
#define SW_BAD_STATE 0xB007

/**
 * Status word for signature fail.
 */
#define SW_SIGNATURE_FAIL 0xB008

/**
 * Status word for fail to check swap params
 */
#define SW_SWAP_CHECKING_FAIL 0xB009

/**
 * Status word for success.
 */
#define SW_OK 0x9000

/**
 * Status word for internal error. (eg. crypto error)
 */
#define SW_INTERNAL_ERROR 0x7000