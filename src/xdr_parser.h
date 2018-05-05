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
#ifndef _XDR_PARSER_H_
#define _XDR_PARSER_H_

#include <stdint.h>

typedef struct {
    uint8_t opType;
    uint8_t opCount;
    uint8_t opIdx;
    char txDetails[4][29];
    char opDetails[5][50];
} tx_content_t;

uint16_t parseTxXdr(uint8_t *buffer, tx_content_t *content, uint16_t offset);

#endif