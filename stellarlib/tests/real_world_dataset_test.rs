use base64::{engine::general_purpose::STANDARD, Engine};
use csv::Reader;
use std::fs::{self, File};
use std::path::Path;
use stellarlib::{
    format_transaction_signature_payload, FormatConfig, Operation, Parser,
    TransactionSignaturePayload, XdrParse,
};

#[derive(Debug)]
struct SorobanTestCase {
    tx_envelope: String,
    row_number: usize,
}

// Example BigQuery queries to extract real-world Soroban transactions:
// SELECT tx_envelope FROM `crypto-stellar.crypto_stellar.history_transactions` WHERE successful = true AND operation_count < 50 AND refundable_fee is null AND batch_run_date BETWEEN DATETIME("2025-11-12") AND DATETIME_ADD("2025-11-12", INTERVAL 1 MONTH) LIMIT 20000
// SELECT tx_envelope FROM `crypto-stellar.crypto_stellar.history_transactions` WHERE successful = true AND (soroban_resources_read_bytes !=0 OR soroban_resources_write_bytes != 0 OR soroban_resources_instructions != 0) AND batch_run_date BETWEEN DATETIME("2025-11-12") AND DATETIME_ADD("2025-11-12", INTERVAL 1 MONTH) LIMIT 20000

/// Load test cases from a single CSV file
fn load_test_cases_from_csv(csv_path: &Path) -> Vec<SorobanTestCase> {
    let file =
        File::open(csv_path).unwrap_or_else(|e| panic!("Failed to open {:?}: {}", csv_path, e));
    let mut reader = Reader::from_reader(file);
    let mut test_cases = Vec::new();

    // Read headers to find tx_envelope column
    let headers = reader
        .headers()
        .unwrap_or_else(|e| panic!("Failed to read headers from {:?}: {}", csv_path, e));
    let tx_envelope_idx = headers
        .iter()
        .position(|h| h == "tx_envelope")
        .unwrap_or_else(|| panic!("'tx_envelope' column not found in {:?}", csv_path));

    for (row_idx, result) in reader.records().enumerate() {
        let record = result.unwrap_or_else(|e| {
            panic!(
                "Failed to read row {} in {:?}: {}",
                row_idx + 2,
                csv_path,
                e
            )
        });

        let tx_envelope = record
            .get(tx_envelope_idx)
            .unwrap_or_else(|| {
                panic!(
                    "Missing tx_envelope in row {} of {:?}",
                    row_idx + 2,
                    csv_path
                )
            })
            .trim()
            .to_string();

        // Skip empty rows
        if !tx_envelope.is_empty() {
            test_cases.push(SorobanTestCase {
                tx_envelope,
                row_number: row_idx + 2, // +2 because row 1 is header, and we start counting from 1
            });
        }
    }

    test_cases
}

/// Load all test cases from all CSV files in the datasets directory
fn load_all_test_cases(datasets_dir: &str) -> Vec<(String, Vec<SorobanTestCase>)> {
    let path = Path::new(datasets_dir);

    if !path.exists() {
        panic!("Datasets directory '{}' does not exist", datasets_dir);
    }

    let mut all_datasets = Vec::new();

    // Read all CSV files in the directory
    let entries = fs::read_dir(path)
        .unwrap_or_else(|e| panic!("Failed to read directory '{}': {}", datasets_dir, e));

    for entry in entries {
        let entry = entry.unwrap();
        let file_path = entry.path();

        // Only process CSV files
        if file_path.extension().and_then(|s| s.to_str()) == Some("csv") {
            let file_name = file_path
                .file_name()
                .and_then(|n| n.to_str())
                .unwrap_or("unknown")
                .to_string();

            println!("Loading dataset: {}", file_name);
            let test_cases = load_test_cases_from_csv(&file_path);
            println!("  Found {} test cases", test_cases.len());

            if !test_cases.is_empty() {
                all_datasets.push((file_name, test_cases));
            }
        }
    }

    all_datasets.sort_by(|a, b| a.0.cmp(&b.0)); // Sort by filename for consistent ordering
    all_datasets
}

/// Test a single transaction
fn test_transaction(test_case: &SorobanTestCase) -> Result<(), String> {
    // Decode base64 envelope
    let envelope_data = STANDARD
        .decode(&test_case.tx_envelope)
        .map_err(|e| format!("Failed to decode base64: {}", e))?;

    // Prepend 32 bytes of network ID
    let mut raw_data = vec![0u8; 32];
    raw_data.extend_from_slice(&envelope_data);

    // Parse transaction
    let mut parser = Parser::new(&raw_data);
    let tx_signature_payload = TransactionSignaturePayload::parse(&mut parser)
        .map_err(|e| format!("Failed to parse transaction: {:?}", e))?;

    // Extract transaction details
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

    // Parse operations
    for i in 0..op_count {
        Operation::parse(&mut parser)
            .map_err(|e| format!("Failed to parse operation {}: {:?}", i, e))?;
    }

    // Reset parser for formatting test
    let mut parser = Parser::new(&raw_data);
    let tx_signature_payload = TransactionSignaturePayload::parse(&mut parser)
        .map_err(|e| format!("Failed to re-parse for formatting: {:?}", e))?;

    // Format transaction
    let config = FormatConfig {
        show_sequence_and_nonce: true,
        show_preconditions: true,
        show_nested_authorization: true,
        show_tx_source_if_matches_signer: true,
    };

    let signer = "GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7";

    let mut entries = format_transaction_signature_payload(&tx_signature_payload, &config, signer)
        .map_err(|e| format!("Failed to format transaction: {:?}", e))?;

    // Format operations
    for i in 0..op_count {
        if op_count > 1 {
            entries.push(stellarlib::DataEntry {
                title: "Operation".to_string(),
                content: format!("{} of {}", i + 1, op_count),
            });
        }
        let operation = Operation::parse(&mut parser)
            .map_err(|e| format!("Failed to parse operation {} for formatting: {:?}", i, e))?;
        let op_entries = stellarlib::formatter::format_operation(&operation, &config, &tx_source)
            .map_err(|e| format!("Failed to format operation {}: {:?}", i, e))?;
        entries.extend(op_entries);
    }

    // If we got here, both parse and format succeeded
    Ok(())
}

