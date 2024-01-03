#include "./globals.h"

uint8_t G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];
ux_state_t G_ux;
bolos_ux_params_t G_ux_params;
io_state_e G_io_state;
uint32_t G_output_len;

global_ctx_t G_context;

bool G_called_from_swap;

swap_or_ui_t G;
