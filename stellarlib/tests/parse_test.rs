use std::fs;
use stellarlib::{
    format_operation, FormatConfig, HashIDPreimage, Operation, Parser, TransactionSignaturePayload,
    XdrParse,
};

fn test_parse_tx_case(case_name: &str) {
    let raw_path = format!("tests/testcases/{}.raw", case_name);
    let raw_data = fs::read(&raw_path).unwrap_or_else(|_| panic!("Failed to read {}", raw_path));

    let mut parser = Parser::new(&raw_data);
    let tx_signature_payload = TransactionSignaturePayload::parse(&mut parser)
        .unwrap_or_else(|_| panic!("Failed to parse XDR for {}", case_name));

    let op_count = match &tx_signature_payload.tagged_transaction {
        stellarlib::TaggedTransaction::EnvelopeTypeTx(transaction) => transaction.op_count,
        stellarlib::TaggedTransaction::EnvelopeTypeTxFeeBump(fee_bump_transaction) => {
            match &fee_bump_transaction.inner_tx {
                stellarlib::InnerTransaction::EnvelopeTypeTx(transaction) => transaction.op_count,
            }
        }
    };

    for _ in 0..op_count {
        Operation::parse(&mut parser)
            .unwrap_or_else(|_| panic!("Failed to parse operation for {}", case_name));
    }

    tx_signature_payload
        .tagged_transaction
        .parse_trailing(&mut parser)
        .unwrap_or_else(|e| panic!("Failed to parse trailing fields for {}: {:?}", case_name, e));

    // These fixtures are `signature_base()` output, i.e. exactly the bytes the
    // device signs, so parsing has to end on the last byte.
    parser
        .ensure_fully_consumed()
        .unwrap_or_else(|e| panic!("Payload not fully consumed for {}: {:?}", case_name, e));
}

fn test_parse_soroban_auth_case(case_name: &str) {
    let raw_path = format!("tests/testcases/{}.raw", case_name);
    let raw_data = fs::read(&raw_path).unwrap_or_else(|_| panic!("Failed to read {}", raw_path));

    let mut parser = Parser::new(&raw_data);
    HashIDPreimage::parse(&mut parser)
        .unwrap_or_else(|_| panic!("Failed to parse Soroban Auth for {}", case_name));
    parser
        .ensure_fully_consumed()
        .unwrap_or_else(|e| panic!("Preimage not fully consumed for {}: {:?}", case_name, e));
}

fn run_test_cases<F>(title: &str, cases: &[&str], test_fn: F)
where
    F: Fn(&str) + std::panic::RefUnwindSafe,
{
    println!("\n🚀 {}", title);
    println!("{}", "=".repeat(60));

    let mut passed = 0;
    let mut failed_cases = Vec::new();

    for (idx, case) in cases.iter().enumerate() {
        print!("[{}/{}] Testing {} ... ", idx + 1, cases.len(), case);
        match std::panic::catch_unwind(|| test_fn(case)) {
            Ok(_) => {
                println!("✅");
                passed += 1;
            }
            Err(_) => {
                println!("❌");
                failed_cases.push(*case);
            }
        }
    }

    let failed = failed_cases.len();
    println!("\n{}", "=".repeat(60));
    println!("📊 SUMMARY:");
    println!("   ✅ Passed: {}", passed);
    println!("   ❌ Failed: {}", failed);
    println!("   📦 Total:  {}", cases.len());

    if !failed_cases.is_empty() {
        println!("\n❌ Failed cases:");
        for case in &failed_cases {
            println!("   - {}", case);
        }
        panic!("Some test cases failed!");
    }
}

