use stellarlib::parser::{
    BytesM, ClaimPredicate, ContractExecutable, Int128Parts, Int256Parts, PublicKey,
    SCContractInstance, SCError, SCErrorCode, ScAddress, ScMapEntry, ScVal, StringM, UInt128Parts,
    UInt256Parts, Uint256, VecM,
};
use stellarlib::serialize::scval_to_key_string;

// ============================================================================
// Basic Types Tests
// ============================================================================

#[test]
fn test_serialize_bool() {
    let val_true = ScVal::Bool(true);
    let json = serde_json::to_string(&val_true).unwrap();
    assert_eq!(json, "true");

    let val_false = ScVal::Bool(false);
    let json = serde_json::to_string(&val_false).unwrap();
    assert_eq!(json, "false");
}

#[test]
fn test_serialize_void() {
    let val = ScVal::Void;
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, "null");

    // Test as map key
    let key_str = scval_to_key_string(&val);
    assert_eq!(key_str, "[void]");
}

#[test]
fn test_serialize_u32() {
    // Small number
    let val = ScVal::U32(42);
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""42""#);

    // Large number with commas
    let val = ScVal::U32(1234567);
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""1,234,567""#);

    // Test as map key
    let key_str = scval_to_key_string(&ScVal::U32(1000000));
    assert_eq!(key_str, "1,000,000");
}

#[test]
fn test_serialize_i32() {
    // Positive
    let val = ScVal::I32(42);
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""42""#);

    // Negative with commas
    let val = ScVal::I32(-1234567);
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""-1,234,567""#);

    // Test as map key
    let key_str = scval_to_key_string(&ScVal::I32(-999999));
    assert_eq!(key_str, "-999,999");
}

#[test]
fn test_serialize_u64() {
    let val = ScVal::U64(1000000000);
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""1,000,000,000""#);

    let val = ScVal::U64(u64::MAX);
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""18,446,744,073,709,551,615""#);
}

#[test]
fn test_serialize_i64() {
    let val = ScVal::I64(-1000000000);
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""-1,000,000,000""#);

    let val = ScVal::I64(i64::MIN);
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""-9,223,372,036,854,775,808""#);
}

#[test]
fn test_serialize_timepoint() {
    // Unix timestamp for 2025-10-21 08:19:03 UTC
    let val = ScVal::Timepoint(1761034743);
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""2025-10-21 08:19:03 UTC""#);
}

#[test]
fn test_serialize_duration() {
    // 4 days, 5 hours, 41 minutes, 51 seconds
    let val = ScVal::Duration(366111);
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""4d 5h 41m 51s""#);
}

// ============================================================================
// Large Number Types Tests
// ============================================================================

#[test]
fn test_serialize_u128() {
    let val = ScVal::U128(UInt128Parts {
        hi: 0,
        lo: 123456789012345,
    });
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""123,456,789,012,345""#);

    // Large number
    let val = ScVal::U128(UInt128Parts { hi: 1, lo: 0 });
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""18,446,744,073,709,551,616""#);
}

