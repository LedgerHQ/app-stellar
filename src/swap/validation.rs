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

use crate::{
    bip32::{Bip32Path, ALLOWED_PATH_LEN},
    crypto::get_public_key,
};
use alloc::string::ToString;
use ledger_device_sdk::libcall::swap::{CheckAddressParams, CreateTxParams};
use stellarlib::{
    parser::{Parser, TransactionSignaturePayload},
    Operation, TaggedTransaction, XdrParse, PUBLIC_NETWORK_HASH,
};
use swap_utils::format_stellar_address;

/// Validate that a derived address matches the reference address
pub fn check_address(params: &CheckAddressParams) -> Result<(), &'static str> {
    // Validate path length (Stellar uses 3 segments: 44'/148'/account')
    if params.dpath_len != ALLOWED_PATH_LEN {
        return Err("Invalid derivation path length");
    }

    // Parse and derive the public key
    let path = Bip32Path::parse(&params.dpath).map_err(|_| "Derivation path failure")?;
    let public_key = get_public_key(&path).map_err(|_| "Key derivation failure")?;

    // Format the address on the stack to avoid BSS writes
    const ADDRESS_BUFFER_SIZE: usize = 64; // Stellar addresses are 56 chars
    let mut address_buffer = [0u8; ADDRESS_BUFFER_SIZE];

    let address_len = format_stellar_address(&public_key, &mut address_buffer)
        .map_err(|_| "Address formatting failure")?;

    let address_str = core::str::from_utf8(&address_buffer[..address_len])
        .map_err(|_| "Address encoding failure")?;

    // Compare with reference address
    let ref_address = core::str::from_utf8(&params.ref_address[..params.ref_address_len])
        .map_err(|_| "Invalid UTF-8 in reference address")?;

    if address_str == ref_address {
        Ok(())
    } else {
        Err("Address mismatch")
    }
}

/// Validate a swap transaction against expected parameters
///
/// This function performs comprehensive validation to ensure:
/// - The transaction is on the public network
/// - The source account matches the signer
/// - Contains exactly one payment operation
/// - Payment is in native XLM
/// - Amount, fee, and destination match expected values
/// - Memo matches the extra ID
pub fn validate_swap_transaction(
    raw_data: &[u8],
    tx_params: &CreateTxParams,
    signer: &str,
) -> Result<(), &'static str> {
    // Parse the transaction envelope
    let mut parser = Parser::new(raw_data);
    let payload = TransactionSignaturePayload::parse(&mut parser)
        .map_err(|_| "Failed to parse transaction payload")?;

    // Validate network
    validate_network(&payload)?;

    // Extract and validate the transaction
    let envelope = extract_transaction(payload)?;

    if envelope.source_account.to_string() != signer {
        return Err("Transaction source account does not match signer");
    }

    // Validate memo
    validate_memo(&envelope, tx_params)?;

    // Validate single operation
    if envelope.op_count != 1 {
        return Err("Swap transaction must contain exactly one operation");
    }

    // Parse and validate the payment operation
    let operation =
        Operation::parse(&mut parser).map_err(|_| "Failed to parse payment operation")?;

    validate_payment_operation(operation, tx_params, signer)?;

    // Validate transaction fee
    validate_fee(&envelope, tx_params)?;

    Ok(())
}

/// Validate the network ID
fn validate_network(payload: &TransactionSignaturePayload) -> Result<(), &'static str> {
    if payload.network_id.as_bytes() != &PUBLIC_NETWORK_HASH {
        return Err("Unexpected network ID for swap transaction");
    }
    Ok(())
}

/// Extract a regular transaction
fn extract_transaction(
    payload: TransactionSignaturePayload,
) -> Result<stellarlib::Transaction, &'static str> {
    match payload.tagged_transaction {
        TaggedTransaction::EnvelopeTypeTx(transaction) => Ok(transaction),
        _ => Err("Swap transaction must be a regular transaction"),
    }
}