#[test]
fn test_sign_tx() {
    let cases = [
        "op_create_account",
        "op_payment_asset_native",
        "op_payment_asset_alphanum4",
        "op_payment_asset_alphanum12",
        "op_payment_with_muxed_destination",
        "op_path_payment_strict_receive",
        "op_path_payment_strict_receive_with_empty_path",
        "op_path_payment_strict_receive_swap",
        "op_path_payment_strict_receive_swap_with_source",
        "op_path_payment_strict_receive_swap_with_muxed_source",
        "op_path_payment_strict_receive_swap_with_op_source_not_equals_destination",
        "op_path_payment_strict_receive_with_muxed_destination",
        "op_manage_sell_offer_create",
        "op_manage_sell_offer_update",
        "op_manage_sell_offer_delete",
        "op_create_passive_sell_offer",
        "op_set_options",
        "op_set_options_with_empty_body",
        "op_set_options_add_public_key_signer",
        "op_set_options_remove_public_key_signer",
        "op_set_options_add_hash_x_signer",
        "op_set_options_remove_hash_x_signer",
        "op_set_options_add_pre_auth_tx_signer",
        "op_set_options_remove_pre_auth_tx_signer",
        "op_set_options_add_ed25519_signed_payload_signer",
        "op_set_options_remove_ed25519_signed_payload_signer",
        "op_change_trust_add_trust_line",
        "op_change_trust_add_trust_line_with_unlimited_limit",
        "op_change_trust_remove_trust_line",
        "op_change_trust_with_liquidity_pool_asset_add_trust_line",
        "op_change_trust_with_liquidity_pool_asset_remove_trust_line",
        "op_allow_trust_deauthorize",
        "op_allow_trust_authorize",
        "op_allow_trust_authorize_to_maintain_liabilities",
        "op_account_merge",
        "op_account_merge_with_muxed_destination",
        "op_inflation",
        "op_manage_data_add",
        "op_manage_data_add_with_unprintable_data",
        "op_manage_data_remove",
        "op_bump_sequence",
        "op_manage_buy_offer_create",
        "op_manage_buy_offer_update",
        "op_manage_buy_offer_delete",
        "op_path_payment_strict_send",
        "op_path_payment_strict_send_with_empty_path",
        "op_path_payment_strict_send_swap",
        "op_path_payment_strict_send_swap_with_source",
        "op_path_payment_strict_send_swap_with_muxed_source",
        "op_path_payment_strict_send_swap_with_op_source_not_equals_destination",
        "op_path_payment_strict_send_with_muxed_destination",
        "op_create_claimable_balance",
        "op_claim_claimable_balance",
        "op_begin_sponsoring_future_reserves",
        "op_end_sponsoring_future_reserves",
        "op_revoke_sponsorship_account",
        "op_revoke_sponsorship_trust_line_with_asset",
        "op_revoke_sponsorship_trust_line_with_liquidity_pool_id",
        "op_revoke_sponsorship_offer",
        "op_revoke_sponsorship_data",
        "op_revoke_sponsorship_claimable_balance",
        "op_revoke_sponsorship_liquidity_pool",
        "op_revoke_sponsorship_ed25519_public_key_signer",
        "op_revoke_sponsorship_hash_x_signer",
        "op_revoke_sponsorship_pre_auth_tx_signer",
        "op_revoke_sponsorship_ed25519_signed_payload_signer",
        "op_clawback",
        "op_clawback_with_muxed_from",
        "op_clawback_claimable_balance",
        "op_set_trust_line_flags_unauthorized",
        "op_set_trust_line_flags_authorized",
        "op_set_trust_line_flags_authorized_to_maintain_liabilities",
        "op_set_trust_line_flags_authorized_and_clawback_enabled",
        "op_liquidity_pool_deposit",
        "op_liquidity_pool_withdraw",
        "op_invoke_host_function_upload_wasm",
        "op_invoke_host_function_create_contract_wasm_id",
        "op_invoke_host_function_create_contract_v2_wasm_id",
        "op_invoke_host_function_create_contract_new_asset",
        "op_invoke_host_function_create_contract_wrap_asset",
        "op_invoke_host_function_without_args",
        "op_invoke_host_function_with_complex_sub_invocation",
        "op_invoke_host_function_asset_transfer",
        "op_invoke_host_function_asset_approve",
        "op_invoke_host_function_transfer_xlm",
        "op_invoke_host_function_transfer_usdc",
        "op_invoke_host_function_transfer_from_usdc",
        "op_invoke_host_function_burn_usdc",
        "op_invoke_host_function_burn_from_usdc",
        "op_invoke_host_function_with_auth",
        "op_invoke_host_function_with_auth_and_no_args_and_no_source",
        "op_invoke_host_function_with_auth_and_no_args",
        "op_invoke_host_function_with_auth_address_root_matches_host_function",
        "op_invoke_host_function_with_auth_address_v2",
        "op_invoke_host_function_with_auth_delegates",
        "op_invoke_host_function_with_multiple_auth_delegates",
        "op_invoke_host_function_with_auth_delegates_complex",
        "op_invoke_host_function_without_auth_and_no_source",
        "op_invoke_host_function_approve_usdc",
        "op_invoke_host_function_scvals_case0",
        "op_invoke_host_function_scvals_case1",
        "op_invoke_host_function_scvals_case2",
        "op_invoke_host_function_scvals_case3",
        "op_invoke_host_function_scvals_case4",
        "op_extend_footprint_ttl",
        "op_restore_footprint",
        "op_with_source",
        "op_with_muxed_source",
        "tx_memo_none",
        "tx_memo_id",
        "tx_memo_text",
        "tx_memo_text_unprintable",
        "tx_memo_hash",
        "tx_memo_return_hash",
        "tx_cond_with_all_items",
        "tx_cond_is_none",
        "tx_cond_time_bounds",
        "tx_cond_time_bounds_max_is_zero",
        "tx_cond_time_bounds_min_is_zero",
        "tx_cond_time_bounds_are_zero",
        "tx_cond_time_bounds_is_none",
        "tx_cond_ledger_bounds",
        "tx_cond_ledger_bounds_max_is_zero",
        "tx_cond_ledger_bounds_min_is_zero",
        "tx_cond_ledger_bounds_are_zero",
        "tx_cond_min_account_sequence",
        "tx_cond_min_account_sequence_age",
        "tx_cond_min_account_sequence_ledger_gap",
        "tx_cond_extra_signers_with_one_signer",
        "tx_cond_extra_signers_with_two_signers",
        "tx_multi_operations",
        "tx_custom_base_fee",
        "tx_with_muxed_source",
        "tx_with_different_source",
        "tx_network_public",
        "tx_network_testnet",
        "tx_network_custom",
        "fee_bump_tx",
        "fee_bump_tx_with_muxed_fee_source",
    ];

    run_test_cases(
        "Testing Sign TransactionSignaturePayload",
        &cases,
        test_parse_tx_case,
    );
}

