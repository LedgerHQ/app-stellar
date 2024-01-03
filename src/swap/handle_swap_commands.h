#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "swap_lib_calls.h"

int handle_check_address(const check_address_parameters_t* params);
int handle_get_printable_amount(get_printable_amount_parameters_t* params);
bool copy_transaction_parameters(create_transaction_parameters_t* params);
void handle_swap_sign_transaction(void);
bool swap_check();
bool swap_str_to_u64(const uint8_t* src, size_t length, uint64_t* result);

void __attribute__((noreturn)) finalize_exchange_sign_transaction(bool is_success);
