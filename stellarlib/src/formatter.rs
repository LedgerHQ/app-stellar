//! Formatter module for Stellar transactions
//!
//! This module provides functionality to format Stellar transactions
//! into human-readable data entries suitable for display on hardware wallets.

extern crate alloc;

use crate::display::{
    format_decimal, format_native_amount, format_number_with_commas, format_pool_id,
    format_unix_timestamp, STELLAR_NATIVE_DECIMAL_PLACES,
};
use crate::parser::*;
use crate::serialize::scval_to_key_string;
use alloc::format;
use alloc::string::{String, ToString};
use alloc::vec;
use alloc::vec::Vec;

/// Number of stroops per XLM (10^7)
const STROOPS_PER_XLM: u64 = 10_000_000;

/// Liquidity pool fees are expressed in basis points (1/100 of a percent)
const BASIS_POINTS_TO_PERCENT: f64 = 100.0;

/// Represents a formatted data entry with a title and content
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct DataEntry {
    /// The field title/name
    pub title: String,
    /// The formatted content value
    pub content: String,
}

impl DataEntry {
    /// Creates a new DataEntry with the given title and content
    #[inline]
    pub fn new(title: &str, content: String) -> Self {
        Self {
            title: title.to_string(),
            content,
        }
    }

    /// Creates a DataEntry from string slices
    #[inline]
    pub fn from_parts(title: &str, content: &str) -> Self {
        Self {
            title: title.to_string(),
            content: content.to_string(),
        }
    }
}

/// Configuration for transaction formatting
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FormatConfig {
    /// Whether to show sequence and nonce in output
    pub show_sequence_and_nonce: bool,
    /// Whether to show transaction preconditions
    pub show_preconditions: bool,
    /// Whether to show nested Soroban authorization details
    pub show_nested_authorization: bool,
    /// Whether to show transaction source when it matches the signer address
    pub show_tx_source_if_matches_signer: bool,
}

impl Default for FormatConfig {
    fn default() -> Self {
        Self {
            show_sequence_and_nonce: true,
            show_preconditions: true,
            show_nested_authorization: true,
            show_tx_source_if_matches_signer: false,
        }
    }
}

/// Error types for transaction formatting operations
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum FormatError {
    /// Unsupported ledger key type
    UnsupportedLedgerKey,
    /// Invalid price (denominator is zero or price.n is negative or price.d is negative)
    InvalidPrice,
}

impl core::fmt::Display for FormatError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            FormatError::UnsupportedLedgerKey => write!(f, "Unsupported ledger key type"),
            FormatError::InvalidPrice => write!(f, "Invalid price"),
        }
    }
}

/// Formats a transaction signature payload into a complete format result
///
/// # Arguments
/// * `tx_signature_payload` - The transaction signature payload to format
/// * `config` - Configuration options for formatting
/// * `signer_address` - The signer address to compare with transaction source
///
/// # Returns
/// A result containing a `FormatResult` with formatted transaction information
pub fn format_transaction_signature_payload(
    tx_signature_payload: &TransactionSignaturePayload,
    config: &FormatConfig,
    signer_address: &str,
) -> Result<Vec<DataEntry>, FormatError> {
    let mut entries = Vec::with_capacity(8);

    // Network ID (only show if not public network)
    add_network_info_if_needed(&mut entries, &tx_signature_payload.network_id);

    match &tx_signature_payload.tagged_transaction {
        TaggedTransaction::EnvelopeTypeTx(tx) => {
            let tx_entries = format_transaction(tx, config, signer_address, false)?;
            entries.extend(tx_entries);
        }
        TaggedTransaction::EnvelopeTypeTxFeeBump(tx) => {
            entries.push(DataEntry::new("Fee Source", tx.fee_source.to_string()));
            entries.push(DataEntry::new(
                "Max Fee",
                format!("{} XLM", format_native_amount(tx.fee)),
            ));
            entries.push(DataEntry::new(
                "Inner Tx",
                "The following details are for the inner transaction".to_string(),
            ));

            match &tx.inner_tx {
                InnerTransaction::EnvelopeTypeTx(tx) => {
                    let tx_entries = format_transaction(tx, config, signer_address, true)?;
                    entries.extend(tx_entries);
                }
            }
        }
    }

    Ok(entries)
}

/// Formats a hash ID preimage for Soroban authorization into data entries
///
/// # Arguments
/// * `auth` - The hash ID preimage Soroban authorization to format
/// * `config` - Configuration options for formatting
///
/// # Returns
/// A vector of data entries containing the authorization details
pub fn format_hash_id_preimage_soroban_authorization(
    auth: &HashIdPreimageSorobanAuthorization,
    config: &FormatConfig,
) -> Vec<DataEntry> {
    let mut entries = Vec::new();

    // Network ID (only show if not public network)
    add_network_info_if_needed(&mut entries, &auth.network_id);

    if config.show_sequence_and_nonce {
        entries.push(DataEntry::new("Nonce", auth.nonce.to_string()));
    }

    entries.push(DataEntry::new(
        "Sig Exp Ledger",
        auth.signature_expiration_ledger.to_string(),
    ));

    entries.extend(format_soroban_authorized_invocation(
        &auth.invocation,
        None,
        config,
    ));

    entries
}

/// Formats a price as a decimal string with 7 decimal places and comma separators
///
/// # Arguments
/// * `price` - The price with numerator and denominator
///
/// # Returns
/// Result containing formatted price string with up to 7 decimal places and comma separators,
/// or an error if the denominator is zero or the price is negative
///
/// # Examples
/// ```ignore
/// let price = Price { n: 1, d: 2 };
/// assert_eq!(format_price(&price).unwrap(), "0.5");
///
/// let price = Price { n: 10000000, d: 3 };
/// assert_eq!(format_price(&price).unwrap(), "3,333,333.3333333");
///
/// let price = Price { n: 1, d: 0 };
/// assert!(format_price(&price).is_err());
///
/// let price = Price { n: -1, d: 2 };
/// assert!(format_price(&price).is_err());
/// ```
fn format_price(price: &Price) -> Result<String, FormatError> {
    // Validate price: denominator must be positive, numerator must be non-negative
    if price.d <= 0 || price.n < 0 {
        return Err(FormatError::InvalidPrice);
    }

    // Safe conversion to u64 since we've validated both are non-negative
    let numerator_u64 = price.n as u64;
    let denominator_u64 = price.d as u64;

    // Calculate with proper overflow checking
    // Multiply by 10^7 to get 7 decimal places
    let scaled_numerator = numerator_u64
        .checked_mul(STROOPS_PER_XLM)
        .ok_or(FormatError::InvalidPrice)?;

    let result = scaled_numerator / denominator_u64;

    let decimal_str = format_decimal(result, STELLAR_NATIVE_DECIMAL_PLACES);
    let formatted = format_number_with_commas(&decimal_str);

    Ok(formatted)
}