#[test]
fn test_sign_soroban_auth() {
    let cases = [
        "soroban_auth_network_testnet",
        "soroban_auth_network_public",
        "soroban_auth_network_custom",
        "soroban_auth_create_smart_contract",
        "soroban_auth_with_address_create_smart_contract",
        "soroban_auth_create_smart_contract_v2",
        "soroban_auth_invoke_contract",
        "soroban_auth_invoke_contract_without_args",
        "soroban_auth_invoke_contract_with_complex_sub_invocation",
        "soroban_auth_with_address_invoke_contract",
        "soroban_auth_with_address_network_testnet",
        "soroban_auth_with_address_contract_address",
    ];

    run_test_cases(
        "Testing Sign Soroban Auth",
        &cases,
        test_parse_soroban_auth_case,
    );
}

/// Reads a fixture and runs it through the full signing-review parse: payload,
/// operations, then the fields that follow them.
fn parse_full_tx(raw_data: &[u8]) -> Result<(), stellarlib::ParseError> {
    let mut parser = Parser::new(raw_data);
    let payload = TransactionSignaturePayload::parse(&mut parser)?;

    let op_count = match &payload.tagged_transaction {
        stellarlib::TaggedTransaction::EnvelopeTypeTx(tx) => tx.op_count,
        stellarlib::TaggedTransaction::EnvelopeTypeTxFeeBump(fee_bump) => {
            match &fee_bump.inner_tx {
                stellarlib::InnerTransaction::EnvelopeTypeTx(tx) => tx.op_count,
            }
        }
    };
    for _ in 0..op_count {
        Operation::parse(&mut parser)?;
    }

    payload.tagged_transaction.parse_trailing(&mut parser)?;
    parser.ensure_fully_consumed()
}

