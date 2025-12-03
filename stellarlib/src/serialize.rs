//! Serde serialization support for Stellar XDR types

extern crate alloc;

use crate::{
    display::format_duration,
    display::format_number_with_commas,
    display::format_unix_timestamp,
    parser::{ClaimPredicate, Claimant, ClaimantV0, SCError, SCErrorCode, ScVal},
};
use alloc::format;
use alloc::string::ToString;
use alloc::vec::Vec;
use serde::ser::{Serialize, SerializeMap, SerializeSeq, Serializer};

impl Serialize for SCErrorCode {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
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
        serializer.serialize_str(code_str)
    }
}

impl Serialize for SCError {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        let mut map = serializer.serialize_map(Some(2))?;

        match self {
            SCError::SceContract(contract_code) => {
                map.serialize_entry("error_type", "Contract")?;
                map.serialize_entry("error_value", contract_code)?;
            }
            SCError::SceWasmVm(code) => {
                map.serialize_entry("error_type", "WasmVm")?;
                map.serialize_entry("error_value", code)?;
            }
            SCError::SceContext(code) => {
                map.serialize_entry("error_type", "Context")?;
                map.serialize_entry("error_value", code)?;
            }
            SCError::SceStorage(code) => {
                map.serialize_entry("error_type", "Storage")?;
                map.serialize_entry("error_value", code)?;
            }
            SCError::SceObject(code) => {
                map.serialize_entry("error_type", "Object")?;
                map.serialize_entry("error_value", code)?;
            }
            SCError::SceCrypto(code) => {
                map.serialize_entry("error_type", "Crypto")?;
                map.serialize_entry("error_value", code)?;
            }
            SCError::SceEvents(code) => {
                map.serialize_entry("error_type", "Events")?;
                map.serialize_entry("error_value", code)?;
            }
            SCError::SceBudget(code) => {
                map.serialize_entry("error_type", "Budget")?;
                map.serialize_entry("error_value", code)?;
            }
            SCError::SceValue(code) => {
                map.serialize_entry("error_type", "Value")?;
                map.serialize_entry("error_value", code)?;
            }
            SCError::SceAuth(code) => {
                map.serialize_entry("error_type", "Auth")?;
                map.serialize_entry("error_value", code)?;
            }
        }

        map.end()
    }
}

// Helper to convert ScVal to string representation for use as JSON map keys
// This is needed because JSON only supports string keys, and we want to
// maintain the same formatting as the value serialization
pub fn scval_to_key_string<'a>(val: &ScVal<'a>) -> alloc::string::String {
    // For most types, we can use serde_json to get the serialized form
    // and then strip the quotes if it's a string
    match serde_json::to_value(val) {
        Ok(json_val) => match json_val {
            serde_json::Value::String(s) => s,
            serde_json::Value::Bool(b) => b.to_string(),
            serde_json::Value::Null => "[void]".to_string(),
            serde_json::Value::Number(_) => {
                // Numbers lose formatting in serde_json::Value, so we handle them specially
                // This should not happen as our Serialize impl outputs strings for numbers
                serde_json::to_string(&json_val)
                    .unwrap_or_else(|_| "[unserializable data]".to_string())
            }
            serde_json::Value::Array(_) | serde_json::Value::Object(_) => {
                // Complex types get JSON string representation
                serde_json::to_string(&json_val)
                    .unwrap_or_else(|_| "[unserializable data]".to_string())
            }
        },
        Err(e) => format!("[serialization_error: {}]", e),
    }
}