/// Formats a transaction into data entries
///
/// Processes the transaction memo, fee, sequence number, preconditions,
/// source account, and operations according to the configuration settings.
///
/// # Arguments
/// * `transaction` - The transaction to format
/// * `config` - Configuration options for formatting (controls visibility of sequence/nonce and preconditions, etc.)
/// * `signer_address` - The signer address to compare with transaction source
/// * `is_inner_tx` - Whether this is an inner transaction of a fee bump transaction
///
/// # Returns
/// A result containing a vector of data entries representing the formatted transaction
///
/// # Note
/// The order of entries follows Stellar transaction structure:
/// 1. Memo (if present)
/// 2. Max Fee
/// 3. Sequence Number (if enabled in config)
/// 4. Preconditions (if enabled in config)
/// 5. Transaction Source (conditionally shown based on config)
/// 6. Operations
fn format_transaction(
    transaction: &Transaction,
    config: &FormatConfig,
    signer_address: &str,
    is_inner_tx: bool,
) -> Result<Vec<DataEntry>, FormatError> {
    let mut entries = Vec::with_capacity(16);

    match &transaction.memo {
        Memo::None => {}
        Memo::Text(text) => {
            let text_str = text.to_string();
            entries.push(DataEntry::new("Memo Text", text_str));
        }
        Memo::Id(id) => {
            let id_str = id.to_string();
            entries.push(DataEntry::new("Memo ID", id_str));
        }
        Memo::Hash(hash) => {
            let hash_str = hex::encode(hash.as_bytes());
            entries.push(DataEntry::new("Memo Hash", hash_str));
        }
        Memo::Return(hash) => {
            let hash_str = hex::encode(hash.as_bytes());
            entries.push(DataEntry::new("Memo Return", hash_str));
        }
    }

    entries.push(DataEntry::new(
        "Max Fee",
        format!("{} XLM", format_native_amount(transaction.fee)),
    ));

    if config.show_sequence_and_nonce {
        let seq_str = transaction.seq_num.to_string();
        entries.push(DataEntry::new("Sequence Num", seq_str));
    }

    if config.show_preconditions {
        match &transaction.cond {
            Preconditions::None => {}
            Preconditions::Time(time_bounds) => {
                if time_bounds.min_time != 0 {
                    entries.push(DataEntry::new(
                        "Valid After",
                        format_unix_timestamp(time_bounds.min_time),
                    ));
                }

                if time_bounds.max_time != 0 {
                    // only show if not zero
                    entries.push(DataEntry::new(
                        "Valid Before",
                        format_unix_timestamp(time_bounds.max_time),
                    ));
                }
            }
            Preconditions::V2(cond) => {
                if let Some(time_bounds) = &cond.time_bounds {
                    if time_bounds.min_time != 0 {
                        entries.push(DataEntry::new(
                            "Valid After",
                            format_unix_timestamp(time_bounds.min_time),
                        ));
                    }

                    if time_bounds.max_time != 0 {
                        // only show if not zero
                        entries.push(DataEntry::new(
                            "Valid Before",
                            format_unix_timestamp(time_bounds.max_time),
                        ));
                    }
                }
                if let Some(ledger_bounds) = &cond.ledger_bounds {
                    if ledger_bounds.min_ledger != 0 {
                        entries.push(DataEntry::new(
                            "Min Ledger",
                            ledger_bounds.min_ledger.to_string(),
                        ));
                    }

                    if ledger_bounds.max_ledger != 0 {
                        entries.push(DataEntry::new(
                            "Max Ledger",
                            ledger_bounds.max_ledger.to_string(),
                        ));
                    }
                }
                if let Some(min_seq_num) = cond.min_seq_num {
                    if min_seq_num != 0 {
                        entries.push(DataEntry::new("Min Seq Num", min_seq_num.to_string()));
                    }
                }
                if cond.min_seq_age != 0 {
                    entries.push(DataEntry::new("Min Seq Age", cond.min_seq_age.to_string()));
                }
                if cond.min_seq_ledger_gap != 0 {
                    entries.push(DataEntry::new(
                        "Min Seq Ledger Gap",
                        cond.min_seq_ledger_gap.to_string(),
                    ));
                }
                for (i, signer) in cond.extra_signers.iter().enumerate() {
                    entries.push(DataEntry::new(
                        &format!("Extra Signer {}", i + 1),
                        signer.to_string(),
                    ));
                }
            }
        }
    }

    // Only show transaction source if:
    // 1. This is an inner transaction of a fee bump (always show), or
    // 2. Config allows showing when it matches the signer, or
    // 3. It doesn't match the signer address
    let source_str = transaction.source_account.to_string();
    if is_inner_tx || config.show_tx_source_if_matches_signer || source_str != signer_address {
        entries.push(DataEntry::new("Tx Source", source_str));
    }

    Ok(entries)
}

