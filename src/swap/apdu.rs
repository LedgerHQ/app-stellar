/*****************************************************************************
 *   Ledger App Stellar Rust.
 *   (c) 2025 overcat
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

use crate::{
    context::AppContext,
    crypto::{get_public_key, hash, sign},
    sw::AppSW,
    Instruction,
};
use alloc::format;
use ledger_device_sdk::{
    io::Comm,
    libcall::swap::{self, CreateTxParams},
    testing::debug_print,
};

use super::validation;

pub fn handle_swap_apdu<const MAX: usize>(
    comm: &mut Comm,
    ins: Instruction,
    tx_params: &mut CreateTxParams,
    ctx: &mut AppContext<MAX>,
) {
    debug_print("[swap] invoke handle_swap_apdu\n");

    match ins {
        Instruction::SignTx { first, more } => {
            // Handle the transaction data collection
            let result = handle_swap_sign_tx(comm, first, more, tx_params, ctx);

            match result {
                Ok(Some(signature)) => {
                    // Transaction complete and validated
                    comm.append(&signature);
                    comm.swap_reply_ok();
                    swap::swap_return(swap::SwapResult::CreateTxResult(tx_params, 1));
                }
                Ok(None) => {
                    // More data expected
                    comm.swap_reply_ok();
                }
                Err(sw) => {
                    // Error occurred
                    debug_print("[swap] Error during swap transaction handling\n");
                    comm.swap_reply(sw);
                    swap::swap_return(swap::SwapResult::CreateTxResult(tx_params, 0));
                }
            }
        }
        Instruction::GetPubkey { display } => {
            debug_print("[swap] handle_swap_apdu => GetPubkey\n");

            // Display is not supported in swap mode
            if display {
                comm.swap_reply(AppSW::InsNotSupported);
                return;
            }

            match crate::handlers::get_public_key::handler_get_public_key(comm, display) {
                Ok(()) => comm.swap_reply_ok(),
                Err(sw) => comm.swap_reply(sw),
            }
        }
        _ => comm.swap_reply(AppSW::InsNotSupported),
    }
}

fn handle_swap_sign_tx<const MAX: usize>(
    comm: &mut Comm,
    first: bool,
    more: bool,
    tx_params: &CreateTxParams,
    ctx: &mut AppContext<MAX>,
) -> Result<Option<[u8; 64]>, AppSW> {
    debug_print(&format!(
        "[swap] handle_swap_sign_tx called, first: {first}, more: {more}\n"
    ));
    ctx.handle_chunk(comm, first)?;

    // If more data expected, return None
    if more {
        debug_print("[swap] More swap payload expected\n");
        return Ok(None);
    }

    debug_print("[swap] All data received, validating swap transaction\n");

    let signer = get_public_key(&ctx.path)?;
    let signer = stellar_strkey::ed25519::PublicKey::from_payload(&signer)
        .expect("caller guarantees 32-byte Stellar public key")
        .to_string();

    // Validate the swap transaction
    if let Err(msg) = validation::validate_swap_transaction(&ctx.raw_data, tx_params, &signer) {
        debug_print(&format!("[swap] {msg}\n"));
        return Err(AppSW::SwapCheckFail);
    }

    debug_print("[swap] Swap transaction validated, signing\n");

    // Sign the transaction
    let hash_value = hash(&ctx.raw_data)?;
    let signature = sign(&hash_value, &ctx.path)?;

    Ok(Some(signature))
}
