use stellarlib::display::{
    add_commas_to_integer, format_decimal, format_duration, format_native_amount,
    format_number_with_commas, format_token_amount, format_unix_timestamp,
};

#[test]
fn test_format_decimal() {
    assert_eq!(format_decimal(1234567u64, 7), "0.1234567");
    assert_eq!(format_decimal(10000000u64, 7), "1.0000000");
    assert_eq!(format_decimal(123u64, 0), "123");
    assert_eq!(format_decimal(12345u64, 2), "123.45");
    assert_eq!(format_decimal(42u64, 2), "0.42");
    assert_eq!(format_decimal(-1234i64, 2), "-12.34");

    // u128 extreme values
    assert_eq!(
        format_decimal(u128::MAX, 0),
        "340282366920938463463374607431768211455"
    );
    assert_eq!(
        format_decimal(u128::MAX, 10),
        "34028236692093846346337460743.1768211455"
    );
    assert_eq!(
        format_decimal(u128::MAX, 38),
        "3.40282366920938463463374607431768211455"
    );
    assert_eq!(
        format_decimal(u128::MAX, 39),
        "0.340282366920938463463374607431768211455"
    );
    assert_eq!(
        format_decimal(u128::MAX, 50),
        "0.00000000000340282366920938463463374607431768211455"
    );
    assert_eq!(format_decimal(u128::MIN, 0), "0");
    assert_eq!(format_decimal(u128::MIN, 5), "0.00000");

    // i128 extreme values
    assert_eq!(
        format_decimal(i128::MAX, 0),
        "170141183460469231731687303715884105727"
    );
    assert_eq!(
        format_decimal(i128::MAX, 10),
        "17014118346046923173168730371.5884105727"
    );
    assert_eq!(
        format_decimal(i128::MAX, 38),
        "1.70141183460469231731687303715884105727"
    );
    assert_eq!(
        format_decimal(i128::MAX, 39),
        "0.170141183460469231731687303715884105727"
    );
    assert_eq!(
        format_decimal(i128::MIN, 0),
        "-170141183460469231731687303715884105728"
    );
    assert_eq!(
        format_decimal(i128::MIN, 10),
        "-17014118346046923173168730371.5884105728"
    );
    assert_eq!(
        format_decimal(i128::MIN, 38),
        "-1.70141183460469231731687303715884105728"
    );
    assert_eq!(
        format_decimal(i128::MIN, 39),
        "-0.170141183460469231731687303715884105728"
    );

    // Edge cases with large decimal places
    assert_eq!(
        format_decimal(1u128, 40),
        "0.0000000000000000000000000000000000000001"
    );
    assert_eq!(
        format_decimal(-1i128, 40),
        "-0.0000000000000000000000000000000000000001"
    );

    // decimal places larger than typical integer sizes
    assert_eq!(format_decimal(1u8, 30), "0.000000000000000000000000000001");
    assert_eq!(
        format_decimal(-1i8, 30),
        "-0.000000000000000000000000000001"
    );
}

#[test]
fn test_add_commas_to_integer() {
    assert_eq!(add_commas_to_integer("0"), "0");
    assert_eq!(add_commas_to_integer("123"), "123");
    assert_eq!(add_commas_to_integer("1234567"), "1,234,567");
    assert_eq!(add_commas_to_integer("-1234567"), "-1,234,567");
    assert_eq!(add_commas_to_integer("115792089237316195423570985008687907853269984665640564039457584007913129639935"), 
               "115,792,089,237,316,195,423,570,985,008,687,907,853,269,984,665,640,564,039,457,584,007,913,129,639,935");
    assert_eq!(add_commas_to_integer("-57896044618658097711785492504343953926634992332820282019728792003956564819968"), 
               "-57,896,044,618,658,097,711,785,492,504,343,953,926,634,992,332,820,282,019,728,792,003,956,564,819,968");
}

#[test]
fn test_format_number_with_commas() {
    assert_eq!(format_number_with_commas("0"), "0");
    assert_eq!(format_number_with_commas("1234567.89"), "1,234,567.89");
    assert_eq!(format_number_with_commas("1000"), "1,000");
    assert_eq!(format_number_with_commas("1234567"), "1,234,567");
    assert_eq!(format_number_with_commas("-1234567.89"), "-1,234,567.89");
    assert_eq!(format_number_with_commas("1000.1"), "1,000.1");
    assert_eq!(format_number_with_commas("11579208923731619542357098500868790785326998466564056403945758400791312.9639935"), "11,579,208,923,731,619,542,357,098,500,868,790,785,326,998,466,564,056,403,945,758,400,791,312.9639935");
    assert_eq!(format_number_with_commas("-5789604461865809771178549250434395392663499233282028201972879200395656.4819968"), "-5,789,604,461,865,809,771,178,549,250,434,395,392,663,499,233,282,028,201,972,879,200,395,656.4819968");
}

#[test]
fn test_format_unix_timestamp() {
    assert_eq!(format_unix_timestamp(1703335871), "2023-12-23 12:51:11 UTC");
    assert_eq!(format_unix_timestamp(0), "1970-01-01 00:00:00 UTC");

    let over_i64_max = (i64::MAX as u64).saturating_add(1);
    assert_eq!(
        format_unix_timestamp(over_i64_max),
        over_i64_max.to_string()
    );
}

