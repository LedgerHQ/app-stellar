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

use ledger_device_sdk::nbgl::{Field, NbglReview};

pub fn ui_sign_hash(hash: &[u8]) -> Result<bool, AppSW> {
    let hash_str = hex::encode(hash);
    let my_fields = [Field {
        name: "Hash",
        value: hash_str.as_str(),
    }];

    let review: NbglReview = NbglReview::new()
        .titles("Review hash signing", "", "Sign hash?")
        .blind()
        .glyph(&icons::STELLAR);

    Ok(review.show(&my_fields))
}
