#pragma once

#include <stdbool.h>  // bool

/**
 * Action for public key validation and export.
 *
 * @param[in] choice
 *   User choice (either approved or rejected).
 *
 */
void ui_action_validate_pubkey(bool choice);

/**
 * Action for signature validation and export.
 *
 * @param[in] choice
 *   User choice (either approved or rejected).
 *
 */
void ui_action_validate_transaction(bool choice);