#[test]
fn test_serialize_i128() {
    let val = ScVal::I128(Int128Parts {
        hi: 0,
        lo: 987654321098765,
    });
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""987,654,321,098,765""#);

    // Negative number
    let val = ScVal::I128(Int128Parts {
        hi: -1,
        lo: u64::MAX - 999999,
    });
    let json = serde_json::to_string(&val).unwrap();
    assert!(json.starts_with(r#""-"#));
}

#[test]
fn test_serialize_u256() {
    let val = ScVal::U256(UInt256Parts {
        hi_hi: 0,
        hi_lo: 0,
        lo_hi: 0,
        lo_lo: 999999999999999,
    });
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""999,999,999,999,999""#);
}

#[test]
fn test_serialize_i256() {
    let val = ScVal::I256(Int256Parts {
        hi_hi: 0,
        hi_lo: 0,
        lo_hi: 0,
        lo_lo: 555555555555555,
    });
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""555,555,555,555,555""#);
}

// ============================================================================
// String and Bytes Types Tests
// ============================================================================

#[test]
fn test_serialize_string() {
    let string_data = b"Hello, World!";
    let sc_string = StringM::<{ u32::MAX as usize }>::new(string_data);
    let val = ScVal::String(sc_string);
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""Hello, World!""#);

    // Test with special characters (UTF-8 "你好\nHello\t👋")
    let string_data = b"\xE4\xBD\xA0\xE5\xA5\xBD\nHello\t\xF0\x9F\x91\x8B";
    let sc_string = StringM::<{ u32::MAX as usize }>::new(string_data);
    let val = ScVal::String(sc_string);
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(
        json,
        r#""\\xe4\\xbd\\xa0\\xe5\\xa5\\xbd\\nHello\\t\\xf0\\x9f\\x91\\x8b""#
    );
}

#[test]
fn test_serialize_symbol() {
    let symbol_data = b"TRANSFER";
    let sc_symbol = StringM::<32>::new(symbol_data);
    let val = ScVal::Symbol(sc_symbol);
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""TRANSFER""#);
}

#[test]
fn test_serialize_bytes() {
    let bytes_data = b"\x00\x01\x02\x03\xFF";
    let sc_bytes = BytesM::<{ u32::MAX as usize }>::new(bytes_data);
    let val = ScVal::Bytes(sc_bytes);
    let json = serde_json::to_string(&val).unwrap();
    // Bytes are displayed using escape_bytes format
    assert_eq!(json, "\"\\\\0\\\\x01\\\\x02\\\\x03\\\\xff\"");
}

// ============================================================================
// Error Types Tests
// ============================================================================

#[test]
fn test_serialize_error() {
    // Contract error
    let val = ScVal::Error(SCError::SceContract(12345));
    let json = serde_json::to_string(&val).unwrap();
    let expected_json = r#"{"error_type":"Contract","error_value":12345}"#;
    assert_eq!(json, expected_json);

    // WasmVm error
    let val = ScVal::Error(SCError::SceWasmVm(SCErrorCode::ScecIndexBounds));
    let json = serde_json::to_string(&val).unwrap();
    let expected_json = r#"{"error_type":"WasmVm","error_value":"IndexBounds"}"#;
    assert_eq!(json, expected_json);

    // Context error with enum code
    let val = ScVal::Error(SCError::SceContext(SCErrorCode::ScecArithDomain));
    let json = serde_json::to_string(&val).unwrap();
    let expected_json = r#"{"error_type":"Context","error_value":"ArithDomain"}"#;
    assert_eq!(json, expected_json);
}

// ============================================================================
// Collection Types Tests
// ============================================================================

#[test]
fn test_serialize_vec_basic() {
    let vec_data = vec![
        ScVal::U32(1),
        ScVal::U32(2),
        ScVal::U32(3),
        ScVal::U32(1000),
    ];
    let val = ScVal::Vec(Some(VecM::new(vec_data)));
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#"["1","2","3","1,000"]"#);

    // Test None vec
    let val: ScVal = ScVal::Vec(None);
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, "null");
}

#[test]
fn test_serialize_vec_mixed_types() {
    let vec_data = vec![
        ScVal::Bool(true),
        ScVal::U32(42),
        ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"text")),
        ScVal::Void,
    ];
    let val = ScVal::Vec(Some(VecM::new(vec_data)));
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#"[true,"42","text",null]"#);
}

#[test]
fn test_serialize_map_basic() {
    let map_data = vec![
        ScMapEntry {
            key: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"name")),
            val: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"Alice")),
        },
        ScMapEntry {
            key: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"age")),
            val: ScVal::U32(30),
        },
        ScMapEntry {
            key: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"balance")),
            val: ScVal::U64(1000000),
        },
    ];
    let val = ScVal::Map(Some(VecM::new(map_data)));
    let json = serde_json::to_string(&val).unwrap();

    // Expected JSON - need to parse to handle key ordering
    let expected_json = r#"{"name":"Alice","age":"30","balance":"1,000,000"}"#;
    let parsed: serde_json::Value = serde_json::from_str(&json).unwrap();
    let expected_parsed: serde_json::Value = serde_json::from_str(expected_json).unwrap();
    assert_eq!(parsed, expected_parsed);
}

#[test]
fn test_serialize_map_non_string_keys() {
    // Map with various key types
    let map_data = vec![
        ScMapEntry {
            key: ScVal::U32(100),
            val: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"hundred")),
        },
        ScMapEntry {
            key: ScVal::Bool(true),
            val: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"yes")),
        },
        ScMapEntry {
            key: ScVal::Void,
            val: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"nothing")),
        },
    ];
    let val = ScVal::Map(Some(VecM::new(map_data)));
    let json = serde_json::to_string(&val).unwrap();

    // Expected JSON - need to parse to handle key ordering
    let expected_json = r#"{"100":"hundred","true":"yes","[void]":"nothing"}"#;
    let parsed: serde_json::Value = serde_json::from_str(&json).unwrap();
    let expected_parsed: serde_json::Value = serde_json::from_str(expected_json).unwrap();
    assert_eq!(parsed, expected_parsed);
}

// ============================================================================
// Nested and Complex Structures Tests
// ============================================================================

#[test]
fn test_serialize_nested_vec() {
    // Vec containing another Vec
    let inner_vec = ScVal::Vec(Some(VecM::new(vec![ScVal::U32(1), ScVal::U32(2)])));

    let outer_vec = vec![
        ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"start")),
        inner_vec,
        ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"end")),
    ];
    let val = ScVal::Vec(Some(VecM::new(outer_vec)));
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#"["start",["1","2"],"end"]"#);
}

