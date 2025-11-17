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

#![no_std]
#![no_main]

// Required for using String, Vec, format!...
extern crate alloc;
mod app_ui;
mod bip32;
mod context;
mod crypto;
mod handlers;
mod icons;
mod settings;
mod sw;
mod swap;

use alloc::format;
use app_ui::menu::ui_menu_main;
use handlers::{
    get_configuration::handler_get_configuration, get_public_key::handler_get_public_key,
    sign_hash::handler_sign_hash, sign_message::handler_sign_message,
    sign_soroban_auth::handler_sign_soroban_auth, sign_tx::handler_sign_tx,
};
use ledger_device_sdk::io::{ApduHeader, Comm};

ledger_device_sdk::set_panic!(ledger_device_sdk::exiting_panic);

use crate::app_ui::blind_signing::ui_blind_signing;
use crate::context::{AppContext, MAX_RAW_DATA_LEN, SWAP_MAX_RAW_DATA_LEN};
use crate::sw::AppSW;
use ledger_device_sdk::nbgl::{init_comm, NbglReviewStatus, PageIndex, StatusType};
use ledger_device_sdk::testing::debug_print;

// Application specific INS codes.
const INS_GET_PK: u8 = 0x02;
const INS_SIGN_TX: u8 = 0x04;
const INS_GET_CONF: u8 = 0x06;
const INS_SIGN_HASH: u8 = 0x08;
const INS_SIGN_SOROBAN_AUTH: u8 = 0x0A;
const INS_SIGN_MESSAGE: u8 = 0x0C;

// P1 for first APDU.
const P1_FIRST_APDU: u8 = 0x00;
// P1 for non-first APDU.
const P1_MORE_APDU: u8 = 0x80;
// P2 for last APDU.
const P2_LAST_APDU: u8 = 0x00;
// P2 for more APDU.
const P2_MORE_APDU: u8 = 0x80;
// P2 for no need to ask user confirmation.
const P2_NON_CONFIRM: u8 = 0x00;
// P2 for ask user confirmation.
const P2_CONFIRM: u8 = 0x01;

/// Possible input commands received through APDUs for Stellar.
pub enum Instruction {
    GetPubkey { display: bool },
    SignTx { first: bool, more: bool },
    GetConfiguration,
    SignHash,
    SignSorobanAuth { first: bool, more: bool },
    SignMessage { first: bool, more: bool },
}

impl TryFrom<ApduHeader> for Instruction {
    type Error = AppSW;

    /// APDU parsing logic.
    ///
    /// Parses INS, P1 and P2 bytes to build an [`Instruction`]. P1 and P2 are translated to
    /// strongly typed variables depending on the APDU instruction code. Invalid INS, P1 or P2
    /// values result in errors with a status word, which are automatically sent to the host by the
    /// SDK.
    ///
    /// This design allows a clear separation of the APDU parsing logic and commands handling.
    ///
    /// Note that CLA is not checked here. Instead the method [`Comm::set_expected_cla`] is used in
    /// [`sample_main`] to have this verification automatically performed by the SDK.
    fn try_from(value: ApduHeader) -> Result<Self, Self::Error> {
        debug_print(
            format!(
                "APDU received: CLA={:02x} INS={:02x} P1={:02x} P2={:02x}\n",
                value.cla, value.ins, value.p1, value.p2
            )
            .as_str(),
        );
        match (value.ins, value.p1, value.p2) {
            // GET_PK instruction
            (INS_GET_PK, 0, P2_NON_CONFIRM | P2_CONFIRM) => Ok(Instruction::GetPubkey {
                display: value.p2 == P2_CONFIRM,
            }),
            // SIGN_TX instruction
            (INS_SIGN_TX, P1_FIRST_APDU | P1_MORE_APDU, P2_LAST_APDU | P2_MORE_APDU) => {
                Ok(Instruction::SignTx {
                    first: value.p1 == P1_FIRST_APDU,
                    more: value.p2 == P2_MORE_APDU,
                })
            }
            // GET_CONF instruction
            (INS_GET_CONF, 0, 0) => Ok(Instruction::GetConfiguration),
            // SIGN_HASH instruction
            (INS_SIGN_HASH, 0, 0) => Ok(Instruction::SignHash),
            // SIGN_SOROBAN_AUTHORIZATION instruction
            (INS_SIGN_SOROBAN_AUTH, P1_FIRST_APDU | P1_MORE_APDU, P2_LAST_APDU | P2_MORE_APDU) => {
                Ok(Instruction::SignSorobanAuth {
                    first: value.p1 == P1_FIRST_APDU,
                    more: value.p2 == P2_MORE_APDU,
                })
            }
            // SIGN_MESSAGE instruction
            (INS_SIGN_MESSAGE, P1_FIRST_APDU | P1_MORE_APDU, P2_LAST_APDU | P2_MORE_APDU) => {
                Ok(Instruction::SignMessage {
                    first: value.p1 == P1_FIRST_APDU,
                    more: value.p2 == P2_MORE_APDU,
                })
            }
            // Invalid P1 or P2
            (INS_GET_PK, _, _)
            | (INS_SIGN_TX, _, _)
            | (INS_GET_CONF, _, _)
            | (INS_SIGN_HASH, _, _)
            | (INS_SIGN_SOROBAN_AUTH, _, _)
            | (INS_SIGN_MESSAGE, _, _) => Err(AppSW::WrongP1P2),
            // Unknown instruction
            (_, _, _) => Err(AppSW::InsNotSupported),
        }
    }
}

