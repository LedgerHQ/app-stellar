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
    char networkId[8];
    char fee[26];
    char memo[29];
    uint8_t operationType;
#if defined(TARGET_NANOS)
    char details1[28];
    char details2[28];
    char details4[30];
    char details5[45];
    char details3[50];
#elif defined(TARGET_BLUE)
    char details5[45];
    char details3[50];
    char details1[57];
    char details2[57];
    char details4[57];
#endif
} txContent_t;

void parseTxXdr(uint8_t *buffer, txContent_t *txContent);

#endif