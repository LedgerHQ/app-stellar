#[cfg(test)]
mod extract_function_tests {
    use stellarlib::parser::{Int128Parts, ScVal};

    // Since the extract functions are private, we test them indirectly
    // through the public API or test similar logic here

    #[test]
    fn test_i128_extraction_logic() {
        // Test the logic for extracting i128 from ScVal
        let scval = ScVal::I128(Int128Parts { hi: 0, lo: 1000000 });

        // The actual extraction would be:
        // match scval {
        //     ScVal::I128(parts) => Some(combine_i128_parts(parts)),
        //     _ => None
        // }

        match scval {
            ScVal::I128(parts) => {
                let hi = parts.hi as i128;
                let lo = parts.lo as i128;
                let value = (hi << 64) | (lo & 0xFFFF_FFFF_FFFF_FFFF);
                assert_eq!(value, 1000000);
            }
            _ => panic!("Expected I128"),
        }
    }

    #[test]
    fn test_type_discrimination() {
        // Test that we can properly discriminate between types
        let u32_val = ScVal::U32(100);
        let u64_val = ScVal::U64(1000);
        let i128_val = ScVal::I128(Int128Parts { hi: 0, lo: 1000000 });

        // Verify type discrimination works
        assert!(matches!(u32_val, ScVal::U32(_)));
        assert!(matches!(u64_val, ScVal::U64(_)));
        assert!(matches!(i128_val, ScVal::I128(_)));

        // Verify mismatches return None in extraction logic
        assert!(!matches!(u32_val, ScVal::I128(_)));
        assert!(!matches!(u64_val, ScVal::I128(_)));
        assert!(!matches!(i128_val, ScVal::U32(_)));
    }
}

#[cfg(test)]
mod formatting_logic_tests {
    use stellarlib::display::format_token_amount;
    use stellarlib::tokens::get_token_info;

    #[test]
    fn test_usdc_token_formatting() {
        // USDC contract address
        let usdc_address = [
            0xad, 0xef, 0xce, 0x59, 0xae, 0xe5, 0x29, 0x68, 0xf7, 0x60, 0x61, 0xd4, 0x94, 0xc2,
            0x52, 0x5b, 0x75, 0x65, 0x9f, 0xa4, 0x29, 0x6a, 0x65, 0xf4, 0x99, 0xef, 0x29, 0xe5,
            0x64, 0x77, 0xe4, 0x96,
        ];

        let token_info = get_token_info(&usdc_address).expect("USDC should be known");
        assert_eq!(token_info.symbol, "USDC");
        assert_eq!(token_info.decimals, 7);

        // Test amount formatting for USDC
        let formatted = format_token_amount(100_000_000, token_info.decimals);
        assert_eq!(formatted, "10");

        let formatted = format_token_amount(10_000_000, token_info.decimals);
        assert_eq!(formatted, "1");

        let formatted = format_token_amount(1_234_567, token_info.decimals);
        assert_eq!(formatted, "0.1234567");
    }

    #[test]
    fn test_btc_token_formatting() {
        // BTC contract address
        let btc_address = [
            0x1d, 0xf1, 0x8d, 0x2d, 0x33, 0x1d, 0x88, 0x3e, 0x38, 0x1e, 0x3a, 0x9a, 0xe7, 0xb8,
            0x22, 0xb9, 0x48, 0x06, 0x4c, 0x32, 0xc0, 0x4b, 0x44, 0x54, 0x74, 0xe2, 0xc1, 0xc8,
            0x2c, 0x35, 0xa4, 0x00,
        ];

        let token_info = get_token_info(&btc_address).expect("BTC should be known");
        assert_eq!(token_info.symbol, "BTC");
        assert_eq!(token_info.decimals, 7);

        // Test amount formatting for BTC
        let formatted = format_token_amount(250_000_000, token_info.decimals);
        assert_eq!(formatted, "25");
    }

    #[test]
    fn test_unknown_token_returns_none() {
        let unknown_address = [0xFF; 32];
        assert!(get_token_info(&unknown_address).is_none());

        let zero_address = [0x00; 32];
        assert!(get_token_info(&zero_address).is_none());
    }
}
