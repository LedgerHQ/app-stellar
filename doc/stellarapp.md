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

This command returns the public key for the given BIP 32 path. An optional message can be sent to sign to verify
the validity of the generated keypair.

#### Coding

**Command**

| *CLA* | *INS*  | *P1*               | *P2*       | *Lc*     | *Le* |  
|-------|--------|--------------------|------------|----------|------|
|   E0  |   02   |  00 : don't return signature | 00 : don't return the chain code | | |
|       |        |  01 : return signature | 01 : return the chain code | | |

**Input data**

| *Description*                                                                     | *Length*     |
|-----------------------------------------------------------------------------------|--------------|
| Number of BIP 32 derivations to perform (max 10)                                  | 1            |
| First derivation index (big endian)                                               | 4            |
| ...                                                                               | 4            |
| Last derivation index (big endian)                                                | 4            |
| Message to sign                                                                   | var (max 32) |

**Output data**

| *Description*                                                                     | *Length* |
|-----------------------------------------------------------------------------------|----------|
| Public Key                                                                        | 32       |
| Signature if requested                                                            | 64       |
| Chain code if requested                                                           | 32       |

### Sign Single Operation Transaction

#### Description

This command signs a Stellar any transaction containing a single operation and lets users validate the operation details

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


### Get app configuration

#### Description

This command returns specific application configuration.

#### Coding

**Command**

| *CLA* | *INS*  | *P1*               | *P2*       | *Lc*     | *Le* |
|-------|--------|--------------------|------------|----------|------|
|   E0  |   06   |                    |            |          |      |

**Input data**

None

**Output data**

| *Description*                                                                     | *Length* |
|-----------------------------------------------------------------------------------|----------|
| Flags                                                                             | 1        |
| Application major version                                                         | 1        |
| Application minor version                                                         | 1        |
| Application patch version                                                         | 1        |

## Transport protocol

### General transport description

Ledger APDUs requests and responses are encapsulated using a flexible protocol allowing to fragment large payloads over different underlying transport mechanisms.

The common transport header is defined as follows: 

| *Description*                                                                     | *Length* |
|-----------------------------------------------------------------------------------|----------|
| Communication channel ID (big endian)                                             | 2        |
| Command tag                                                                       | 1        |
| Packet sequence index (big endian)                                                | 2        |
| Payload                                                                           | var      |

The Communication channel ID allows commands multiplexing over the same physical link. It is not used for the time being, and should be set to 0101 to avoid compatibility issues with implementations ignoring a leading 00 byte.

The Command tag describes the message content. Use TAG_APDU (0x05) for standard APDU payloads, or TAG_PING (0x02) for a simple link test.

The Packet sequence index describes the current sequence for fragmented payloads. The first fragment index is 0x00.

### APDU Command payload encoding

APDU Command payloads are encoded as follows:

| *Description*                                                                     | *Length* |
|-----------------------------------------------------------------------------------|----------|
| APDU length (big endian)                                                          | 2        |
| APDU CLA                                                                          | 1        |
| APDU INS                                                                          | 1        |
| APDU P1                                                                           | 1        |
| APDU P2                                                                           | 1        |
| APDU length                                                                       | 1        |
| Optional APDU data                                                                | var      |

APDU payload is encoded according to the APDU case 

| Case Number  | *Lc* | *Le* | Case description                                          |
|--------------|------|------|-----------------------------------------------------------|
|   1          |  0   |  0   | No data in either direction - L is set to 00              |
|   2          |  0   |  !0  | Input Data present, no Output Data - L is set to Lc       |
|   3          |  !0  |  0   | Output Data present, no Input Data - L is set to Le       |
|   4          |  !0  |  !0  | Both Input and Output Data are present - L is set to Lc   |

### APDU Response payload encoding

APDU Response payloads are encoded as follows:

| *Description*                                                                     | *Length* |
|-----------------------------------------------------------------------------------|----------|
| APDU response length (big endian)                                                 | 2        |
| APDU response data and Status Word                                                | var      |

### USB mapping

Messages are exchanged with the dongle over HID endpoints over interrupt transfers, with each chunk being 64 bytes long. The HID Report ID is ignored.

### BLE mapping

A similar encoding is used over BLE, without the Communication channel ID.

The application acts as a GATT server defining service UUID D973F2E0-B19E-11E2-9E96-0800200C9A66

When using this service, the client sends requests to the characteristic D973F2E2-B19E-11E2-9E96-0800200C9A66, and gets notified on the characteristic D973F2E1-B19E-11E2-9E96-0800200C9A66 after registering for it. 

Requests are encoded using the standard BLE 20 bytes MTU size

## Status Words 

The following standard Status Words are returned for all APDUs - some specific Status Words can be used for specific commands.

**Status Words**

| *SW*     | *Description*                                    |
|----------|--------------------------------------------------|
|   6700   | Incorrect length                                 |
|   6982   | Security status not satisfied (Canceled by user) |
|   6A80   | Invalid data                                     |
|   6B00   | Incorrect parameter P1 or P2                     |
|   6C20   | Transaction parsing error                        |
|   6C25   | Transaction contains unsupported operation       |
|   6Fxx   | Technical problem (Internal error, please report)|
|   9000   | Normal ending of the command                     |
