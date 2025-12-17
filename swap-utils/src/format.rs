//! Formatting utilities for swap operations

use crate::crc;
use core::fmt::Write;

/// Error type for formatting operations
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FormatError {
    /// Buffer is too small for the output
    BufferTooSmall,
    /// Invalid input data
    InvalidInput,
}

/// Format a Stellar address from an Ed25519 public key
///
/// Stellar addresses use a base32 encoding with the following structure:
/// - 1 byte: version (0x30 for ed25519 public keys)
/// - 32 bytes: public key
/// - 2 bytes: CRC16 checksum
/// - Total encoded length: 56 characters
///
/// # Arguments
/// * `public_key` - 32-byte Ed25519 public key
/// * `buffer` - Output buffer (must be at least 56 bytes)
///
/// # Returns
/// Number of bytes written (always 56 for success)
pub fn format_stellar_address(
    public_key: &[u8; 32],
    buffer: &mut [u8],
) -> Result<usize, FormatError> {
    if buffer.len() < 56 {
        return Err(FormatError::BufferTooSmall);
    }

    const VERSION_BYTE_PUBLIC_KEY_ED25519: u8 = 6 << 3; // 0x30

    // Build the data to encode: version byte + public key + checksum
    let mut data = [0u8; 35]; // 1 (version) + 32 (pubkey) + 2 (checksum)
    data[0] = VERSION_BYTE_PUBLIC_KEY_ED25519;
    data[1..33].copy_from_slice(public_key);

    // Calculate and append CRC16 checksum
    let checksum = crc::checksum(&data[..33]);
    data[33..35].copy_from_slice(&checksum);

    // Encode to base32
    encode_base32(&data, buffer)?;

    Ok(56)
}

/// Format an amount in stroops as a human-readable XLM value
///
/// Stellar amounts are stored as stroops (1 XLM = 10^7 stroops).
/// This function formats the amount with proper decimal places and
/// removes trailing zeros.
///
/// # Arguments
/// * `amount` - The amount as big-endian bytes (expected to be 16 bytes for u128)
/// * `buffer` - Output buffer for the formatted string
///
/// # Returns
/// A string slice containing the formatted amount
///
/// # Errors
/// Returns an error if the amount exceeds the i64 range
pub fn format_printable_amount<'a>(
    amount: &[u8; 16],
    buffer: &'a mut [u8],
) -> Result<&'a str, FormatError> {
    // Convert amount from big-endian bytes to u128
    let amount_value = u128::from_be_bytes(*amount);

    // Stellar amounts are actually i64, so check if the value fits
    // Maximum i64 value is 9,223,372,036,854,775,807
    if amount_value > i64::MAX as u128 {
        return Err(FormatError::InvalidInput);
    }

    // Now we can safely work with u64 since we know it fits in i64 range
    let amount_u64 = amount_value as u64;

    // Format the amount with 7 decimal places for XLM
    let written_len = {
        let mut writer = BufferWriter::new(buffer);
        format_xlm_amount(amount_u64, &mut writer)?;
        writer.push_str(" XLM")?;
        writer.pos
    };

    // Convert the written bytes to a string
    core::str::from_utf8(&buffer[..written_len]).map_err(|_| FormatError::InvalidInput)
}

/// Base32 encoding using the Stellar alphabet
///
/// This implementation avoids any heap allocation by working directly
/// with the provided buffers.
fn encode_base32(input: &[u8], output: &mut [u8]) -> Result<(), FormatError> {
    if input.len() != 35 {
        return Err(FormatError::InvalidInput);
    }

    const ALPHABET: &[u8] = b"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

    if output.len() < 56 {
        return Err(FormatError::BufferTooSmall);
    }

    let mut out_idx = 0;
    let mut buffer = 0u32;
    let mut bits_in_buffer = 0;

    for &byte in input {
        // Safe: buffer is u32, byte is u8, shift is bounded
        buffer = (buffer << 8) | (byte as u32);
        bits_in_buffer += 8;

        while bits_in_buffer >= 5 {
            bits_in_buffer -= 5;
            let index = ((buffer >> bits_in_buffer) & 0x1f) as usize;
            output[out_idx] = ALPHABET[index];
            out_idx += 1;
        }
    }

    // Handle remaining bits if any
    if bits_in_buffer > 0 {
        let index = ((buffer << (5 - bits_in_buffer)) & 0x1f) as usize;
        output[out_idx] = ALPHABET[index];
        out_idx += 1;
    }

    // Verify we encoded exactly 56 characters for 35 bytes input
    if out_idx != 56 {
        return Err(FormatError::InvalidInput);
    }

    Ok(())
}

