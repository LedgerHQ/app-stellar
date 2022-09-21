#pragma once

#include <stdbool.h>  // bool

#include "../globals.h"

/*
 * Longest string will be "Operation ii of nn"
 */
#define OPERATION_CAPTION_MAX_LENGTH 20

/*
 * the formatter prints the details and defines the order of the details
 * by setting the next formatter to be called
 */
typedef void (*format_function_t)(tx_ctx_t *tx_ctx);

/* 16 formatters in a row ought to be enough for everybody*/
#define MAX_FORMATTERS_PER_OPERATION 16

/* the current formatter */
extern format_function_t formatter_stack[MAX_FORMATTERS_PER_OPERATION];
/* the current details printed by the formatter */
extern char op_caption[OPERATION_CAPTION_MAX_LENGTH];
extern int8_t formatter_index;

void set_state_data(bool forward);