/// Formats transaction operations into data entries
///
/// # Arguments
/// * `operation` - The operation to format
/// * `config` - Configuration options for formatting
/// * `tx_source` - The transaction source account address
///
/// # Returns
/// A result containing a vector of data entries representing the formatted transaction
pub fn format_operation(
    operation: &Operation,
    config: &FormatConfig,
    tx_source: &str,
) -> Result<Vec<DataEntry>, FormatError> {
    let mut entries = Vec::with_capacity(8);

    // Determine the effective operation source (op source if present, otherwise tx source)
    let op_source = operation
        .source_account
        .as_ref()
        .map(|acc| acc.to_string())
        .unwrap_or_else(|| tx_source.to_string());

    match &operation.body {
        OperationBody::CreateAccount(op) => {
            entries.extend(format_create_account_op(op));
        }
        OperationBody::Payment(op) => {
            entries.extend(format_payment_op(op));
        }
        OperationBody::PathPaymentStrictReceive(op) => {
            entries.extend(format_path_payment_strict_receive_op(op, &op_source));
        }
        OperationBody::ManageSellOffer(op) => {
            entries.extend(format_manage_sell_offer_op(op)?);
        }
        OperationBody::CreatePassiveSellOffer(op) => {
            entries.extend(format_create_passive_sell_offer_op(op)?);
        }
        OperationBody::SetOptions(op) => {
            entries.extend(format_set_options_op(op));
        }
        OperationBody::ChangeTrust(op) => {
            entries.extend(format_change_trust_op(op));
        }
        OperationBody::AllowTrust(op) => {
            entries.extend(format_allow_trust_op(op));
        }
        OperationBody::AccountMerge(destination) => {
            entries.extend(format_account_merge_op(destination));
        }
        OperationBody::Inflation => {
            // Disabled in Stellar Protocol 12
            entries.push(DataEntry::new("Operation Type", "Inflation".to_string()));
        }
        OperationBody::ManageData(op) => {
            entries.extend(format_manage_data_op(op));
        }
        OperationBody::BumpSequence(op) => {
            entries.extend(format_bump_sequence_op(op));
        }
        OperationBody::ManageBuyOffer(op) => {
            entries.extend(format_manage_buy_offer_op(op)?);
        }
        OperationBody::PathPaymentStrictSend(op) => {
            entries.extend(format_path_payment_strict_send_op(op, &op_source));
        }
        OperationBody::CreateClaimableBalance(op) => {
            entries.extend(format_create_claimable_balance_op(op));
        }
        OperationBody::ClaimClaimableBalance(op) => {
            entries.extend(format_claim_claimable_balance_op(op));
        }
        OperationBody::BeginSponsoringFutureReserves(op) => {
            entries.extend(format_begin_sponsoring_future_reserves_op(op));
        }
        OperationBody::EndSponsoringFutureReserves => {
            entries.push(DataEntry::new(
                "Operation Type",
                "End Sponsoring Future Reserves".to_string(),
            ));
        }
        OperationBody::RevokeSponsorship(op) => {
            entries.extend(format_revoke_sponsorship_op(op)?);
        }
        OperationBody::Clawback(op) => {
            entries.extend(format_clawback_op(op));
        }
        OperationBody::ClawbackClaimableBalance(op) => {
            entries.extend(format_clawback_claimable_balance_op(op));
        }
        OperationBody::SetTrustLineFlags(op) => {
            entries.extend(format_set_trust_line_flags_op(op));
        }
        OperationBody::LiquidityPoolDeposit(op) => {
            entries.extend(format_liquidity_pool_deposit_op(op)?);
        }
        OperationBody::LiquidityPoolWithdraw(op) => {
            entries.extend(format_liquidity_pool_withdraw_op(op));
        }
        OperationBody::InvokeHostFunction(op) => {
            entries.extend(format_invoke_host_function_op(op, config));
        }
        OperationBody::ExtendFootprintTTL(op) => {
            entries.extend(format_extend_footprint_ttl_op(op));
        }
        OperationBody::RestoreFootprint(op) => {
            entries.extend(format_restore_footprint_op(op));
        }
    }

    if let Some(source) = &operation.source_account {
        entries.push(DataEntry::new("Op Source", source.to_string()));
    }

    Ok(entries)
}

fn offer_action(offer_id: i64, amount: i64) -> String {
    match (offer_id, amount) {
        (0, _) => "create offer",
        (_, 0) => "remove offer",
        _ => "change offer",
    }
    .into()
}

/// Determines the intent string for a single operation
///
/// # Arguments
/// * `operation` - The operation to analyze
/// * `tx_source` - The transaction source account address
///
/// # Returns
/// An optional intent string describing the operation's purpose
pub fn get_operation_intent(operation: &Operation, tx_source: &str) -> Option<String> {
    // Determine the effective operation source (op source if present, otherwise tx source)
    let op_source = operation
        .source_account
        .as_ref()
        .map(|acc| acc.to_string())
        .unwrap_or_else(|| tx_source.to_string());

    match &operation.body {
        OperationBody::CreateAccount(_) => Some("send XLM".into()),
        OperationBody::Payment(op) => Some(format!("send {}", format_asset_code(&op.asset))),
        OperationBody::PathPaymentStrictReceive(op) => {
            let destination = op.destination.to_string();
            if destination == op_source {
                Some("swap".into())
            } else {
                Some(format!("send {}", format_asset_code(&op.send_asset)))
            }
        }
        OperationBody::PathPaymentStrictSend(op) => {
            let destination = op.destination.to_string();
            if destination == op_source {
                Some("swap".into())
            } else {
                Some(format!("send {}", format_asset_code(&op.send_asset)))
            }
        }
        OperationBody::ManageSellOffer(op) => Some(offer_action(op.offer_id, op.amount)),
        OperationBody::ManageBuyOffer(op) => Some(offer_action(op.offer_id, op.buy_amount)),
        OperationBody::CreatePassiveSellOffer(_) => Some("create offer".into()),
        OperationBody::SetOptions(_) => Some("set options".into()),
        OperationBody::ChangeTrust(_) => Some("change trust line".into()),
        OperationBody::AllowTrust(_) => Some("allow trust".into()),
        OperationBody::AccountMerge(_) => Some("merge account".into()),
        OperationBody::Inflation => None,
        OperationBody::ManageData(_) => Some("manage data".into()),
        OperationBody::BumpSequence(_) => Some("bump sequence".into()),
        OperationBody::CreateClaimableBalance(_) => Some("create claimable balance".into()),
        OperationBody::ClaimClaimableBalance(_) => Some("claim claimable balance".into()),
        OperationBody::BeginSponsoringFutureReserves(_) => None,
        OperationBody::EndSponsoringFutureReserves => None,
        OperationBody::RevokeSponsorship(_) => Some("revoke sponsorship".into()),
        OperationBody::Clawback(_) => Some("clawback asset".into()),
        OperationBody::ClawbackClaimableBalance(_) => Some("clawback claimable balance".into()),
        OperationBody::SetTrustLineFlags(_) => Some("set trust line flags".into()),
        OperationBody::LiquidityPoolDeposit(_) => Some("deposit into pool ".into()),
        OperationBody::LiquidityPoolWithdraw(_) => Some("withdraw from pool".into()),
        OperationBody::InvokeHostFunction(op) => match op.host_function {
            HostFunction::InvokeContract(_) => Some("invoke contract".into()),
            HostFunction::CreateContract(_) | HostFunction::CreateContractV2(_) => {
                Some("create contract".into())
            }
            HostFunction::UploadContractWasm(_) => Some("upload WASM".into()),
        },
        OperationBody::ExtendFootprintTTL(_) => Some("extend footprint TTL".into()),
        OperationBody::RestoreFootprint(_) => Some("restore footprint".into()),
    }
}