/// A transaction the device would otherwise display correctly while signing
/// more bytes than it showed. Appending to a valid payload must be rejected,
/// otherwise the review screen describes only a prefix of what gets signed.
#[test]
fn test_trailing_bytes_are_rejected() {
    let cases = [
        "op_payment_asset_native",
        "fee_bump_tx",
        "op_invoke_host_function_asset_transfer",
    ];

    for case_name in cases {
        let raw_path = format!("tests/testcases/{}.raw", case_name);
        let raw_data =
            fs::read(&raw_path).unwrap_or_else(|_| panic!("Failed to read {}", raw_path));

        parse_full_tx(&raw_data)
            .unwrap_or_else(|e| panic!("{} should parse cleanly as-is: {:?}", case_name, e));

        // One appended XDR word, and a single stray byte that is not even
        // 4-byte aligned.
        for suffix in [b"\x00\x00\x00\x00".as_slice(), b"\xff".as_slice()] {
            let mut tampered = raw_data.clone();
            tampered.extend_from_slice(suffix);

            assert_eq!(
                parse_full_tx(&tampered),
                Err(stellarlib::ParseError::TrailingData {
                    remaining: suffix.len()
                }),
                "{} must reject {} appended byte(s)",
                case_name,
                suffix.len()
            );
        }
    }
}

/// The same guarantee for Soroban authorization payloads.
#[test]
fn test_soroban_auth_trailing_bytes_are_rejected() {
    let raw_path = "tests/testcases/soroban_auth_invoke_contract.raw";
    let raw_data = fs::read(raw_path).unwrap_or_else(|_| panic!("Failed to read {}", raw_path));

    let mut tampered = raw_data.clone();
    tampered.extend_from_slice(b"\x00\x00\x00\x00");

    let mut parser = Parser::new(&tampered);
    HashIDPreimage::parse(&mut parser).expect("preimage prefix still parses");
    assert_eq!(
        parser.ensure_fully_consumed(),
        Err(stellarlib::ParseError::TrailingData { remaining: 4 })
    );
}

/// `tx.ext` carries the Soroban resource data, including the resource fee.
/// Truncating it must fail rather than leave the tail unparsed.
#[test]
fn test_soroban_transaction_data_is_parsed() {
    let raw_path = "tests/testcases/op_invoke_host_function_asset_transfer.raw";
    let raw_data = fs::read(raw_path).unwrap_or_else(|_| panic!("Failed to read {}", raw_path));

    let mut parser = Parser::new(&raw_data);
    let payload = TransactionSignaturePayload::parse(&mut parser).expect("payload parses");
    let op_count = match &payload.tagged_transaction {
        stellarlib::TaggedTransaction::EnvelopeTypeTx(tx) => tx.op_count,
        _ => unreachable!("fixture is a plain transaction"),
    };
    for _ in 0..op_count {
        Operation::parse(&mut parser).expect("operation parses");
    }

    let ext = payload
        .tagged_transaction
        .parse_trailing(&mut parser)
        .expect("trailing fields parse");

    match ext {
        stellarlib::TransactionExt::V1(data) => {
            assert!(
                data.resource_fee > 0,
                "expected a resource fee on a Soroban transaction"
            );
        }
        stellarlib::TransactionExt::V0 => panic!("expected Soroban resource data"),
    }

    // Dropping the last word of the resource data must be an error, not a
    // silently ignored tail.
    let truncated = &raw_data[..raw_data.len() - 4];
    assert!(parse_full_tx(truncated).is_err());
}

