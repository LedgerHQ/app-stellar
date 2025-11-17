use std::fs;
use stellarlib::{
    format_hash_id_preimage_soroban_authorization, format_transaction_signature_payload,
    FormatConfig, HashIDPreimage, Parser, TransactionSignaturePayload, XdrParse,
};

fn format_entries_to_text(entries: &[stellarlib::DataEntry]) -> String {
    entries
        .iter()
        .map(|entry| format!("{}; {}", entry.title, entry.content))
        .collect::<Vec<_>>()
        .join("\n")
        + "\n"
}

fn compare_with_diff(actual: &str, expected: &str, case_name: &str) {
    if actual == expected {
        return;
    }

    // If not equal, show detailed diff
    println!("\n❌ Output mismatch for: {}", case_name);
    println!("{}", "=".repeat(60));

    let actual_lines: Vec<&str> = actual.lines().collect();
    let expected_lines: Vec<&str> = expected.lines().collect();

    println!("\n📄 EXPECTED ({} lines):", expected_lines.len());
    println!("{}", "-".repeat(60));
    for (i, line) in expected_lines.iter().enumerate() {
        println!("{:4}: {}", i + 1, line);
    }

    println!("\n📄 ACTUAL ({} lines):", actual_lines.len());
    println!("{}", "-".repeat(60));
    for (i, line) in actual_lines.iter().enumerate() {
        println!("{:4}: {}", i + 1, line);
    }

    println!("\n🔍 DIFFERENCES:");
    println!("{}", "-".repeat(60));

    let max_len = actual_lines.len().max(expected_lines.len());
    for i in 0..max_len {
        let actual_line = actual_lines.get(i).unwrap_or(&"");
        let expected_line = expected_lines.get(i).unwrap_or(&"");

        if actual_line != expected_line {
            println!("Line {}:", i + 1);
            println!("  Expected: {}", expected_line);
            println!("  Actual:   {}", actual_line);
        }
    }

    panic!("Output mismatch for {}", case_name);
}

fn test_sign_tx_format_case(case_name: &str) {
    let raw_path = format!("tests/testcases/{}.raw", case_name);
    let txt_path = format!("tests/testcases/{}.txt", case_name);

    let raw_data = fs::read(&raw_path).unwrap_or_else(|_| panic!("Failed to read {}", raw_path));
    let expected_output =
        fs::read_to_string(&txt_path).unwrap_or_else(|_| panic!("Failed to read {}", txt_path));

    let mut parser = Parser::new(&raw_data);
    let tx_signature_payload = TransactionSignaturePayload::parse(&mut parser)
        .unwrap_or_else(|_| panic!("Failed to parse XDR for {}", case_name));

    let config = FormatConfig {
        show_sequence_and_nonce: true,
        show_preconditions: true,
        show_nested_authorization: true,
        show_tx_source_if_matches_signer: true,
    };

    let signer = "GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7";

    let mut entries = format_transaction_signature_payload(&tx_signature_payload, &config, signer)
        .unwrap_or_else(|_| panic!("Failed to format transaction for {}", case_name));

    let (op_count, tx_source) = match &tx_signature_payload.tagged_transaction {
        stellarlib::TaggedTransaction::EnvelopeTypeTx(tx) => {
            (tx.op_count, tx.source_account.to_string())
        }
        stellarlib::TaggedTransaction::EnvelopeTypeTxFeeBump(fee_bump_tx) => {
            match &fee_bump_tx.inner_tx {
                stellarlib::InnerTransaction::EnvelopeTypeTx(tx) => {
                    (tx.op_count, tx.source_account.to_string())
                }
            }
        }
    };

    for i in 0..op_count {
        if op_count > 1 {
            entries.push(stellarlib::DataEntry {
                title: "Operation".to_string(),
                content: format!("{} of {}", i + 1, op_count),
            });
        }
        let operation = stellarlib::Operation::parse(&mut parser)
            .unwrap_or_else(|_| panic!("Failed to parse Operation for {}", case_name));
        let op_entries = stellarlib::formatter::format_operation(&operation, &config, &tx_source)
            .unwrap_or_else(|_| panic!("Failed to format Operation for {}", case_name));
        entries.extend(op_entries);
    }

    let actual_output = format_entries_to_text(&entries);
    compare_with_diff(&actual_output, &expected_output, case_name);
}

fn test_soroban_auth_format_case(case_name: &str) {
    let raw_path = format!("tests/testcases/{}.raw", case_name);
    let txt_path = format!("tests/testcases/{}.txt", case_name);

    let raw_data = fs::read(&raw_path).unwrap_or_else(|_| panic!("Failed to read {}", raw_path));
    let expected_output =
        fs::read_to_string(&txt_path).unwrap_or_else(|_| panic!("Failed to read {}", txt_path));

    let mut parser = Parser::new(&raw_data);
    let hash_id_preimage = HashIDPreimage::parse(&mut parser)
        .unwrap_or_else(|_| panic!("Failed to parse XDR for {}", case_name));
    let HashIDPreimage::SorobanAuthorization(auth) = hash_id_preimage;

    let config = FormatConfig {
        show_sequence_and_nonce: true,
        show_preconditions: true,
        show_nested_authorization: true,
        show_tx_source_if_matches_signer: false,
    };

    let entries = format_hash_id_preimage_soroban_authorization(&auth, &config);
    let actual_output = format_entries_to_text(&entries);
    compare_with_diff(&actual_output, &expected_output, case_name);
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
fn test_sign_tx_formats() {
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
        "Testing All Sign Tx Format Cases",
        &cases,
        test_sign_tx_format_case,
    );
}

#[test]
fn test_sign_soroban_auth_formats() {
    let cases = [
        "soroban_auth_network_testnet",
        "soroban_auth_network_public",
        "soroban_auth_network_custom",
        "soroban_auth_create_smart_contract",
        "soroban_auth_create_smart_contract_v2",
        "soroban_auth_invoke_contract",
        "soroban_auth_invoke_contract_without_args",
        "soroban_auth_invoke_contract_with_complex_sub_invocation",
    ];

    run_test_cases(
        "Testing All Sign Soroban Auth Format Cases",
        &cases,
        test_soroban_auth_format_case,
    );
}
