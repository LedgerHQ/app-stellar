//! Display implementations and formatting utilities for Stellar types

extern crate alloc;

use crate::parser::{
    AssetCode, AssetCode12, AssetCode4, BytesM, ClaimableBalanceId, Ed25519SignedPayload,
    Int128Parts, Int256Parts, MuxedAccount, MuxedEd25519, PublicKey, ScAddress, SignerKey, StringM,
    UInt128Parts, UInt256Parts, Uint256,
};
use alloc::string::ToString;
use chrono::DateTime;
use core::fmt;

use alloc::format;
use alloc::string::String;
use alloc::vec::Vec;
use core::fmt::Display;
use ethnum::{I256, U256};

/// Stellar uses 7 decimal places for precision (1 XLM = 10^7 stroops)
pub const STELLAR_NATIVE_DECIMAL_PLACES: u32 = 7;

// ============================================================================
// Display implementations for numeric types
// ============================================================================

pub fn u128_str_from_pieces(hi: u64, lo: u64) -> impl Display {
    (u128::from(hi) << 64) | u128::from(lo)
}

pub fn i128_str_from_pieces(hi: i64, lo: u64) -> impl Display {
    (i128::from(hi) << 64) | i128::from(lo)
}

pub fn u256_str_from_pieces(hi_hi: u64, hi_lo: u64, lo_hi: u64, lo_lo: u64) -> impl Display {
    u256_from_pieces(hi_hi, hi_lo, lo_hi, lo_lo)
}

pub fn i256_str_from_pieces(hi_hi: i64, hi_lo: u64, lo_hi: u64, lo_lo: u64) -> impl Display {
    i256_from_pieces(hi_hi, hi_lo, lo_hi, lo_lo)
}

// The following functions were copied from:
// https://github.com/stellar/rs-soroban-env/blob/fdf898963d314f2edec4a2b1e609f70c6737638a/soroban-env-common/src/num.rs#L431-L455

fn u256_from_pieces(hi_hi: u64, hi_lo: u64, lo_hi: u64, lo_lo: u64) -> U256 {
    let high = (u128::from(hi_hi) << 64) | u128::from(hi_lo);
    let low = (u128::from(lo_hi) << 64) | u128::from(lo_lo);
    U256::from_words(high, low)
}

#[allow(clippy::cast_possible_wrap)]
#[allow(clippy::cast_sign_loss)]
fn i256_from_pieces(hi_hi: i64, hi_lo: u64, lo_hi: u64, lo_lo: u64) -> I256 {
    let high = ((u128::from(hi_hi as u64) << 64) | u128::from(hi_lo)) as i128;
    let low = ((u128::from(lo_hi) << 64) | u128::from(lo_lo)) as i128;
    I256::from_words(high, low)
}

impl fmt::Display for UInt128Parts {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let v = u128_str_from_pieces(self.hi, self.lo);
        write!(f, "{v}")
    }
}

impl fmt::Display for Int128Parts {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let v = i128_str_from_pieces(self.hi, self.lo);
        write!(f, "{v}")
    }
}

impl fmt::Display for UInt256Parts {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let u256 = u256_str_from_pieces(self.hi_hi, self.hi_lo, self.lo_hi, self.lo_lo);
        write!(f, "{u256}")
    }
}

impl fmt::Display for Int256Parts {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let i256 = i256_str_from_pieces(self.hi_hi, self.hi_lo, self.lo_hi, self.lo_lo);
        write!(f, "{i256}")
    }
}

// ============================================================================
// Display implementations for strkey types
// ============================================================================

impl<'a> fmt::Display for PublicKey<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            PublicKey::PublicKeyTypeEd25519(Uint256(k)) => {
                write!(f, "{}", stellar_strkey::ed25519::PublicKey(**k))
            }
        }
    }
}

// Note: ContractId and PoolId are type aliases for Hash/Uint256, so we cannot
// implement Display for them separately. Use the formatting functions below instead.

/// Format a ContractId (Hash) as a strkey-encoded string
pub fn format_contract_id(contract_id: &Uint256) -> alloc::string::String {
    let Uint256(h) = contract_id;
    stellar_strkey::Contract(**h).to_string()
}

/// Format a PoolId (Hash) as a strkey-encoded string
pub fn format_pool_id(pool_id: &Uint256) -> alloc::string::String {
    let Uint256(p_id) = pool_id;
    stellar_strkey::Strkey::LiquidityPool(stellar_strkey::LiquidityPool(**p_id)).to_string()
}

