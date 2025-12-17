//! Token contract call formatting utilities
//!
//! This module provides specialized formatting for known token contract calls,
//! displaying token symbols and properly formatted amounts with decimals.

extern crate alloc;

use crate::display::{format_number_with_commas, format_token_amount};
use crate::formatter::DataEntry;
use crate::parser::{InvokeContractArgs, ScAddress, ScVal, Uint256};
use crate::serialize::scval_to_key_string;
use crate::tokens::{get_token_info, TokenInfo};
use alloc::format;
use alloc::string::{String, ToString};
use alloc::vec::Vec;

/// Attempts to format a contract call as a known token contract interaction
///
/// Returns Some(entries) if the contract is a known token and the function is recognized,
/// None otherwise to fall back to default formatting.
pub fn try_format_token_contract_call(args: &InvokeContractArgs) -> Option<Vec<DataEntry>> {
    // Extract contract address bytes
    let contract_bytes = match &args.contract_address {
        ScAddress::ScAddressTypeContract(contract_id) => {
            let Uint256(bytes) = contract_id;
            bytes
        }
        _ => return None,
    };

    // Check if this is a known token contract
    let token_info = get_token_info(contract_bytes)?;

    // Format based on token function type
    match args.function_name.to_string().as_str() {
        "approve" => format_token_approve(args, token_info),
        "transfer" => format_token_transfer(args, token_info),
        "transfer_from" => format_token_transfer_from(args, token_info),
        "burn" => format_token_burn(args, token_info),
        "burn_from" => format_token_burn_from(args, token_info),
        _ => None, // Unknown function, use default formatting
    }
}

// Helper macro to validate argument count
macro_rules! validate_args {
    ($args:expr, $expected:expr) => {
        if $args.args.len() != $expected {
            return None;
        }
    };
}

/// Creates a formatted token operation entry
fn create_token_operation_entry(
    operation: &str,
    amount: i128,
    token_info: &TokenInfo,
    contract_address: &ScAddress,
) -> DataEntry {
    let formatted_amount = format_token_amount(amount, token_info.decimals);
    DataEntry::new(
        operation,
        format!(
            "{} {}@{}",
            formatted_amount, token_info.symbol, contract_address
        ),
    )
}

/// Helper struct to build token operation entries
struct TokenOperationBuilder<'a> {
    entries: Vec<DataEntry>,
    args: &'a [ScVal<'a>],
}

impl<'a> TokenOperationBuilder<'a> {
    fn new(args: &'a [ScVal<'a>]) -> Self {
        Self {
            entries: Vec::new(),
            args,
        }
    }

    fn add_operation(
        mut self,
        operation: &str,
        amount_index: usize,
        token_info: &TokenInfo,
        contract_address: &ScAddress,
    ) -> Option<Self> {
        let amount = extract_i128(&self.args[amount_index])?;
        self.entries.push(create_token_operation_entry(
            operation,
            amount,
            token_info,
            contract_address,
        ));
        Some(self)
    }

    fn add_address(mut self, label: &str, index: usize) -> Option<Self> {
        let address = extract_address(&self.args[index])?;
        self.entries.push(DataEntry::new(label, address));
        Some(self)
    }

    fn add_u32(mut self, label: &str, index: usize) -> Option<Self> {
        let value = extract_u32(&self.args[index])?;
        let num = format_number_with_commas(value.to_string().as_str());
        self.entries.push(DataEntry::new(label, num));
        Some(self)
    }

    fn build(self) -> Vec<DataEntry> {
        self.entries
    }
}

/// Formats a token transfer call: transfer(from: Address, to: Address, amount: i128)
fn format_token_transfer(
    args: &InvokeContractArgs,
    token_info: &TokenInfo,
) -> Option<Vec<DataEntry>> {
    validate_args!(args, 3);

    Some(
        TokenOperationBuilder::new(args.args.as_slice())
            .add_operation("Transfer", 2, token_info, &args.contract_address)?
            .add_address("From", 0)?
            .add_address("To", 1)?
            .build(),
    )
}

/// Formats a token approve call: approve(from: Address, spender: Address, amount: i128, expiration_ledger: u32)
fn format_token_approve(
    args: &InvokeContractArgs,
    token_info: &TokenInfo,
) -> Option<Vec<DataEntry>> {
    validate_args!(args, 4);

    Some(
        TokenOperationBuilder::new(args.args.as_slice())
            .add_operation("Approve", 2, token_info, &args.contract_address)?
            .add_address("From", 0)?
            .add_address("Spender", 1)?
            .add_u32("Exp Ledger", 3)?
            .build(),
    )
}

/// Formats a token transfer_from call: transfer_from(spender: Address, from: Address, to: Address, amount: i128)
fn format_token_transfer_from(
    args: &InvokeContractArgs,
    token_info: &TokenInfo,
) -> Option<Vec<DataEntry>> {
    validate_args!(args, 4);

    Some(
        TokenOperationBuilder::new(args.args.as_slice())
            .add_operation("Transfer", 3, token_info, &args.contract_address)?
            .add_address("From", 1)?
            .add_address("To", 2)?
            .add_address("Spender", 0)?
            .build(),
    )
}

/// Formats a token burn call: burn(from: Address, amount: i128)
fn format_token_burn(args: &InvokeContractArgs, token_info: &TokenInfo) -> Option<Vec<DataEntry>> {
    validate_args!(args, 2);

    Some(
        TokenOperationBuilder::new(args.args.as_slice())
            .add_operation("Burn", 1, token_info, &args.contract_address)?
            .add_address("From", 0)?
            .build(),
    )
}

/// Formats a token burn_from call: burn_from(spender: Address, from: Address, amount: i128)
fn format_token_burn_from(
    args: &InvokeContractArgs,
    token_info: &TokenInfo,
) -> Option<Vec<DataEntry>> {
    validate_args!(args, 3);

    Some(
        TokenOperationBuilder::new(args.args.as_slice())
            .add_operation("Burn", 2, token_info, &args.contract_address)?
            .add_address("From", 1)?
            .add_address("Spender", 0)?
            .build(),
    )
}

// Type extraction helpers - shortened names for internal use
fn extract_i128(scval: &ScVal) -> Option<i128> {
    match scval {
        ScVal::I128(parts) => {
            let hi = parts.hi as i128;
            let lo = parts.lo as i128;
            Some((hi << 64) | (lo & 0xFFFF_FFFF_FFFF_FFFF))
        }
        _ => None,
    }
}

fn extract_address(scval: &ScVal) -> Option<String> {
    match scval {
        ScVal::Address(_) => Some(scval_to_key_string(scval)),
        _ => None,
    }
}

fn extract_u32(scval: &ScVal) -> Option<u32> {
    match scval {
        ScVal::U32(value) => Some(*value),
        _ => None,
    }
}
