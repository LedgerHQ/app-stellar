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

use crate::{icons, sw::AppSW};

use ledger_device_sdk::nbgl::NbglAddressReview;

pub fn ui_get_public_key(public_key: &[u8]) -> Result<bool, AppSW> {
    let address = stellar_strkey::ed25519::PublicKey::from_payload(public_key)
        .expect("caller guarantees 32-byte Stellar public key")
        .to_string();

    // Display the address confirmation screen.
    Ok(NbglAddressReview::new()
        .glyph(&icons::STELLAR)
        .review_title("Verify Stellar address")
        .show(&address))
}
