#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <cmocka.h>

#include "common/base58.h"
#include "utils.h"
#include "types.h"

static void test_encode_ed25519_public_key() {
    uint8_t raw_key[] = {0xe9, 0x33, 0x88, 0xbb, 0xfd, 0x2f, 0xbd, 0x11, 0x80, 0x6d, 0xd0,
                         0xbd, 0x59, 0xce, 0xa9, 0x7,  0x9e, 0x7c, 0xc7, 0xc,  0xe7, 0xb1,
                         0xe1, 0x54, 0xf1, 0x14, 0xcd, 0xfe, 0x4e, 0x46, 0x6e, 0xcd};
    char *encoded_key = "GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7";
    char out[ENCODED_ED25519_PUBLIC_KEY_LENGTH];
    size_t out_len = sizeof(out);
    assert_true(encode_ed25519_public_key(raw_key, out, out_len));
    assert_string_equal(out, encoded_key);
}

static void test_encode_hash_x_key() {
    uint8_t raw_key[] = {0xe9, 0x33, 0x88, 0xbb, 0xfd, 0x2f, 0xbd, 0x11, 0x80, 0x6d, 0xd0,
                         0xbd, 0x59, 0xce, 0xa9, 0x7,  0x9e, 0x7c, 0xc7, 0xc,  0xe7, 0xb1,
                         0xe1, 0x54, 0xf1, 0x14, 0xcd, 0xfe, 0x4e, 0x46, 0x6e, 0xcd};
    char *encoded_key = "XDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM242X";
    char out[ENCODED_ED25519_PUBLIC_KEY_LENGTH];
    size_t out_len = sizeof(out);
    assert_true(encode_hash_x_key(raw_key, out, out_len));
    assert_string_equal(out, encoded_key);
}

static void test_encode_pre_auth_x_key() {
    uint8_t raw_key[] = {0xe9, 0x33, 0x88, 0xbb, 0xfd, 0x2f, 0xbd, 0x11, 0x80, 0x6d, 0xd0,
                         0xbd, 0x59, 0xce, 0xa9, 0x7,  0x9e, 0x7c, 0xc7, 0xc,  0xe7, 0xb1,
                         0xe1, 0x54, 0xf1, 0x14, 0xcd, 0xfe, 0x4e, 0x46, 0x6e, 0xcd};
    char *encoded_key = "TDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM3Y7O";
    char out[ENCODED_ED25519_PUBLIC_KEY_LENGTH];
    size_t out_len = sizeof(out);
    assert_true(encode_pre_auth_x_key(raw_key, out, out_len));
    assert_string_equal(out, encoded_key);
}

