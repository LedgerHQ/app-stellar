#pragma once

/**
 * Status word for fail of transaction formatting.
 */
#define SW_FORMATTING_FAIL 0x6125
/**
 * Status word for too many pages to display. (40 pages max on Stax)
 */
#define SW_TOO_MANY_PAGES 0x6126
/**
 * Status word for denied by user.
 */
#define SW_DENY 0x6985
/**
 * Status word for either wrong Lc or length of APDU command less than 5.
 */
#define SW_WRONG_DATA_LENGTH 0x6A87
/**
 * Status word for incorrect P1 or P2.
 */
#define SW_WRONG_P1P2 0x6B00
/**
 * Status word for unknown command with this INS.
 */
#define SW_INS_NOT_SUPPORTED 0x6D00
/**
 * Status word for instruction class is different than CLA.
 */
#define SW_CLA_NOT_SUPPORTED 0x6E00
/**
 * Status word for fail to display address.
 */
#define SW_DISPLAY_ADDRESS_FAIL 0xB002
/**
 * Status word for fail to display transaction hash.
 */
#define SW_DISPLAY_TRANSACTION_HASH_FAIL 0xB003
/**
 * Status word for the data is too large to be processed.
 */
#define SW_DATA_TOO_LARGE 0xB004
/**
 * Status word for fail of data parsing.
 */
#define SW_DATA_PARSING_FAIL 0xB005
/**
 * Status word for fail of data hash.
 */
#define SW_DATA_HASH_FAIL 0xB006
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