/// Helper macro to format asset with code and issuer
macro_rules! format_asset_with_issuer {
    ($asset:expr) => {{
        let code = $asset.asset_code.to_string();
        let issuer = format_issuer(&$asset.issuer.to_string());
        format!("{}@{}", code, issuer)
    }};
}

fn format_asset(asset: &Asset) -> String {
    match asset {
        Asset::Native => "XLM".to_string(),
        Asset::CreditAlphanum4(alpha4) => format_asset_with_issuer!(alpha4),
        Asset::CreditAlphanum12(alpha12) => format_asset_with_issuer!(alpha12),
    }
}

fn format_asset_code(asset: &Asset) -> String {
    match asset {
        Asset::Native => "XLM".to_string(),
        Asset::CreditAlphanum4(alpha4) => alpha4.asset_code.to_string(),
        Asset::CreditAlphanum12(alpha12) => alpha12.asset_code.to_string(),
    }
}

fn format_trust_line_asset(trust_line_asset: &TrustLineAsset) -> String {
    match trust_line_asset {
        TrustLineAsset::Native => "XLM".to_string(),
        TrustLineAsset::CreditAlphanum4(alpha4) => format_asset_with_issuer!(alpha4),
        TrustLineAsset::CreditAlphanum12(alpha12) => format_asset_with_issuer!(alpha12),
        TrustLineAsset::PoolShare(pool_id) => {
            format!("Liquidity Pool ID: {}", format_pool_id(pool_id))
        }
    }
}

/// Adds network information to entries if not public network
fn add_network_info_if_needed(entries: &mut Vec<DataEntry>, network_id: &Hash) {
    if network_id.as_bytes() == &TESTNET_NETWORK_HASH {
        entries.push(DataEntry::new("Network", "Testnet".to_string()));
    } else if network_id.as_bytes() != &PUBLIC_NETWORK_HASH {
        entries.push(DataEntry::new("Network", "Unknown".to_string()));
    }
}

/// Formats an amount with its asset as "amount asset" string
///
/// # Arguments
/// * `amount` - The amount in stroops
/// * `asset` - The asset to format
///
/// # Returns
/// A formatted string like "1.23 XLM" or "100 USDC:GABCD..XYZA"
///
/// # Examples
/// ```ignore
/// use stellar_xdr::curr::Asset;
///
/// let asset = Asset::Native;
/// assert_eq!(format_amount_with_asset(10000000u64, &asset), "1 XLM");
/// ```
fn format_amount_with_asset<T>(amount: T, asset: &Asset) -> String
where
    T: ToString + Copy,
{
    format!("{} {}", format_native_amount(amount), format_asset(asset))
}

/// Formats an issuer address by showing first 3 and last 4 characters
///
/// # Arguments
/// * `issuer` - The full issuer address
///
/// # Returns
/// A shortened version like "GABCDE..UVWXYZ"
fn format_issuer(issuer: &str) -> String {
    if issuer.len() > 12 {
        format!("{}..{}", &issuer[..3], &issuer[issuer.len() - 4..])
    } else {
        issuer.to_string()
    }
}

/// Generic helper function to format flags from a bitmask
///
/// # Arguments
/// * `flags` - The flags bitmask
/// * `flag_definitions` - Array of (flag_value, flag_name) tuples
/// * `zero_value` - String to return when flags is 0
///
/// # Returns
/// A string listing the flag names or the zero_value
fn format_flags_generic(flags: u32, flag_definitions: &[(u32, &str)], zero_value: &str) -> String {
    if flags == 0 {
        return zero_value.to_string();
    }

    let flag_names: Vec<&str> = flag_definitions
        .iter()
        .filter_map(
            |(flag, name)| {
                if flags & flag != 0 {
                    Some(*name)
                } else {
                    None
                }
            },
        )
        .collect();

    if flag_names.is_empty() {
        format!("{} (unknown flags)", flags)
    } else {
        flag_names.join(", ")
    }
}

/// Formats account flags from a bitmask into flag names
///
/// # Arguments
/// * `flags` - The account flags bitmask
///
/// # Returns
/// A string listing the flag names
fn format_account_flags(flags: u32) -> String {
    const FLAG_DEFS: &[(u32, &str)] = &[
        (AccountFlags::RequiredFlag as u32, "AUTH_REQUIRED"),
        (AccountFlags::RevocableFlag as u32, "AUTH_REVOCABLE"),
        (AccountFlags::ImmutableFlag as u32, "AUTH_IMMUTABLE"),
        (
            AccountFlags::ClawbackEnabledFlag as u32,
            "AUTH_CLAWBACK_ENABLED",
        ),
    ];
    format_flags_generic(flags, FLAG_DEFS, "None")
}

