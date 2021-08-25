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
#ifndef _STELLAR_FORMAT_H_
#define _STELLAR_FORMAT_H_

#include "stellar_types.h"

/*
 * the formatter prints the details and defines the order of the details
 * by setting the next formatter to be called
 */
typedef void (*format_function_t)(tx_context_t *txCtx);

/* 17 formatters in a row ought to be enough for everybody*/
#define MAX_FORMATTERS_PER_OPERATION 17

/* the current formatter */
extern format_function_t formatter_stack[MAX_FORMATTERS_PER_OPERATION];
extern int8_t formatter_index;
extern uint8_t current_data_index;

/* the current details printed by the formatter */
extern char opCaption[OPERATION_CAPTION_MAX_SIZE];
extern char detailCaption[DETAIL_CAPTION_MAX_SIZE];
extern char detailValue[DETAIL_VALUE_MAX_SIZE];

void set_state_data(bool forward);

#endif
