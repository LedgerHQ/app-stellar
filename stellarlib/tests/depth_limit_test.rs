use stellarlib::parser::{ClaimPredicate, ParseError, Parser, ScVal, XdrParse};

#[test]
fn test_depth_limit_scval_vec() {
    // Create deeply nested ScVal::Vec structure
    // This would create: Vec([Vec([Vec([...32 levels...])])])

    // Build a test payload that creates nested ScVal::Vec
    // ScValType::ScvVec = 17
    let mut data = Vec::new();

    // Each level: type (4 bytes) + optional flag (4 bytes) + array length (4 bytes)
    for _ in 0..35 {
        // Try to exceed MAX_PARSE_DEPTH (32)
        data.extend_from_slice(&[0, 0, 0, 17]); // ScValType::ScvVec
        data.extend_from_slice(&[0, 0, 0, 1]); // Optional: present
        data.extend_from_slice(&[0, 0, 0, 1]); // Array length: 1
    }

    // Add a leaf node (ScVal::Void)
    data.extend_from_slice(&[0, 0, 0, 1]); // ScValType::ScvVoid

    let mut parser = Parser::new(&data);
    let result = ScVal::parse(&mut parser);

    assert!(matches!(result, Err(ParseError::MaxDepthExceeded { .. })));

    if let Err(ParseError::MaxDepthExceeded { depth, max }) = result {
        assert!(depth > max);
        assert_eq!(max, 32);
        println!("Successfully caught ScVal depth limit: {} > {}", depth, max);
    }
}

#[test]
fn test_depth_limit_claim_predicate() {
    // Create deeply nested ClaimPredicate::Not structure
    // This would create: Not(Not(Not(...32 levels...)))

    let mut data = Vec::new();

    // Each level: type (4 bytes) + optional flag (4 bytes)
    for _ in 0..35 {
        // Try to exceed MAX_PARSE_DEPTH
        data.extend_from_slice(&[0, 0, 0, 3]); // ClaimPredicateType::Not = 3
        data.extend_from_slice(&[0, 0, 0, 1]); // Optional: present
    }

    // Add a leaf node (Unconditional)
    data.extend_from_slice(&[0, 0, 0, 0]); // ClaimPredicateType::Unconditional

    let mut parser = Parser::new(&data);
    let result = ClaimPredicate::parse(&mut parser);

    assert!(matches!(result, Err(ParseError::MaxDepthExceeded { .. })));

    if let Err(ParseError::MaxDepthExceeded { depth, max }) = result {
        println!(
            "Successfully caught ClaimPredicate depth limit: {} > {}",
            depth, max
        );
    }
}
