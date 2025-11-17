//! A `no_std` Rust library for parsing, formatting, and displaying Stellar blockchain data structures.

#![cfg_attr(not(test), no_std)]

extern crate alloc;

pub mod display;
pub mod formatter;
pub mod token_formatter;
pub mod tokens;

pub mod parser;
pub mod serialize;

pub use formatter::{
    format_hash_id_preimage_soroban_authorization, format_operation,
    format_transaction_signature_payload, get_operation_intent, DataEntry, FormatConfig,
    FormatError,
};
pub use parser::*;
pub use token_formatter::try_format_token_contract_call;
pub use tokens::{get_token_info, TokenInfo, KNOWN_TOKENS};
