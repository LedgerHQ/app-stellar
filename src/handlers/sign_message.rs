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
use crate::crypto::{hash_parts, sign};
use crate::sw::AppSW;
use alloc::string::String;
use ledger_device_sdk::io::Comm;

const MESSAGE_PREFIX: &[u8] = b"Stellar Signed Message:\n";

/// Maximum accepted message length in bytes.
///
/// The escaped display form expands each non-printable byte to 4 characters
/// (`\xNN`), so the raw message must stay well below the 16 KB heap: 2 KB of
/// raw input bounds the escaped string at 8 KB, leaving the rest for the UI.
///
/// 2 KB is generous in practice: SEP-53 messages are meant to be reviewed by
/// a human on the device screen (auth challenges, wallet ownership proofs,
/// short statements — typically well under a few hundred bytes), and anything
/// this long would take dozens of screens to page through.
const MAX_MESSAGE_LEN: usize = 2048;

pub fn handler_sign_message<const MAX: usize>(
    comm: &mut Comm,
    first: bool,
    more: bool,
    ctx: &mut AppContext<MAX>,
) -> Result<(), AppSW> {
    ctx.handle_chunk(comm, first)?;

    if ctx.raw_data.len() > MAX_MESSAGE_LEN {
        return Err(AppSW::RequestDataTooLarge);
    }

    if more {
        return Ok(());
    }

    ctx.review_finished = true;

    let hash = hash_parts(&[MESSAGE_PREFIX, &ctx.raw_data])?;

    // Allocate the escaped form exactly once at its final size to avoid
    // realloc growth peaks on the small heap.
    let escaped_len = escape_bytes::Escape::new(&*ctx.raw_data).count();
    let mut message = String::with_capacity(escaped_len);
    message.extend(escape_bytes::Escape::new(&*ctx.raw_data).map(char::from));

    if !ui_sign_message(&message)? {
        return Err(AppSW::Deny);
    }

    let signature = sign(&hash, &ctx.path)?;
    comm.append(&signature);

    Ok(())
}
