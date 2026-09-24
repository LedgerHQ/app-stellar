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

use crate::context::AppContext;
use ledger_device_sdk::{
    io::Comm,
    libcall::{
        self,
        swap::{self},
    },
};

mod apdu;
mod validation;

use swap_utils::format_printable_amount;

pub fn swap_main<const MAX: usize>(arg0: u32, ctx: &mut AppContext<MAX>) {
    ledger_device_sdk::log::debug!("[swap] invoke swap_main");

    match libcall::get_command(arg0) {
        libcall::LibCallCommand::SwapCheckAddress => handle_check_address_command(arg0),
        libcall::LibCallCommand::SwapGetPrintableAmount => handle_printable_amount_command(arg0),
        libcall::LibCallCommand::SwapSignTransaction => handle_sign_transaction_command(arg0, ctx),
    }
}

fn handle_check_address_command(arg0: u32) {
    ledger_device_sdk::log::debug!("[swap] invoke handle_check_address_command");

    let mut params = swap::get_check_address_params(arg0);

    let result = match validation::check_address(&params) {
        Ok(_) => 1,
        Err(err) => {
            ledger_device_sdk::log::error!("[swap] Error checking address: {}", err);
            0
        }
    };

    swap::swap_return(swap::SwapResult::CheckAddressResult(&mut params, result));
}

fn handle_printable_amount_command(arg0: u32) {
    ledger_device_sdk::log::debug!("[swap] invoke handle_printable_amount_command");

    // Use default buffer sizes from SDK
    let mut params: swap::PrintableAmountParams = swap::get_printable_amount_params(arg0);

    // Use a static buffer on the stack to avoid BSS writes
    // '922337203685.4775808 XLM' is the longest possible amount string (24 chars with null terminator)
    const BUFFER_SIZE: usize = 32;
    let mut buffer = [0u8; BUFFER_SIZE];

    match format_printable_amount(&params.amount, &mut buffer) {
        Ok(formatted_str) => swap::swap_return(swap::SwapResult::PrintableAmountResult(
            &mut params,
            formatted_str,
        )),
        Err(_) => {
            // Unreachable today (XLM supply is far below i64::MAX), but the
            // app-exchange contract requires an empty printable_amount to
            // signal failure — a non-empty string would be displayed as a
            // real amount in the Exchange UI.
            ledger_device_sdk::log::error!("[swap] Error formatting amount");
            swap::swap_return(swap::SwapResult::PrintableAmountResult(&mut params, ""))
        }
    }
}

fn handle_sign_transaction_command<const MAX: usize>(arg0: u32, ctx: &mut AppContext<MAX>) {
    ledger_device_sdk::log::debug!("[swap] invoke handle_sign_transaction_command");

    let mut params = swap::sign_tx_params(arg0);
    let mut comm = Comm::new().set_expected_cla(0xe0);

    // Enter APDU processing loop
    loop {
        let instruction: crate::Instruction = comm.next_command();
        apdu::handle_swap_apdu(&mut comm, instruction, &mut params, ctx);
    }
}
