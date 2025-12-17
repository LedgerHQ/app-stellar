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

use ledger_device_sdk::io::Reply;

/// Application status words for Stellar.
#[repr(u16)]
#[derive(Clone, Copy, PartialEq, Debug)]
pub enum AppSW {
    Deny = 0x6985,
    KeyDeriveFail = 0xB001,
    RequestDataTooLarge = 0xB004,
    DataParsingFail = 0xB005,
    DataHashFail = 0xB006,
    DataSignFail = 0xB008,
    SwapCheckFail = 0xB009,
    DataFormattingFail = 0x6125,
    WrongApduLength = 0x6A87,
    WrongP1P2 = 0x6B00,
    InsNotSupported = 0x6D00,
    BlindSigningModeNotEnabled = 0x6C66,
    Ok = 0x9000,
}

impl From<AppSW> for Reply {
    fn from(sw: AppSW) -> Reply {
        Reply(sw as u16)
    }
}
