#pragma once
#include <stdbool.h>

/**
 * Callback to reuse action with approve/reject in step FLOW.
 */
typedef void (*action_validate_cb)(bool);

/**
 * Show main menu (ready screen, version, about, quit).
 */
void ui_menu_main(void);

/**
 * Display address on the device and ask confirmation to export.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_display_address();

/**
 * Display transaction on the device and ask confirmation to sign.
 *
 * @return 0 if success, negative integer otherwise.
 */
int ui_display_transaction();

/**
 * Display hash on the device and ask confirmation to sign.
 *
 * @return 0 if success, negative integer otherwise.
 */
int ui_display_hash();

/**
 * Display Soroban auth on the device and ask confirmation to sign.
 *
 * @return 0 if success, negative integer otherwise.
 */
int ui_display_auth();

/**
 * Display message on the device and ask confirmation to sign.
 *
 * @return 0 if success, negative integer otherwise.
 */
int ui_display_message();
