#pragma once
#include <stdbool.h>

/**
 * Callback to reuse action with approve/reject in step FLOW.
 */
typedef void (*action_validate_cb)(bool);

/**
 * Display address on the device and ask confirmation to export.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_display_address(void);

/**
 * Display transaction on the device and ask confirmation to sign.
 *
 * @return 0 if success, negative integer otherwise.
 */
int ui_display_transaction(void);

/**
 * Display hash on the device and ask confirmation to sign.
 *
 * @return 0 if success, negative integer otherwise.
 */
int ui_display_hash(void);

/**
 * Display Soroban auth on the device and ask confirmation to sign.
 *
 * @return 0 if success, negative integer otherwise.
 */
int ui_display_auth(void);

/**
 * Display message on the device and ask confirmation to sign.
 *
 * @return 0 if success, negative integer otherwise.
 */
int ui_display_message(void);

/**
 * Display error for blind signing if not enabled.
 */
void ui_error_blind_signing(void);

/**
 * Go to home screen
 */
void ui_idle(void);

#ifdef HAVE_NBGL
/**
 * Display settings menu.
 */
void ui_settings(void);
#endif  // HAVE_NBGL

#if defined(TARGET_STAX) || defined(TARGET_FLEX)
#define ICON_APP_HOME C_icon_stellar_64px
#elif defined(TARGET_APEX_P)
#define ICON_APP_HOME C_icon_stellar_48px
#endif
