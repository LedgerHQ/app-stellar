/*******************************************************************************
 *   Ledger Stellar App
 *   (c) 2017 Ledger
 *
 *  adapted from https://github.com/mjg59/tpmtotp/blob/master/base32.h
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

#ifndef _STELLAR_UTILS_H_
#define _STELLAR_UTILS_H_

#include <stdint.h>

#ifdef TEST
#include <stdio.h>
#define THROW(code) { printf("error: %d", code); return; }
#define PRINTF(msg, arg) printf(msg, arg)
#define PIC(code) code
#define TARGET_NANOS 1
#else
#include "os.h"
#endif // TEST

#define ASSET_TYPE_NATIVE 0
#define ASSET_TYPE_CREDIT_ALPHANUM4 1
#define ASSET_TYPE_CREDIT_ALPHANUM12 2

#define MEMO_TYPE_NONE 0
#define MEMO_TYPE_TEXT 1
#define MEMO_TYPE_ID 2
#define MEMO_TYPE_HASH 3
#define MEMO_TYPE_RETURN 4

#define XDR_OPERATION_TYPE_CREATE_ACCOUNT 0
#define XDR_OPERATION_TYPE_PAYMENT 1
#define XDR_OPERATION_TYPE_PATH_PAYMENT 2
#define XDR_OPERATION_TYPE_MANAGE_OFFER 3
#define XDR_OPERATION_TYPE_CREATE_PASSIVE_OFFER 4
#define XDR_OPERATION_TYPE_SET_OPTIONS 5
#define XDR_OPERATION_TYPE_CHANGE_TRUST 6
#define XDR_OPERATION_TYPE_ALLOW_TRUST 7
#define XDR_OPERATION_TYPE_ACCOUNT_MERGE 8
#define XDR_OPERATION_TYPE_INFLATION 9
#define XDR_OPERATION_TYPE_MANAGE_DATA 10

#define OPERATION_TYPE_CREATE_ACCOUNT 0
#define OPERATION_TYPE_PAYMENT 1
#define OPERATION_TYPE_PATH_PAYMENT 2
#define OPERATION_TYPE_CREATE_OFFER 3
#define OPERATION_TYPE_DELETE_OFFER 4
#define OPERATION_TYPE_CHANGE_OFFER 5
#define OPERATION_TYPE_CREATE_PASSIVE_OFFER 6
#define OPERATION_TYPE_SET_OPTIONS 7
#define OPERATION_TYPE_CHANGE_TRUST 8
#define OPERATION_TYPE_REMOVE_TRUST 9
#define OPERATION_TYPE_ALLOW_TRUST 10
#define OPERATION_TYPE_REVOKE_TRUST 11
#define OPERATION_TYPE_ACCOUNT_MERGE 12
#define OPERATION_TYPE_INFLATION 13
#define OPERATION_TYPE_MANAGE_DATA 14

#define CAPTION_TYPE_OPERATION 0
#define CAPTION_TYPE_DETAILS1 1
#define CAPTION_TYPE_DETAILS2 2
#define CAPTION_TYPE_DETAILS3 3
#define CAPTION_TYPE_DETAILS4 4
#define CAPTION_TYPE_DETAILS5 5


void public_key_to_address(uint8_t *in, char *out);

void print_summary(char *in, char *out);

void print_public_key(uint8_t *in, char *out);

void print_amount(uint64_t amount, char *asset, char *out);

void print_long(uint64_t id, char *out);

void print_network_id(uint8_t *in, char *out);

void print_caption(uint8_t operationType, uint8_t captionType, char *out);

void print_hash_summary(uint8_t *in, char *out);

void print_hash(uint8_t *in, char *out);

void print_bits(uint32_t in, char *out);

void print_int(uint32_t in, char *out);


#endif // _STELLAR_UTILS_H_