/// Format a number with decimal places (specialized for XLM)
///
/// This function is optimized for Stellar XLM amounts which always use
/// 7 decimal places. Uses compile-time constants for better performance.
fn format_xlm_amount(num: u64, writer: &mut BufferWriter) -> Result<(), FormatError> {
    const DECIMAL_PLACES: u32 = 7;
    const DIVISOR: u64 = 10_000_000; // 10^7, compile-time constant

    let integer_part = num / DIVISOR;
    let decimal_part = num % DIVISOR;

    // Handle zero case - check the input directly for clarity
    if num == 0 {
        writer.write_byte(b'0')?;
        return Ok(());
    }

    // Write integer part
    write_integer_u64(integer_part, writer)?;

    // Write decimal part if non-zero
    if decimal_part > 0 {
        writer.write_byte(b'.')?;
        write_decimal_digits_u64(decimal_part, DECIMAL_PLACES as usize, writer)?;
    }

    Ok(())
}

/// Write a u64 integer to the buffer (optimized version)
fn write_integer_u64(mut value: u64, writer: &mut BufferWriter) -> Result<(), FormatError> {
    if value == 0 {
        writer.write_byte(b'0')?;
        return Ok(());
    }

    // u64::MAX is 20 digits, i64::MAX is 19 digits
    // We only need 20 bytes maximum
    let mut digits = [0u8; 20];
    let mut digit_count = 0;

    while value > 0 {
        digits[digit_count] = (value % 10) as u8 + b'0';
        value /= 10;
        digit_count += 1;
    }

    // Write digits in correct order
    for i in (0..digit_count).rev() {
        writer.write_byte(digits[i])?;
    }

    Ok(())
}

/// Write decimal digits for u64 values, removing trailing zeros
fn write_decimal_digits_u64(
    mut value: u64,
    places: usize,
    writer: &mut BufferWriter,
) -> Result<(), FormatError> {
    // For XLM, we only need 7 decimal places
    const MAX_DECIMAL_PLACES: usize = 7;

    if places > MAX_DECIMAL_PLACES {
        return Err(FormatError::InvalidInput);
    }

    let mut decimal_digits = [0u8; MAX_DECIMAL_PLACES];

    // Fill decimal digits from right to left
    for slot in (0..places).rev() {
        decimal_digits[slot] = (value % 10) as u8 + b'0';
        value /= 10;
    }

    // Find the last non-zero digit
    let mut last_nonzero = places;
    while last_nonzero > 0 && decimal_digits[last_nonzero - 1] == b'0' {
        last_nonzero -= 1;
    }

    // Write the significant digits
    if last_nonzero > 0 {
        writer.write_bytes(&decimal_digits[..last_nonzero])?;
    }

    Ok(())
}

/// A simple buffer writer for building strings without allocation
pub struct BufferWriter<'a> {
    buffer: &'a mut [u8],
    pos: usize,
}

impl<'a> BufferWriter<'a> {
    pub fn new(buffer: &'a mut [u8]) -> Self {
        Self { buffer, pos: 0 }
    }

    fn write_byte(&mut self, byte: u8) -> Result<(), FormatError> {
        if self.pos == self.buffer.len() {
            return Err(FormatError::BufferTooSmall);
        }
        self.buffer[self.pos] = byte;
        self.pos += 1;
        Ok(())
    }

    fn write_bytes(&mut self, bytes: &[u8]) -> Result<(), FormatError> {
        // Check for integer overflow and buffer capacity
        let new_pos = self
            .pos
            .checked_add(bytes.len())
            .ok_or(FormatError::BufferTooSmall)?;
        if new_pos > self.buffer.len() {
            return Err(FormatError::BufferTooSmall);
        }
        self.buffer[self.pos..new_pos].copy_from_slice(bytes);
        self.pos = new_pos;
        Ok(())
    }

    pub fn push_str(&mut self, s: &str) -> Result<(), FormatError> {
        self.write_bytes(s.as_bytes())
    }
}

