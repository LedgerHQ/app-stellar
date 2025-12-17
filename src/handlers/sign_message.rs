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

use crate::app_ui::sign_message::ui_sign_message;
use crate::context::AppContext;
use crate::crypto::{hash, sign};
use crate::sw::AppSW;
use alloc::string::String;
use alloc::vec::Vec;
use ledger_device_sdk::io::Comm;

const MESSAGE_PREFIX: &[u8] = b"Stellar Signed Message:\n";

pub fn handler_sign_message<const MAX: usize>(
    comm: &mut Comm,
    first: bool,
    more: bool,
    ctx: &mut AppContext<MAX>,
) -> Result<(), AppSW> {
    ctx.handle_chunk(comm, first)?;

    if more {
        return Ok(());
    }

    ctx.review_finished = true;

    let mut prefixed_message = Vec::with_capacity(MESSAGE_PREFIX.len() + ctx.raw_data.len());
    prefixed_message.extend_from_slice(MESSAGE_PREFIX);
    prefixed_message.extend_from_slice(&ctx.raw_data);

    let hash = hash(&prefixed_message)?;
    let message: String = escape_bytes::Escape::new(&*ctx.raw_data)
        .map(char::from)
        .collect();

    if !ui_sign_message(&message)? {
        return Err(AppSW::Deny);
    }

    let signature = sign(&hash, &ctx.path)?;
    comm.append(&signature);

    Ok(())
}
