use stellarlib::parser::{
    ParseError, Parser, ScVal, Transaction, XdrParse, MAX_OPS, MAX_SOROBAN_VEC_ITEMS,
};

// Regression tests for the VecM eager-allocation DoS: a huge length prefix on a
// vector type must fail before allocation, instead of pre-allocating `len`
// elements (which aborts the app on the 16 KB device heap).

#[test]
fn test_vecm_huge_length_prefix_no_data() {
    // ScVal::Vec whose VecM length prefix claims u32::MAX elements, with no
    // element data behind it.
    let mut data = Vec::new();
    data.extend_from_slice(&[0, 0, 0, 16]); // ScValType::ScvVec
    data.extend_from_slice(&[0, 0, 0, 1]); // Optional: present
    data.extend_from_slice(&[0xFF, 0xFF, 0xFF, 0xFF]); // VecM length: 4294967295

    let mut parser = Parser::new(&data);
    let result = ScVal::parse(&mut parser);

    assert!(matches!(
        result,
        Err(ParseError::LengthExceedsMax { actual, max })
            if actual == u32::MAX as usize && max == MAX_SOROBAN_VEC_ITEMS as usize
    ));
}

#[test]
fn test_vecm_length_prefix_exceeds_remaining_data() {
    // Length prefix claims more elements than the remaining bytes can hold;
    // parsing must reject the impossible length before allocating.
    let mut data = Vec::new();
    data.extend_from_slice(&[0, 0, 0, 16]); // ScValType::ScvVec
    data.extend_from_slice(&[0, 0, 0, 1]); // Optional: present
    data.extend_from_slice(&[0, 0, 0, 4]); // VecM length: 4
    for _ in 0..3 {
        data.extend_from_slice(&[0, 0, 0, 1]); // ScValType::ScvVoid
    }

    let mut parser = Parser::new(&data);
    let result = ScVal::parse(&mut parser);

    assert!(matches!(
        result,
        Err(ParseError::LengthExceedsMax { actual: 4, max: 3 })
    ));
}

fn scval_void_vector(item_count: u32) -> Vec<u8> {
    let mut data = Vec::new();
    data.extend_from_slice(&[0, 0, 0, 16]); // ScValType::ScvVec
    data.extend_from_slice(&[0, 0, 0, 1]); // Optional: present
    data.extend_from_slice(&item_count.to_be_bytes());
    for _ in 0..item_count {
        data.extend_from_slice(&[0, 0, 0, 1]); // ScValType::ScvVoid
    }
    data
}

#[test]
fn test_vecm_default_limit_boundary() {
    let accepted = scval_void_vector(MAX_SOROBAN_VEC_ITEMS);
    let mut parser = Parser::new(&accepted);
    let parsed = ScVal::parse(&mut parser).expect("vector at limit must parse");
    assert!(matches!(
        parsed,
        ScVal::Vec(Some(items)) if items.len() == MAX_SOROBAN_VEC_ITEMS as usize
    ));

    let rejected_count = MAX_SOROBAN_VEC_ITEMS + 1;
    let rejected = scval_void_vector(rejected_count);
    let mut parser = Parser::new(&rejected);
    assert!(matches!(
        ScVal::parse(&mut parser),
        Err(ParseError::LengthExceedsMax { actual, max })
            if actual == rejected_count as usize && max == MAX_SOROBAN_VEC_ITEMS as usize
    ));
}

#[test]
fn test_vecm_valid_vector_still_parses() {
    // Positive control: a well-formed two-element vector is unaffected.
    let mut data = Vec::new();
    data.extend_from_slice(&[0, 0, 0, 16]); // ScValType::ScvVec
    data.extend_from_slice(&[0, 0, 0, 1]); // Optional: present
    data.extend_from_slice(&[0, 0, 0, 2]); // VecM length: 2
    data.extend_from_slice(&[0, 0, 0, 1]); // ScValType::ScvVoid
    data.extend_from_slice(&[0, 0, 0, 1]); // ScValType::ScvVoid

    let mut parser = Parser::new(&data);
    let result = ScVal::parse(&mut parser);

    assert!(result.is_ok());
}

fn minimal_tx_bytes(op_count: u32) -> Vec<u8> {
    let mut data = Vec::new();
    data.extend_from_slice(&[0, 0, 0, 0]); // CryptoKeyType::KeyTypeEd25519
    data.extend_from_slice(&[0u8; 32]); // source account ed25519 key
    data.extend_from_slice(&[0, 0, 0, 100]); // fee
    data.extend_from_slice(&[0u8; 8]); // seq_num
    data.extend_from_slice(&[0, 0, 0, 0]); // Preconditions::None
    data.extend_from_slice(&[0, 0, 0, 0]); // Memo::None
    data.extend_from_slice(&op_count.to_be_bytes());
    data
}

#[test]
fn test_transaction_op_count_above_max_rejected() {
    // Each operation is formatted into several heap strings, so an unbounded
    // attacker-controlled count could exhaust the device heap. MAX_OPS is the
    // Stellar protocol limit, so no network-valid transaction is refused.
    for op_count in [MAX_OPS + 1, u32::MAX] {
        let data = minimal_tx_bytes(op_count);
        let mut parser = Parser::new(&data);
        let result = Transaction::parse(&mut parser);
        assert!(matches!(
            result,
            Err(ParseError::LengthExceedsMax { max: 100, .. })
        ));
    }
}

#[test]
fn test_transaction_op_count_at_max_accepted() {
    let data = minimal_tx_bytes(MAX_OPS);
    let mut parser = Parser::new(&data);
    let result = Transaction::parse(&mut parser);
    assert_eq!(result.unwrap().op_count, MAX_OPS);
}