/// Builds a bare `Operation` (no source account) around an operation body.
fn operation_bytes(op_type: u32, body: &[u8]) -> Vec<u8> {
    let mut v = Vec::new();
    v.extend_from_slice(&0u32.to_be_bytes()); // sourceAccount: not present
    v.extend_from_slice(&op_type.to_be_bytes());
    v.extend_from_slice(body);
    v
}

fn account_id_bytes() -> Vec<u8> {
    let mut v = 0u32.to_be_bytes().to_vec(); // PUBLIC_KEY_TYPE_ED25519
    v.extend_from_slice(&[7u8; 32]);
    v
}

fn alphanum4_asset_code() -> Vec<u8> {
    let mut v = 1u32.to_be_bytes().to_vec(); // ASSET_TYPE_CREDIT_ALPHANUM4
    v.extend_from_slice(b"USD\0");
    v
}

fn allow_trust_op(authorize: u32) -> Vec<u8> {
    let mut body = account_id_bytes();
    body.extend_from_slice(&alphanum4_asset_code());
    body.extend_from_slice(&authorize.to_be_bytes());
    operation_bytes(7, &body)
}

fn set_options_flags_op(clear_flags: u32, set_flags: u32) -> Vec<u8> {
    let mut body = Vec::new();
    body.extend_from_slice(&0u32.to_be_bytes()); // inflationDest absent
    body.extend_from_slice(&1u32.to_be_bytes()); // clearFlags present
    body.extend_from_slice(&clear_flags.to_be_bytes());
    body.extend_from_slice(&1u32.to_be_bytes()); // setFlags present
    body.extend_from_slice(&set_flags.to_be_bytes());
    for _ in 0..5 {
        // masterWeight, thresholds, homeDomain: all absent
        body.extend_from_slice(&0u32.to_be_bytes());
    }
    body.extend_from_slice(&0u32.to_be_bytes()); // signer absent
    operation_bytes(5, &body)
}

fn set_trust_line_flags_op(clear_flags: u32, set_flags: u32) -> Vec<u8> {
    let mut body = account_id_bytes();
    body.extend_from_slice(&1u32.to_be_bytes()); // ASSET_TYPE_CREDIT_ALPHANUM4
    body.extend_from_slice(b"USD\0");
    body.extend_from_slice(&account_id_bytes()); // issuer
    body.extend_from_slice(&clear_flags.to_be_bytes());
    body.extend_from_slice(&set_flags.to_be_bytes());
    operation_bytes(21, &body)
}

fn parse_op(bytes: &[u8]) -> Result<(), stellarlib::ParseError> {
    let mut parser = Parser::new(bytes);
    Operation::parse(&mut parser)?;
    parser.ensure_fully_consumed()
}

