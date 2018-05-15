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
uint16_t parse_tx_xdr(uint8_t *buffer, tx_content_t *content, uint16_t offset);

/** raw public key to base32 encoded (summarized) address */
void print_public_key(uint8_t *in, char *out, uint8_t numCharsL, uint8_t numCharsR);

/** print short public key summary or no summary at all depending on nanos or blue */
void print_public_key_short(uint8_t *in, char *out);

/** print long public key summary or no summary at all depending on nanos or blue */
void print_public_key_long(uint8_t *in, char *out);

/** output first numCharsL of input + last numCharsR of input separated by ".." */
void print_summary(char *in, char *out, uint8_t numCharsL, uint8_t numCharsR);

/** raw buffer of 32 byte hash to hexadecimal string representation */
void print_hash(uint8_t *in, char *out);

/** raw buffer of 32 byte hash to summarized hexadecimal string representation */
void print_hash_summary(uint8_t *in, char *out);

/** raw amount integer to asset-qualified string representation */
void print_amount(uint64_t amount, char *asset, char *out);

/** represent integer as bits for display of account flag options */
void print_bits(uint32_t in, char *out);

/** concatenate assetCode and assetIssuer */
void print_asset(char *assetCode, char *assetIssuer, char *out);

/** integer to string for display of offerid, sequence number, threshold weights, etc */
void print_int(int64_t l, char *out);
void print_uint(uint64_t l, char *out);

/** handles app configuration request */
void handle_get_app_configuration(volatile unsigned int *tx);

/** handles get public key request */
void handle_get_public_key(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx);

/** handles sign transaction request (displays transaction details) */
void handle_sign_tx(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx);

/** handles sign transaction hash request (displays only transaction hash) */
void handle_sign_tx_hash(uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx);

/** u2f keep alive sink */
void handle_keep_alive(volatile unsigned int *flags);

/** puts the signature in the result buffer */
uint32_t set_result_sign_tx(void);

/** puts the public key in the result buffer */
uint32_t set_result_get_public_key();

void u2f_proxy_response(u2f_service_t *service, unsigned int tx);

#endif