static void test_encode_ed25519_signed_payload() {
    char out[166];
    uint8_t raw_key[] = {0x3f, 0xc,  0x34, 0xbf, 0x93, 0xad, 0xd,  0x99, 0x71, 0xd0, 0x4c,
                         0xcc, 0x90, 0xf7, 0x5,  0x51, 0x1c, 0x83, 0x8a, 0xad, 0x97, 0x34,
                         0xa4, 0xa2, 0xfb, 0xd,  0x7a, 0x3,  0xfc, 0x7f, 0xe8, 0x9a};
    uint8_t payload1[] = {0x1,  0x2,  0x3,  0x4,  0x5,  0x6,  0x7,  0x8,  0x9,  0xa,  0xb,
                          0xc,  0xd,  0xe,  0xf,  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
                          0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20};
    ed25519_signed_payload_t ed25519_signed_payload1 = {.ed25519 = raw_key,
                                                        .payload_len = 32,
                                                        .payload = payload1};
    assert_true(encode_ed25519_signed_payload(&ed25519_signed_payload1, out, sizeof(out)));
    assert_string_equal(out,
                        "PA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUAAAAAQACAQDAQCQMBYIBE"
                        "FAWDANBYHRAEISCMKBKFQXDAMRUGY4DUPB6IBZGM");

    uint8_t payload2[] = {0x1,  0x2,  0x3,  0x4,  0x5,  0x6,  0x7,  0x8,  0x9,  0xa,
                          0xb,  0xc,  0xd,  0xe,  0xf,  0x10, 0x11, 0x12, 0x13, 0x14,
                          0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d};
    ed25519_signed_payload_t ed25519_signed_payload2 = {.ed25519 = raw_key,
                                                        .payload_len = 29,
                                                        .payload = payload2};
    assert_true(encode_ed25519_signed_payload(&ed25519_signed_payload2, out, sizeof(out)));
    assert_string_equal(out,
                        "PA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUAAAAAOQCAQDAQCQMBYIBE"
                        "FAWDANBYHRAEISCMKBKFQXDAMRUGY4DUAAAAFGBU");

    uint8_t payload3[] = {0x1a, 0xc9, 0x18, 0xbd, 0x8b, 0x1b, 0x7,  0x93, 0x56, 0x32, 0x6f,
                          0x6b, 0x3,  0xe2, 0x85, 0x79, 0xa4, 0xdd, 0xa2, 0x9c, 0xc2, 0x7a,
                          0x67, 0xf4, 0x2c, 0x26, 0x75, 0x6f, 0x6e, 0xe5, 0x23, 0x79, 0xbc,
                          0x19, 0x7c, 0x47, 0x6f, 0xa6, 0x5a, 0x6c, 0xc7, 0x73, 0xdc, 0x14,
                          0xbc, 0x6e, 0x9d, 0xfa, 0x1b, 0x70, 0x78, 0x6c, 0xaf, 0xe4, 0x89,
                          0x7d, 0xa6, 0xad, 0x3,  0x2b, 0x78, 0x6e, 0xda, 0xfa};
    ed25519_signed_payload_t ed25519_signed_payload3 = {.ed25519 = raw_key,
                                                        .payload_len = 64,
                                                        .payload = payload3};
    assert_true(encode_ed25519_signed_payload(&ed25519_signed_payload3, out, sizeof(out)));
    assert_string_equal(
        out,
        "PA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUAAAABABVSIYXWFRWB4TKYZG62YD4KCXTJG5UK"
        "OME6TH6QWCM5LPN3SSG6N4DF6EO35GLJWMO464CS6G5HP2DNYHQ3FP4SEX3JVNAMVXQ3W27JBW2");
}

static void test_print_ed25519_signed_payload() {
    char out[89];
    uint8_t raw_key[] = {0x3f, 0xc,  0x34, 0xbf, 0x93, 0xad, 0xd,  0x99, 0x71, 0xd0, 0x4c,
                         0xcc, 0x90, 0xf7, 0x5,  0x51, 0x1c, 0x83, 0x8a, 0xad, 0x97, 0x34,
                         0xa4, 0xa2, 0xfb, 0xd,  0x7a, 0x3,  0xfc, 0x7f, 0xe8, 0x9a};
    uint8_t payload1[] = {0x1,  0x2,  0x3,  0x4,  0x5,  0x6,  0x7,  0x8,  0x9,  0xa,  0xb,
                          0xc,  0xd,  0xe,  0xf,  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
                          0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20};
    ed25519_signed_payload_t ed25519_signed_payload1 = {.ed25519 = raw_key,
                                                        .payload_len = 32,
                                                        .payload = payload1};
    assert_true(print_ed25519_signed_payload(&ed25519_signed_payload1, out, sizeof(out), 12, 12));
    assert_string_equal(out, "PA7QYNF7SOWQ..Y4DUPB6IBZGM");

    uint8_t payload2[] = {0x1,  0x2,  0x3,  0x4,  0x5,  0x6,  0x7,  0x8,  0x9,  0xa,
                          0xb,  0xc,  0xd,  0xe,  0xf,  0x10, 0x11, 0x12, 0x13, 0x14,
                          0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d};
    ed25519_signed_payload_t ed25519_signed_payload2 = {.ed25519 = raw_key,
                                                        .payload_len = 29,
                                                        .payload = payload2};
    assert_true(print_ed25519_signed_payload(&ed25519_signed_payload2, out, sizeof(out), 12, 12));
    assert_string_equal(out, "PA7QYNF7SOWQ..Y4DUAAAAFGBU");

    uint8_t payload3[] = {0x1a, 0xc9, 0x18, 0xbd, 0x8b, 0x1b, 0x7,  0x93, 0x56, 0x32, 0x6f,
                          0x6b, 0x3,  0xe2, 0x85, 0x79, 0xa4, 0xdd, 0xa2, 0x9c, 0xc2, 0x7a,
                          0x67, 0xf4, 0x2c, 0x26, 0x75, 0x6f, 0x6e, 0xe5, 0x23, 0x79, 0xbc,
                          0x19, 0x7c, 0x47, 0x6f, 0xa6, 0x5a, 0x6c, 0xc7, 0x73, 0xdc, 0x14,
                          0xbc, 0x6e, 0x9d, 0xfa, 0x1b, 0x70, 0x78, 0x6c, 0xaf, 0xe4, 0x89,
                          0x7d, 0xa6, 0xad, 0x3,  0x2b, 0x78, 0x6e, 0xda, 0xfa};
    ed25519_signed_payload_t ed25519_signed_payload3 = {.ed25519 = raw_key,
                                                        .payload_len = 64,
                                                        .payload = payload3};
    assert_true(print_ed25519_signed_payload(&ed25519_signed_payload3, out, sizeof(out), 12, 12));
    assert_string_equal(out, "PA7QYNF7SOWQ..MVXQ3W27JBW2");
}

