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

use crate::icons;
use crate::settings::Settings;
use ledger_device_sdk::nbgl::NbglHomeAndSettings;

pub fn ui_menu_main() -> NbglHomeAndSettings {
    let settings_strings = Settings::get_all_settings_info();
    let mut settings = Settings;

    // Display the home screen.
    NbglHomeAndSettings::new()
        .glyph(&icons::STELLAR)
        .settings(settings.get_mut(), &settings_strings)
        .infos(
            "Stellar",
            env!("CARGO_PKG_VERSION"),
            env!("CARGO_PKG_AUTHORS"),
        )
}
