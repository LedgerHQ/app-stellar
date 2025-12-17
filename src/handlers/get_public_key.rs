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

use crate::app_ui::get_public_key::ui_get_public_key;
use crate::bip32::Bip32Path;
use crate::crypto::get_public_key;
use crate::sw::AppSW;
use ledger_device_sdk::io::Comm;

pub fn handler_get_public_key(comm: &mut Comm, display: bool) -> Result<(), AppSW> {
    let data = comm.get_data().map_err(|_| AppSW::WrongApduLength)?;
    let path: Bip32Path = data.try_into()?;
    let public_key = get_public_key(&path)?;

    if display && !ui_get_public_key(&public_key)? {
        return Err(AppSW::Deny);
    }

    comm.append(&public_key);
    Ok(())
}