fn format_allow_trust_flags(flags: u32) -> String {
    const FLAG_DEFS: &[(u32, &str)] = &[
        (TrustLineFlags::AuthorizedFlag as u32, "AUTHORIZED"),
        (
            TrustLineFlags::AuthorizedToMaintainLiabilitiesFlag as u32,
            "AUTHORIZED_TO_MAINTAIN_LIABILITIES",
        ),
    ];
    format_flags_generic(flags, FLAG_DEFS, "UNAUTHORIZED")
}

fn format_trust_line_flags(flags: u32) -> String {
    const FLAG_DEFS: &[(u32, &str)] = &[
        (TrustLineFlags::AuthorizedFlag as u32, "AUTHORIZED"),
        (
            TrustLineFlags::AuthorizedToMaintainLiabilitiesFlag as u32,
            "AUTHORIZED_TO_MAINTAIN_LIABILITIES",
        ),
        (
            TrustLineFlags::TrustlineClawbackEnabledFlag as u32,
            "TRUSTLINE_CLAWBACK_ENABLED",
        ),
    ];
    format_flags_generic(flags, FLAG_DEFS, "[none]")
}

fn format_create_account_op(op: &CreateAccountOp) -> Vec<DataEntry> {
    let mut entries = Vec::with_capacity(2);

    entries.push(DataEntry::new(
        "Send",
        format!("{} XLM", format_native_amount(op.starting_balance)),
    ));
    entries.push(DataEntry::new("To", op.destination.to_string()));

    entries
}

fn format_payment_op(op: &PaymentOp) -> Vec<DataEntry> {
    vec![
        DataEntry::new("Send", format_amount_with_asset(op.amount, &op.asset)),
        DataEntry::new("To", op.destination.to_string()),
    ]
}

fn format_path_payment_strict_receive_op(
    op: &PathPaymentStrictReceiveOp,
    op_source: &str,
) -> Vec<DataEntry> {
    let destination = op.destination.to_string();
    let is_swap = destination == op_source;

    if is_swap {
        // This is a swap operation (sender and receiver are the same)
        vec![
            DataEntry::new(
                "Send Max",
                format_amount_with_asset(op.send_max, &op.send_asset),
            ),
            DataEntry::new(
                "Receive",
                format_amount_with_asset(op.dest_amount, &op.dest_asset),
            ),
        ]
    } else {
        // This is a payment operation
        vec![
            DataEntry::new(
                "Send Max",
                format_amount_with_asset(op.send_max, &op.send_asset),
            ),
            DataEntry::new("To", destination),
            DataEntry::new(
                "They Receive",
                format_amount_with_asset(op.dest_amount, &op.dest_asset),
            ),
        ]
    }
}

fn format_manage_sell_offer_op(op: &ManageSellOfferOp) -> Result<Vec<DataEntry>, FormatError> {
    let mut entries = Vec::new();
    if op.offer_id != 0 {
        let offer_id = format!("Offer ID: {}", op.offer_id);
        if op.amount == 0 {
            entries.push(DataEntry::new("Remove Offer", offer_id));
            return Ok(entries);
        }
        entries.push(DataEntry::new("Change Offer", offer_id));
    }

    entries.push(DataEntry::new(
        "Selling",
        format_amount_with_asset(op.amount, &op.selling),
    ));
    entries.push(DataEntry::new(
        "Buying",
        format_asset(&op.buying).to_string(),
    ));
    // Price of 1 unit of selling in terms of buying
    entries.push(DataEntry::new(
        "Price",
        format!(
            "{} {}/{}",
            format_price(&op.price)?,
            format_asset_code(&op.buying),
            format_asset_code(&op.selling)
        ),
    ));
    Ok(entries)
}

fn format_create_passive_sell_offer_op(
    op: &CreatePassiveSellOfferOp,
) -> Result<Vec<DataEntry>, FormatError> {
    let mut entries = Vec::new();
    entries.push(DataEntry::new(
        "Selling",
        format_amount_with_asset(op.amount, &op.selling),
    ));
    entries.push(DataEntry::new(
        "Buying",
        format_asset(&op.buying).to_string(),
    ));
    entries.push(DataEntry::new(
        "Price",
        format!(
            "{} {}/{}",
            format_price(&op.price)?,
            format_asset_code(&op.buying),
            format_asset_code(&op.selling)
        ),
    ));
    Ok(entries)
}

fn format_set_options_op(op: &SetOptionsOp) -> Vec<DataEntry> {
    let mut entries = Vec::new();
    entries.push(DataEntry::new("Operation Type", "Set Options".to_string()));
    if let Some(inflation_dest) = &op.inflation_dest {
        entries.push(DataEntry::new("Inflation Dest", inflation_dest.to_string()));
    }

    if let Some(clear_flags) = &op.clear_flags {
        entries.push(DataEntry::new(
            "Clear Flags",
            format_account_flags(*clear_flags),
        ));
    }

    if let Some(set_flags) = &op.set_flags {
        entries.push(DataEntry::new(
            "Set Flags",
            format_account_flags(*set_flags),
        ));
    }

    if let Some(master_weight) = &op.master_weight {
        entries.push(DataEntry::new("Master Weight", master_weight.to_string()));
    }

    if let Some(low_threshold) = &op.low_threshold {
        entries.push(DataEntry::new("Low Threshold", low_threshold.to_string()));
    }

    if let Some(med_threshold) = &op.med_threshold {
        entries.push(DataEntry::new(
            "Medium Threshold",
            med_threshold.to_string(),
        ));
    }

    if let Some(high_threshold) = &op.high_threshold {
        entries.push(DataEntry::new("High Threshold", high_threshold.to_string()));
    }

    if let Some(home_domain) = &op.home_domain {
        let domain_entry = if home_domain.is_empty() {
            DataEntry::new("Home Domain", "[set home domain to empty]".to_string())
        } else {
            DataEntry::new("Home Domain", home_domain.to_string())
        };
        entries.push(domain_entry);
    }

    if let Some(signer) = &op.signer {
        let signer_entry = if signer.weight == 0 {
            DataEntry::new("Remove Signer", signer.key.to_string())
        } else {
            DataEntry::new(
                "Add Signer",
                format!("{} (weight {})", signer.key, signer.weight),
            )
        };
        entries.push(signer_entry);
    }

    entries
}