impl<'a> Write for BufferWriter<'a> {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        self.write_bytes(s.as_bytes()).map_err(|_| core::fmt::Error)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_format_xlm_amount_whole_numbers() {
        let mut buffer = [0u8; 32];

        // Test 1000000000 stroops = 100 XLM
        let written_len = {
            let mut writer = BufferWriter::new(&mut buffer);
            format_xlm_amount(1000000000, &mut writer).unwrap();
            writer.pos
        };

        assert_eq!(core::str::from_utf8(&buffer[..written_len]).unwrap(), "100");

        // Test 0 XLM
        let mut buffer = [0u8; 32];
        let written_len = {
            let mut writer = BufferWriter::new(&mut buffer);
            format_xlm_amount(0, &mut writer).unwrap();
            writer.pos
        };
        assert_eq!(core::str::from_utf8(&buffer[..written_len]).unwrap(), "0");

        // Test 10000000 stroops = 1 XLM exactly
        let mut buffer = [0u8; 32];
        let written_len = {
            let mut writer = BufferWriter::new(&mut buffer);
            format_xlm_amount(10000000, &mut writer).unwrap();
            writer.pos
        };
        assert_eq!(core::str::from_utf8(&buffer[..written_len]).unwrap(), "1");
    }

    #[test]
    fn test_format_xlm_amount_with_decimals() {
        let mut buffer = [0u8; 32];

        // Test 12345678 stroops = 1.2345678 XLM
        let written_len = {
            let mut writer = BufferWriter::new(&mut buffer);
            format_xlm_amount(12345678, &mut writer).unwrap();
            writer.pos
        };

        assert_eq!(
            core::str::from_utf8(&buffer[..written_len]).unwrap(),
            "1.2345678"
        );

        // Test minimal amount 1 stroop = 0.0000001 XLM
        let mut buffer = [0u8; 32];
        let written_len = {
            let mut writer = BufferWriter::new(&mut buffer);
            format_xlm_amount(1, &mut writer).unwrap();
            writer.pos
        };
        assert_eq!(
            core::str::from_utf8(&buffer[..written_len]).unwrap(),
            "0.0000001"
        );

        // Test 9999999 stroops = 0.9999999 XLM
        let mut buffer = [0u8; 32];
        let written_len = {
            let mut writer = BufferWriter::new(&mut buffer);
            format_xlm_amount(9999999, &mut writer).unwrap();
            writer.pos
        };
        assert_eq!(
            core::str::from_utf8(&buffer[..written_len]).unwrap(),
            "0.9999999"
        );
    }

    #[test]
    fn test_format_xlm_amount_trailing_zeros() {
        let mut buffer = [0u8; 32];

        // Test 15000000 stroops = 1.5 XLM (trailing zeros removed)
        let written_len = {
            let mut writer = BufferWriter::new(&mut buffer);
            format_xlm_amount(15000000, &mut writer).unwrap();
            writer.pos
        };

        assert_eq!(core::str::from_utf8(&buffer[..written_len]).unwrap(), "1.5");

        // Test 10000 stroops = 0.001 XLM (trailing zeros removed)
        let mut buffer = [0u8; 32];
        let written_len = {
            let mut writer = BufferWriter::new(&mut buffer);
            format_xlm_amount(10000, &mut writer).unwrap();
            writer.pos
        };
        assert_eq!(
            core::str::from_utf8(&buffer[..written_len]).unwrap(),
            "0.001"
        );

        // Test 100 stroops = 0.00001 XLM
        let mut buffer = [0u8; 32];
        let written_len = {
            let mut writer = BufferWriter::new(&mut buffer);
            format_xlm_amount(100, &mut writer).unwrap();
            writer.pos
        };
        assert_eq!(
            core::str::from_utf8(&buffer[..written_len]).unwrap(),
            "0.00001"
        );

        // Test 5000000 stroops = 0.5 XLM
        let mut buffer = [0u8; 32];
        let written_len = {
            let mut writer = BufferWriter::new(&mut buffer);
            format_xlm_amount(5000000, &mut writer).unwrap();
            writer.pos
        };
        assert_eq!(core::str::from_utf8(&buffer[..written_len]).unwrap(), "0.5");
    }

