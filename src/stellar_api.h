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
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "u2f_service.h"
#include "u2f_transport.h"

extern bool fidoActivated;

extern void USB_power_U2F(unsigned char enabled, unsigned char fido);

/**
 * Parsing of the raw transaction XDR.
 * Starts parsing the buffer at the given offset to populate the content struct.
 * Only a single operation is parsed during each call.
 * Returns the offset to the next operation in the buffer or 0 if this was the last operation in the buffer.
 */
void parse_tx_xdr(uint8_t *buffer, tx_context_t *txCtx);

/** raw public key to base32 encoded (summarized) address */
void print_public_key(uint8_t *in, char *out, uint8_t numCharsL, uint8_t numCharsR);

/** output first numCharsL of input + last numCharsR of input separated by ".." */
void print_summary(char *in, char *out, uint8_t numCharsL, uint8_t numCharsR);

/** raw byte buffer to hexadecimal string representation.
  * len is length of input, provided output must be twice that size */
void print_binary(uint8_t *in, char *out, uint8_t len);

/** raw byte buffer to summarized hexadecimal string representation
  * len is length of input, provided output must be at least length 19 */
void print_binary_summary(uint8_t *in, char *out, uint8_t len);

/** raw amount integer to asset-qualified string representation */
void print_amount(uint64_t amount, char *asset, char *out);

/** concatenate assetCode and assetIssuer summary */
void print_asset_t(asset_t *asset, char *out);

/** concatenate assetCode and assetIssuer */
void print_asset(char *assetCode, char *assetIssuer, char *out);

/** "XLM" or "native" depending on the network id */
void print_native_asset_code(uint8_t network, char *out);

/** string representation of flags present */
void print_flags(uint32_t flags, char *out);

/** integer to string for display of offerid, sequence number, threshold weights, etc */
void print_int(int64_t l, char *out);
void print_uint(uint64_t l, char *out);

/** handles app configuration request */
void handle_get_app_configuration(volatile unsigned int *tx);

/** handles get public key request */
void handle_get_public_key(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx);

/** handles sign transaction request (displays transaction details) */
void handle_sign_tx(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags);

/** handles sign transaction hash request (displays only transaction hash) */
void handle_sign_tx_hash(uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags);

/** u2f keep alive sink */
void handle_keep_alive(volatile unsigned int *flags);

/** puts the signature in the result buffer */
uint32_t set_result_sign_tx(void);

/** puts the public key in the result buffer */
uint32_t set_result_get_public_key();

void u2f_proxy_response(u2f_service_t *service, unsigned int tx);

#endif