impl<'a> fmt::Display for MuxedEd25519<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let MuxedEd25519 {
            ed25519: Uint256(ed25519),
            id,
        } = self;
        write!(
            f,
            "{}",
            stellar_strkey::ed25519::MuxedAccount {
                ed25519: **ed25519,
                id: *id,
            }
        )
    }
}

impl<'a> fmt::Display for MuxedAccount<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            MuxedAccount::KeyTypeEd25519(Uint256(k)) => {
                write!(f, "{}", stellar_strkey::ed25519::PublicKey(**k))
            }
            MuxedAccount::KeyTypeMuxedEd25519(m) => m.fmt(f),
        }
    }
}

impl<'a> fmt::Display for Ed25519SignedPayload<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let Ed25519SignedPayload {
            ed25519: Uint256(ed25519),
            payload,
        } = self;
        write!(
            f,
            "{}",
            stellar_strkey::ed25519::SignedPayload {
                ed25519: **ed25519,
                payload: (*payload).into(),
            }
        )
    }
}

impl<'a> fmt::Display for SignerKey<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            SignerKey::SignerKeyTypeEd25519(Uint256(k)) => {
                write!(f, "{}", stellar_strkey::ed25519::PublicKey(**k))
            }
            SignerKey::SignerKeyTypePreAuthTx(Uint256(h)) => {
                write!(f, "{}", stellar_strkey::PreAuthTx(**h))
            }
            SignerKey::SignerKeyTypeHashX(Uint256(h)) => {
                write!(f, "{}", stellar_strkey::HashX(**h))
            }
            SignerKey::SignerKeyTypeEd25519SignedPayload(p) => p.fmt(f),
        }
    }
}

impl<'a> fmt::Display for ScAddress<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ScAddress::ScAddressTypeAccount(a) => a.fmt(f),
            ScAddress::ScAddressTypeContract(contract) => {
                // ContractId is a type alias for Hash/Uint256
                let Uint256(h) = contract;
                write!(f, "{}", stellar_strkey::Contract(**h))
            }
            ScAddress::ScAddressTypeMuxedAccount(muxed_ed25519_account) => {
                write!(
                    f,
                    "{}",
                    stellar_strkey::Strkey::MuxedAccountEd25519(
                        stellar_strkey::ed25519::MuxedAccount {
                            ed25519: *muxed_ed25519_account.ed25519.0,
                            id: muxed_ed25519_account.id,
                        },
                    )
                )
            }
            ScAddress::ScAddressTypeClaimableBalance(claimable_balance_id) => {
                claimable_balance_id.fmt(f)
            }
            ScAddress::ScAddressTypeLiquidityPool(pool_id) => {
                // PoolId is a type alias for Hash/Uint256
                let Uint256(p_id) = pool_id;
                write!(
                    f,
                    "{}",
                    stellar_strkey::Strkey::LiquidityPool(stellar_strkey::LiquidityPool(**p_id))
                )
            }
        }
    }
}

impl<'a> fmt::Display for ClaimableBalanceId<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ClaimableBalanceId::ClaimableBalanceIdTypeV0(hash) => {
                let Uint256(cb_id) = hash;
                let key = stellar_strkey::Strkey::ClaimableBalance(
                    stellar_strkey::ClaimableBalance::V0(**cb_id),
                );
                key.fmt(f)
            }
        }
    }
}

// ============================================================================
// Display implementations for AssetCode types
// ============================================================================

impl<'a> fmt::Display for AssetCode4<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if let Some(last_idx) = self.0.iter().rposition(|c| *c != 0) {
            for b in escape_bytes::Escape::new(&self.0[..=last_idx]) {
                write!(f, "{}", b as char)?;
            }
        }
        Ok(())
    }
}

impl<'a> fmt::Display for AssetCode12<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // AssetCode12's are always rendered as at least 5 characters, because
        // any asset code shorter than 5 characters is an AssetCode4.
        const MIN_LENGTH: usize = 5;
        let len = MIN_LENGTH
            + self
                .0
                .iter()
                .skip(MIN_LENGTH)
                .rposition(|c| *c != 0)
                .map_or(0, |last_idx| last_idx + 1);
        for b in escape_bytes::Escape::new(&self.0[..len]) {
            write!(f, "{}", b as char)?;
        }
        Ok(())
    }
}

impl<'a> fmt::Display for AssetCode<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            AssetCode::AssetCode4(c) => c.fmt(f),
            AssetCode::AssetCode12(c) => c.fmt(f),
        }
    }
}

// ============================================================================
// Display implementations for basic types
// ============================================================================

impl<'a> fmt::Display for Uint256<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // Display bytes with printable chars shown directly, non-printable as \xNN
        for b in escape_bytes::Escape::new(self.0) {
            write!(f, "{}", b as char)?;
        }
        Ok(())
    }
}