    #[test]
    fn test_format_stellar_address_valid_keys() {
        // Test case 1: Valid account GA3D5KRYM6CB7OWQ6TWYRR3Z4T7GNZLKERYNZGGA5SOAOPIFY6YQHES5
        let public_key_1: [u8; 32] = [
            0x36, 0x3e, 0xaa, 0x38, 0x67, 0x84, 0x1f, 0xba, 0xd0, 0xf4, 0xed, 0x88, 0xc7, 0x79,
            0xe4, 0xfe, 0x66, 0xe5, 0x6a, 0x24, 0x70, 0xdc, 0x98, 0xc0, 0xec, 0x9c, 0x07, 0x3d,
            0x05, 0xc7, 0xb1, 0x03,
        ];
        let mut buffer = [0u8; 56];
        let result = format_stellar_address(&public_key_1, &mut buffer);
        assert_eq!(result, Ok(56));
        assert_eq!(
            core::str::from_utf8(&buffer).unwrap(),
            "GA3D5KRYM6CB7OWQ6TWYRR3Z4T7GNZLKERYNZGGA5SOAOPIFY6YQHES5"
        );

        // Test case 2: Valid account GA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJVSGZ
        let public_key_2: [u8; 32] = [
            0x3f, 0x0c, 0x34, 0xbf, 0x93, 0xad, 0x0d, 0x99, 0x71, 0xd0, 0x4c, 0xcc, 0x90, 0xf7,
            0x05, 0x51, 0x1c, 0x83, 0x8a, 0xad, 0x97, 0x34, 0xa4, 0xa2, 0xfb, 0x0d, 0x7a, 0x03,
            0xfc, 0x7f, 0xe8, 0x9a,
        ];
        let mut buffer = [0u8; 56];
        let result = format_stellar_address(&public_key_2, &mut buffer);
        assert_eq!(result, Ok(56));
        assert_eq!(
            core::str::from_utf8(&buffer).unwrap(),
            "GA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJVSGZ"
        );

        // Test case 3: All zeros public key
        let public_key_3 = [0u8; 32];
        let mut buffer = [0u8; 56];
        let result = format_stellar_address(&public_key_3, &mut buffer);
        assert_eq!(result, Ok(56));
        // Should produce a valid base32 address starting with GA (G for public key, A for first zero byte)
        assert!(core::str::from_utf8(&buffer).unwrap().starts_with("GA"));

        // Test case 4: All 0xFF public key
        let public_key_4 = [0xFF; 32];
        let mut buffer = [0u8; 56];
        let result = format_stellar_address(&public_key_4, &mut buffer);
        assert_eq!(result, Ok(56));
        // The first byte after version will be 0xFF, which when combined with version byte
        // will produce a different pattern. Let's just verify it's valid base32
        let address = core::str::from_utf8(&buffer).unwrap();
        assert!(address.starts_with("G")); // All Stellar public keys start with G
        for &byte in &buffer {
            assert!(b"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567".contains(&byte));
        }
    }

    #[test]
    fn test_format_stellar_address_buffer_validation() {
        let public_key = [0u8; 32];

        // Test with exact buffer size (56 bytes)
        let mut buffer = [0u8; 56];
        assert_eq!(format_stellar_address(&public_key, &mut buffer), Ok(56));

        // Test with larger buffer
        let mut buffer = [0u8; 100];
        assert_eq!(format_stellar_address(&public_key, &mut buffer), Ok(56));

        // Test with insufficient buffer
        let mut buffer = [0u8; 55];
        assert_eq!(
            format_stellar_address(&public_key, &mut buffer),
            Err(FormatError::BufferTooSmall)
        );

        let mut buffer = [0u8; 10];
        assert_eq!(
            format_stellar_address(&public_key, &mut buffer),
            Err(FormatError::BufferTooSmall)
        );
    }