static void test_encode_muxed_account() {
    // https://github.com/stellar/stellar-protocol/blob/master/ecosystem/sep-0023.md#valid-test-cases
    char out[89];
    // GA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJVSGZ
    const uint8_t ed25519[] = {
        0x3f, 0x0c, 0x34, 0xbf, 0x93, 0xad, 0x0d, 0x99, 0x71, 0xd0, 0x4c,
        0xcc, 0x90, 0xf7, 0x05, 0x51, 0x1c, 0x83, 0x8a, 0xad, 0x97, 0x34,
        0xa4, 0xa2, 0xfb, 0x0d, 0x7a, 0x03, 0xfc, 0x7f, 0xe8, 0x9a,
    };
    // Valid non-multiplexed account
    muxed_account_t account1 = {.type = KEY_TYPE_ED25519, .ed25519 = ed25519};
    assert_true(encode_muxed_account(&account1, out, sizeof(out)));
    assert_string_equal(out, "GA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJVSGZ");

    // Valid multiplexed account
    muxed_account_t account2 = {.type = KEY_TYPE_MUXED_ED25519,
                                .med25519 = {.id = 0, .ed25519 = ed25519}};
    assert_true(encode_muxed_account(&account2, out, sizeof(out)));
    assert_string_equal(out,
                        "MA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUAAAAAAAAAAAACJUQ");

    // Valid multiplexed account in which unsigned id exceeds maximum signed 64-bit integer
    muxed_account_t account3 = {.type = KEY_TYPE_MUXED_ED25519,
                                .med25519 = {.id = 9223372036854775808, .ed25519 = ed25519}};
    assert_true(encode_muxed_account(&account3, out, sizeof(out)));
    assert_string_equal(out,
                        "MA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJVAAAAAAAAAAAAAJLK");
}

