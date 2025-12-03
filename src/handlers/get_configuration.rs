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

use crate::context::MAX_RAW_DATA_LEN;
use crate::settings::Settings;
use crate::sw::AppSW;
use ledger_device_sdk::io::Comm;

fn parse_version() -> (u8, u8, u8) {
    fn parse_part(part: Option<&str>) -> u8 {
        part.and_then(|s| s.parse().ok()).unwrap_or(0)
    }

    let mut parts = env!("CARGO_PKG_VERSION").split('.');
    let major = parse_part(parts.next());
    let minor = parse_part(parts.next());
    let patch = parse_part(parts.next());

    (major, minor, patch)
}

pub fn handler_get_configuration(comm: &mut Comm) -> Result<(), AppSW> {
    let settings = Settings;
    let (major, minor, patch) = parse_version();

    let blind_signing_enabled = u8::from(settings.is_blind_signing_enabled());

    let config = [
        blind_signing_enabled,
        major,
        minor,
        patch,
        (MAX_RAW_DATA_LEN >> 8) as u8,
        (MAX_RAW_DATA_LEN & 0xFF) as u8,
    ];

    comm.append(&config);
    Ok(())
}