    #[test]
    fn test_format_printable_amount() {
        let mut buffer = [0u8; 64];

        // Test 1 XLM (10000000 stroops)
        let amount_bytes = 10000000u128.to_be_bytes();
        let result = format_printable_amount(&amount_bytes, &mut buffer).unwrap();
        assert_eq!(result, "1 XLM");

        // Test 123.456789 XLM
        let amount_bytes = 1234567890u128.to_be_bytes();
        let result = format_printable_amount(&amount_bytes, &mut buffer).unwrap();
        assert_eq!(result, "123.456789 XLM");

        // Test 0 XLM
        let amount_bytes = 0u128.to_be_bytes();
        let result = format_printable_amount(&amount_bytes, &mut buffer).unwrap();
        assert_eq!(result, "0 XLM");

        // Test maximum practical amount (1 billion XLM)
        let amount_bytes = 10_000_000_000_000_000u128.to_be_bytes();
        let result = format_printable_amount(&amount_bytes, &mut buffer).unwrap();
        assert_eq!(result, "1000000000 XLM");

        // Test small amounts with trailing zeros removed
        let amount_bytes = 15000000u128.to_be_bytes();
        let result = format_printable_amount(&amount_bytes, &mut buffer).unwrap();
        assert_eq!(result, "1.5 XLM");

        // Test minimal amount (1 stroop)
        let amount_bytes = 1u128.to_be_bytes();
        let result = format_printable_amount(&amount_bytes, &mut buffer).unwrap();
        assert_eq!(result, "0.0000001 XLM");

        // Test various amounts
        let amount_bytes = 9999999u128.to_be_bytes();
        let result = format_printable_amount(&amount_bytes, &mut buffer).unwrap();
        assert_eq!(result, "0.9999999 XLM");

        let amount_bytes = 10000001u128.to_be_bytes();
        let result = format_printable_amount(&amount_bytes, &mut buffer).unwrap();
        assert_eq!(result, "1.0000001 XLM");

        let amount_bytes = 100000000u128.to_be_bytes();
        let result = format_printable_amount(&amount_bytes, &mut buffer).unwrap();
        assert_eq!(result, "10 XLM");

        // Test maximum i64 value (9,223,372,036,854,775,807 stroops)
        let amount_bytes = (i64::MAX as u128).to_be_bytes();
        let result = format_printable_amount(&amount_bytes, &mut buffer).unwrap();
        assert_eq!(result, "922337203685.4775807 XLM");

        // Test value just above i64 max (should fail)
        let amount_bytes = ((i64::MAX as u128) + 1).to_be_bytes();
        let result = format_printable_amount(&amount_bytes, &mut buffer);
        assert_eq!(result, Err(FormatError::InvalidInput));

        // Test much larger value (should also fail)
        let amount_bytes = u128::MAX.to_be_bytes();
        let result = format_printable_amount(&amount_bytes, &mut buffer);
        assert_eq!(result, Err(FormatError::InvalidInput));
    }

    #[test]
    fn test_buffer_writer() {
        // Test write_byte
        let mut buffer = [0u8; 10];
        {
            let mut writer = BufferWriter::new(&mut buffer);

            assert_eq!(writer.write_byte(b'A'), Ok(()));
            assert_eq!(writer.pos, 1);
        }
        assert_eq!(buffer[0], b'A');

        // Test write_bytes
        {
            let mut writer = BufferWriter::new(&mut buffer);
            writer.write_byte(b'A').unwrap(); // Restore state
            assert_eq!(writer.write_bytes(b"BCD"), Ok(()));
            assert_eq!(writer.pos, 4);
        }
        assert_eq!(&buffer[..4], b"ABCD");

        // Test push_str
        {
            let mut writer = BufferWriter::new(&mut buffer);
            writer.write_bytes(b"ABCD").unwrap(); // Restore state
            assert_eq!(writer.push_str("EF"), Ok(()));
            assert_eq!(writer.pos, 6);
        }
        assert_eq!(&buffer[..6], b"ABCDEF");

        // Test buffer overflow
        {
            let mut writer = BufferWriter::new(&mut buffer);
            writer.write_bytes(b"ABCDEF").unwrap(); // Restore state
            assert_eq!(
                writer.write_bytes(b"12345"),
                Err(FormatError::BufferTooSmall)
            );
            assert_eq!(writer.pos, 6); // Position unchanged on error
        }

        // Test Write trait
        {
            let mut writer = BufferWriter::new(&mut buffer);
            writer.write_bytes(b"ABCDEF").unwrap(); // Restore state
            use core::fmt::Write;
            write!(&mut writer, "GH").unwrap();
            assert_eq!(writer.pos, 8);
        }
        assert_eq!(&buffer[..8], b"ABCDEFGH");
    }