impl<'a, const MAX: usize> fmt::Display for BytesM<'a, MAX> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // Display bytes with printable chars shown directly, non-printable as \xNN
        for b in escape_bytes::Escape::new(self.as_bytes()) {
            write!(f, "{}", b as char)?;
        }
        Ok(())
    }
}

impl<'a, const MAX: usize> fmt::Display for StringM<'a, MAX> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // Already using escape_bytes for proper string display
        let v = self.as_bytes();
        for b in escape_bytes::Escape::new(v) {
            write!(f, "{}", b as char)?;
        }
        Ok(())
    }
}

// Note: Hash is a type alias for Uint256, so it uses Uint256's Display implementation

// ============================================================================
// Generic formatting utilities
// ============================================================================

/// Formats a numeric value as a decimal string with the specified number of decimal places
///
/// # Arguments
/// * `num` - The numeric value to format (u64, u32, i32, i64)
/// * `decimal_places` - Number of decimal places to show
///
/// # Examples
/// ```ignore
/// use stellarlib::display::format_decimal;
///
/// assert_eq!(format_decimal(1234567u64, 7), "0.1234567");
/// assert_eq!(format_decimal(10000000u64, 7), "1.0000000");
/// ```
pub fn format_decimal<T>(num: T, decimal_places: u32) -> alloc::string::String
where
    T: ToString + Copy,
{
    let num_str = num.to_string();

    if decimal_places == 0 {
        return num_str;
    }

    // Handle negative numbers
    let (is_negative, abs_str) = if let Some(stripped) = num_str.strip_prefix('-') {
        (true, stripped)
    } else {
        (false, num_str.as_str())
    };

    let abs_len = abs_str.len();

    if decimal_places as usize >= abs_len {
        let zeros_needed = decimal_places as usize - abs_len;
        let mut result = String::with_capacity(3 + zeros_needed + abs_len);
        if is_negative {
            result.push('-');
        }
        result.push_str("0.");
        for _ in 0..zeros_needed {
            result.push('0');
        }
        result.push_str(abs_str);
        result
    } else {
        let split_point = abs_len - decimal_places as usize;
        let integer_part = &abs_str[..split_point];
        let decimal_part = &abs_str[split_point..];

        let mut result = String::with_capacity(abs_len + 2);
        if is_negative {
            result.push('-');
        }
        if integer_part.is_empty() {
            result.push_str("0.");
            result.push_str(decimal_part);
        } else {
            result.push_str(integer_part);
            result.push('.');
            result.push_str(decimal_part);
        }
        result
    }
}

/// Adds comma separators to an integer string
///
/// # Arguments
/// * `integer_str` - The integer string to format
///
/// # Examples
/// ```ignore
/// use stellarlib::display::add_commas_to_integer;
///
/// assert_eq!(add_commas_to_integer("1234567"), "1,234,567");
/// assert_eq!(add_commas_to_integer("123"), "123");
/// ```
pub fn add_commas_to_integer(integer_str: &str) -> alloc::string::String {
    // Handle negative numbers
    let (is_negative, integer_part) = if let Some(stripped) = integer_str.strip_prefix('-') {
        (true, stripped)
    } else {
        (false, integer_str)
    };

    let len = integer_part.len();
    let mut result = String::with_capacity(len + (len / 3) + 2);

    if is_negative {
        result.push('-');
    }

    for (i, ch) in integer_part.chars().enumerate() {
        if i > 0 && (len - i) % 3 == 0 {
            result.push(',');
        }
        result.push(ch);
    }

    result
}

/// Formats a decimal number string with comma separators and removes trailing zeros
///
/// # Arguments
/// * `decimal_str` - A decimal string (e.g., "1234567.890")
///
/// # Examples
/// ```ignore
/// use stellarlib::display::format_number_with_commas;
///
/// assert_eq!(format_number_with_commas("1234567.890"), "1,234,567.89");
/// assert_eq!(format_number_with_commas("1000.000"), "1,000");
/// ```
pub fn format_number_with_commas(decimal_str: &str) -> alloc::string::String {
    // Handle negative numbers
    let (is_negative, number_part) = if let Some(stripped) = decimal_str.strip_prefix('-') {
        (true, stripped)
    } else {
        (false, decimal_str)
    };

    let result = if let Some(dot_pos) = number_part.find('.') {
        let integer_part = &number_part[..dot_pos];
        let mut decimal_part = &number_part[dot_pos + 1..];

        while decimal_part.ends_with('0') && !decimal_part.is_empty() {
            decimal_part = &decimal_part[..decimal_part.len() - 1];
        }

        let formatted_integer = add_commas_to_integer(integer_part);

        if decimal_part.is_empty() {
            formatted_integer
        } else {
            let mut result =
                String::with_capacity(formatted_integer.len() + decimal_part.len() + 1);
            result.push_str(&formatted_integer);
            result.push('.');
            result.push_str(decimal_part);
            result
        }
    } else {
        add_commas_to_integer(number_part)
    };

    if is_negative {
        let mut negative_result = String::with_capacity(result.len() + 1);
        negative_result.push('-');
        negative_result.push_str(&result);
        negative_result
    } else {
        result
    }
}

