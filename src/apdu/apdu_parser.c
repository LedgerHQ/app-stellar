/*****************************************************************************
 *   Ledger Stellar App.
 *   (c) 2022 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include <stddef.h>   // size_t
#include <stdint.h>   // uint*_t
#include <stdbool.h>  // bool

#include "./apdu_parser.h"

/**
 * Offset of instruction class.
 */
#define OFFSET_CLA 0
/**
 * Offset of instruction code.
 */
#define OFFSET_INS 1
/**
 * Offset of instruction parameter 1.
 */
#define OFFSET_P1 2
/**
 * Offset of instruction parameter 2.
 */
#define OFFSET_P2 3
/**
 * Offset of command data length.
 */
#define OFFSET_LC 4
/**
 * Offset of command data.
 */
#define OFFSET_CDATA 5

bool apdu_parser(command_t *cmd, uint8_t *buf, size_t buf_len) {
    // Check minimum length and Lc field of APDU command
    if (buf_len < OFFSET_CDATA || buf_len - OFFSET_CDATA != buf[OFFSET_LC]) {
        return false;
    }

    cmd->cla = buf[OFFSET_CLA];
    cmd->ins = (command_e) buf[OFFSET_INS];
    cmd->p1 = buf[OFFSET_P1];
    cmd->p2 = buf[OFFSET_P2];
    cmd->lc = buf[OFFSET_LC];
    cmd->data = (buf[OFFSET_LC] > 0) ? buf + OFFSET_CDATA : NULL;

    return true;
}
