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
#ifndef _STELLAR_API_H_
#define _STELLAR_API_H_

#include "stellar_types.h"

#ifndef TEST
#include "os.h"
#include "cx.h"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

// ------------------------------------------------------------------------- //
//                             Request handlers                              //
// ------------------------------------------------------------------------- //

/** handles app configuration request */
void handle_get_app_configuration(volatile unsigned int *tx);

/** handles get public key request */
void handle_get_public_key(uint8_t p1,
                           uint8_t p2,
                           uint8_t *dataBuffer,
                           uint16_t dataLength,
                           volatile unsigned int *flags,
                           volatile unsigned int *tx);

/** handles sign transaction request (displays transaction details) */
void handle_sign_tx(uint8_t p1,
                    uint8_t p2,
                    uint8_t *dataBuffer,
                    uint16_t dataLength,
                    volatile unsigned int *flags);

/** handles sign transaction hash request (displays only transaction hash) */
void handle_sign_tx_hash(uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags);

/** u2f keep alive */
void handle_keep_alive(volatile unsigned int *flags);

// ------------------------------------------------------------------------- //
//                           TRANSACTION PARSING                             //
// ------------------------------------------------------------------------- //

/**
 * Parsing of the raw transaction XDR.
 * Starts parsing the buffer at txCtx.offset to populate the content struct.
 * Only a single operation is parsed during each call.
 * Subsequent calls resume parsing where the last call left off
 * or from the beginning when the end was reached.
 */
bool parse_tx_xdr(const uint8_t *data, size_t size, tx_context_t *txCtx);

// ------------------------------------------------------------------------- //
//                           DATA STRUCTURES                                 //
// ------------------------------------------------------------------------- //

typedef struct {
    const uint8_t *ptr;
    size_t size;
    off_t offset;
} buffer_t;

// ------------------------------------------------------------------------- //
//                                UTILITIES                                  //
// ------------------------------------------------------------------------- //

#ifndef TEST
/**  derive a private key from a bip32 path */
int derive_private_key(cx_ecfp_private_key_t *privateKey, uint32_t *bip32, uint8_t bip32Len);

/**  intialize a public key the Stellar way */
int init_public_key(cx_ecfp_private_key_t *privateKey,
                    cx_ecfp_public_key_t *publicKey,
                    uint8_t *buffer);
#endif

/**  parse a bip32 path from a byte stream */
bool parse_bip32_path(uint8_t *path,
                      size_t path_length,
                      uint32_t *path_parsed,
                      size_t path_parsed_length);

/**  base32 encode public key */
void encode_public_key(const uint8_t *in, char *out);

/** base32 encode pre-auth transaction hash */
void encode_pre_auth_key(const uint8_t *in, char *out);

/** base32 encode sha256 hash */
void encode_hash_x_key(const uint8_t *in, char *out);

/** raw public key to base32 encoded (summarized) address */
void print_public_key(const uint8_t *in, char *out, uint8_t numCharsL, uint8_t numCharsR);

/**  base32 encode muxed account */
void encode_muxed_account(const MuxedAccount *in, char *out);

/** raw muxed account to base32 encoded muxed address */
void print_muxed_account(const MuxedAccount *in, char *out, uint8_t numCharsL, uint8_t numCharsR);

/** output first numCharsL of input + last numCharsR of input separated by ".." */
void print_summary(const char *in, char *out, uint8_t numCharsL, uint8_t numCharsR);

/** raw byte buffer to hexadecimal string representation.
 * len is length of input, provided output must be twice that size */
void print_binary(const uint8_t *in, char *out, uint8_t len);

/** raw byte buffer to summarized hexadecimal string representation
 * len is length of input, provided output must be at least length 19 */
void print_binary_summary(const uint8_t *in, char *out, uint8_t len);

/** raw amount integer to asset-qualified string representation */
int print_amount(uint64_t amount,
                 const Asset *asset,
                 uint8_t network_id,
                 char *out,
                 size_t out_len);

/** concatenate assetCode and assetIssuer summary */
void print_asset_t(const Asset *asset, uint8_t network_id, char *out, size_t out_len);

/** asset name */
int print_asset_name(const Asset *asset, uint8_t network_id, char *out, size_t out_len);

/** concatenate code and issuer */
void print_asset(const char *code, char *issuer, char *out, size_t out_len);

/** "XLM" or "native" depending on the network id */
void print_native_asset_code(uint8_t network, char *out, size_t out_len);

/** string representation of flags present */
void print_flags(uint32_t flags, char *out, size_t out_len);

/** string representation of trust line flags present */
void print_trust_line_flags(uint32_t flags, char *out, size_t out_len);

/** integer to string for display of sequence number */
int print_int(int64_t l, char *out, size_t out_len);

/** integer to string for display of offerid, sequence number, threshold weights, etc */
int print_uint(uint64_t l, char *out, size_t out_len);

/** base64 encoding function used to display managed data values */
void base64_encode(const uint8_t *data, int inLen, char *out);

/** hex representation of flags claimable balance id */
void print_claimable_balance_id(const ClaimableBalanceID *claimableBalanceID, char *out);

/** converts the timestamp in seconds to a readable utc time string */
bool print_time(uint64_t timestamp_in_seconds, char *out, size_t out_len);
#endif