/// Formats Unix timestamp to UTC datetime string (YYYY-MM-DD HH:MM:SS UTC)
///
/// # Arguments
/// * `timestamp` - Unix timestamp in seconds
///
/// # Returns
/// Formatted datetime string in "YYYY-MM-DD HH:MM:SS UTC" format
///
/// # Examples
/// ```ignore
/// use stellarlib::display::format_unix_timestamp;
///
/// assert_eq!(format_unix_timestamp(1703335871), "2023-12-23 12:51:11 UTC");
/// ```
pub fn format_unix_timestamp(timestamp: u64) -> alloc::string::String {
    // Check if timestamp exceeds i64::MAX to avoid overflow
    if timestamp > i64::MAX as u64 {
        // Fallback to timestamp string if too large
        return timestamp.to_string();
    }

    // Convert to DateTime<Utc>
    if let Some(datetime) = DateTime::from_timestamp(timestamp as i64, 0) {
        datetime.to_string()
    } else {
        // Fallback to timestamp string if conversion fails
        timestamp.to_string()
    }
}

/// Formats a duration in seconds to a human-readable string
///
/// # Arguments
/// * `seconds` - Duration in seconds
///
/// # Returns
/// Formatted duration string like "1d 2h 30m 45s" or "30s" or "0s"
///
/// # Examples
/// ```ignore
/// use stellarlib::display::format_duration;
///
/// assert_eq!(format_duration(90), "1m 30s");
/// assert_eq!(format_duration(3661), "1h 1m 1s");
/// assert_eq!(format_duration(90061), "1d 1h 1m 1s");
/// assert_eq!(format_duration(0), "0s");
/// ```
pub fn format_duration(seconds: u64) -> alloc::string::String {
    if seconds == 0 {
        return "0s".to_string();
    }

    let days = seconds / 86400;
    let hours = (seconds % 86400) / 3600;
    let minutes = (seconds % 3600) / 60;
    let secs = seconds % 60;

    let mut parts = Vec::with_capacity(4);
    let mut has_higher_unit = false;

    if days > 0 {
        parts.push(format!("{}d", format_number_with_commas(&days.to_string())));
        has_higher_unit = true;
    }
    if hours > 0 || has_higher_unit {
        parts.push(format!("{}h", hours));
        has_higher_unit = true;
    }
    if minutes > 0 || has_higher_unit {
        parts.push(format!("{}m", minutes));
        has_higher_unit = true;
    }
    if secs > 0 || has_higher_unit {
        parts.push(format!("{}s", secs));
    }

    parts.join(" ")
}

/// Formats a Stellar amount (stroops) as a readable decimal string with comma separators
///
/// Stellar uses 7 decimal places (1 XLM = 10,000,000 stroops)
///
/// # Arguments
/// * `stroops` - The amount in stroops
///
/// # Examples
/// ```ignore
/// assert_eq!(format_native_amount(10000000u64), "1");
/// assert_eq!(format_native_amount(12345678u64), "1.2345678");
/// assert_eq!(format_native_amount(1234567890u64), "123.456789");
/// ```
pub fn format_native_amount<T>(stroops: T) -> String
where
    T: ToString + Copy,
{
    format_token_amount(stroops, STELLAR_NATIVE_DECIMAL_PLACES)
}

/// Formats a token amount with specified decimal places as a readable decimal string with comma separators
///
/// # Arguments
/// * `amount` - The token amount in smallest units (e.g., for USDC with 7 decimals, 10000000 represents 1 USDC)
/// * `decimals` - Number of decimal places for the token
///
/// # Returns
/// A formatted string with decimal places
///
/// # Examples
/// ```ignore
/// assert_eq!(format_token_amount(10000000u64, 7), "1");
/// assert_eq!(format_token_amount(12345678u64, 7), "1.2345678");
/// assert_eq!(format_token_amount(1234567890u64, 7), "123.456789");
/// ```
pub fn format_token_amount<T>(amount: T, decimals: u32) -> String
where
    T: ToString + Copy,
{
    let decimal_str = format_decimal(amount, decimals);
    format_number_with_commas(&decimal_str)
}
