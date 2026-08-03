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

use crate::bip32::Bip32Path;
use crate::sw::AppSW;
use core::ops::Deref;
use ledger_device_sdk::io::Comm;

#[cfg(target_os = "nanox")]
pub const MAX_RAW_DATA_LEN: usize = 1024 * 4;
#[cfg(not(target_os = "nanox"))]
pub const MAX_RAW_DATA_LEN: usize = 1024 * 8;

pub const SWAP_MAX_RAW_DATA_LEN: usize = 1024;

/// Identifies the signing instruction that owns an in-progress chunk stream.
///
/// Variant names intentionally mirror the APDU instruction names used throughout
/// the app, making mismatched-flow checks explicit at each handler call site.
#[allow(clippy::enum_variant_names)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub(crate) enum ActiveFlow {
    SignTx,
    SignSorobanAuth,
    SignMessage,
}

pub struct RawDataBuffer<const MAX: usize> {
    data: [u8; MAX],
    len: usize,
}

impl<const MAX: usize> RawDataBuffer<MAX> {
    pub const fn new() -> Self {
        Self {
            data: [0u8; MAX],
            len: 0,
        }
    }

    pub fn len(&self) -> usize {
        self.len
    }

    /// Safely extends the buffer with the given slice.
    ///
    /// # Arguments
    /// * `slice` - The data to append to the buffer
    ///
    /// # Returns
    /// * `Ok(())` if the data was successfully appended
    /// * `Err(AppSW::RequestDataTooLarge)` if appending would exceed the buffer capacity
    pub fn extend_from_slice(&mut self, slice: &[u8]) -> Result<(), AppSW> {
        if slice.is_empty() {
            return Ok(());
        }

        let new_len = self
            .len
            .checked_add(slice.len())
            .ok_or(AppSW::RequestDataTooLarge)?;

        if new_len > MAX {
            return Err(AppSW::RequestDataTooLarge);
        }

        self.data[self.len..new_len].copy_from_slice(slice);
        self.len = new_len;
        Ok(())
    }

    pub fn clear(&mut self) {
        self.len = 0;
    }

    pub fn as_slice(&self) -> &[u8] {
        &self.data[..self.len]
    }
}

impl<const MAX: usize> AsRef<[u8]> for RawDataBuffer<MAX> {
    fn as_ref(&self) -> &[u8] {
        self.as_slice()
    }
}

impl<const MAX: usize> Deref for RawDataBuffer<MAX> {
    type Target = [u8];

    fn deref(&self) -> &Self::Target {
        self.as_slice()
    }
}

impl<const MAX: usize> Default for RawDataBuffer<MAX> {
    fn default() -> Self {
        Self::new()
    }
}

#[derive(Default)]
pub struct AppContext<const MAX: usize> {
    pub path: Bip32Path,
    pub raw_data: RawDataBuffer<MAX>,
    pub review_finished: bool,
    /// The signing instruction that started the current chunk stream.
    /// Continuation chunks must belong to the same instruction.
    active_flow: Option<ActiveFlow>,
}

impl<const MAX: usize> AppContext<MAX> {
    pub fn new() -> Self {
        Self::default()
    }
}

impl<const MAX: usize> AppContext<MAX> {
    pub fn finished(&self) -> bool {
        self.review_finished
    }

    pub fn reset(&mut self) {
        self.raw_data.clear();
        self.path = Default::default();
        self.review_finished = false;
        self.active_flow = None;
    }

    /// Handles multi-chunk data reception for handlers that accumulate data across multiple APDUs.
    ///
    /// This method encapsulates the common pattern of:
    /// - Getting data from the communication channel
    /// - Resetting context on first chunk and parsing the path
    /// - Accumulating data across chunks with size validation
    ///
    /// # Arguments
    /// * `comm` - The communication channel for receiving APDU data
    /// * `flow` - Signing instruction receiving this chunk
    /// * `first` - Whether this is the first chunk of data
    ///
    /// # Returns
    /// * `Ok(())` if the chunk was successfully processed
    /// * `Err(AppSW)` if an error occurred (wrong length, data too large,
    ///   continuation chunk without a first chunk, etc.)
    pub(crate) fn handle_chunk(
        &mut self,
        comm: &mut Comm,
        flow: ActiveFlow,
        first: bool,
    ) -> Result<(), AppSW> {
        let data = comm.get_data().map_err(|_| AppSW::WrongApduLength)?;
        if first {
            // Reset context for new transaction
            self.reset();

            // Parse the derivation path from the beginning of the data
            self.path = data.try_into()?;

            // Extract remaining data after the path
            let remaining_data = data
                .get(self.path.serialized_size()..)
                .ok_or(AppSW::WrongApduLength)?;

            // extend_from_slice now handles the size check internally
            self.raw_data.extend_from_slice(remaining_data)?;

            self.active_flow = Some(flow);
        } else {
            // Reject both orphaned continuations and chunks that switch signing
            // instructions in the middle of a stream.
            if self.active_flow != Some(flow) {
                self.reset();
                return Err(AppSW::DataParsingFail);
            }

            // extend_from_slice now handles the size check internally
            self.raw_data.extend_from_slice(data)?;
        }

        Ok(())
    }
}
