//! Manual JSON serialization for Stellar XDR types
//!
//! This module provides lightweight JSON serialization without serde_json dependency.

extern crate alloc;

use crate::{
    display::format_duration,
    display::format_number_with_commas,
    display::format_unix_timestamp,
    parser::{ClaimPredicate, Claimant, ClaimantV0, SCError, SCErrorCode, ScVal},
};
use alloc::format;
use alloc::string::{String, ToString};
use alloc::vec::Vec;

/// Trait for types that can be serialized to JSON string
pub trait ToJson {
    fn to_json(&self) -> String;
}

/// Escape a string for JSON output
fn escape_json_string(s: &str) -> String {
    let mut result = String::with_capacity(s.len() + 2);
    result.push('"');
    for c in s.chars() {
        match c {
            '"' => result.push_str("\\\""),
            '\\' => result.push_str("\\\\"),
            '\n' => result.push_str("\\n"),
            '\r' => result.push_str("\\r"),
            '\t' => result.push_str("\\t"),
            c if c.is_control() => {
                result.push_str(&format!("\\u{:04x}", c as u32));
            }
            c => result.push(c),
        }
    }
    result.push('"');
    result
}

impl ToJson for SCErrorCode {
    fn to_json(&self) -> String {
        let code_str = match self {
            SCErrorCode::ScecArithDomain => "ArithDomain",
            SCErrorCode::ScecIndexBounds => "IndexBounds",
            SCErrorCode::ScecInvalidInput => "InvalidInput",
            SCErrorCode::ScecMissingValue => "MissingValue",
            SCErrorCode::ScecExistingValue => "ExistingValue",
            SCErrorCode::ScecExceededLimit => "ExceededLimit",
            SCErrorCode::ScecInvalidAction => "InvalidAction",
            SCErrorCode::ScecInternalError => "InternalError",
            SCErrorCode::ScecUnexpectedType => "UnexpectedType",
            SCErrorCode::ScecUnexpectedSize => "UnexpectedSize",
        };
        escape_json_string(code_str)
    }
}

impl ToJson for SCError {
    fn to_json(&self) -> String {
        let (error_type, error_value) = match self {
            SCError::SceContract(code) => ("Contract", code.to_string()),
            SCError::SceWasmVm(code) => ("WasmVm", code.to_json()),
            SCError::SceContext(code) => ("Context", code.to_json()),
            SCError::SceStorage(code) => ("Storage", code.to_json()),
            SCError::SceObject(code) => ("Object", code.to_json()),
            SCError::SceCrypto(code) => ("Crypto", code.to_json()),
            SCError::SceEvents(code) => ("Events", code.to_json()),
            SCError::SceBudget(code) => ("Budget", code.to_json()),
            SCError::SceValue(code) => ("Value", code.to_json()),
            SCError::SceAuth(code) => ("Auth", code.to_json()),
        };

        format!(
            "{{\"error_type\":{},\"error_value\":{}}}",
            escape_json_string(error_type),
            error_value
        )
    }
}

/// Convert ScVal to string representation for use as JSON map keys
pub fn scval_to_key_string<'a>(val: &ScVal<'a>) -> String {
    match val {
        ScVal::Bool(b) => b.to_string(),
        ScVal::Void => "[void]".to_string(),
        ScVal::Error(e) => e.to_json(),
        ScVal::U32(v) => format_number_with_commas(&v.to_string()),
        ScVal::I32(v) => format_number_with_commas(&v.to_string()),
        ScVal::U64(v) => format_number_with_commas(&v.to_string()),
        ScVal::I64(v) => format_number_with_commas(&v.to_string()),
        ScVal::Timepoint(v) => format_unix_timestamp(*v),
        ScVal::Duration(v) => format_duration(*v),
        ScVal::U128(v) => format_number_with_commas(&v.to_string()),
        ScVal::I128(v) => format_number_with_commas(&v.to_string()),
        ScVal::U256(v) => format_number_with_commas(&v.to_string()),
        ScVal::I256(v) => format_number_with_commas(&v.to_string()),
        ScVal::Bytes(b) => b.to_string(),
        ScVal::String(s) => s.to_string(),
        ScVal::Symbol(s) => s.to_string(),
        ScVal::Vec(_) | ScVal::Map(_) => val.to_json(),
        ScVal::Address(addr) => addr.to_string(),
        ScVal::ContractInstance(_) => "[ContractInstance]".to_string(),
        ScVal::LedgerKeyContractInstance => "[LedgerKeyContractInstance]".to_string(),
        ScVal::LedgerKeyNonce(_) => "[LedgerKeyNonce]".to_string(),
    }
}

