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

uint16_t parseTxXdr(uint8_t *buffer, tx_content_t *content, uint16_t offset);

void print_public_key(uint8_t *in, char *out, uint8_t numCharsL, uint8_t numCharsR);
void print_summary(char *in, char *out, uint8_t numCharsL, uint8_t numCharsR);
void print_hash_summary(uint8_t *in, char *out);
void print_amount(uint64_t amount, char *asset, char *out);
void print_network_id(uint8_t *in, char *out);
void print_bits(uint32_t in, char *out);
void print_asset(char *code, char *issuer, char *out);
void print_int(uint64_t l, char *out);

unsigned short crc16(char *ptr, int count);
int base32_encode(const uint8_t *data, int length, char *result, int bufSize);

#endif