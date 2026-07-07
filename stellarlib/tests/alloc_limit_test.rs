use stellarlib::parser::{ParseError, Parser, ScVal, XdrParse};

// Regression tests for the VecM eager-allocation DoS: a huge length prefix on a
// vector type must fail with BufferOverflow once the input runs out, instead of
// pre-allocating `len` elements (which aborts the app on the 16 KB device heap).

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

    assert!(matches!(result, Err(ParseError::BufferOverflow)));
}

#[test]
fn test_vecm_length_prefix_exceeds_remaining_data() {
    // Length prefix claims more elements than the remaining bytes can hold;
    // parsing must stop cleanly when the real data runs out.
    let mut data = Vec::new();
    data.extend_from_slice(&[0, 0, 0, 16]); // ScValType::ScvVec
    data.extend_from_slice(&[0, 0, 0, 1]); // Optional: present
    data.extend_from_slice(&[0, 0, 4, 0]); // VecM length: 1024
    for _ in 0..3 {
        data.extend_from_slice(&[0, 0, 0, 1]); // ScValType::ScvVoid
    }

    let mut parser = Parser::new(&data);
    let result = ScVal::parse(&mut parser);

    assert!(matches!(result, Err(ParseError::BufferOverflow)));
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
