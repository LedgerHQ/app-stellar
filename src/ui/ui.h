#pragma once

#include <stdbool.h>  // bool

#include "os.h"
#include "ux.h"

#include "glyphs.h"

#define INSIDE_BORDERS 0
#define OUT_OF_BORDERS 1

/**
 * Display address on the device and ask confirmation to export.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_display_address();

/**
 * Show main menu (ready screen, version, about, quit).
 */
void ui_menu_main();

/**
 * Shows the process of signing a transaction hash.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_approve_tx_hash_init();

/**
 * Shows the process of signing a transaction.
 *
 * @return 0 if success, negative integer otherwise.
 */
int ui_approve_tx_init();

/**
 * Shows the process of signing a Soroban authorization.
 *
 * @return 0 if success, negative integer otherwise.
 */
int ui_approve_soroban_auth_init();