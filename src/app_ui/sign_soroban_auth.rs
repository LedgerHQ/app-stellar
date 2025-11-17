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

use crate::settings::Settings;
use crate::{icons, sw::AppSW};
use alloc::vec::Vec;

use ledger_device_sdk::nbgl::{Field, NbglReview};
use stellarlib::{format_hash_id_preimage_soroban_authorization, HashIDPreimage, Parser, XdrParse};

pub fn ui_sign_soroban_auth(raw_data: &[u8]) -> Result<bool, AppSW> {
    let settings = Settings;
    if !settings.is_blind_signing_enabled() {
        return Err(AppSW::BlindSigningModeNotEnabled);
    }

    let mut parser = Parser::new(raw_data);
    let preimage = HashIDPreimage::parse(&mut parser).map_err(|_| AppSW::DataParsingFail)?;
    let HashIDPreimage::SorobanAuthorization(auth) = preimage;

    let data_entries =
        format_hash_id_preimage_soroban_authorization(&auth, &Settings.to_format_config());
    let fields: Vec<Field> = data_entries
        .iter()
        .map(|entry| Field {
            name: entry.title.as_str(),
            value: entry.content.as_str(),
        })
        .collect();

    let review = NbglReview::new()
        .titles(
            "Review Soroban Authorization",
            "",
            "Sign Soroban Authorization?",
        )
        .glyph(&icons::STELLAR)
        .blind();
    let result = review.show(&fields);

    Ok(result)
}
