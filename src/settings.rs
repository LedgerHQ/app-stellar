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

use ledger_device_sdk::nbgl::SETTINGS_SIZE;
use ledger_device_sdk::nvm::*;
use ledger_device_sdk::NVMData;
use stellarlib::FormatConfig;

/// Identifiers for every user-visible setting toggle.
#[derive(Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum SettingId {
    BlindSigning = 0,
    TransactionSource = 1,
    SequenceAndNonce = 2,
    Precondition = 3,
    NestedAuthorization = 4,
}

impl SettingId {
    pub const fn as_index(self) -> usize {
        self as usize
    }

    /// Returns the name/description pair displayed in the settings UI.
    #[cfg(any(target_os = "nanosplus", target_os = "nanox"))]
    pub fn setting_info(self) -> (&'static str, &'static str) {
        match self {
            SettingId::BlindSigning => ("Blind Signing", "Allow blind signing"),
            SettingId::TransactionSource => ("Tx Source", "Always display tx source"),

            SettingId::SequenceAndNonce => {
                ("Sequence & Nonce", "Allow display of tx sequence and nonce")
            }
            SettingId::Precondition => ("Precondition", "Allow display of tx precondition"),
            SettingId::NestedAuthorization => {
                ("Nested Authz", "Allow display of nested authorizations")
            }
        }
    }

    #[cfg(not(any(target_os = "nanosplus", target_os = "nanox")))]
    pub fn setting_info(self) -> (&'static str, &'static str) {
        match self {
            SettingId::BlindSigning => ("Blind Signing", "Allow blind signing"),
            SettingId::TransactionSource => {
                ("Transaction Source", "Always display transaction source")
            }

            SettingId::SequenceAndNonce => (
                "Sequence & Nonce",
                "Allow display of transaction sequence and nonce",
            ),
            SettingId::Precondition => {
                ("Precondition", "Allow display of transaction precondition")
            }
            SettingId::NestedAuthorization => (
                "Nested Authorization",
                "Allow display of nested authorizations",
            ),
        }
    }
}

// Default values for settings (1 = enabled, 0 = disabled)
const DEFAULT_SETTINGS: [u8; SETTINGS_SIZE] = [
    0, // BlindSigning: disabled by default
    0, // TransactionSource: disabled by default
    0, // SequenceAndNonce: disabled by default
    0, // Precondition: disabled by default
    1, // Nested Authorization: enabled by default
    0, 0, 0, 0, 0, // Reserved for future settings
];

const SETTINGS_STORAGE_SIZE: usize = DEFAULT_SETTINGS.len();

const EXPOSED_SETTINGS: [SettingId; 5] = [
    SettingId::BlindSigning,
    SettingId::TransactionSource,
    SettingId::SequenceAndNonce,
    SettingId::Precondition,
    SettingId::NestedAuthorization,
];

const EXPOSED_SETTINGS_COUNT: usize = EXPOSED_SETTINGS.len();

#[link_section = ".nvm_data"]
static mut DATA: NVMData<AtomicStorage<[u8; SETTINGS_STORAGE_SIZE]>> =
    NVMData::new(AtomicStorage::new(&DEFAULT_SETTINGS));

/// Zero-sized handle that gives access to persisted app settings.
#[derive(Clone, Copy)]
pub struct Settings;

impl Default for Settings {
    fn default() -> Self {
        Settings
    }
}

impl Settings {
    #[inline(never)]
    /// Returns mutable access to the raw persisted settings storage.
    pub fn get_mut(&mut self) -> &mut AtomicStorage<[u8; SETTINGS_STORAGE_SIZE]> {
        let data = &raw mut DATA;
        unsafe { (*data).get_mut() }
    }

    #[inline(never)]
    /// Returns read-only access to the raw persisted settings storage.
    pub fn get_ref(&self) -> &AtomicStorage<[u8; SETTINGS_STORAGE_SIZE]> {
        let data = &raw const DATA;
        unsafe { (*data).get_ref() }
    }

    fn read_setting_byte(&self, setting: SettingId) -> u8 {
        self.get_ref().get_ref()[setting.as_index()]
    }

    #[allow(unused)]
    /// Persists a boolean value for the given setting.
    pub fn set_setting(&mut self, setting: SettingId, value: bool) {
        let storage = self.get_mut();
        let mut updated_data = *storage.get_ref();
        updated_data[setting.as_index()] = value as u8;
        unsafe { storage.update(&updated_data) };
    }

    /// Returns `true` if the given setting is enabled in persistent storage.
    pub fn get_setting(&self, setting: SettingId) -> bool {
        self.read_setting_byte(setting) != 0
    }

    pub fn is_blind_signing_enabled(&self) -> bool {
        self.get_setting(SettingId::BlindSigning)
    }

    pub fn is_show_transaction_source_if_matches_signer(&self) -> bool {
        self.get_setting(SettingId::TransactionSource)
    }

    pub fn is_show_sequence_and_nonce_enabled(&self) -> bool {
        self.get_setting(SettingId::SequenceAndNonce)
    }

    pub fn is_show_precondition_enabled(&self) -> bool {
        self.get_setting(SettingId::Precondition)
    }

    pub fn is_show_nested_authorization_enabled(&self) -> bool {
        self.get_setting(SettingId::NestedAuthorization)
    }

    pub fn to_format_config(self) -> FormatConfig {
        FormatConfig {
            show_sequence_and_nonce: self.is_show_sequence_and_nonce_enabled(),
            show_preconditions: self.is_show_precondition_enabled(),
            show_nested_authorization: self.is_show_nested_authorization_enabled(),
            show_tx_source_if_matches_signer: self.is_show_transaction_source_if_matches_signer(),
        }
    }

    /// Collects all settings labels/descriptions for use in the main menu.
    pub fn get_all_settings_info() -> [[&'static str; 2]; EXPOSED_SETTINGS_COUNT] {
        EXPOSED_SETTINGS.map(|setting| {
            let (name, desc) = setting.setting_info();
            [name, desc]
        })
    }
}