impl<'a> ToJson for ScVal<'a> {
    fn to_json(&self) -> String {
        match self {
            ScVal::Bool(b) => {
                if *b {
                    "true".to_string()
                } else {
                    "false".to_string()
                }
            }
            ScVal::Void => "null".to_string(),
            ScVal::Error(e) => e.to_json(),
            ScVal::U32(v) => escape_json_string(&format_number_with_commas(&v.to_string())),
            ScVal::I32(v) => escape_json_string(&format_number_with_commas(&v.to_string())),
            ScVal::U64(v) => escape_json_string(&format_number_with_commas(&v.to_string())),
            ScVal::I64(v) => escape_json_string(&format_number_with_commas(&v.to_string())),
            ScVal::Timepoint(v) => escape_json_string(&format_unix_timestamp(*v)),
            ScVal::Duration(v) => escape_json_string(&format_duration(*v)),
            ScVal::U128(v) => escape_json_string(&format_number_with_commas(&v.to_string())),
            ScVal::I128(v) => escape_json_string(&format_number_with_commas(&v.to_string())),
            ScVal::U256(v) => escape_json_string(&format_number_with_commas(&v.to_string())),
            ScVal::I256(v) => escape_json_string(&format_number_with_commas(&v.to_string())),
            ScVal::Bytes(b) => escape_json_string(&b.to_string()),
            ScVal::String(s) => escape_json_string(&s.to_string()),
            ScVal::Symbol(s) => escape_json_string(&s.to_string()),
            ScVal::Vec(opt_vec) => match opt_vec {
                Some(vec) => {
                    let items: Vec<String> = vec.iter().map(|item| item.to_json()).collect();
                    format!("[{}]", items.join(","))
                }
                None => "null".to_string(),
            },
            ScVal::Map(opt_map) => match opt_map {
                Some(map) => {
                    let entries: Vec<String> = map
                        .iter()
                        .map(|entry| {
                            let key_str = scval_to_key_string(&entry.key);
                            format!("{}:{}", escape_json_string(&key_str), entry.val.to_json())
                        })
                        .collect();
                    format!("{{{}}}", entries.join(","))
                }
                None => "null".to_string(),
            },
            ScVal::Address(addr) => escape_json_string(&addr.to_string()),
            ScVal::ContractInstance(_) => escape_json_string("[ContractInstance]"),
            ScVal::LedgerKeyContractInstance => escape_json_string("[LedgerKeyContractInstance]"),
            ScVal::LedgerKeyNonce(_) => escape_json_string("[LedgerKeyNonce]"),
        }
    }
}

impl ToJson for ClaimPredicate {
    fn to_json(&self) -> String {
        match self {
            ClaimPredicate::Unconditional => escape_json_string("unconditional"),
            ClaimPredicate::And(predicates) => {
                let items: Vec<String> = predicates.as_slice().iter().map(|p| p.to_json()).collect();
                format!("{{\"and\":[{}]}}", items.join(","))
            }
            ClaimPredicate::Or(predicates) => {
                let items: Vec<String> = predicates.as_slice().iter().map(|p| p.to_json()).collect();
                format!("{{\"or\":[{}]}}", items.join(","))
            }
            ClaimPredicate::Not(predicate) => {
                format!("{{\"not\":{}}}", predicate.to_json())
            }
            ClaimPredicate::BeforeAbsoluteTime(timestamp) => {
                format!(
                    "{{\"before_absolute_time\":{}}}",
                    escape_json_string(&timestamp.to_string())
                )
            }
            ClaimPredicate::BeforeRelativeTime(seconds) => {
                format!(
                    "{{\"before_relative_time\":{}}}",
                    escape_json_string(&seconds.to_string())
                )
            }
        }
    }
}

impl<'a> ToJson for ClaimantV0<'a> {
    fn to_json(&self) -> String {
        format!(
            "{{\"destination\":{},\"predicate\":{}}}",
            escape_json_string(&self.destination.to_string()),
            self.predicate.to_json()
        )
    }
}

impl<'a> ToJson for Claimant<'a> {
    fn to_json(&self) -> String {
        match self {
            Claimant::V0(claimant_v0) => claimant_v0.to_json(),
        }
    }
}
