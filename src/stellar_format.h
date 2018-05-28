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

/*
 * the formatter prints the details and defines the order of the details
 * by setting the next formatter to be called
 */
typedef void (*format_function_t)(tx_context_t *txCtx);

/* the current formatter */
extern volatile format_function_t formatter;

/* the current details printed by the formatter */
extern char opCaption[20];
extern char detailCaption[20];
extern char detailValue[89];

void format_confirm_transaction(tx_context_t *txCtx);
void format_confirm_operation(tx_context_t *txCtx);
void format_confirm_transaction_details(tx_context_t *txCtx);

void format_confirm_hash(tx_context_t *txCtx);

#endif