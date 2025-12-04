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

use ledger_device_sdk::include_gif;
use ledger_device_sdk::nbgl::NbglGlyph;

#[cfg(target_os = "apex_p")]
pub const STELLAR: NbglGlyph =
    NbglGlyph::from_include(include_gif!("glyphs/stellar_48x48.png", NBGL));

#[cfg(any(target_os = "stax", target_os = "flex"))]
pub const STELLAR: NbglGlyph =
    NbglGlyph::from_include(include_gif!("glyphs/stellar_64x64.gif", NBGL));

#[cfg(any(target_os = "nanosplus", target_os = "nanox"))]
pub const STELLAR: NbglGlyph =
    NbglGlyph::from_include(include_gif!("glyphs/stellar_14x14.gif", NBGL));
