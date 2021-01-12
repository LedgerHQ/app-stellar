/*******************************************************************************
 *   Ledger Stellar App
 *   (c) 2017-2018 Ledger
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
 ********************************************************************************/

#include "os.h"
#include "cx.h"
#include "os_io_seproxyhal.h"

#include "stellar_api.h"
#include "stellar_types.h"
#include "stellar_vars.h"
#include "stellar_ux.h"

#include "swap/swap_lib_calls.h"

static void app_set_state(enum app_state_t state) {
    ctx.state = state;
}

static enum app_state_t app_get_state() {
    return ctx.state;
}

int read_bip32(const uint8_t *dataBuffer, size_t size, uint32_t *bip32) {
    size_t bip32Len = dataBuffer[0];
    dataBuffer += 1;
    if (bip32Len < 0x01 || bip32Len > MAX_BIP32_LEN) {
        THROW(0x6a80);
    }
    if (1 + 4 * bip32Len > size) {
        THROW(0x6a80);
    }

    for (unsigned int i = 0; i < bip32Len; i++) {
        bip32[i] = (dataBuffer[0] << 24u) | (dataBuffer[1] << 16u) | (dataBuffer[2] << 8u) |
                   (dataBuffer[3]);
        dataBuffer += 4;
    }
    return bip32Len;
}

void derive_private_key(cx_ecfp_private_key_t *privateKey, uint32_t *bip32, uint8_t bip32Len) {
    int error = 0;
    uint8_t privateKeyData[32];
    BEGIN_TRY {
        TRY {
            io_seproxyhal_io_heartbeat();
            os_perso_derive_node_bip32_seed_key(HDW_ED25519_SLIP10,
                                                CX_CURVE_Ed25519,
                                                bip32,
                                                bip32Len,
                                                privateKeyData,
                                                NULL,
                                                (unsigned char *) "ed25519 seed",
                                                12);
            io_seproxyhal_io_heartbeat();
            cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyData, 32, privateKey);
        }
        CATCH_OTHER(e) {
            explicit_bzero(privateKey, sizeof(cx_ecfp_private_key_t));
            error = e;
        }
        FINALLY {
            explicit_bzero(privateKeyData, sizeof(privateKeyData));
        }
    }
    END_TRY;

    if (error) {
        THROW(error);
    }
}

void init_public_key(cx_ecfp_private_key_t *privateKey,
                     cx_ecfp_public_key_t *publicKey,
                     uint8_t *buffer) {
    int error = 0;
    BEGIN_TRY {
        TRY {
            cx_ecfp_generate_pair(CX_CURVE_Ed25519, publicKey, privateKey, 1);
        }
        CATCH_OTHER(e) {
            explicit_bzero(privateKey, sizeof(cx_ecfp_private_key_t));
            error = e;
        }
        FINALLY {
        }
    }
    END_TRY;

    if (error) {
        THROW(error);
    }
    // copy public key little endian to big endian
    uint8_t i;
    for (i = 0; i < 32; i++) {
        buffer[i] = publicKey->W[64 - i];
    }
    if ((publicKey->W[32] & 1) != 0) {
        buffer[31] |= 0x80;
    }
}

void handle_get_app_configuration(volatile unsigned int *tx) {
    app_set_state(STATE_NONE);

    G_io_apdu_buffer[0] = N_stellar_pstate.hashSigning;
    G_io_apdu_buffer[1] = LEDGER_MAJOR_VERSION;
    G_io_apdu_buffer[2] = LEDGER_MINOR_VERSION;
    G_io_apdu_buffer[3] = LEDGER_PATCH_VERSION;
    *tx = 4;
    THROW(0x9000);
}

static uint32_t set_result_get_public_key(void) {
    os_memmove(G_io_apdu_buffer, ctx.req.pk.publicKey, 32);
    uint32_t tx = 32;
    if (ctx.req.pk.returnSignature) {
        os_memmove(G_io_apdu_buffer + tx, ctx.req.pk.signature, 64);
        tx += 64;
    }
    return tx;
}

void handle_get_public_key(uint8_t p1,
                           uint8_t p2,
                           uint8_t *dataBuffer,
                           uint16_t dataLength,
                           volatile unsigned int *flags,
                           volatile unsigned int *tx) {
    app_set_state(STATE_NONE);

    if ((p1 != P1_SIGNATURE) && (p1 != P1_NO_SIGNATURE)) {
        THROW(0x6B00);
    }
    if ((p2 != P2_CONFIRM) && (p2 != P2_NO_CONFIRM)) {
        THROW(0x6B00);
    }
    ctx.req.pk.returnSignature = (p1 == P1_SIGNATURE);

    uint32_t bip32[MAX_BIP32_LEN];
    int bip32Len = read_bip32(dataBuffer, dataLength, bip32);
    dataBuffer += 1 + bip32Len * 4;
    dataLength -= 1 + bip32Len * 4;

    uint16_t msgLength;
    uint8_t msg[32];
    if (ctx.req.pk.returnSignature) {
        uint8_t i;
        msgLength = dataLength;
        /* Enforce msg length to be strictly lower than the size of a SHA-256
         * digest. This ensures the message is not the resulting hash of a
         * transaction, to prevent blind signatures on transactions. */
        if (msgLength >= 32) {
            THROW(0x6a80);
        }
        for (i = 0; i < msgLength; i++) {
            if ((dataBuffer[i] < 0x20) || (dataBuffer[i] > 0x7e)) {
                THROW(0x6a80);
            }
        }
        os_memmove(msg, dataBuffer, msgLength);
    }

    cx_ecfp_private_key_t privateKey;
    cx_ecfp_public_key_t publicKey;
    derive_private_key(&privateKey, bip32, bip32Len);
    init_public_key(&privateKey, &publicKey, ctx.req.pk.publicKey);

    int error = 0;
    BEGIN_TRY {
        TRY {
            if (ctx.req.pk.returnSignature) {
                io_seproxyhal_io_heartbeat();
                cx_eddsa_sign(&privateKey,
                              CX_LAST,
                              CX_SHA512,
                              msg,
                              msgLength,
                              NULL,
                              0,
                              ctx.req.pk.signature,
                              64,
                              NULL);
                io_seproxyhal_io_heartbeat();
            }
        }
        CATCH_OTHER(e) {
            error = e;
        }
        FINALLY {
            explicit_bzero(&privateKey, sizeof(privateKey));
        }
    }
    END_TRY;

    if (error) {
        THROW(error);
    }

    uint32_t pk_tx = set_result_get_public_key();
    if (p2 & P2_CONFIRM) {
        ctx.reqType = CONFIRM_ADDRESS;
        ctx.req.pk.tx = pk_tx;
        ui_show_address_init();
        *flags |= IO_ASYNCH_REPLY;
    } else {
        *tx = pk_tx;
        THROW(0x9000);
    }
}