fn format_change_trust_op(op: &ChangeTrustOp) -> Vec<DataEntry> {
    let mut entries = Vec::new();
    let action = if op.limit == 0 {
        "Remove Trust"
    } else {
        "Change Trust"
    };

    match &op.line {
        ChangeTrustAsset::Native => entries.push(DataEntry::new(action, "XLM".to_string())),
        ChangeTrustAsset::CreditAlphanum4(alpha_num4) => {
            entries.push(DataEntry::new(action, alpha_num4.asset_code.to_string()));
            entries.push(DataEntry::new(
                "Asset Issuer",
                alpha_num4.issuer.to_string(),
            ));
        }
        ChangeTrustAsset::CreditAlphanum12(alpha_num12) => {
            entries.push(DataEntry::new(action, alpha_num12.asset_code.to_string()));
            entries.push(DataEntry::new(
                "Asset Issuer",
                alpha_num12.issuer.to_string(),
            ));
        }
        ChangeTrustAsset::PoolShare(liquidity_pool_parameters) => {
            entries.push(DataEntry::new(action, "Liquidity Pool Asset".to_string()));
            match liquidity_pool_parameters {
                LiquidityPoolParameters::ConstantProduct(
                    liquidity_pool_constant_product_parameters,
                ) => {
                    entries.push(DataEntry::new(
                        "Asset A",
                        format_asset(&liquidity_pool_constant_product_parameters.asset_a),
                    ));
                    entries.push(DataEntry::new(
                        "Asset B",
                        format_asset(&liquidity_pool_constant_product_parameters.asset_b),
                    ));
                    entries.push(DataEntry::new(
                        "Pool Fee Rate",
                        format!(
                            "{}%",
                            liquidity_pool_constant_product_parameters.fee as f64
                                / BASIS_POINTS_TO_PERCENT
                        ),
                    ));
                }
            }
        }
    }

    if op.limit != 0 {
        if op.limit == i64::MAX {
            entries.push(DataEntry::new("Trust Limit", "[unlimited]".to_string()));
        } else {
            entries.push(DataEntry::new(
                "Trust Limit",
                format_native_amount(op.limit),
            ));
        }
    }

    entries
}

fn format_allow_trust_op(op: &AllowTrustOp) -> Vec<DataEntry> {
    vec![
        DataEntry::new("Operation Type", "Allow Trust".to_string()),
        DataEntry::new("Trustor", op.trustor.to_string()),
        DataEntry::new("Asset Code", op.asset.to_string()),
        DataEntry::new("Authorize", format_allow_trust_flags(op.authorize)),
    ]
}

fn format_account_merge_op(destination: &MuxedAccount) -> Vec<DataEntry> {
    vec![
        DataEntry::new(
            "Account Merge",
            "Send all XLM to destination and close account".to_string(),
        ),
        DataEntry::new("Destination", destination.to_string()),
    ]
}

fn format_manage_data_op(op: &ManageDataOp) -> Vec<DataEntry> {
    let mut entries = Vec::new();
    if let Some(data_value) = &op.data_value {
        entries.push(DataEntry::new("Set Data", op.data_name.to_string()));

        entries.push(DataEntry::new("Data Value", data_value.to_string()));
    } else {
        entries.push(DataEntry::new("Remove Data", op.data_name.to_string()));
    }
    entries
}

fn format_bump_sequence_op(op: &BumpSequenceOp) -> Vec<DataEntry> {
    vec![
        DataEntry::new("Operation Type", "Bump Sequence".to_string()),
        DataEntry::new("Bump To", op.bump_to.to_string()),
    ]
}

fn format_manage_buy_offer_op(op: &ManageBuyOfferOp) -> Result<Vec<DataEntry>, FormatError> {
    let mut entries = Vec::new();
    if op.offer_id != 0 {
        let offer_id = format!("Offer ID: {}", op.offer_id);
        if op.buy_amount == 0 {
            entries.push(DataEntry::new("Remove Offer", offer_id));
            return Ok(entries);
        }
        entries.push(DataEntry::new("Change Offer", offer_id));
    }

    entries.push(DataEntry::new(
        "Buying",
        format_amount_with_asset(op.buy_amount, &op.buying),
    ));
    entries.push(DataEntry::new(
        "Selling",
        format_asset(&op.selling).to_string(),
    ));
    entries.push(DataEntry::new(
        "Price",
        format!(
            "{} {}/{}",
            format_price(&op.price)?,
            format_asset_code(&op.selling),
            format_asset_code(&op.buying)
        ),
    ));
    Ok(entries)
}

fn format_path_payment_strict_send_op(
    op: &PathPaymentStrictSendOp,
    op_source: &str,
) -> Vec<DataEntry> {
    let destination = op.destination.to_string();
    let is_swap = destination == op_source;

    if is_swap {
        // This is a swap operation (sender and receiver are the same)
        vec![
            DataEntry::new(
                "Send",
                format_amount_with_asset(op.send_amount, &op.send_asset),
            ),
            DataEntry::new(
                "Receive Min",
                format_amount_with_asset(op.dest_min, &op.dest_asset),
            ),
        ]
    } else {
        // This is a payment operation
        vec![
            DataEntry::new(
                "Send",
                format_amount_with_asset(op.send_amount, &op.send_asset),
            ),
            DataEntry::new("To", destination),
            DataEntry::new(
                "They Receive Min",
                format_amount_with_asset(op.dest_min, &op.dest_asset),
            ),
        ]
    }
}

fn format_create_claimable_balance_op(op: &CreateClaimableBalanceOp) -> Vec<DataEntry> {
    let mut entries = Vec::new();
    entries.push(DataEntry::new(
        "Operation Type",
        "Create Claimable Balance".to_string(),
    ));
    entries.push(DataEntry::new(
        "Balance",
        format_amount_with_asset(op.amount, &op.asset),
    ));
    for (index, claimant) in op.claimants.iter().enumerate() {
        entries.push(DataEntry::new(
            &format!("Claimant {}", index + 1),
            serde_json::to_string(&claimant)
                .unwrap_or_else(|_| "[unserializable data]".to_string()),
        ));
    }
    entries
}

