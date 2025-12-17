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

use crate::sw::AppSW;

pub const ALLOWED_PATH_LEN: usize = 3;
const BYTES_PER_SEGMENT: usize = 4;
const SERIALIZED_SIZE: usize = 1 + ALLOWED_PATH_LEN * BYTES_PER_SEGMENT;

/// BIP32 path with fixed size to avoid BSS allocation.
/// Stellar always uses exactly 3 segments: 44'/148'/account'
#[derive(Default)]
pub struct Bip32Path([u32; ALLOWED_PATH_LEN]);

impl AsRef<[u32]> for Bip32Path {
    fn as_ref(&self) -> &[u32] {
        &self.0
    }
}

impl Bip32Path {
    /// Parses a BIP32 path from raw bytes without a length prefix.
    ///
    /// # Arguments
    ///
    /// * `data` - Raw BIP32 path data. Must be at least `ALLOWED_PATH_LEN * BYTES_PER_SEGMENT` bytes (12 bytes).
    ///            Only the first 12 bytes will be used.
    ///
    /// # Returns
    ///
    /// Returns `Ok(Bip32Path)` if parsing succeeds, or `Err(AppSW::DataParsingFail)` if the
    /// data length is insufficient.
    pub fn parse(data: &[u8]) -> Result<Self, AppSW> {
        // Check that data length is at least the expected size
        if data.len() < ALLOWED_PATH_LEN * BYTES_PER_SEGMENT {
            return Err(AppSW::DataParsingFail);
        }

        let mut path = [0u32; ALLOWED_PATH_LEN];
        for (i, element) in path.iter_mut().enumerate() {
            let start = i * BYTES_PER_SEGMENT;
            let end = start + BYTES_PER_SEGMENT;
            // We've already validated the length, so this conversion is safe
            let bytes: [u8; BYTES_PER_SEGMENT] = data[start..end].try_into().unwrap();
            *element = u32::from_be_bytes(bytes);
        }

        Ok(Bip32Path(path))
    }

    /// Returns the number of bytes this BIP32 path consumes in its serialized form.
    /// This includes the length byte plus BYTES_PER_SEGMENT bytes per path segment.
    pub const fn serialized_size(&self) -> usize {
        SERIALIZED_SIZE
    }
}

impl TryFrom<&[u8]> for Bip32Path {
    type Error = AppSW;

    /// Constructs a [`Bip32Path`] from a given byte array.
    ///
    /// This method will return an error in the following cases:
    /// - the input array is empty,
    /// - the path length is not exactly 3 segments,
    /// - the total data length is insufficient.
    ///
    /// # Arguments
    ///
    /// * `data` - Encoded BIP32 path. First byte is the length of the path, as encoded by ragger.
    fn try_from(data: &[u8]) -> Result<Self, Self::Error> {
        // At least the length byte is required
        if data.is_empty() {
            return Err(AppSW::WrongApduLength);
        }

        // Check path length limit
        if data[0] != ALLOWED_PATH_LEN as u8 {
            return Err(AppSW::WrongApduLength);
        }

        if data.len() < SERIALIZED_SIZE {
            return Err(AppSW::WrongApduLength);
        }

        // Skip the length byte and parse the remaining data
        Self::parse(&data[1..])
    }
}
