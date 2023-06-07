#pragma once

#include <stdint.h>

#include "ux.h"

#include "./types.h"

/**
 * Global buffer for interactions between SE and MCU.
 */
extern uint8_t G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

/**
 * Global variable with the lenght of APDU response to send back.
 */
extern uint32_t G_output_len;

/**
 * Global structure with the parameters to exchange with the BOLOS UX application.
 */
extern bolos_ux_params_t G_ux_params;

/**
 * Global enumeration with the state of IO (READY, RECEIVING, WAITING).
 */
extern io_state_e G_io_state;

/**
 * Global context for user requests.
 */
extern global_ctx_t G_context;

/**
 * Global context for swap requests.
 */
extern swap_values_t G_swap_values;

/**
 * Is it called through the exchange app
 */
extern bool G_called_from_swap;

/**
 * Global variable with the caption of the current UI detail.
 */
extern char G_ui_detail_caption[DETAIL_CAPTION_MAX_LENGTH];

/**
 * Global variable with the value of the current UI detail.
 */
extern char G_ui_detail_value[DETAIL_VALUE_MAX_LENGTH];

extern volatile uint8_t G_ui_current_state;

extern uint8_t G_ui_current_data_index;

extern ui_action_validate_cb G_ui_validate_callback;
