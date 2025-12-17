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

use crate::app_ui::sign_hash::ui_sign_hash;
use crate::bip32::Bip32Path;
use crate::context::AppContext;
use crate::crypto::sign;
use crate::settings::Settings;
use crate::sw::AppSW;
use ledger_device_sdk::io::Comm;

const HASH_SIZE: usize = 32;

pub fn handler_sign_hash<const MAX: usize>(
    comm: &mut Comm,
    ctx: &mut AppContext<MAX>,
) -> Result<(), AppSW> {
    ctx.reset();
    let data = comm.get_data().map_err(|_| AppSW::WrongApduLength)?;
    let path: Bip32Path = data.try_into()?;

    let hash = data
        .get(path.serialized_size()..)
        .ok_or(AppSW::WrongApduLength)?;
    if hash.len() != HASH_SIZE {
        return Err(AppSW::WrongApduLength);
    }

    let settings = Settings;
    if !settings.is_blind_signing_enabled() {
        return Err(AppSW::BlindSigningModeNotEnabled);
    }

    if !ui_sign_hash(hash)? {
        return Err(AppSW::Deny);
    }

    let signature = sign(hash, &path)?;
    comm.append(&signature);

    Ok(())
}
