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
#ifndef _XDR_PARSER_H_
#define _XDR_PARSER_H_

#include <stdint.h>

typedef struct txContent_t {
    char networkId[32];
    char source[57];
    char destination[57];
    uint64_t amount;
    uint32_t fee;
    char assetCode[13];
    uint8_t memoType;
    char memo[29];
} txContent_t;

void parseTxXdr(uint8_t *buffer, txContent_t *txContent);

#endif