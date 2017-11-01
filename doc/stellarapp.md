# Stellar application : Common Technical Specifications

## About

This document describes the APDU messages interface to communicate with the Stellar application.

The application covers the following functionalities on ed25519:

  - Retrieve a public Stellar address given a BIP 32 path
  - Sign a single Stellar payment transaction given a BIP 32 path and a stellar transaction xdr
  - Sign a any Stellar transaction given a BIP 32 path and a transaction hash

The application interface can be accessed over HID or BLE

## General purpose APDUs

### Get Stellar Public Key

#### Description

This command returns the public key for the given BIP 32 path.

#### Coding

| *CLA* | *INS*  | *P1*               | *P2*       | *Lc*     | *Le* |  
|---------------------------------------------------------------------|
|   E0  |   02   |  00 : don't return signature | 00 : don't return the chain code | | |
|       |        |  01 : return signature | 01 : return the chain code | | |