/// The protocol enumerates every valid flag value, so a bit outside that set
/// cannot be displayed honestly and must not be signed. Confirms the legal
/// values still parse, so the check cannot reject a real transaction.
#[test]
fn test_invalid_flag_values_are_rejected() {
    // AllowTrustOp.authorize is "One of 0, AUTHORIZED_FLAG, or
    // AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG" — not a bitmask, so 0x3 is
    // invalid even though both bits are individually known.
    for authorize in [0, 1, 2] {
        assert!(
            parse_op(&allow_trust_op(authorize)).is_ok(),
            "authorize={} is valid and must parse",
            authorize
        );
    }
    for authorize in [3, 4, 5, 0x8000_0000] {
        assert_eq!(
            parse_op(&allow_trust_op(authorize)),
            Err(stellarlib::ParseError::InvalidFlags(authorize)),
            "authorize={} must be rejected",
            authorize
        );
    }

    // Account flags: the protocol defines 0x1 through 0x8.
    for flags in [0x0, 0x1, 0x8, 0xF] {
        assert!(parse_op(&set_options_flags_op(flags, flags)).is_ok());
    }
    for flags in [0x10, 0x11, 0xFFFF_FFFF] {
        assert_eq!(
            parse_op(&set_options_flags_op(0, flags)),
            Err(stellarlib::ParseError::InvalidFlags(flags))
        );
        assert_eq!(
            parse_op(&set_options_flags_op(flags, 0)),
            Err(stellarlib::ParseError::InvalidFlags(flags))
        );
    }

    // Trust line flags: the protocol defines 0x1 through 0x4.
    for flags in [0x0, 0x1, 0x4, 0x7] {
        assert!(parse_op(&set_trust_line_flags_op(flags, flags)).is_ok());
    }
    for flags in [0x8, 0x9, 0xFFFF_FFFF] {
        assert_eq!(
            parse_op(&set_trust_line_flags_op(0, flags)),
            Err(stellarlib::ParseError::InvalidFlags(flags))
        );
        assert_eq!(
            parse_op(&set_trust_line_flags_op(flags, 0)),
            Err(stellarlib::ParseError::InvalidFlags(flags))
        );
    }
}

/// The formatter names flags from its own tables while the parser decides which
/// values are accepted. If the parser ever widens without the formatter's table
/// growing to match, a bit would pass validation and then vanish from the review
/// screen — the exact failure the validation exists to prevent. Every value the
/// parser accepts must therefore render with every one of its bits named.
#[test]
fn test_flag_definitions_cover_every_accepted_value() {
    let cfg = FormatConfig {
        show_sequence_and_nonce: true,
        show_preconditions: true,
        show_authorization_details: true,
        show_tx_source_if_matches_signer: true,
    };
    let source = "GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7";

    let rendered = |bytes: &[u8], field: &str| -> String {
        let mut parser = Parser::new(bytes);
        let op = Operation::parse(&mut parser).expect("accepted by the parser");
        // Guards the hand-built operations above: a stray byte would otherwise
        // leave this exercising a different value than intended.
        parser.ensure_fully_consumed().expect("operation is exact");
        format_operation(&op, &cfg, source)
            .expect("formats")
            .into_iter()
            .find(|e| e.title == field)
            .unwrap_or_else(|| panic!("no `{}` entry", field))
            .content
    };

    // One name per set bit, so the rendered list must have as many entries.
    // An empty render is zero names, not one: `"".split(", ").count()` is 1,
    // which would let a dropped single-bit name pass unnoticed.
    let named_count = |s: &str| {
        if s.is_empty() {
            0
        } else {
            s.split(", ").count()
        }
    };

    for flags in 1..=stellarlib::ALL_ACCOUNT_FLAGS {
        let content = rendered(&set_options_flags_op(flags, flags), "Set Flags");
        assert_eq!(
            named_count(&content),
            flags.count_ones() as usize,
            "account flags {:#x} rendered as {:?}, dropping a bit",
            flags,
            content
        );
    }

    for flags in 1..=stellarlib::ALL_TRUSTLINE_FLAGS {
        let content = rendered(&set_trust_line_flags_op(0, flags), "Set Flags");
        assert_eq!(
            named_count(&content),
            flags.count_ones() as usize,
            "trust line flags {:#x} rendered as {:?}, dropping a bit",
            flags,
            content
        );
    }

    // AllowTrust is an enumeration, not a mask: each accepted value has exactly
    // one name.
    for authorize in [1, 2] {
        let content = rendered(&allow_trust_op(authorize), "Authorize");
        assert_eq!(named_count(&content), 1, "authorize={}", authorize);
    }
}