#[test]
fn test_serialize_nested_map() {
    // Map containing another Map
    let inner_map = ScVal::Map(Some(VecM::new(vec![
        ScMapEntry {
            key: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"x")),
            val: ScVal::U32(10),
        },
        ScMapEntry {
            key: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"y")),
            val: ScVal::U32(20),
        },
    ])));

    let outer_map = vec![
        ScMapEntry {
            key: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"coordinates")),
            val: inner_map,
        },
        ScMapEntry {
            key: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"label")),
            val: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"Point A")),
        },
    ];
    let val = ScVal::Map(Some(VecM::new(outer_map)));
    let json = serde_json::to_string(&val).unwrap();

    // Expected JSON - need to parse to handle key ordering
    let expected_json = r#"{"coordinates":{"x":"10","y":"20"},"label":"Point A"}"#;
    let parsed: serde_json::Value = serde_json::from_str(&json).unwrap();
    let expected_parsed: serde_json::Value = serde_json::from_str(expected_json).unwrap();
    assert_eq!(parsed, expected_parsed);
}

#[test]
fn test_serialize_complex_structure() {
    // Create a complex structure with multiple levels of nesting
    let user_data = ScVal::Map(Some(VecM::new(vec![
        ScMapEntry {
            key: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"id")),
            val: ScVal::U64(123456789),
        },
        ScMapEntry {
            key: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"username")),
            val: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"alice_doe")),
        },
        ScMapEntry {
            key: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"scores")),
            val: ScVal::Vec(Some(VecM::new(vec![
                ScVal::U32(95),
                ScVal::U32(87),
                ScVal::U32(92),
                ScVal::U32(100),
            ]))),
        },
        ScMapEntry {
            key: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"metadata")),
            val: ScVal::Map(Some(VecM::new(vec![
                ScMapEntry {
                    key: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"created_at")),
                    val: ScVal::Timepoint(1704067200),
                },
                ScMapEntry {
                    key: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"premium")),
                    val: ScVal::Bool(true),
                },
                ScMapEntry {
                    key: ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"balance")),
                    val: ScVal::U128(UInt128Parts {
                        hi: 0,
                        lo: 999999999999,
                    }),
                },
            ]))),
        },
    ])));

    let json = serde_json::to_string(&user_data).unwrap();

    // Expected JSON - need to parse to handle key ordering
    let expected_json = r#"{"id":"123,456,789","username":"alice_doe","scores":["95","87","92","100"],"metadata":{"created_at":"2024-01-01 00:00:00 UTC","premium":true,"balance":"999,999,999,999"}}"#;
    let parsed: serde_json::Value = serde_json::from_str(&json).unwrap();
    let expected_parsed: serde_json::Value = serde_json::from_str(expected_json).unwrap();
    assert_eq!(parsed, expected_parsed);
}

// ============================================================================
// Special Types Tests
// ============================================================================