fn format_claim_claimable_balance_op(op: &ClaimClaimableBalanceOp) -> Vec<DataEntry> {
    vec![
        DataEntry::new("Operation Type", "Claim Claimable Balance".to_string()),
        DataEntry::new("Balance ID", op.balance_id.to_string()),
    ]
}

fn format_begin_sponsoring_future_reserves_op(
    op: &BeginSponsoringFutureReservesOp,
) -> Vec<DataEntry> {
    vec![
        DataEntry::new(
            "Operation Type",
            "Begin Sponsoring Future Reserves".to_string(),
        ),
        DataEntry::new("Sponsored ID", op.sponsored_id.to_string()),
    ]
}

fn format_revoke_sponsorship_op(op: &RevokeSponsorshipOp) -> Result<Vec<DataEntry>, FormatError> {
    let mut entries = Vec::new();

    match op {
        RevokeSponsorshipOp::LedgerEntry(key) => match key {
            LedgerKey::Account(account) => {
                entries.push(DataEntry::new(
                    "Operation Type",
                    "Revoke Sponsorship (ACCOUNT)".to_string(),
                ));
                entries.push(DataEntry::new("Account ID", account.account_id.to_string()));
            }
            LedgerKey::Trustline(trust_line) => {
                entries.push(DataEntry::new(
                    "Operation Type",
                    "Revoke Sponsorship (TRUSTLINE)".to_string(),
                ));
                entries.push(DataEntry::new(
                    "Account ID",
                    trust_line.account_id.to_string(),
                ));
                entries.push(DataEntry::new(
                    "Asset",
                    format_trust_line_asset(&trust_line.asset).to_string(),
                ));
            }
            LedgerKey::Offer(offer) => {
                entries.push(DataEntry::new(
                    "Operation Type",
                    "Revoke Sponsorship (OFFER)".to_string(),
                ));
                entries.push(DataEntry::new("Seller ID", offer.seller_id.to_string()));
                entries.push(DataEntry::new("Offer ID", offer.offer_id.to_string()));
            }
            LedgerKey::Data(data) => {
                entries.push(DataEntry::new(
                    "Operation Type",
                    "Revoke Sponsorship (DATA)".to_string(),
                ));
                entries.push(DataEntry::new("Account ID", data.account_id.to_string()));
                entries.push(DataEntry::new("Data Name", data.data_name.to_string()));
            }
            LedgerKey::ClaimableBalance(balance) => {
                entries.push(DataEntry::new(
                    "Operation Type",
                    "Revoke Sponsorship (CLAIMABLE_BALANCE)".to_string(),
                ));
                entries.push(DataEntry::new("Balance ID", balance.balance_id.to_string()));
            }
            LedgerKey::LiquidityPool(pool) => {
                entries.push(DataEntry::new(
                    "Operation Type",
                    "Revoke Sponsorship (LIQUIDITY_POOL)".to_string(),
                ));
                entries.push(DataEntry::new(
                    "Pool ID",
                    format_pool_id(&pool.liquidity_pool_id),
                ));
            }
            _ => {
                return Err(FormatError::UnsupportedLedgerKey);
            }
        },
        RevokeSponsorshipOp::Signer(signer) => {
            entries.push(DataEntry::new(
                "Operation Type",
                "Revoke Sponsorship (SIGNER_KEY)".to_string(),
            ));
            entries.push(DataEntry::new("Account ID", signer.account_id.to_string()));
            entries.push(DataEntry::new("Signer Key", signer.signer_key.to_string()));
        }
    }
    Ok(entries)
}

fn format_clawback_op(op: &ClawbackOp) -> Vec<DataEntry> {
    vec![
        DataEntry::new("Operation Type", "Clawback".to_string()),
        DataEntry::new("Clawback", format_amount_with_asset(op.amount, &op.asset)),
        DataEntry::new("From", op.from.to_string()),
    ]
}

fn format_clawback_claimable_balance_op(op: &ClawbackClaimableBalanceOp) -> Vec<DataEntry> {
    vec![
        DataEntry::new("Operation Type", "Clawback Claimable Balance".to_string()),
        DataEntry::new("Balance ID", op.balance_id.to_string()),
    ]
}

fn format_set_trust_line_flags_op(op: &SetTrustLineFlagsOp) -> Vec<DataEntry> {
    vec![
        DataEntry::new("Operation Type", "Set Trust Line Flags".to_string()),
        DataEntry::new("Trustor", op.trustor.to_string()),
        DataEntry::new("Asset", format_asset(&op.asset).to_string()),
        DataEntry::new("Clear Flags", format_trust_line_flags(op.clear_flags)),
        DataEntry::new("Set Flags", format_trust_line_flags(op.set_flags)),
    ]
}

fn format_liquidity_pool_deposit_op(
    op: &LiquidityPoolDepositOp,
) -> Result<Vec<DataEntry>, FormatError> {
    Ok(vec![
        DataEntry::new("Operation Type", "Liquidity Pool Deposit".to_string()),
        DataEntry::new("Pool ID", format_pool_id(&op.liquidity_pool_id)),
        DataEntry::new(
            "Max Amount A",
            format_native_amount(op.max_amount_a).to_string(),
        ),
        DataEntry::new(
            "Max Amount B",
            format_native_amount(op.max_amount_b).to_string(),
        ),
        DataEntry::new("Min Price", format_price(&op.min_price)?),
        DataEntry::new("Max Price", format_price(&op.max_price)?),
    ])
}

fn format_liquidity_pool_withdraw_op(op: &LiquidityPoolWithdrawOp) -> Vec<DataEntry> {
    vec![
        DataEntry::new("Operation Type", "Liquidity Pool Withdraw".to_string()),
        DataEntry::new("Pool ID", format_pool_id(&op.liquidity_pool_id)),
        DataEntry::new("Amount", format_native_amount(op.amount).to_string()),
        DataEntry::new(
            "Min Amount A",
            format_native_amount(op.min_amount_a).to_string(),
        ),
        DataEntry::new(
            "Min Amount B",
            format_native_amount(op.min_amount_b).to_string(),
        ),
    ]
}