/// Validate the memo matches the extra ID
fn validate_memo(
    envelope: &stellarlib::Transaction,
    tx_params: &CreateTxParams,
) -> Result<(), &'static str> {
    let memo_text = match &envelope.memo {
        stellarlib::Memo::Text(memo) => memo.to_string(),
        _ => return Err("Swap transaction memo must be text"),
    };

    let extra_id = utf8_from_prefix(
        &tx_params.dest_address_extra_id,
        tx_params.dest_address_extra_id_len,
    )?;

    if memo_text != extra_id {
        return Err("Memo does not match provided extra ID");
    }

    Ok(())
}

/// Validate the payment operation parameters
fn validate_payment_operation(
    operation: Operation,
    tx_params: &CreateTxParams,
    signer: &str,
) -> Result<(), &'static str> {
    if let Some(source) = operation.source_account.as_ref() {
        if source.to_string() != signer {
            return Err("Operation source account does not match signer");
        }
    }

    // Extract payment operation
    let payment_op = match operation.body {
        stellarlib::OperationBody::Payment(payment) => payment,
        _ => return Err("Swap transaction operation is not a payment"),
    };

    // Validate native asset
    if !payment_op.asset.is_native() {
        return Err("Swap payment asset must be native XLM");
    }

    // Validate amount
    validate_amount(&payment_op, tx_params)?;

    // Validate destination
    validate_destination(&payment_op, tx_params)?;

    Ok(())
}

/// Validate the payment amount matches expected value
fn validate_amount(
    payment_op: &stellarlib::PaymentOp,
    tx_params: &CreateTxParams,
) -> Result<(), &'static str> {
    let expected_amount = bytes_to_u64_be(&tx_params.amount, tx_params.amount_len)?;

    let on_chain_amount: u64 = payment_op
        .amount
        .try_into()
        .map_err(|_| "Payment amount is negative or invalid")?;

    if on_chain_amount != expected_amount {
        return Err("Amount mismatch between on-chain payment and expected amount");
    }

    Ok(())
}

/// Validate the destination address matches expected value
fn validate_destination(
    payment_op: &stellarlib::PaymentOp,
    tx_params: &CreateTxParams,
) -> Result<(), &'static str> {
    let expected_dest = utf8_from_prefix(&tx_params.dest_address, tx_params.dest_address_len)?;

    if payment_op.destination.to_string() != expected_dest {
        return Err("Destination address mismatch");
    }

    Ok(())
}

/// Validate the transaction fee matches expected value
fn validate_fee(
    envelope: &stellarlib::Transaction,
    tx_params: &CreateTxParams,
) -> Result<(), &'static str> {
    let expected_fee = bytes_to_u64_be(&tx_params.fee_amount, tx_params.fee_amount_len)?;

    let expected_fee_u32: u32 = expected_fee
        .try_into()
        .map_err(|_| "Expected fee does not fit in u32")?;

    if envelope.fee != expected_fee_u32 {
        return Err("Fee mismatch between transaction and expected amount");
    }

    Ok(())
}

/// Convert variable-length big-endian bytes to u64
fn bytes_to_u64_be(bytes: &[u8], len: usize) -> Result<u64, &'static str> {
    if len == 0 || len > 8 || len > bytes.len() {
        return Err("Invalid integer length");
    }

    // The valid data is at the end of the array (right-aligned)
    let start_idx = bytes.len() - len;
    let mut buf = [0u8; 8];
    buf[8 - len..].copy_from_slice(&bytes[start_idx..]);

    Ok(u64::from_be_bytes(buf))
}

/// Extract a UTF-8 string from the first `len` bytes of a buffer
fn utf8_from_prefix(bytes: &[u8], len: usize) -> Result<&str, &'static str> {
    if len > bytes.len() {
        return Err("Invalid UTF-8 length");
    }
    core::str::from_utf8(&bytes[..len]).map_err(|_| "Invalid UTF-8 data")
}
