use stellarlib::tokens::{get_token_info, KNOWN_TOKENS};

#[test]
fn test_get_token_info_known_tokens() {
    // Test USDC lookup
    let usdc_address = [
        0xad, 0xef, 0xce, 0x59, 0xae, 0xe5, 0x29, 0x68, 0xf7, 0x60, 0x61, 0xd4, 0x94, 0xc2, 0x52,
        0x5b, 0x75, 0x65, 0x9f, 0xa4, 0x29, 0x6a, 0x65, 0xf4, 0x99, 0xef, 0x29, 0xe5, 0x64, 0x77,
        0xe4, 0x96,
    ];

    let token_info = get_token_info(&usdc_address);
    assert!(token_info.is_some());

    let token_info = token_info.unwrap();
    assert_eq!(token_info.symbol, "USDC");
    assert_eq!(token_info.decimals, 7);

    // Test BTC lookup
    let btc_address = [
        0x1d, 0xf1, 0x8d, 0x2d, 0x33, 0x1d, 0x88, 0x3e, 0x38, 0x1e, 0x3a, 0x9a, 0xe7, 0xb8, 0x22,
        0xb9, 0x48, 0x06, 0x4c, 0x32, 0xc0, 0x4b, 0x44, 0x54, 0x74, 0xe2, 0xc1, 0xc8, 0x2c, 0x35,
        0xa4, 0x00,
    ];

    let token_info = get_token_info(&btc_address);
    assert!(token_info.is_some());
    assert_eq!(token_info.unwrap().symbol, "BTC");
}

#[test]
fn test_get_token_info_unknown_token() {
    let unknown_address = [0u8; 32];
    assert!(get_token_info(&unknown_address).is_none());

    let random_address = [0xFF; 32];
    assert!(get_token_info(&random_address).is_none());
}

#[test]
fn test_all_tokens_have_unique_addresses() {
    use std::collections::HashSet;
    let mut seen = HashSet::new();

    for token in KNOWN_TOKENS.iter() {
        assert!(
            seen.insert(token.contract_address),
            "Duplicate contract address found for token: {}",
            token.symbol
        );
    }

    // Verify we have the expected number of unique tokens
    assert_eq!(seen.len(), KNOWN_TOKENS.len());
}