void handle_sign_tx(uint8_t p1,
                    uint8_t p2,
                    uint8_t *dataBuffer,
                    uint16_t dataLength,
                    volatile unsigned int *flags) {
    if ((p1 != P1_FIRST) && (p1 != P1_MORE)) {
        THROW(0x6B00);
    }
    if ((p2 != P2_LAST) && (p2 != P2_MORE)) {
        THROW(0x6B00);
    }

    if (p1 == P1_FIRST) {
        app_set_state(STATE_PARSE_TX);

        MEMCLEAR(ctx.req.tx);
        ctx.reqType = CONFIRM_TRANSACTION;
        // read the bip32 path
        ctx.req.tx.bip32Len = read_bip32(dataBuffer, dataLength, ctx.req.tx.bip32);
        dataBuffer += 1 + ctx.req.tx.bip32Len * 4;
        dataLength -= 1 + ctx.req.tx.bip32Len * 4;

        // read raw tx data
        ctx.req.tx.rawLength = dataLength;
        os_memmove(ctx.req.tx.raw, dataBuffer, dataLength);
    } else {
        if (app_get_state() != STATE_PARSE_TX) {
            THROW(0x6700);
        }

        // read more raw tx data
        uint32_t offset = ctx.req.tx.rawLength;
        ctx.req.tx.rawLength += dataLength;
        if (ctx.req.tx.rawLength > MAX_RAW_TX) {
            THROW(0x6700);
        }
        os_memmove(ctx.req.tx.raw + offset, dataBuffer, dataLength);
    }

    if (p2 == P2_MORE) {
        THROW(0x9000);
    }

    // hash transaction
#if CX_APILEVEL >= 8
    cx_hash_sha256(ctx.req.tx.raw, ctx.req.tx.rawLength, ctx.req.tx.hash, 32);
#else
    cx_hash_sha256(ctx.req.tx.raw, ctx.req.tx.rawLength, ctx.req.tx.hash);
#endif

    if (!parse_tx_xdr(ctx.req.tx.raw, ctx.req.tx.rawLength, &ctx.req.tx)) {
        THROW(0x6800);
    }

    cx_ecfp_private_key_t privateKey;
    derive_private_key(&privateKey, ctx.req.tx.bip32, ctx.req.tx.bip32Len);

    int error = 0;
    BEGIN_TRY {
        TRY {
            // sign hash
            io_seproxyhal_io_heartbeat();
            ctx.req.tx.tx = cx_eddsa_sign(&privateKey,
                                          CX_LAST,
                                          CX_SHA512,
                                          ctx.req.tx.hash,
                                          32,
                                          NULL,
                                          0,
                                          G_io_apdu_buffer,
                                          64,
                                          NULL);
            io_seproxyhal_io_heartbeat();
        }
        CATCH_OTHER(e) {
            error = e;
        }
        FINALLY {
            explicit_bzero(&privateKey, sizeof(privateKey));
        }
    }
    END_TRY;

    if (error) {
        THROW(error);
    }

    if (called_from_swap) {
        swap_check();
        os_sched_exit(0);
    }
    ui_approve_tx_init();

    *flags |= IO_ASYNCH_REPLY;
    app_set_state(STATE_APPROVE_TX);
}

void handle_sign_tx_hash(uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags) {
    app_set_state(STATE_NONE);

    if (!N_stellar_pstate.hashSigning) {
        THROW(0x6c66);
    }

    MEMCLEAR(ctx.req.tx);
    ctx.reqType = CONFIRM_TRANSACTION;

    ctx.req.tx.bip32Len = read_bip32(dataBuffer, dataLength, ctx.req.tx.bip32);
    dataBuffer += 1 + ctx.req.tx.bip32Len * 4;
    dataLength -= 1 + ctx.req.tx.bip32Len * 4;

    if (dataLength != 32) {
        THROW(0x6a80);
    }
    os_memmove(ctx.req.tx.hash, dataBuffer, dataLength);

    ui_approve_tx_hash_init();

    *flags |= IO_ASYNCH_REPLY;
    app_set_state(STATE_APPROVE_TX_HASH);
}

void handle_keep_alive(volatile unsigned int *flags) {
    *flags |= IO_ASYNCH_REPLY;
}