impl<'a> Serialize for ScVal<'a> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        match self {
            ScVal::Bool(b) => b.serialize(serializer),
            ScVal::Void => serializer.serialize_unit(),
            ScVal::Error(e) => e.serialize(serializer),
            ScVal::U32(v) => format_number_with_commas(&v.to_string()).serialize(serializer),
            ScVal::I32(v) => format_number_with_commas(&v.to_string()).serialize(serializer),
            ScVal::U64(v) => format_number_with_commas(&v.to_string()).serialize(serializer),
            ScVal::I64(v) => format_number_with_commas(&v.to_string()).serialize(serializer),
            ScVal::Timepoint(v) => format_unix_timestamp(*v).serialize(serializer),
            ScVal::Duration(v) => format_duration(*v).serialize(serializer),
            ScVal::U128(v) => format_number_with_commas(&v.to_string()).serialize(serializer),
            ScVal::I128(v) => format_number_with_commas(&v.to_string()).serialize(serializer),
            ScVal::U256(v) => format_number_with_commas(&v.to_string()).serialize(serializer),
            ScVal::I256(v) => format_number_with_commas(&v.to_string()).serialize(serializer),
            ScVal::Bytes(b) => b.to_string().serialize(serializer),
            ScVal::String(s) => s.to_string().serialize(serializer),
            ScVal::Symbol(s) => s.to_string().serialize(serializer),
            ScVal::Vec(opt_vec) => match opt_vec {
                Some(vec) => {
                    let mut seq = serializer.serialize_seq(Some(vec.len()))?;
                    for item in vec.iter() {
                        seq.serialize_element(item)?;
                    }
                    seq.end()
                }
                None => serializer.serialize_none(),
            },
            ScVal::Map(opt_map) => match opt_map {
                Some(map) => {
                    let mut map_ser = serializer.serialize_map(Some(map.len()))?;
                    for entry in map.iter() {
                        // Convert key to string for JSON compatibility
                        let key_str = scval_to_key_string(&entry.key);
                        map_ser.serialize_entry(&key_str, &entry.val)?;
                    }
                    map_ser.end()
                }
                None => serializer.serialize_none(),
            },
            ScVal::Address(addr) => addr.to_string().serialize(serializer),
            // The following types are actually not used
            ScVal::ContractInstance(_) => "[ContractInstance]".serialize(serializer),
            ScVal::LedgerKeyContractInstance => "[LedgerKeyContractInstance]".serialize(serializer),
            ScVal::LedgerKeyNonce(_) => "[LedgerKeyNonce]".serialize(serializer),
        }
    }
}

impl Serialize for ClaimPredicate {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        match self {
            ClaimPredicate::Unconditional => serializer.serialize_str("unconditional"),
            ClaimPredicate::And(predicates) => {
                let mut map = serializer.serialize_map(Some(1))?;
                let vec: Vec<&ClaimPredicate> = predicates.as_slice().iter().collect();
                map.serialize_entry("and", &vec)?;
                map.end()
            }
            ClaimPredicate::Or(predicates) => {
                let mut map = serializer.serialize_map(Some(1))?;
                // Serialize the inner predicates as an array
                let vec: Vec<&ClaimPredicate> = predicates.as_slice().iter().collect();
                map.serialize_entry("or", &vec)?;
                map.end()
            }
            ClaimPredicate::Not(predicate) => {
                let mut map = serializer.serialize_map(Some(1))?;
                map.serialize_entry("not", predicate.as_ref())?;
                map.end()
            }
            ClaimPredicate::BeforeAbsoluteTime(timestamp) => {
                let mut map = serializer.serialize_map(Some(1))?;
                map.serialize_entry("before_absolute_time", &timestamp.to_string())?;
                map.end()
            }
            ClaimPredicate::BeforeRelativeTime(seconds) => {
                let mut map = serializer.serialize_map(Some(1))?;
                map.serialize_entry("before_relative_time", &seconds.to_string())?;
                map.end()
            }
        }
    }
}

impl<'a> Serialize for ClaimantV0<'a> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        let mut map = serializer.serialize_map(Some(2))?;
        map.serialize_entry("destination", &self.destination.to_string())?;
        map.serialize_entry("predicate", &self.predicate)?;
        map.end()
    }
}

impl<'a> Serialize for Claimant<'a> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        match self {
            Claimant::V0(claimant_v0) => claimant_v0.serialize(serializer),
        }
    }
}
