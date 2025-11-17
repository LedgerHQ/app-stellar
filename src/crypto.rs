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

use crate::{bip32::Bip32Path, sw::AppSW};
use ledger_device_sdk::{
    ecc::{ECPublicKey, Ed25519},
    hash::{sha2::Sha2_256, HashInit},
};

const STELLAR_COMPRESSED_PK_LEN: usize = 32;
const LEDGER_UNCOMPRESSED_PK_LEN: usize = 65;
const HASH_LEN: usize = 32;
const SIGNATURE_LEN: usize = 64;

/// Derives the Ed25519 public key for a given BIP32 path and converts it to Stellar format.
///
/// This function derives a private key using SLIP-10 derivation from the provided BIP32 path,
/// computes its corresponding public key, and converts it to Stellar's 32-byte compressed format.
///
/// # Arguments
///
/// * `path` - The BIP32 derivation path (typically 44'/148'/account' for Stellar)
///
/// # Returns
///
/// * `Ok([u8; 32])` - The 32-byte Stellar-formatted public key on success
/// * `Err(AppSW::KeyDeriveFail)` - If key derivation fails
///
/// # Security
///
/// The private key is derived from the device's secure element and is never stored in memory.
/// It exists only temporarily during the execution of this function.
pub fn get_public_key(path: &Bip32Path) -> Result<[u8; STELLAR_COMPRESSED_PK_LEN], AppSW> {
    let private_key = Ed25519::derive_from_path_slip10(path.as_ref());
    let public_key = private_key.public_key().map_err(|_| AppSW::KeyDeriveFail)?;
    Ok(convert_to_stellar_pk(&public_key))
}

/// Computes the SHA-256 hash of the provided data.
///
/// This function uses the hardware-accelerated SHA-256 implementation from the Ledger SDK
/// to compute a cryptographic hash of the input data.
///
/// # Arguments
///
/// * `data` - The byte slice to hash
///
/// # Returns
///
/// * `Ok([u8; 32])` - The 32-byte SHA-256 hash on success
/// * `Err(AppSW::DataHashFail)` - If the hashing operation fails
///
/// # Example Use Cases
///
/// - Hashing transaction data before signing
/// - Computing message digests for signature verification
pub fn hash(data: &[u8]) -> Result<[u8; HASH_LEN], AppSW> {
    let mut hash: [u8; HASH_LEN] = [0u8; HASH_LEN];
    Sha2_256::new()
        .hash(data, &mut hash)
        .map_err(|_| AppSW::DataHashFail)?;
    Ok(hash)
}

/// Signs data using Ed25519 with a key derived from the given BIP32 path.
///
/// This function derives a private key using SLIP-10 derivation from the provided BIP32 path
/// and uses it to create an Ed25519 signature over the input data (typically a hash).
///
/// # Arguments
///
/// * `data` - The data to sign (typically a 32-byte hash of the actual message/transaction)
/// * `path` - The BIP32 derivation path (typically 44'/148'/account' for Stellar)
///
/// # Returns
///
/// * `Ok([u8; 64])` - The 64-byte Ed25519 signature on success
/// * `Err(AppSW::DataSignFail)` - If the signing operation fails
pub fn sign(data: &[u8], path: &Bip32Path) -> Result<[u8; SIGNATURE_LEN], AppSW> {
    let private_key = Ed25519::derive_from_path_slip10(path.as_ref());
    let (signature, _) = private_key.sign(data).map_err(|_| AppSW::DataSignFail)?;
    Ok(signature)
}

/// Converts an Ed25519 public key from Ledger's uncompressed format to Stellar's compressed format.
///
/// # Arguments
///
/// * `input` - A 65-byte Ed25519 public key in big-endian uncompressed format from the Ledger SDK
///
/// # Returns
///
/// * `[u8; 32]` - The 32-byte Stellar-formatted public key
pub fn convert_to_stellar_pk(
    input: &ECPublicKey<LEDGER_UNCOMPRESSED_PK_LEN, 'E'>,
) -> [u8; STELLAR_COMPRESSED_PK_LEN] {
    let mut out = [0u8; STELLAR_COMPRESSED_PK_LEN];

    for (i, out_byte) in out.iter_mut().enumerate() {
        *out_byte = input.pubkey[LEDGER_UNCOMPRESSED_PK_LEN - 1 - i];
    }

    if (input.pubkey[STELLAR_COMPRESSED_PK_LEN] & 1) != 0 {
        out[STELLAR_COMPRESSED_PK_LEN - 1] |= 0x80;
    }

    out
}