    #[test]
    fn test_encode_base32() {
        // Test encoding all zeros
        let input = [0u8; 35];
        let mut output = [0u8; 56];
        assert_eq!(encode_base32(&input, &mut output), Ok(()));

        // Verify output is valid base32
        for &byte in &output {
            assert!(b"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567".contains(&byte));
        }

        // Test with insufficient output buffer
        let mut small_output = [0u8; 55];
        assert_eq!(
            encode_base32(&input, &mut small_output),
            Err(FormatError::BufferTooSmall)
        );
    }

    #[test]
    fn test_format_xlm_amount_max_value() {
        let mut buffer = [0u8; 32];

        // Test maximum i64 value (9,223,372,036,854,775,807 stroops)
        let written_len = {
            let mut writer = BufferWriter::new(&mut buffer);
            format_xlm_amount(i64::MAX as u64, &mut writer).unwrap();
            writer.pos
        };
        assert_eq!(
            core::str::from_utf8(&buffer[..written_len]).unwrap(),
            "922337203685.4775807"
        );
    }

    #[test]
    fn test_write_integer_u64() {
        let mut buffer = [0u8; 30];

        // Test zero
        let mut writer = BufferWriter::new(&mut buffer);
        write_integer_u64(0, &mut writer).unwrap();
        assert_eq!(writer.pos, 1);
        assert_eq!(&buffer[..1], b"0");

        // Test small number
        let mut writer = BufferWriter::new(&mut buffer);
        write_integer_u64(123, &mut writer).unwrap();
        assert_eq!(writer.pos, 3);
        assert_eq!(&buffer[..3], b"123");

        // Test large number
        let mut writer = BufferWriter::new(&mut buffer);
        write_integer_u64(9876543210, &mut writer).unwrap();
        assert_eq!(writer.pos, 10);
        assert_eq!(&buffer[..10], b"9876543210");

        // Test maximum i64 value
        let mut buffer = [0u8; 30];
        let mut writer = BufferWriter::new(&mut buffer);
        write_integer_u64(i64::MAX as u64, &mut writer).unwrap();
        assert_eq!(writer.pos, 19);
        assert_eq!(&buffer[..19], b"9223372036854775807");

        // Test maximum u64 value (for completeness, though not used in XLM)
        let mut buffer = [0u8; 30];
        let mut writer = BufferWriter::new(&mut buffer);
        write_integer_u64(u64::MAX, &mut writer).unwrap();
        assert_eq!(writer.pos, 20);
        assert_eq!(&buffer[..20], b"18446744073709551615");
    }

    #[test]
    fn test_write_decimal_digits_u64() {
        let mut buffer = [0u8; 30];

        // Test with trailing zeros (should be removed)
        let mut writer = BufferWriter::new(&mut buffer);
        write_decimal_digits_u64(5000000, 7, &mut writer).unwrap();
        assert_eq!(writer.pos, 1);
        assert_eq!(&buffer[..1], b"5");

        // Test with all significant digits
        let mut writer = BufferWriter::new(&mut buffer);
        write_decimal_digits_u64(1234567, 7, &mut writer).unwrap();
        assert_eq!(writer.pos, 7);
        assert_eq!(&buffer[..7], b"1234567");

        // Test with some trailing zeros
        let mut writer = BufferWriter::new(&mut buffer);
        write_decimal_digits_u64(1230000, 7, &mut writer).unwrap();
        assert_eq!(writer.pos, 3);
        assert_eq!(&buffer[..3], b"123");

        // Test zero (all digits are zero)
        let mut writer = BufferWriter::new(&mut buffer);
        write_decimal_digits_u64(0, 7, &mut writer).unwrap();
        assert_eq!(writer.pos, 0); // No digits written when all are zero

        // Test single digit
        let mut writer = BufferWriter::new(&mut buffer);
        write_decimal_digits_u64(1, 7, &mut writer).unwrap();
        assert_eq!(writer.pos, 7);
        assert_eq!(&buffer[..7], b"0000001");

        // Test edge case: max places exceeded
        let mut writer = BufferWriter::new(&mut buffer);
        assert_eq!(
            write_decimal_digits_u64(123, 8, &mut writer),
            Err(FormatError::InvalidInput)
        );
    }
}
