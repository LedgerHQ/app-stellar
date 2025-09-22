#pragma once

/**
 * Instruction class of the Stellar application.
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
#define DETAIL_CAPTION_MAX_LENGTH 21

/*
 * DETAIL_VALUE_MAX_LENGTH value of 105 is chosen to fit the maximum length of 2**256 - 1 with
 * commas and decimals.
 */
#define DETAIL_VALUE_MAX_LENGTH 105

#define RAW_DATA_MAX_SIZE 10240