#[test]
fn test_format_duration() {
    // Basic cases
    assert_eq!(format_duration(0), "0s");
    assert_eq!(format_duration(59), "59s");
    assert_eq!(format_duration(90), "1m 30s");
    assert_eq!(format_duration(3661), "1h 1m 1s");
    assert_eq!(format_duration(90061), "1d 1h 1m 1s");
    assert_eq!(format_duration(u64::MAX), "213,503,982,334,601d 7h 0m 15s");

    // Cases with zero components in the middle
    assert_eq!(format_duration(3600 + 1), "1h 0m 1s");
    assert_eq!(format_duration(3600 + 60), "1h 1m 0s");
    assert_eq!(format_duration(86400 + 1), "1d 0h 0m 1s");
    assert_eq!(format_duration(86400 + 60), "1d 0h 1m 0s");
    assert_eq!(format_duration(86400 + 3600), "1d 1h 0m 0s");
    assert_eq!(format_duration(86400 + 3600 + 1), "1d 1h 0m 1s");
    assert_eq!(format_duration(86400 + 3600 + 60), "1d 1h 1m 0s");

    // Cases with only higher units (all lower units are 0)
    assert_eq!(format_duration(120), "2m 0s");
    assert_eq!(format_duration(3600 * 2), "2h 0m 0s");
    assert_eq!(format_duration(86400 * 2), "2d 0h 0m 0s");
}

#[test]
fn test_format_native_amount() {
    // Test standard native XLM amounts (7 decimal places)
    assert_eq!(format_native_amount(10_000_000u64), "1");
    assert_eq!(format_native_amount(12_345_678u64), "1.2345678");
    assert_eq!(format_native_amount(123_456_789u64), "12.3456789");
    assert_eq!(format_native_amount(1_234_567_890u64), "123.456789");

    // Test with commas in large numbers
    assert_eq!(format_native_amount(10_000_000_000_000u64), "1,000,000");
    assert_eq!(
        format_native_amount(123_456_789_012_345u64),
        "12,345,678.9012345"
    );

    // Test small amounts
    assert_eq!(format_native_amount(1_000_000u64), "0.1");
    assert_eq!(format_native_amount(100_000u64), "0.01");
    assert_eq!(format_native_amount(10_000u64), "0.001");
    assert_eq!(format_native_amount(1_000u64), "0.0001");
    assert_eq!(format_native_amount(100u64), "0.00001");
    assert_eq!(format_native_amount(10u64), "0.000001");
    assert_eq!(format_native_amount(1u64), "0.0000001");

    // Test zero
    assert_eq!(format_native_amount(0u64), "0");

    // Test negative amounts
    assert_eq!(format_native_amount(-10_000_000i64), "-1");
    assert_eq!(format_native_amount(-1_234_567i64), "-0.1234567");
    assert_eq!(
        format_native_amount(-123_456_789_000_000i64),
        "-12,345,678.9"
    );
}

#[test]
fn test_format_token_amount_standard_decimals() {
    // Standard 7 decimals (most Stellar tokens)
    assert_eq!(format_token_amount(10_000_000u64, 7), "1");
    assert_eq!(format_token_amount(1_000_000u64, 7), "0.1");
    assert_eq!(format_token_amount(100_000u64, 7), "0.01");
    assert_eq!(format_token_amount(10_000u64, 7), "0.001");
    assert_eq!(format_token_amount(1_000u64, 7), "0.0001");
    assert_eq!(format_token_amount(100u64, 7), "0.00001");
    assert_eq!(format_token_amount(10u64, 7), "0.000001");
    assert_eq!(format_token_amount(1u64, 7), "0.0000001");
}

#[test]
fn test_format_token_amount_various_decimals() {
    // 0 decimals
    assert_eq!(format_token_amount(1000u64, 0), "1,000");

    // 2 decimals (like traditional currencies)
    assert_eq!(format_token_amount(100u64, 2), "1");
    assert_eq!(format_token_amount(1234u64, 2), "12.34");

    // 6 decimals
    assert_eq!(format_token_amount(1_000_000u64, 6), "1");
    assert_eq!(format_token_amount(123_456u64, 6), "0.123456");

    // 18 decimals (Ethereum-style)
    assert_eq!(format_token_amount(1_000_000_000_000_000_000u64, 18), "1");
}

#[test]
fn test_format_token_amount_large_numbers() {
    // Test with commas in large numbers
    assert_eq!(format_token_amount(10_000_000_000_000u64, 7), "1,000,000");
    assert_eq!(
        format_token_amount(123_456_789_012_345u64, 7),
        "12,345,678.9012345"
    );
}

#[test]
fn test_format_token_amount_negative() {
    assert_eq!(format_token_amount(-10_000_000i64, 7), "-1");
    assert_eq!(format_token_amount(-1_234_567i64, 7), "-0.1234567");
    assert_eq!(
        format_token_amount(-123_456_789_000_000i64, 7),
        "-12,345,678.9"
    );
}

#[test]
fn test_format_token_amount_edge_cases() {
    // Zero
    assert_eq!(format_token_amount(0u64, 7), "0");
    assert_eq!(format_token_amount(0u64, 0), "0");

    // Maximum precision
    assert_eq!(format_token_amount(12_345_678u64, 7), "1.2345678");

    // Very small amounts
    assert_eq!(format_token_amount(1u64, 7), "0.0000001");
    assert_eq!(format_token_amount(9u64, 7), "0.0000009");
}