fn format_create_contract_args(args: &CreateContractArgs) -> Vec<DataEntry> {
    let mut entries = Vec::new();
    match &args.executable {
        ContractExecutable::Wasm(wasm) => {
            entries.push(DataEntry::new(
                "Create Contract",
                format!(
                    "Deploy contract from WASM hash: {}",
                    hex::encode(wasm.as_bytes())
                ),
            ));
        }
        ContractExecutable::StellarAsset => {
            entries.push(DataEntry::new(
                "Create Contract",
                "Deploy Stellar asset contract".to_string(),
            ));
        }
    }
    entries
}

fn format_create_contract_args_v2(args: &CreateContractArgsV2) -> Vec<DataEntry> {
    let mut entries = Vec::new();
    match &args.executable {
        ContractExecutable::Wasm(wasm) => {
            entries.push(DataEntry::new(
                "Create Contract",
                format!(
                    "Deploy contract from WASM hash: {}",
                    hex::encode(wasm.as_bytes())
                ),
            ));
        }
        ContractExecutable::StellarAsset => {
            entries.push(DataEntry::new(
                "Create Contract",
                "Deploy Stellar asset contract".to_string(),
            ));
        }
    }
    for (i, arg) in args.constructor_args.iter().enumerate() {
        entries.push(DataEntry::new(
            &format!("Arg {} of {}", i + 1, args.constructor_args.len()),
            scval_to_key_string(arg),
        ));
    }
    entries
}

fn format_invoke_contract_args(args: &InvokeContractArgs) -> Vec<DataEntry> {
    // Try to format as a known token contract call first
    if let Some(entries) = crate::token_formatter::try_format_token_contract_call(args) {
        return entries;
    }

    // Default formatting for unknown contracts
    let mut entries = Vec::new();
    entries.push(DataEntry::new(
        "Contract ID",
        args.contract_address.to_string(),
    ));
    entries.push(DataEntry::new("Function", args.function_name.to_string()));
    for (i, arg) in args.args.iter().enumerate() {
        entries.push(DataEntry::new(
            &format!("Arg {} of {}", i + 1, args.args.len()),
            scval_to_key_string(arg),
        ));
    }
    entries
}

/// Formats a Soroban authorized invocation into data entries
///
/// # Arguments
/// * `invocation` - The Soroban authorized invocation to format
/// * `parent_index` - Optional parent index for nested invocations (e.g., "1.2")
///
/// # Returns
/// A vector of data entries containing the invocation details and any sub-invocations
///
/// # Examples
/// ```ignore
/// // Top-level invocation (no parent)
/// let entries = format_soroban_authorized_invocation(&invocation, None);
///
/// // Nested invocation with parent index
/// let entries = format_soroban_authorized_invocation(&invocation, Some("1.2"));
/// ```
fn format_soroban_authorized_invocation(
    invocation: &SorobanAuthorizedInvocation,
    parent_index: Option<&str>,
    config: &FormatConfig,
) -> Vec<DataEntry> {
    let mut entries = Vec::new();
    match &invocation.function {
        SorobanAuthorizedFunction::ContractFn(args) => {
            entries.extend(format_invoke_contract_args(args));
        }
        SorobanAuthorizedFunction::CreateContractHostFn(args) => {
            entries.extend(format_create_contract_args(args));
        }
        SorobanAuthorizedFunction::CreateContractV2HostFn(args) => {
            entries.extend(format_create_contract_args_v2(args));
        }
    }

    if config.show_nested_authorization {
        for (i, sub_invocation) in invocation.sub_invocations.iter().enumerate() {
            let nested_index = match parent_index {
                None => (i + 1).to_string(),
                Some(parent) => format!("{}-{}", parent, i + 1),
            };

            entries.push(DataEntry::new("Nested Authorization", nested_index.clone()));
            entries.extend(format_soroban_authorized_invocation(
                sub_invocation,
                Some(&nested_index),
                config,
            ));
        }
    }

    entries
}

fn format_invoke_host_function_op(
    op: &InvokeHostFunctionOp,
    config: &FormatConfig,
) -> Vec<DataEntry> {
    let mut entries = Vec::new();
    match &op.host_function {
        HostFunction::InvokeContract(args) => {
            entries.extend(format_invoke_contract_args(args));
        }
        HostFunction::CreateContract(args) => {
            entries.extend(format_create_contract_args(args));
        }
        HostFunction::UploadContractWasm(_) => {
            entries.push(DataEntry::new(
                "Upload WASM",
                "Upload Smart Contract WASM".to_string(),
            ));
        }
        HostFunction::CreateContractV2(args) => {
            entries.extend(format_create_contract_args_v2(args));
        }
    }

    if config.show_nested_authorization {
        let mut auth_index = 1;
        for auth in op.auth.iter() {
            match &auth.credentials {
                SorobanCredentials::SourceAccount => {
                    for (j, sub_invocation) in
                        auth.root_invocation.sub_invocations.iter().enumerate()
                    {
                        let index = format!("{}-{}", auth_index, j + 1);
                        entries.push(DataEntry::new("Nested Authorization", index.clone()));
                        entries.extend(format_soroban_authorized_invocation(
                            sub_invocation,
                            Some(&index),
                            config,
                        ));
                    }
                    auth_index += 1;
                }
                SorobanCredentials::Address(_) => {
                    // skip, these are not related to the current signatories,
                    // and we will not authorize these things.
                }
            }
        }
    }

    entries
}

fn format_extend_footprint_ttl_op(_op: &ExtendFootprintTTLOp) -> Vec<DataEntry> {
    vec![DataEntry::new(
        "Operation Type",
        "Extend Footprint TTL".to_string(),
    )]
}

fn format_restore_footprint_op(_op: &RestoreFootprintOp) -> Vec<DataEntry> {
    vec![DataEntry::new(
        "Operation Type",
        "Restore Footprint".to_string(),
    )]
}
