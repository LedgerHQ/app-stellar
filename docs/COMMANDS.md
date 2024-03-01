# Stellar App Commands

## Overview

| Command name            | INS  | Description                                            |
| ----------------------- | ---- | ------------------------------------------------------ |
| `GET_PUBLIC_KEY`        | 0x02 | Get public key given BIP32 path                        |
| `SIGN_TX`               | 0x04 | Sign transaction given BIP32 path and raw transaction  |
| `GET_APP_CONFIGURATION` | 0x06 | Get application configuration information              |
| `SIGN_TX_HASH`          | 0x08 | Sign transaction given BIP32 path and transaction hash |

## GET_PUBLIC_KEY

### Command

| CLA  | INS  | P1   | P2                                    | Lc     | CData                                                                                        |
| ---- | ---- | ---- | ------------------------------------- | ------ | -------------------------------------------------------------------------------------------- |
| 0xE0 | 0x02 | 0x00 | 0x00 (no display) <br> 0x01 (display) | 1 + 4n | `len(bip32_path) (1)` \|\|<br> `bip32_path{1} (4)` \|\|<br>`...` \|\|<br>`bip32_path{n} (4)` |

### Response

| Response length (bytes) | SW     | RData                         |
| ----------------------- | ------ | ----------------------------- |
| 32                      | 0x9000 | `raw_ed25519_public_key (32)` |

## SIGN_TX

### Command

| CLA  | INS  | P1                                 | P2                           | Lc                                                                | CData                                                                                                                        |
| ---- | ---- | ---------------------------------- | ---------------------------- | ----------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------- |
| 0xE0 | 0x04 | 0x00 (first) <br> 0x80 (not_first) | 0x00 (last) <br> 0x80 (more) | 1 + 4n + k<br/>Only the first data chunk contains bip32 path data | `len(bip32_path) (1)` \|\|<br> `bip32_path{1} (4)` \|\|<br>`...` \|\|<br>`bip32_path{n} (4)` \|\|<br> `transaction_chunk(k)` |

### Response

| Response length (bytes) | SW     | RData            |
| ----------------------- | ------ | ---------------- |
| 64                      | 0x9000 | `signature (64)` |

## GET_APP_CONFIGURATION

### Command

| CLA  | INS  | P1   | P2   | Lc   | CData |
| ---- | ---- | ---- | ---- | ---- | ----- |
| 0xE0 | 0x06 | 0x00 | 0x00 | 0x00 | -     |

### Response

| Response length (bytes) | SW     | RData                                                                        |
| ----------------------- | ------ | ---------------------------------------------------------------------------- |
| 4                       | 0x9000 | `MAJOR (1)` \|\| `MINOR (1)` \|\| `PATCH (1)`\|\| `HASH_SIGNING_ENABLED (1)` |

## SIGN_TX_HASH

### Command

| CLA  | INS  | P1   | P2   | Lc          | CData                                                                                                                   |
| ---- | ---- | ---- | ---- | ----------- | ----------------------------------------------------------------------------------------------------------------------- |
| 0xE0 | 0x08 | 0x00 | 0x00 | 1 + 4n + 32 | `len(bip32_path) (1)` \|\|<br> `bip32_path{1} (4)` \|\|<br>`...` \|\|<br>`bip32_path{n} (4)`<br>`transaction_hash (32)` |

### Response

| Response length (bytes) | SW     | RData            |
| ----------------------- | ------ | ---------------- |
| 64                      | 0x9000 | `signature (64)` |

## INS_SIGN_SOROBAN_AUTHORATION

### Command

| CLA  | INS  | P1                                 | P2                           | Lc                                                                | CData                                                                                                                        |
| ---- | ---- | ---------------------------------- | ---------------------------- | ----------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------- |
| 0xE0 | 0x0A | 0x00 (first) <br> 0x80 (not_first) | 0x00 (last) <br> 0x80 (more) | 1 + 4n + k<br/>Only the first data chunk contains bip32 path data | `len(bip32_path) (1)` \|\|<br> `bip32_path{1} (4)` \|\|<br>`...` \|\|<br>`bip32_path{n} (4)` \|\|<br> `hash_id_preimage_chunk(k)` |

### Response

| Response length (bytes) | SW     | RData            |
| ----------------------- | ------ | ---------------- |
| 64                      | 0x9000 | `signature (64)` |

## Status Words

| SW     | SW name                               | Description                                             |
| ------ | ------------------------------------- | ------------------------------------------------------- |
| 0x6125 | `SW_TX_FORMATTING_FAIL`               | Failed to format transaction data                       |
| 0x6985 | `SW_DENY`                             | Rejected by user                                        |
| 0x6A87 | `SW_WRONG_DATA_LENGTH`                | `Lc` or minimum APDU lenght is incorrect                |
| 0x6B00 | `SW_WRONG_P1P2`                       | Either `P1` or `P2` is incorrect                        |
| 0x6C24 | `SW_UNKNOWN_OP`                       | Unknown Stellar operation                               |
| 0x6C25 | `SW_UNKNOWN_ENVELOPE_TYPE`            | Unknown Stellar envelope type                           |
| 0x6C66 | `SW_TX_HASH_SIGNING_MODE_NOT_ENABLED` | Hash signing model not enabled                          |
| 0x6C66 | `SW_CUSTOM_CONTRACT_MODE_NOT_ENABLED` | Custom contract model not enabled                       |
| 0x6D00 | `SW_INS_NOT_SUPPORTED`                | No command exists with `INS`                            |
| 0x6E00 | `SW_CLA_NOT_SUPPORTED`                | Bad `CLA` used for this application                     |
| 0xB000 | `SW_WRONG_RESPONSE_LENGTH`            | Wrong response length (buffer too small or too big)     |
| 0xB002 | `SW_DISPLAY_ADDRESS_FAIL`             | Failed to display address                               |
| 0xB003 | `SW_DISPLAY_TRANSACTION_HASH_FAIL`    | Failed to display transaction hash                      |
| 0xB004 | `SW_WRONG_TX_LENGTH`                  | Wrong raw transaction length                            |
| 0xB005 | `SW_TX_PARSING_FAIL`                  | Failed to parse raw transaction                         |
| 0xB006 | `SW_TX_HASH_FAIL`                     | Failed to compute hash digest of raw transaction        |
| 0xB007 | `SW_BAD_STATE`                        | Security issue with bad state                           |
| 0xB008 | `SW_SIGNATURE_FAIL`                   | Signature of raw transaction or transaction hash failed |
| 0xB009 | `SW_SWAP_CHECKING_FAIL`               | Failed to check swap params (maybe the data is invalid) |
| 0x9000 | `SW_OK`                               | Success                                                 |
