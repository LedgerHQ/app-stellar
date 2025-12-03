# Stellar App Commands

## Overview

| Command name                 | INS  | Description                               |
| ---------------------------- | ---- | ----------------------------------------- |
| `GET_PUBLIC_KEY`             | 0x02 | Get public key given BIP32 path           |
| `SIGN_TX`                    | 0x04 | Sign the raw transaction                  |
| `GET_APP_CONFIGURATION`      | 0x06 | Get application configuration information |
| `SIGN_HASH`                  | 0x08 | Sign the hash                             |
| `SIGN_SOROBAN_AUTHORIZATION` | 0x0A | Sign the Soroban Authorization            |
| `SIGN_MESSAGE`               | 0x0C | Sign the SEP-0053 message                 |

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

| Response length (bytes) | SW     | RData                                                                                                      |
| ----------------------- | ------ | ---------------------------------------------------------------------------------------------------------- |
| 4                       | 0x9000 | `HASH_SIGNING_ENABLED (1)` \|\| `MAJOR (1)` \|\| `MINOR (1)` \|\| `PATCH (1)` \|\| `RAW_DATA_MAX_SIZE (2)` |

## SIGN_HASH

### Command

| CLA  | INS  | P1   | P2   | Lc          | CData                                                                                                       |
| ---- | ---- | ---- | ---- | ----------- | ----------------------------------------------------------------------------------------------------------- |
| 0xE0 | 0x08 | 0x00 | 0x00 | 1 + 4n + 32 | `len(bip32_path) (1)` \|\|<br> `bip32_path{1} (4)` \|\|<br>`...` \|\|<br>`bip32_path{n} (4)`<br>`hash (32)` |

### Response

| Response length (bytes) | SW     | RData            |
| ----------------------- | ------ | ---------------- |
| 64                      | 0x9000 | `signature (64)` |

## SIGN_SOROBAN_AUTHORIZATION

### Command

| CLA  | INS  | P1                                 | P2                           | Lc                                                                | CData                                                                                                                             |
| ---- | ---- | ---------------------------------- | ---------------------------- | ----------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------- |
| 0xE0 | 0x0A | 0x00 (first) <br> 0x80 (not_first) | 0x00 (last) <br> 0x80 (more) | 1 + 4n + k<br/>Only the first data chunk contains bip32 path data | `len(bip32_path) (1)` \|\|<br> `bip32_path{1} (4)` \|\|<br>`...` \|\|<br>`bip32_path{n} (4)` \|\|<br> `hash_id_preimage_chunk(k)` |

### Response

| Response length (bytes) | SW     | RData            |
| ----------------------- | ------ | ---------------- |
| 64                      | 0x9000 | `signature (64)` |

## SIGN_MESSAGE

### Command

| CLA  | INS  | P1                                 | P2                           | Lc                                                                | CData                                                                                                                    |
| ---- | ---- | ---------------------------------- | ---------------------------- | ----------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------ |
| 0xE0 | 0x0C | 0x00 (first) <br> 0x80 (not_first) | 0x00 (last) <br> 0x80 (more) | 1 + 4n + k<br/>Only the first data chunk contains bip32 path data | `len(bip32_path) (1)` \|\|<br> `bip32_path{1} (4)` \|\|<br>`...` \|\|<br>`bip32_path{n} (4)` \|\|<br> `message_chunk(k)` |

### Response

| Response length (bytes) | SW     | RData            |
| ----------------------- | ------ | ---------------- |
| 64                      | 0x9000 | `signature (64)` |

## Status Words

| SW     | SW name                      | Description                                             |
| ------ | ---------------------------- | ------------------------------------------------------- |
| 0x6985 | `Deny`                       | Rejected by user                                        |
| 0xB001 | `KeyDeriveFail`              | Failed to derive the key                                |
| 0xB004 | `RequestDataTooLarge`        | The data is too large to be processed                   |
| 0xB005 | `DataParsingFail`            | Failed to parse raw data                                |
| 0xB006 | `DataHashFail`               | Failed to compute hash digest of raw data               |
| 0xB008 | `DataSignFail`               | Generating signature failed                             |
| 0xB009 | `SwapCheckFail`              | Failed to check swap params (maybe the data is invalid) |
| 0x6125 | `DataFormattingFail`         | Failed to format the data                               |
| 0x6A87 | `WrongApduLength`            | `Lc` or minimum APDU length is incorrect                |
| 0x6B00 | `WrongP1P2`                  | Either `P1` or `P2` is incorrect                        |
| 0x6D00 | `InsNotSupported`            | No command exists with `INS`                            |
| 0x6C66 | `BlindSigningModeNotEnabled` | Blind signing model not enabled.                        |
| 0x9000 | `Ok`                         | Success                                                 |