void test_print_binary() {
    const uint8_t binary[32] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
                                16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
    char hex[2 * sizeof(binary) + 1];

    assert_true(print_binary(binary, sizeof(binary), hex, sizeof(hex), 0, 0));
    assert_string_equal(hex, "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
}

void test_print_claimable_balance_id() {
    const uint8_t v0[] = {0xc9, 0xc4, 0xa9, 0xe3, 0xa4, 0x68, 0x91, 0xa3, 0x60, 0x15, 0xc3,
                          0x17, 0xb3, 0xdf, 0x17, 0xb4, 0x2b, 0xf,  0x2a, 0xd8, 0xa2, 0xee,
                          0xa6, 0xc9, 0x34, 0xc9, 0xf7, 0xc8, 0x42, 0x5d, 0xa7, 0xad};
    claimable_balance_id id = {.type = CLAIMABLE_BALANCE_ID_TYPE_V0, .v0 = v0};

    char out[36 * 2 + 1];
    assert_true(print_claimable_balance_id(&id, out, sizeof(out), 0, 0));
    assert_string_equal(out,
                        "00000000c9c4a9e3a46891a36015c317b3df17b42b0f2ad8a2eea6c934c9f7c8425da7ad");
    assert_true(print_claimable_balance_id(&id, out, sizeof(out), 12, 12));
    assert_string_equal(out, "00000000c9c4..f7c8425da7ad");
}

void test_print_time() {
    char out[20];
    assert_true(print_time(0, out, sizeof(out)));
    assert_string_equal(out, "1970-01-01 00:00:00");
    assert_true(print_time(1648263853, out, sizeof(out)));
    assert_string_equal(out, "2022-03-26 03:04:13");
    assert_true(print_time(2147483647, out, sizeof(out)));
    assert_string_equal(out, "2038-01-19 03:14:07");
    assert_true(print_time(4294967295, out, sizeof(out)));
    assert_string_equal(out, "2106-02-07 06:28:15");
    assert_true(print_time(253402300799, out, sizeof(out)));
    assert_string_equal(out, "9999-12-31 23:59:59");
    assert_false(print_time(253402300800, out, sizeof(out)));
    assert_false(print_time(18446744073709551615, out, sizeof(out)));
}

void test_print_uint() {
    char out[24];

    assert_true(print_uint(0, out, sizeof(out)));
    assert_string_equal(out, "0");

    assert_true(print_uint(1230, out, sizeof(out)));
    assert_string_equal(out, "1230");

    assert_true(print_uint((uint64_t) 18446744073709551615, out, sizeof(out)));
    assert_string_equal(out, "18446744073709551615");

    // output buffer too small
    assert_false(print_uint(1230, out, 4));

    // output buffer just big enough to store output data
    assert_true(print_uint(9999, out, 5));
    assert_string_equal(out, "9999");

    // output buffer just big enough to store output data
    assert_true(print_uint(9999, out, 5));
    assert_string_equal(out, "9999");
}

void test_print_int() {
    char out[24];

    assert_true(print_int((int64_t) 0, out, sizeof(out)));
    assert_string_equal(out, "0");

    assert_true(print_int((int64_t) 1230, out, sizeof(out)));
    assert_string_equal(out, "1230");

    assert_true(print_int((int64_t) -1230, out, sizeof(out)));
    assert_string_equal(out, "-1230");

    // test overflow, ignore the warngins
    assert_true(print_int((int64_t) 9223372036854775807, out, sizeof(out)));
    assert_string_equal(out, "9223372036854775807");

    // test overflow, ignore the warngins
    assert_true(print_int((int64_t) -9223372036854775808, out, sizeof(out)));
    assert_string_equal(out, "-9223372036854775808");

    // output buffer too small
    assert_false(print_int((int64_t) -1230, out, 5));
    assert_false(print_int(1230, out, 4));

    // output buffer just big enough to store output data
    assert_true(print_int((int64_t) -9999, out, 6));
    assert_string_equal(out, "-9999");
    assert_true(print_int((int64_t) 9999, out, 5));
    assert_string_equal(out, "9999");
}

void test_print_asset() {
    asset_t assert_native = {.type = ASSET_TYPE_NATIVE};
    char out[24];
    assert_true(print_asset(&assert_native, 0, out, sizeof(out)));
    assert_string_equal(out, "XLM");
    // testnet asset
    assert_true(print_asset(&assert_native, 1, out, sizeof(out)));
    assert_string_equal(out, "XLM");
    // private network asset
    assert_true(print_asset(&assert_native, 2, out, sizeof(out)));
    assert_string_equal(out, "native");

    const uint8_t ed25519[] = {
        0x3f, 0x0c, 0x34, 0xbf, 0x93, 0xad, 0x0d, 0x99, 0x71, 0xd0, 0x4c,
        0xcc, 0x90, 0xf7, 0x05, 0x51, 0x1c, 0x83, 0x8a, 0xad, 0x97, 0x34,
        0xa4, 0xa2, 0xfb, 0x0d, 0x7a, 0x03, 0xfc, 0x7f, 0xe8, 0x9a,
    };
    asset_t assert_alphanum4 = {.type = ASSET_TYPE_CREDIT_ALPHANUM4,
                                .alpha_num4 = {.asset_code = "CAT", .issuer = ed25519}};
    assert_true(print_asset(&assert_alphanum4, 0, out, sizeof(out)));
    assert_string_equal(out, "CAT@GA7..VSGZ");

    asset_t assert_alphanum12 = {.type = ASSET_TYPE_CREDIT_ALPHANUM12,
                                 .alpha_num12 = {.asset_code = "BANANANANANA", .issuer = ed25519}};
    assert_true(print_asset(&assert_alphanum12, 0, out, sizeof(out)));
    assert_string_equal(out, "BANANANANANA@GA7..VSGZ");
}

void test_print_summary() {
    char *data1 = "abcdefghijklmnopqrstuvwxyz";
    char out1[10];
    assert_true(print_summary(data1, out1, sizeof(out1), 3, 4));
    assert_string_equal(out1, "abc..wxyz");

    char *data2 = "abcdef";
    char out2[11];
    assert_true(print_summary(data2, out2, sizeof(out2), 4, 4));
    assert_string_equal(out2, "abcdef");

    char *data3 = "abcdefghijklmnopqrstuvwxyz";
    char out3[10];
    assert_false(print_summary(data3, out3, sizeof(out3), 4, 4));
}

void test_print_amount_asset_native(void **state) {
    (void) state;

    char printed[28];
    const asset_t asset = {.type = ASSET_TYPE_NATIVE};
    assert_true(print_amount(1, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "0.0000001 XLM");
    assert_true(print_amount(10000000, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "1 XLM");
    assert_true(print_amount(1000000000, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "100 XLM");
    assert_true(print_amount(1001000000, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "100.1 XLM");
    assert_true(
        print_amount(10000000000001, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "1,000,000.0000001 XLM");
    assert_true(print_amount(100000001, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "10.0000001 XLM");
    assert_true(
        print_amount(100000001000000, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "10,000,000.1 XLM");
    assert_true(
        print_amount(9222036854775807, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "922,203,685.4775807 XLM");
    assert_true(
        print_amount(9223372036854775807, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "922,337,203,685.4775807 XLM");
}

void test_print_amount_asset_alphanum4(void **state) {
    (void) state;
    // GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN
    uint8_t issuer[] = {0x3b, 0x99, 0x11, 0x38, 0xe,  0xfe, 0x98, 0x8b, 0xa0, 0xa8, 0x90,
                        0xe,  0xb1, 0xcf, 0xe4, 0x4f, 0x36, 0x6f, 0x7d, 0xbe, 0x94, 0x6b,
                        0xed, 0x7,  0x72, 0x40, 0xf7, 0xf6, 0x24, 0xdf, 0x15, 0xc5};
    char printed[39];
    const asset_t asset = {.type = ASSET_TYPE_CREDIT_ALPHANUM4,
                           .alpha_num4 = {.asset_code = "USDC", .issuer = issuer}};

    assert_true(print_amount(1, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "0.0000001 USDC@GA5..KZVN");
    assert_true(print_amount(10000000, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "1 USDC@GA5..KZVN");
    assert_true(print_amount(1000000000, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "100 USDC@GA5..KZVN");
    assert_true(print_amount(1001000000, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "100.1 USDC@GA5..KZVN");
    assert_true(
        print_amount(10000000000001, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "1,000,000.0000001 USDC@GA5..KZVN");
    assert_true(print_amount(100000001, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "10.0000001 USDC@GA5..KZVN");
    assert_true(
        print_amount(100000001000000, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "10,000,000.1 USDC@GA5..KZVN");
    assert_true(
        print_amount(9222036854775807, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "922,203,685.4775807 USDC@GA5..KZVN");
    assert_true(
        print_amount(9223372036854775807, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "922,337,203,685.4775807 USDC@GA5..KZVN");
}

void test_print_amount_asset_alphanum12(void **state) {
    (void) state;
    // GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN
    uint8_t issuer[] = {0x3b, 0x99, 0x11, 0x38, 0xe,  0xfe, 0x98, 0x8b, 0xa0, 0xa8, 0x90,
                        0xe,  0xb1, 0xcf, 0xe4, 0x4f, 0x36, 0x6f, 0x7d, 0xbe, 0x94, 0x6b,
                        0xed, 0x7,  0x72, 0x40, 0xf7, 0xf6, 0x24, 0xdf, 0x15, 0xc5};
    char printed[47];
    const asset_t asset = {.type = ASSET_TYPE_CREDIT_ALPHANUM12,
                           .alpha_num12 = {.asset_code = "BANANANANANA", .issuer = issuer}};

    assert_true(print_amount(1, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "0.0000001 BANANANANANA@GA5..KZVN");
    assert_true(print_amount(10000000, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "1 BANANANANANA@GA5..KZVN");
    assert_true(print_amount(1000000000, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "100 BANANANANANA@GA5..KZVN");
    assert_true(print_amount(1001000000, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "100.1 BANANANANANA@GA5..KZVN");
    assert_true(
        print_amount(10000000000001, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "1,000,000.0000001 BANANANANANA@GA5..KZVN");
    assert_true(print_amount(100000001, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "10.0000001 BANANANANANA@GA5..KZVN");
    assert_true(
        print_amount(100000001000000, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "10,000,000.1 BANANANANANA@GA5..KZVN");
    assert_true(
        print_amount(9222036854775807, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "922,203,685.4775807 BANANANANANA@GA5..KZVN");
    assert_true(
        print_amount(9223372036854775807, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed)));
    assert_string_equal(printed, "922,337,203,685.4775807 BANANANANANA@GA5..KZVN");
}

void test_is_printable_binary(void **state) {
    (void) state;
    uint8_t data1[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
                       0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                       0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43,
                       0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
                       0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b,
                       0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
                       0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73,
                       0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e};
    assert_true(is_printable_binary(data1, sizeof(data1)));

    uint8_t data2[] = {0x19, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a,
                       0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
                       0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42,
                       0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e,
                       0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a,
                       0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
                       0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72,
                       0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e};
    assert_false(is_printable_binary(data2, sizeof(data2)));

    uint8_t data3[] = {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
                       0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                       0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43,
                       0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
                       0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b,
                       0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
                       0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73,
                       0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f};
    assert_false(is_printable_binary(data3, sizeof(data3)));
}

void test_print_account_flags(void **state) {
    (void) state;
    char out[89];
    memset(out, 0, sizeof(out));
    assert_true(print_account_flags(1, out, sizeof(out)));
    assert_string_equal(out, "AUTH_REQUIRED");
    memset(out, 0, sizeof(out));
    assert_true(print_account_flags(2, out, sizeof(out)));
    assert_string_equal(out, "AUTH_REVOCABLE");
    memset(out, 0, sizeof(out));
    assert_true(print_account_flags(4, out, sizeof(out)));
    assert_string_equal(out, "AUTH_IMMUTABLE");
    memset(out, 0, sizeof(out));
    assert_true(print_account_flags(8, out, sizeof(out)));
    assert_string_equal(out, "AUTH_CLAWBACK_ENABLED");
    memset(out, 0, sizeof(out));
    assert_true(print_account_flags(15, out, sizeof(out)));
    assert_string_equal(out,
                        "AUTH_REQUIRED, AUTH_REVOCABLE, AUTH_IMMUTABLE, AUTH_CLAWBACK_ENABLED");
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_encode_ed25519_public_key),
        cmocka_unit_test(test_encode_hash_x_key),
        cmocka_unit_test(test_encode_pre_auth_x_key),
        cmocka_unit_test(test_encode_muxed_account),
        cmocka_unit_test(test_encode_ed25519_signed_payload),
        cmocka_unit_test(test_print_ed25519_signed_payload),
        cmocka_unit_test(test_print_binary),
        cmocka_unit_test(test_print_claimable_balance_id),
        cmocka_unit_test(test_print_time),
        cmocka_unit_test(test_print_uint),
        cmocka_unit_test(test_print_int),
        cmocka_unit_test(test_print_asset),
        cmocka_unit_test(test_print_summary),
        cmocka_unit_test(test_print_amount_asset_native),
        cmocka_unit_test(test_print_amount_asset_alphanum4),
        cmocka_unit_test(test_print_amount_asset_alphanum12),
        cmocka_unit_test(test_is_printable_binary),
        cmocka_unit_test(test_print_account_flags),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}