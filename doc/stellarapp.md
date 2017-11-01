# Stellar application : Common Technical Specifications

## About

This document describes the APDU messages interface to communicate with the Stellar application.

The application covers the following functionalities on ed25519:

  - Retrieve a public Stellar address given a BIP 32 path
  - Sign a single Stellar payment transaction given a BIP 32 path and a stellar transaction xdr
  - Sign a any Stellar transaction given a BIP 32 path and a transaction hash

The application interface can be accessed over HID or BLE

## General purpose APDUs

### Get Public Key

#### Description

This command returns the public key for the given BIP 32 path.

#### Coding

**Command**

| *CLA* | *INS*  | *P1*               | *P2*       | *Lc*     | *Le* |  
|-------|--------|--------------------|------------|----------|------|
|   E0  |   02   |  00 : don't return signature | 00 : don't return the chain code | | |
|       |        |  01 : return signature | 01 : return the chain code | | |

**Input data**

| *Description*                                                                     | *Length* |
|-----------------------------------------------------------------------------------|----------|
| Number of BIP 32 derivations to perform (max 10)                                  | 1        |
| First derivation index (big endian)                                               | 4        |
| ...                                                                               | 4        |
| Last derivation index (big endian)                                                | 4        |

**Output data**

| *Description*                                                                     | *Length* |
|-----------------------------------------------------------------------------------|----------|
| Public Key length                                                                 | 1        |
| Public Key                                                                        | 56       |
| Signature length if requested                                                     | 1        |
| Signature if requested                                                            | 64       |
| Chain code length if requested                                                    | 1        |
| Chain code if requested                                                           | 32       |


### Sign Single Payment Transaction

#### Description

This command signs a Stellar payment transaction after having the user validate the following parameters

  - Amount
  - Destination
  - Memo
  - Fee

#### Coding

**Command**

| *CLA* | *INS*  | *P1*               | *P2*            | *Lc*     | *Le* |
|-------|--------|--------------------|-----------------|----------|------|
|   E0  |   04   |00: first apdu      |00: last apdu    |          |      |
|       |        |80: not first apdu  |80: not last apdu|          |      |


**Input data (first transaction data block)**

| *Description*                                                                     | *Length* |
|-----------------------------------------------------------------------------------|----------|
| Number of BIP 32 derivations to perform (max 10)                                  | 1        |
| First derivation index (big endian)                                               | 4        |
| ...                                                                               | 4        |
| Last derivation index (big endian)                                                | 4        |
| Serialized transaction chunk size                                                 | 1        |
| Serialized transaction chunk                                                      | variable |

**Input data (other transaction data block)**

| *Description*                                                                     | *Length* |
|-----------------------------------------------------------------------------------|----------|
| Serialized transaction chunk size                                                 | 1        |
| Serialized transaction chunk                                                      | variable |

**Output data**

| *Description*                                                                     | *Length* |
|-----------------------------------------------------------------------------------|----------|
| EDDSA encoded signature (ed25519)                                                 | variable |

### Sign Transaction Hash

#### Description

This command signs any Stellar transaction given a transaction hash. The user is warned there is no data available to inspect.

#### Coding

**Command**

| *CLA* | *INS*  | *P1*               | *P2*            | *Lc*     | *Le* |
|-------|--------|--------------------|-----------------|----------|------|
|   E0  |   04   |                    |                 |          |      |


**Input data**

| *Description*                                                                     | *Length* |
|-----------------------------------------------------------------------------------|----------|
| Number of BIP 32 derivations to perform (max 10)                                  | 1        |
| First derivation index (big endian)                                               | 4        |
| ...                                                                               | 4        |
| Last derivation index (big endian)                                                | 4        |
| Transaction hash                                                                  | 64       |

**Output data**

| *Description*                                                                     | *Length* |
|-----------------------------------------------------------------------------------|----------|
| EDDSA encoded signature (ed25519)                                                 | variable |