#[test]
fn test_serialize_address() {
    // Test Account Address - all zeros
    let account_bytes: [u8; 32] = [0; 32];
    let public_key = PublicKey::PublicKeyTypeEd25519(Uint256(&account_bytes));
    let account_addr = ScAddress::ScAddressTypeAccount(public_key);
    let val = ScVal::Address(account_addr);
    let json = serde_json::to_string(&val).unwrap();
    // Account addresses use stellar_strkey::ed25519::PublicKey encoding (starts with 'G')
    assert_eq!(
        json,
        r#""GAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAWHF""#
    );

    // Test Contract Address - specific pattern
    let contract_bytes: [u8; 32] = [
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e,
        0x1f, 0x20,
    ];
    let contract_id = Uint256(&contract_bytes);
    let contract_addr = ScAddress::ScAddressTypeContract(contract_id);
    let val = ScVal::Address(contract_addr);
    let json = serde_json::to_string(&val).unwrap();
    // Contract addresses use stellar_strkey::Contract encoding (starts with 'C')
    assert!(json.starts_with(r#""C"#));
    assert!(json.ends_with(r#"""#));
    // The actual encoded value depends on the stellar_strkey implementation
}

#[test]
fn test_serialize_contract_instance() {
    // ContractInstance is currently serialized as a placeholder
    let val = ScVal::ContractInstance(SCContractInstance {
        executable: ContractExecutable::StellarAsset,
        storage: None,
    });
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""[ContractInstance]""#);
}

#[test]
fn test_serialize_ledger_key_contract_instance() {
    let val = ScVal::LedgerKeyContractInstance;
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, r#""[LedgerKeyContractInstance]""#);
}

// ============================================================================
// Edge Cases and Error Handling Tests
// ============================================================================

#[test]
fn test_serialize_empty_collections() {
    // Empty Vec
    let val = ScVal::Vec(Some(VecM::new(vec![])));
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, "[]");

    // Empty Map
    let val = ScVal::Map(Some(VecM::new(vec![])));
    let json = serde_json::to_string(&val).unwrap();
    assert_eq!(json, "{}");
}

#[test]
fn test_map_key_conversion() {
    // Test scval_to_key_string for all types
    assert_eq!(scval_to_key_string(&ScVal::Bool(true)), "true");
    assert_eq!(scval_to_key_string(&ScVal::Bool(false)), "false");
    assert_eq!(scval_to_key_string(&ScVal::Void), "[void]");
    assert_eq!(scval_to_key_string(&ScVal::U32(1234567)), "1,234,567");
    assert_eq!(scval_to_key_string(&ScVal::I32(-987654)), "-987,654");

    let symbol = ScVal::Symbol(StringM::<32>::new(b"TEST"));
    assert_eq!(scval_to_key_string(&symbol), "TEST");

    let string = ScVal::String(StringM::<{ u32::MAX as usize }>::new(b"hello"));
    assert_eq!(scval_to_key_string(&string), "hello");
}

// ============================================================================
// ClaimPredicate Tests
// ============================================================================

#[test]
fn test_serialize_claim_predicate_unconditional() {
    let predicate = ClaimPredicate::Unconditional;
    let json = serde_json::to_string(&predicate).unwrap();
    assert_eq!(json, r#""unconditional""#);
}

#[test]
fn test_serialize_claim_predicate_before_absolute_time() {
    let predicate = ClaimPredicate::BeforeAbsoluteTime(1629344902);
    let json = serde_json::to_string(&predicate).unwrap();
    assert_eq!(json, r#"{"before_absolute_time":"1629344902"}"#);
}

#[test]
fn test_serialize_claim_predicate_before_relative_time() {
    let predicate = ClaimPredicate::BeforeRelativeTime(180);
    let json = serde_json::to_string(&predicate).unwrap();
    assert_eq!(json, r#"{"before_relative_time":"180"}"#);
}

#[test]
fn test_serialize_claim_predicate_not() {
    let inner = ClaimPredicate::BeforeRelativeTime(180);
    let predicate = ClaimPredicate::Not(Box::new(inner));
    let json = serde_json::to_string(&predicate).unwrap();
    assert_eq!(json, r#"{"not":{"before_relative_time":"180"}}"#);
}

#[test]
fn test_serialize_claim_predicate_and() {
    let pred1 = ClaimPredicate::BeforeAbsoluteTime(1629344902);
    let pred2 = ClaimPredicate::BeforeRelativeTime(180);
    let predicates = VecM::new(vec![pred1, pred2]);
    let predicate = ClaimPredicate::And(predicates);
    let json = serde_json::to_string(&predicate).unwrap();
    assert_eq!(
        json,
        r#"{"and":[{"before_absolute_time":"1629344902"},{"before_relative_time":"180"}]}"#
    );
}

#[test]
fn test_serialize_claim_predicate_or() {
    let pred1 = ClaimPredicate::BeforeAbsoluteTime(1629344902);
    let pred2 = ClaimPredicate::BeforeAbsoluteTime(1629300000);
    let predicates = VecM::new(vec![pred1, pred2]);
    let predicate = ClaimPredicate::Or(predicates);
    let json = serde_json::to_string(&predicate).unwrap();
    assert_eq!(
        json,
        r#"{"or":[{"before_absolute_time":"1629344902"},{"before_absolute_time":"1629300000"}]}"#
    );
}

#[test]
fn test_serialize_claim_predicate_complex_nested() {
    // Create the complex nested predicate from the example
    let before_abs_time1 = ClaimPredicate::BeforeAbsoluteTime(1629344902);
    let before_abs_time2 = ClaimPredicate::BeforeAbsoluteTime(1629300000);
    let or_predicates = VecM::new(vec![before_abs_time1, before_abs_time2]);
    let or_pred = ClaimPredicate::Or(or_predicates);

    let before_rel_time = ClaimPredicate::BeforeRelativeTime(180);
    let not_pred = ClaimPredicate::Not(Box::new(before_rel_time));

    let and_predicates = VecM::new(vec![or_pred, not_pred]);
    let predicate = ClaimPredicate::And(and_predicates);

    let json = serde_json::to_string(&predicate).unwrap();
    assert_eq!(
        json,
        r#"{"and":[{"or":[{"before_absolute_time":"1629344902"},{"before_absolute_time":"1629300000"}]},{"not":{"before_relative_time":"180"}}]}"#
    );
}
