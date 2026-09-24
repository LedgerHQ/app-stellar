use stellarlib::parser::{ClaimPredicate, ParseError, Parser, ScVal, SorobanCredentials, XdrParse};

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
        data.extend_from_slice(&[0, 0, 0, 16]); // ScValType::ScvVec
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

#[test]
fn test_depth_limit_soroban_delegate_signature() {
    // CAP-71 (Protocol 27) SorobanDelegateSignature is recursive through its
    // nestedDelegates<> list; nesting deeper than MAX_PARSE_DEPTH must fail
    // instead of overflowing the device stack.

    let mut data = Vec::new();
    data.extend_from_slice(&[0, 0, 0, 3]); // SorobanCredentialsType::AddressWithDelegates

    // SorobanAddressCredentials
    data.extend_from_slice(&[0, 0, 0, 0]); // ScAddressType::ScAddressTypeAccount
    data.extend_from_slice(&[0, 0, 0, 0]); // PublicKey type: ed25519
    data.extend_from_slice(&[0u8; 32]); // ed25519 key
    data.extend_from_slice(&[0u8; 8]); // nonce
    data.extend_from_slice(&[0, 0, 0, 1]); // signature_expiration_ledger
    data.extend_from_slice(&[0, 0, 0, 1]); // signature: ScValType::ScvVoid

    // Each level: a one-element delegate list whose element nests the next level
    for _ in 0..35 {
        // Try to exceed MAX_PARSE_DEPTH (32)
        data.extend_from_slice(&[0, 0, 0, 1]); // delegates/nestedDelegates length: 1
        data.extend_from_slice(&[0, 0, 0, 0]); // ScAddressType::ScAddressTypeAccount
        data.extend_from_slice(&[0, 0, 0, 0]); // PublicKey type: ed25519
        data.extend_from_slice(&[0u8; 32]); // ed25519 key
        data.extend_from_slice(&[0, 0, 0, 1]); // signature: ScValType::ScvVoid
    }

    // Innermost nestedDelegates: empty
    data.extend_from_slice(&[0, 0, 0, 0]);

    let mut parser = Parser::new(&data);
    let result = SorobanCredentials::parse(&mut parser);

    assert!(matches!(result, Err(ParseError::MaxDepthExceeded { .. })));

    if let Err(ParseError::MaxDepthExceeded { depth, max }) = result {
        assert!(depth > max);
        assert_eq!(max, 32);
        println!(
            "Successfully caught SorobanDelegateSignature depth limit: {} > {}",
            depth, max
        );
    }
}
