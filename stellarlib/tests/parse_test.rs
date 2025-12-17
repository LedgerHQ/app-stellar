use std::fs;
use stellarlib::{HashIDPreimage, Operation, Parser, TransactionSignaturePayload, XdrParse};

fn test_parse_tx_case(case_name: &str) {
    let raw_path = format!("tests/testcases/{}.raw", case_name);
    let raw_data = fs::read(&raw_path).unwrap_or_else(|_| panic!("Failed to read {}", raw_path));

    let mut parser = Parser::new(&raw_data);
    let tx_signature_payload = TransactionSignaturePayload::parse(&mut parser)
        .unwrap_or_else(|_| panic!("Failed to parse XDR for {}", case_name));

    let tx = match tx_signature_payload.tagged_transaction {
        stellarlib::TaggedTransaction::EnvelopeTypeTx(transaction) => transaction,
        stellarlib::TaggedTransaction::EnvelopeTypeTxFeeBump(fee_bump_transaction) => {
            match fee_bump_transaction.inner_tx {
                stellarlib::InnerTransaction::EnvelopeTypeTx(transaction) => transaction,
            }
        }
    };

    for _ in 0..tx.op_count {
        Operation::parse(&mut parser)
            .unwrap_or_else(|_| panic!("Failed to parse operation for {}", case_name));
    }
}

fn test_parse_soroban_auth_case(case_name: &str) {
    let raw_path = format!("tests/testcases/{}.raw", case_name);
    let raw_data = fs::read(&raw_path).unwrap_or_else(|_| panic!("Failed to read {}", raw_path));

    let mut parser = Parser::new(&raw_data);
    HashIDPreimage::parse(&mut parser)
        .unwrap_or_else(|_| panic!("Failed to parse Soroban Auth for {}", case_name));
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
        "soroban_auth_create_smart_contract_v2",
        "soroban_auth_invoke_contract",
        "soroban_auth_invoke_contract_without_args",
        "soroban_auth_invoke_contract_with_complex_sub_invocation",
    ];

    run_test_cases(
        "Testing Sign Soroban Auth",
        &cases,
        test_parse_soroban_auth_case,
    );
}