#[test]
fn test_real_world_datasets() {
    println!("\n🚀 Testing Real-World Transaction Datasets");
    println!("{}", "=".repeat(80));

    let datasets_dir = "tests/real_world_datasets";
    let all_datasets = load_all_test_cases(datasets_dir);

    if all_datasets.is_empty() {
        println!("⚠️  No CSV files found in {}", datasets_dir);
        println!("   Please add CSV files with 'tx_envelope' column to the directory.");
        return;
    }

    let mut total_passed = 0;
    let mut total_failed = 0;
    let mut all_failed_cases = Vec::new();

    for (dataset_name, test_cases) in &all_datasets {
        println!("\n📁 Dataset: {}", dataset_name);
        println!("{}", "-".repeat(60));

        let dataset_total = test_cases.len();
        let mut dataset_passed = 0;
        let mut dataset_failed_cases = Vec::new();

        for test_case in test_cases {
            match test_transaction(test_case) {
                Ok(_) => {
                    dataset_passed += 1;
                }
                Err(e) => {
                    println!(
                        "  [Row {}] Testing transaction ... ❌",
                        test_case.row_number
                    );
                    println!("      Error: {}", e);
                    dataset_failed_cases.push((test_case.row_number, e));
                }
            }
        }

        let dataset_failed = dataset_failed_cases.len();

        println!("\n  📊 Dataset Summary:");
        println!("     ✅ Passed: {}/{}", dataset_passed, dataset_total);
        println!("     ❌ Failed: {}/{}", dataset_failed, dataset_total);

        if dataset_failed > 0 {
            println!("     Failed rows:");
            for (row, _) in &dataset_failed_cases {
                print!(" {}", row);
            }
            println!();
        }

        total_passed += dataset_passed;
        total_failed += dataset_failed;

        if !dataset_failed_cases.is_empty() {
            all_failed_cases.push((dataset_name.clone(), dataset_failed_cases));
        }
    }

    // Overall summary
    println!("\n{}", "=".repeat(80));
    println!("📊 OVERALL SUMMARY:");
    println!("   📁 Datasets tested: {}", all_datasets.len());
    println!("   ✅ Total passed:    {}", total_passed);
    println!("   ❌ Total failed:    {}", total_failed);
    println!("   📦 Total tests:     {}", total_passed + total_failed);

    let pass_rate = if (total_passed + total_failed) > 0 {
        (total_passed as f64 / (total_passed + total_failed) as f64) * 100.0
    } else {
        0.0
    };
    println!("   📈 Pass rate:       {:.1}%", pass_rate);

    if !all_failed_cases.is_empty() {
        println!("\n❌ Failed Test Details:");
        for (dataset_name, failed_cases) in &all_failed_cases {
            println!("\n   Dataset: {}", dataset_name);
            for (row, error) in failed_cases {
                println!("     Row {}: {}", row, error);
            }
        }
        panic!("\n⛔ Some real-world dataset tests failed!");
    } else if total_passed > 0 {
        println!(
            "\n🎉 All {} real-world transaction tests passed!",
            total_passed
        );
    }
}

#[test]
fn test_specific_csv_files() {
    // This test allows testing specific CSV files by name
    // Useful for debugging or focusing on specific datasets

    let specific_files: Vec<&str> = vec!["generic_txs.csv"];

    if specific_files.is_empty() {
        // Skip this test if no specific files are listed
        return;
    }

    println!("\n🔍 Testing Specific CSV Files");
    println!("{}", "=".repeat(80));

    for file_name in specific_files {
        let file_path = format!("tests/real_world_datasets/{}", file_name);
        let path = Path::new(&file_path);

        if !path.exists() {
            println!("⚠️  File not found: {}", file_path);
            continue;
        }

        println!("\n📁 Testing: {}", file_name);
        let test_cases = load_test_cases_from_csv(path);

        let mut passed = 0;
        let mut failed = Vec::new();

        for test_case in &test_cases {
            match test_transaction(test_case) {
                Ok(_) => passed += 1,
                Err(e) => failed.push((test_case.row_number, e)),
            }
        }

        println!("  ✅ Passed: {}/{}", passed, test_cases.len());
        if !failed.is_empty() {
            println!("  ❌ Failed: {}/{}", failed.len(), test_cases.len());
            for (row, error) in &failed {
                println!("     Row {}: {}", row, error);
            }
        }
    }
}
