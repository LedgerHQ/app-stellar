#pragma once

/**
 * Instruction class of the Boilerplate application.
 */
#define CLA 0xE0

/**
 * Maximum length of MAJOR_VERSION || MINOR_VERSION || PATCH_VERSION.
 */
#define APP_VERSION_SIZE 3

/**
 * Length of hash_signing_enabled
 */
#define APP_CONFIGURATION_SIZE 1

/**
 * Signature length (bytes).
 */
#define SIGNATURE_SIZE 64

/*
 * Captions don't scroll so there is no use in having more capacity than can fit on screen at once.
 */
#define DETAIL_CAPTION_MAX_LENGTH 20

/*
 * DETAIL_VALUE_MAX_LENGTH value of 89 is due to the maximum length of managed data value which can
 * be 64 bytes long. Managed data values are displayed as base64 encoded strings, which are
 * 4*((len+2)/3) characters long. (An additional slot is required for the end-of-string character of
 * course)
 */
#define DETAIL_VALUE_MAX_LENGTH 89

#ifdef TARGET_NANOS
#define RAW_TX_MAX_SIZE 1120
#else
#define RAW_TX_MAX_SIZE 5120
#endif