fn show_status_and_home_if_needed(
    ins: &Instruction,
    app_ctx: &mut AppContext<MAX_RAW_DATA_LEN>,
    home: &mut ledger_device_sdk::nbgl::NbglHomeAndSettings,
    status: &AppSW,
) {
    debug_print("show_status_and_home_if_needed\n");
    if status == &AppSW::BlindSigningModeNotEnabled {
        if ui_blind_signing() {
            home.set_start_page(PageIndex::Settings(0));
            home.show_and_return();
            home.set_start_page(PageIndex::Home);
        } else {
            home.show_and_return()
        }
    }

    let (show_status, status_type) = match (ins, status) {
        (Instruction::GetPubkey { display: true }, AppSW::Deny | AppSW::Ok) => {
            (true, StatusType::Address)
        }
        (Instruction::SignTx { .. }, AppSW::Deny | AppSW::Ok) if app_ctx.finished() => {
            (true, StatusType::Transaction)
        }
        (Instruction::SignHash, AppSW::Deny | AppSW::Ok) => (true, StatusType::Transaction),
        (Instruction::SignSorobanAuth { .. }, AppSW::Deny | AppSW::Ok) if app_ctx.finished() => {
            (true, StatusType::Transaction)
        }
        (Instruction::SignMessage { .. }, AppSW::Deny | AppSW::Ok) if app_ctx.finished() => {
            (true, StatusType::Message)
        }
        (_, _) => (false, StatusType::Transaction),
    };

    if show_status {
        let success = *status == AppSW::Ok;
        NbglReviewStatus::new()
            .status_type(status_type)
            .show(success);

        home.show_and_return();
    }
}

#[no_mangle]
extern "C" fn sample_main(arg0: u32) {
    if arg0 != 0 {
        // Swap mode: use smaller buffer (1024 bytes)
        let mut app_ctx: AppContext<SWAP_MAX_RAW_DATA_LEN> = AppContext::new();
        swap::swap_main(arg0, &mut app_ctx);
    } else {
        // Normal mode: use larger buffer (1024 * 8 bytes)
        let mut app_ctx: AppContext<MAX_RAW_DATA_LEN> = AppContext::new();

        // Create the communication manager, and configure it to accept only APDU from the 0xe0 class.
        // If any APDU with a wrong class value is received, comm will respond automatically with
        // BadCla status word.
        let mut comm = Comm::new().set_expected_cla(0xe0);

        // Initialize reference to Comm instance for NBGL
        // API calls.
        init_comm(&mut comm);
        let mut home = ui_menu_main();
        home.show_and_return();

        loop {
            let ins: Instruction = comm.next_command();

            let status = match handle_apdu(&mut comm, &ins, &mut app_ctx) {
                Ok(()) => {
                    comm.reply_ok();
                    AppSW::Ok
                }
                Err(sw) => {
                    comm.reply(sw);
                    sw
                }
            };
            show_status_and_home_if_needed(&ins, &mut app_ctx, &mut home, &status);
        }
    }
}

fn handle_apdu(
    comm: &mut Comm,
    ins: &Instruction,
    ctx: &mut AppContext<MAX_RAW_DATA_LEN>,
) -> Result<(), AppSW> {
    match ins {
        Instruction::GetPubkey { display } => handler_get_public_key(comm, *display),
        Instruction::GetConfiguration => handler_get_configuration(comm),
        Instruction::SignTx { first, more } => handler_sign_tx(comm, *first, *more, ctx),
        Instruction::SignHash => handler_sign_hash(comm, ctx),
        Instruction::SignSorobanAuth { first, more } => {
            handler_sign_soroban_auth(comm, *first, *more, ctx)
        }
        Instruction::SignMessage { first, more } => handler_sign_message(comm, *first, *more, ctx),
    }
}
