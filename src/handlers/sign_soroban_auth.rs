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
use crate::app_ui::sign_soroban_auth::ui_sign_soroban_auth;
use crate::context::AppContext;
use crate::crypto::{hash, sign};
use crate::sw::AppSW;
use ledger_device_sdk::io::Comm;

pub fn handler_sign_soroban_auth<const MAX: usize>(
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

    if !ui_sign_soroban_auth(&ctx.raw_data)? {
        return Err(AppSW::Deny);
    }

    let hash = hash(&ctx.raw_data)?;
    let signature = sign(&hash, &ctx.path)?;
    comm.append(&signature);

    Ok(())
}
