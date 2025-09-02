#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <cmocka.h>

#include "stellar/printer.h"

void test_print_account_id() {
    uint8_t raw_key[] = {0xe9, 0x33, 0x88, 0xbb, 0xfd, 0x2f, 0xbd, 0x11, 0x80, 0x6d, 0xd0,
                         0xbd, 0x59, 0xce, 0xa9, 0x7,  0x9e, 0x7c, 0xc7, 0xc,  0xe7, 0xb1,
                         0xe1, 0x54, 0xf1, 0x14, 0xcd, 0xfe, 0x4e, 0x46, 0x6e, 0xcd};
    char *encoded_key = "GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7";
    char out[ENCODED_ED25519_PUBLIC_KEY_LENGTH];
    size_t out_len = sizeof(out);
    assert_true(print_account_id(raw_key, out, out_len, 0, 0));
    assert_string_equal(out, encoded_key);

    assert_true(print_account_id(raw_key, out, out_len, 7, 9));
    assert_string_equal(out, "GDUTHCF..OIZXM2FN7");
}

void test_print_hash_x_key() {
    uint8_t raw_key[] = {0xe9, 0x33, 0x88, 0xbb, 0xfd, 0x2f, 0xbd, 0x11, 0x80, 0x6d, 0xd0,
                         0xbd, 0x59, 0xce, 0xa9, 0x7,  0x9e, 0x7c, 0xc7, 0xc,  0xe7, 0xb1,
                         0xe1, 0x54, 0xf1, 0x14, 0xcd, 0xfe, 0x4e, 0x46, 0x6e, 0xcd};
    char *encoded_key = "XDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM242X";
    char out[ENCODED_ED25519_PUBLIC_KEY_LENGTH];
    size_t out_len = sizeof(out);
    assert_true(print_hash_x_key(raw_key, out, out_len, 0, 0));
    assert_string_equal(out, encoded_key);

    assert_true(print_hash_x_key(raw_key, out, out_len, 7, 9));
    assert_string_equal(out, "XDUTHCF..OIZXM242X");
}

void test_print_pre_auth_x_key() {
    uint8_t raw_key[] = {0xe9, 0x33, 0x88, 0xbb, 0xfd, 0x2f, 0xbd, 0x11, 0x80, 0x6d, 0xd0,
                         0xbd, 0x59, 0xce, 0xa9, 0x7,  0x9e, 0x7c, 0xc7, 0xc,  0xe7, 0xb1,
                         0xe1, 0x54, 0xf1, 0x14, 0xcd, 0xfe, 0x4e, 0x46, 0x6e, 0xcd};
    char *encoded_key = "TDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM3Y7O";
    char out[ENCODED_ED25519_PUBLIC_KEY_LENGTH];
    size_t out_len = sizeof(out);
    assert_true(print_pre_auth_x_key(raw_key, out, out_len, 0, 0));
    assert_string_equal(out, encoded_key);

    assert_true(print_pre_auth_x_key(raw_key, out, out_len, 7, 9));
    assert_string_equal(out, "TDUTHCF..OIZXM3Y7O");
}

void test_print_muxed_account() {
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
    assert_true(print_muxed_account(&account1, out, sizeof(out), 0, 0));
    assert_string_equal(out, "GA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJVSGZ");
    assert_true(print_muxed_account(&account1, out, sizeof(out), 7, 9));
    assert_string_equal(out, "GA7QYNF..4P7UJVSGZ");

    // Valid multiplexed account
    muxed_account_t account2 = {.type = KEY_TYPE_MUXED_ED25519,
                                .med25519 = {.id = 0, .ed25519 = ed25519}};
    assert_true(print_muxed_account(&account2, out, sizeof(out), 0, 0));
    assert_string_equal(out,
                        "MA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUAAAAAAAAAAAACJUQ");
    assert_true(print_muxed_account(&account2, out, sizeof(out), 7, 9));
    assert_string_equal(out, "MA7QYNF..AAAAACJUQ");
    // Valid multiplexed account in which unsigned id exceeds maximum signed 64-bit integer
    muxed_account_t account3 = {.type = KEY_TYPE_MUXED_ED25519,
                                .med25519 = {.id = 9223372036854775808, .ed25519 = ed25519}};
    assert_true(print_muxed_account(&account3, out, sizeof(out), 0, 0));
    assert_string_equal(out,
                        "MA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJVAAAAAAAAAAAAAJLK");
    assert_true(print_muxed_account(&account3, out, sizeof(out), 7, 9));
    assert_string_equal(out, "MA7QYNF..AAAAAAJLK");
}

void test_print_sc_address() {
    char out[ENCODED_CONTRACT_KEY_LENGTH];

    uint8_t raw_key[] = {0xe9, 0x33, 0x88, 0xbb, 0xfd, 0x2f, 0xbd, 0x11, 0x80, 0x6d, 0xd0,
                         0xbd, 0x59, 0xce, 0xa9, 0x7,  0x9e, 0x7c, 0xc7, 0xc,  0xe7, 0xb1,
                         0xe1, 0x54, 0xf1, 0x14, 0xcd, 0xfe, 0x4e, 0x46, 0x6e, 0xcd};
    sc_address_t account_address = {
        .address = raw_key,
        .type = SC_ADDRESS_TYPE_ACCOUNT,
    };

    assert_true(print_sc_address(&account_address, out, sizeof(out), 0, 0));
    assert_string_equal(out, "GDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM2FN7");

    assert_true(print_sc_address(&account_address, out, sizeof(out), 12, 12));
    assert_string_equal(out, "GDUTHCF37UX3..37SOIZXM2FN7");

    sc_address_t contract_address = {
        .address = raw_key,
        .type = SC_ADDRESS_TYPE_CONTRACT,
    };

    assert_true(print_sc_address(&contract_address, out, sizeof(out), 0, 0));
    assert_string_equal(out, "CDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM3BIG");

    assert_true(print_sc_address(&contract_address, out, sizeof(out), 12, 12));
    assert_string_equal(out, "CDUTHCF37UX3..37SOIZXM3BIG");
}

void test_print_med25519_key() {
    uint8_t raw_key[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // id (8 bytes)
        0x3f, 0x0c, 0x34, 0xbf, 0x93, 0xad, 0x0d, 0x99,  // ed25519 public key (32 bytes)
        0x71, 0xd0, 0x4c, 0xcc, 0x90, 0xf7, 0x05, 0x51, 0x1c, 0x83, 0x8a, 0xad,
        0x97, 0x34, 0xa4, 0xa2, 0xfb, 0x0d, 0x7a, 0x03, 0xfc, 0x7f, 0xe8, 0x9a};
    char *encoded_key = "MA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUAAAAAAAAAAAACJUQ";
    char out[ENCODED_MUXED_ACCOUNT_KEY_LENGTH];
    size_t out_len = sizeof(out);
    assert_true(print_med25519_key(raw_key, out, out_len, 0, 0));
    assert_string_equal(out, encoded_key);

    assert_true(print_med25519_key(raw_key, out, out_len, 7, 9));
    assert_string_equal(out, "MA7QYNF..AAAAACJUQ");

    // Test with large ID value (9223372036854775808)
    uint8_t raw_key_large[] = {
        0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // id (8 bytes) - 9223372036854775808
        0x3f, 0x0c, 0x34, 0xbf, 0x93, 0xad, 0x0d, 0x99,  // ed25519 public key (32 bytes)
        0x71, 0xd0, 0x4c, 0xcc, 0x90, 0xf7, 0x05, 0x51, 0x1c, 0x83, 0x8a, 0xad,
        0x97, 0x34, 0xa4, 0xa2, 0xfb, 0x0d, 0x7a, 0x03, 0xfc, 0x7f, 0xe8, 0x9a};
    char *encoded_key_large =
        "MA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJVAAAAAAAAAAAAAJLK";
    assert_true(print_med25519_key(raw_key_large, out, out_len, 0, 0));
    assert_string_equal(out, encoded_key_large);

    assert_true(print_med25519_key(raw_key_large, out, out_len, 7, 9));
    assert_string_equal(out, "MA7QYNF..AAAAAAJLK");
}

void test_print_claimable_balance() {
    uint8_t raw_key[] = {0x00, 0x00, 0x00, 0x00,  // version (4 bytes, v0)
                         0xe9, 0x33, 0x88, 0xbb, 0xfd, 0x2f, 0xbd, 0x11, 0x80, 0x6d, 0xd0,
                         0xbd, 0x59, 0xce, 0xa9, 0x07, 0x9e, 0x7c, 0xc7, 0x0c, 0xe7, 0xb1,
                         0xe1, 0x54, 0xf1, 0x14, 0xcd, 0xfe, 0x4e, 0x46, 0x6e, 0xcd, 0x00};
    char *encoded_key = "BAAOSM4IXP6S7PIRQBW5BPKZZ2UQPHT4Y4GOPMPBKTYRJTP6JZDG5TPB7M";
    char out[ENCODED_CLAIMABLE_BALANCE_KEY_LENGTH + 1];
    size_t out_len = sizeof(out);
    assert_true(print_claimable_balance(raw_key, out, out_len, 0, 0));
    assert_string_equal(out, encoded_key);

    assert_true(print_claimable_balance(raw_key, out, out_len, 7, 9));
    assert_string_equal(out, "BAAOSM4..ZDG5TPB7M");
}

void test_print_liquidity_pool() {
    uint8_t raw_key[] = {0xe9, 0x33, 0x88, 0xbb, 0xfd, 0x2f, 0xbd, 0x11, 0x80, 0x6d, 0xd0,
                         0xbd, 0x59, 0xce, 0xa9, 0x07, 0x9e, 0x7c, 0xc7, 0x0c, 0xe7, 0xb1,
                         0xe1, 0x54, 0xf1, 0x14, 0xcd, 0xfe, 0x4e, 0x46, 0x6e, 0xcd};
    char *encoded_key = "LDUTHCF37UX32EMANXIL2WOOVEDZ47GHBTT3DYKU6EKM37SOIZXM3YCL";
    char out[ENCODED_LIQUIDITY_POOL_KEY_LENGTH];
    size_t out_len = sizeof(out);
    assert_true(print_liquidity_pool(raw_key, out, out_len, 0, 0));
    assert_string_equal(out, encoded_key);

    assert_true(print_liquidity_pool(raw_key, out, out_len, 7, 9));
    assert_string_equal(out, "LDUTHCF..OIZXM3YCL");
}

void test_print_ed25519_signed_payload() {
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
    assert_true(print_ed25519_signed_payload(&ed25519_signed_payload1, out, sizeof(out), 0, 0));
    assert_string_equal(out,
                        "PA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUAAAAAQACAQDAQCQMBYIBE"
                        "FAWDANBYHRAEISCMKBKFQXDAMRUGY4DUPB6IBZGM");
    assert_true(print_ed25519_signed_payload(&ed25519_signed_payload1, out, sizeof(out), 12, 12));
    assert_string_equal(out, "PA7QYNF7SOWQ..Y4DUPB6IBZGM");
    uint8_t payload2[] = {0x1,  0x2,  0x3,  0x4,  0x5,  0x6,  0x7,  0x8,  0x9,  0xa,
                          0xb,  0xc,  0xd,  0xe,  0xf,  0x10, 0x11, 0x12, 0x13, 0x14,
                          0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d};
    ed25519_signed_payload_t ed25519_signed_payload2 = {.ed25519 = raw_key,
                                                        .payload_len = 29,
                                                        .payload = payload2};
    assert_true(print_ed25519_signed_payload(&ed25519_signed_payload2, out, sizeof(out), 0, 0));
    assert_string_equal(out,
                        "PA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUAAAAAOQCAQDAQCQMBYIBE"
                        "FAWDANBYHRAEISCMKBKFQXDAMRUGY4DUAAAAFGBU");
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
    assert_true(print_ed25519_signed_payload(&ed25519_signed_payload3, out, sizeof(out), 0, 0));
    assert_string_equal(
        out,
        "PA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUAAAABABVSIYXWFRWB4TKYZG62YD4KCXTJG5UK"
        "OME6TH6QWCM5LPN3SSG6N4DF6EO35GLJWMO464CS6G5HP2DNYHQ3FP4SEX3JVNAMVXQ3W27JBW2");

    assert_true(print_ed25519_signed_payload(&ed25519_signed_payload3, out, sizeof(out), 12, 12));
    assert_string_equal(out, "PA7QYNF7SOWQ..MVXQ3W27JBW2");
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
    uint8_t asset_code4[] = {'C', 'A', 'T', 0};
    asset_t assert_alphanum4 = {.type = ASSET_TYPE_CREDIT_ALPHANUM4,
                                .alpha_num4 = {.asset_code = asset_code4, .issuer = ed25519}};
    assert_true(print_asset(&assert_alphanum4, 0, out, sizeof(out)));
    assert_string_equal(out, "CAT@GA7..VSGZ");

    uint8_t asset_code12[] = {'B', 'A', 'N', 'A', 'N', 'A', 'N', 'A', 'N', 'A', 'N', 'A'};
    asset_t assert_alphanum12 = {.type = ASSET_TYPE_CREDIT_ALPHANUM12,
                                 .alpha_num12 = {.asset_code = asset_code12, .issuer = ed25519}};
    assert_true(print_asset(&assert_alphanum12, 0, out, sizeof(out)));
    assert_string_equal(out, "BANANANANANA@GA7..VSGZ");
}

void test_print_amount_asset_native() {
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

void test_print_amount_asset_alphanum4() {  // GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN
    uint8_t issuer[] = {0x3b, 0x99, 0x11, 0x38, 0xe,  0xfe, 0x98, 0x8b, 0xa0, 0xa8, 0x90,
                        0xe,  0xb1, 0xcf, 0xe4, 0x4f, 0x36, 0x6f, 0x7d, 0xbe, 0x94, 0x6b,
                        0xed, 0x7,  0x72, 0x40, 0xf7, 0xf6, 0x24, 0xdf, 0x15, 0xc5};
    char printed[39];
    uint8_t asset_code[] = {'U', 'S', 'D', 'C'};
    const asset_t asset = {.type = ASSET_TYPE_CREDIT_ALPHANUM4,
                           .alpha_num4 = {.asset_code = asset_code, .issuer = issuer}};

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

void test_print_amount_asset_alphanum12() {  // GA5ZSEJYB37JRC5AVCIA5MOP4RHTM335X2KGX3IHOJAPP5RE34K4KZVN
    uint8_t issuer[] = {0x3b, 0x99, 0x11, 0x38, 0xe,  0xfe, 0x98, 0x8b, 0xa0, 0xa8, 0x90,
                        0xe,  0xb1, 0xcf, 0xe4, 0x4f, 0x36, 0x6f, 0x7d, 0xbe, 0x94, 0x6b,
                        0xed, 0x7,  0x72, 0x40, 0xf7, 0xf6, 0x24, 0xdf, 0x15, 0xc5};
    char printed[47];
    uint8_t asset_code[] = {'B', 'A', 'N', 'A', 'N', 'A', 'N', 'A', 'N', 'A', 'N', 'A'};
    const asset_t asset = {.type = ASSET_TYPE_CREDIT_ALPHANUM12,
                           .alpha_num12 = {.asset_code = asset_code, .issuer = issuer}};

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

void test_print_claimable_balance_id() {
    const uint8_t v0[] = {0xc9, 0xc4, 0xa9, 0xe3, 0xa4, 0x68, 0x91, 0xa3, 0x60, 0x15, 0xc3,
                          0x17, 0xb3, 0xdf, 0x17, 0xb4, 0x2b, 0xf,  0x2a, 0xd8, 0xa2, 0xee,
                          0xa6, 0xc9, 0x34, 0xc9, 0xf7, 0xc8, 0x42, 0x5d, 0xa7, 0xad};
    claimable_balance_id_t id = {.type = CLAIMABLE_BALANCE_ID_TYPE_V0, .v0 = v0};

    char out[36 * 2 + 1];
    assert_true(print_claimable_balance_id(&id, out, sizeof(out), 0, 0));
    assert_string_equal(out,
                        "00000000C9C4A9E3A46891A36015C317B3DF17B42B0F2AD8A2EEA6C934C9F7C8425DA7AD");
    assert_true(print_claimable_balance_id(&id, out, sizeof(out), 12, 12));
    assert_string_equal(out, "00000000C9C4..F7C8425DA7AD");
}

void test_print_account_flags() {
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

void test_print_trust_line_flags() {
    char out[89];
    memset(out, 0, sizeof(out));
    assert_true(print_trust_line_flags(1, out, sizeof(out)));
    assert_string_equal(out, "AUTHORIZED");
    memset(out, 0, sizeof(out));
    assert_true(print_trust_line_flags(2, out, sizeof(out)));
    assert_string_equal(out, "AUTHORIZED_TO_MAINTAIN_LIABILITIES");
    memset(out, 0, sizeof(out));
    assert_true(print_trust_line_flags(4, out, sizeof(out)));
    assert_string_equal(out, "TRUSTLINE_CLAWBACK_ENABLED");
    memset(out, 0, sizeof(out));
    assert_true(print_trust_line_flags(7, out, sizeof(out)));
    assert_string_equal(
        out,
        "AUTHORIZED, AUTHORIZED_TO_MAINTAIN_LIABILITIES, TRUSTLINE_CLAWBACK_ENABLED");
    memset(out, 0, sizeof(out));
    assert_true(print_trust_line_flags(5, out, sizeof(out)));
    assert_string_equal(out, "AUTHORIZED, TRUSTLINE_CLAWBACK_ENABLED");
}

void test_print_allow_trust_flags() {
    char out[89];
    memset(out, 0, sizeof(out));
    assert_true(print_allow_trust_flags(0, out, sizeof(out)));
    assert_string_equal(out, "UNAUTHORIZED");
    memset(out, 0, sizeof(out));
    assert_true(print_allow_trust_flags(1, out, sizeof(out)));
    assert_string_equal(out, "AUTHORIZED");
    memset(out, 0, sizeof(out));
    assert_true(print_allow_trust_flags(2, out, sizeof(out)));
    assert_string_equal(out, "AUTHORIZED_TO_MAINTAIN_LIABILITIES");
}

void test_print_uint64_num() {
    char out[24];

    assert_true(print_uint64_num(0, out, sizeof(out)));
    assert_string_equal(out, "0");

    assert_true(print_uint64_num(1230, out, sizeof(out)));
    assert_string_equal(out, "1230");

    assert_true(print_uint64_num((uint64_t) 18446744073709551615, out, sizeof(out)));
    assert_string_equal(out, "18446744073709551615");

    // output buffer too small
    assert_false(print_uint64_num(1230, out, 4));

    // output buffer just big enough to store output data
    assert_true(print_uint64_num(9999, out, 5));
    assert_string_equal(out, "9999");

    // output buffer just big enough to store output data
    assert_true(print_uint64_num(9999, out, 5));
    assert_string_equal(out, "9999");
}

void test_print_int64_num() {
    char out[24];

    assert_true(print_int64_num((int64_t) 0, out, sizeof(out)));
    assert_string_equal(out, "0");

    assert_true(print_int64_num((int64_t) 1230, out, sizeof(out)));
    assert_string_equal(out, "1230");

    assert_true(print_int64_num((int64_t) -1230, out, sizeof(out)));
    assert_string_equal(out, "-1230");

    // test overflow, ignore the warngins
    assert_true(print_int64_num((int64_t) 9223372036854775807, out, sizeof(out)));
    assert_string_equal(out, "9223372036854775807");

    // test overflow, ignore the warngins
    assert_true(print_int64_num((int64_t) -9223372036854775808, out, sizeof(out)));
    assert_string_equal(out, "-9223372036854775808");

    // output buffer too small
    assert_false(print_int64_num((int64_t) -1230, out, 5));
    assert_false(print_int64_num(1230, out, 4));

    // output buffer just big enough to store output data
    assert_true(print_int64_num((int64_t) -9999, out, 6));
    assert_string_equal(out, "-9999");
    assert_true(print_int64_num((int64_t) 9999, out, 5));
    assert_string_equal(out, "9999");
}

void test_is_printable_binary() {
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

void test_print_binary() {
    const uint8_t binary[32] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
                                16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
    char hex[2 * sizeof(binary) + 1];

    assert_true(print_binary(binary, sizeof(binary), hex, sizeof(hex), 0, 0));
    assert_string_equal(hex, "000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F");
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

void test_print_int128() {
    char out[104];
    uint8_t data0[] = {0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00};
    assert_true(print_int128(data0, 0, out, sizeof(out), false));
    assert_string_equal(out, "0");
    assert_true(print_int128(data0, 0, out, sizeof(out), true));
    assert_string_equal(out, "0");
    assert_true(print_int128(data0, 7, out, sizeof(out), true));
    assert_string_equal(out, "0");

    uint8_t data1[] = {0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x01};
    assert_true(print_int128(data1, 0, out, sizeof(out), false));
    assert_string_equal(out, "1");
    assert_true(print_int128(data1, 0, out, sizeof(out), true));
    assert_string_equal(out, "1");
    assert_true(print_int128(data1, 7, out, sizeof(out), true));
    assert_string_equal(out, "0.0000001");

    uint8_t data2[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    assert_true(print_int256(data2, 0, out, sizeof(out), false));
    assert_string_equal(out, "-1");
    assert_true(print_int128(data2, 0, out, sizeof(out), true));
    assert_string_equal(out, "-1");
    assert_true(print_int128(data2, 7, out, sizeof(out), true));
    assert_string_equal(out, "-0.0000001");

    uint8_t data3[] = {0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x01,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00};
    assert_true(print_int128(data3, 0, out, sizeof(out), false));
    assert_string_equal(out, "18446744073709551616");
    assert_true(print_int128(data3, 0, out, sizeof(out), true));
    assert_string_equal(out, "18,446,744,073,709,551,616");
    assert_true(print_int128(data3, 7, out, sizeof(out), true));
    assert_string_equal(out, "1,844,674,407,370.9551616");

    uint8_t data4[] = {0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00};
    assert_true(print_int128(data4, 0, out, sizeof(out), false));
    assert_string_equal(out, "-18446744073709551616");
    assert_true(print_int128(data4, 0, out, sizeof(out), true));
    assert_string_equal(out, "-18,446,744,073,709,551,616");
    assert_true(print_int128(data4, 7, out, sizeof(out), true));
    assert_string_equal(out, "-1,844,674,407,370.9551616");

    uint8_t data5[] = {0x7f,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff};
    assert_true(print_int128(data5, 0, out, sizeof(out), false));
    assert_string_equal(out, "170141183460469231731687303715884105727");
    assert_true(print_int128(data5, 0, out, sizeof(out), true));
    assert_string_equal(out, "170,141,183,460,469,231,731,687,303,715,884,105,727");
    assert_true(print_int128(data5, 7, out, sizeof(out), true));
    assert_string_equal(out, "17,014,118,346,046,923,173,168,730,371,588.4105727");

    uint8_t data6[] = {0x80,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00};
    assert_true(print_int128(data6, 0, out, sizeof(out), false));
    assert_string_equal(out, "-170141183460469231731687303715884105728");
    assert_true(print_int128(data6, 0, out, sizeof(out), true));
    assert_string_equal(out, "-170,141,183,460,469,231,731,687,303,715,884,105,728");
    assert_true(print_int128(data6, 7, out, sizeof(out), true));
    assert_string_equal(out, "-17,014,118,346,046,923,173,168,730,371,588.4105728");
}

void test_print_uint128() {
    char out[104];
    uint8_t data0[] = {0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00};
    assert_true(print_uint128(data0, 0, out, sizeof(out), false));
    assert_string_equal(out, "0");
    assert_true(print_uint128(data0, 0, out, sizeof(out), true));
    assert_string_equal(out, "0");
    assert_true(print_uint128(data0, 8, out, sizeof(out), true));
    assert_string_equal(out, "0");

    uint8_t data1[] = {0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x01};
    assert_true(print_uint128(data1, 0, out, sizeof(out), false));
    assert_string_equal(out, "1");
    assert_true(print_uint128(data1, 0, out, sizeof(out), true));
    assert_string_equal(out, "1");
    assert_true(print_uint128(data1, 8, out, sizeof(out), true));
    assert_string_equal(out, "0.00000001");

    uint8_t data2[] = {0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x01,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00};
    assert_true(print_uint128(data2, 0, out, sizeof(out), false));
    assert_string_equal(out, "18446744073709551616");
    assert_true(print_uint128(data2, 0, out, sizeof(out), true));
    assert_string_equal(out, "18,446,744,073,709,551,616");
    assert_true(print_uint128(data2, 8, out, sizeof(out), true));
    assert_string_equal(out, "184,467,440,737.09551616");

    uint8_t data3[] = {0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff};
    assert_true(print_uint128(data3, 0, out, sizeof(out), false));
    assert_string_equal(out, "340282366920938463463374607431768211455");
    assert_true(print_uint128(data3, 0, out, sizeof(out), true));
    assert_string_equal(out, "340,282,366,920,938,463,463,374,607,431,768,211,455");
    assert_true(print_uint128(data3, 8, out, sizeof(out), true));
    assert_string_equal(out, "3,402,823,669,209,384,634,633,746,074,317.68211455");
}

void test_print_int256() {
    char out[104];
    uint8_t data0[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_int256(data0, 0, out, sizeof(out), false));
    assert_string_equal(out, "0");
    assert_true(print_int256(data0, 0, out, sizeof(out), true));
    assert_string_equal(out, "0");
    assert_true(print_int256(data0, 18, out, sizeof(out), true));
    assert_string_equal(out, "0");

    uint8_t data1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    assert_true(print_int256(data1, 0, out, sizeof(out), false));
    assert_string_equal(out, "1");
    assert_true(print_int256(data1, 0, out, sizeof(out), true));
    assert_string_equal(out, "1");
    assert_true(print_int256(data1, 18, out, sizeof(out), true));
    assert_string_equal(out, "0.000000000000000001");

    uint8_t data2[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    assert_true(print_int256(data2, 0, out, sizeof(out), false));
    assert_string_equal(out, "-1");
    assert_true(print_int256(data2, 0, out, sizeof(out), true));
    assert_string_equal(out, "-1");
    assert_true(print_int256(data2, 18, out, sizeof(out), true));
    assert_string_equal(out, "-0.000000000000000001");

    uint8_t data3[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_int256(data3, 0, out, sizeof(out), false));
    assert_string_equal(out, "18446744073709551616");
    assert_true(print_int256(data3, 0, out, sizeof(out), true));
    assert_string_equal(out, "18,446,744,073,709,551,616");
    assert_true(print_int256(data3, 18, out, sizeof(out), true));
    assert_string_equal(out, "18.446744073709551616");

    uint8_t data4[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                       0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_int256(data4, 0, out, sizeof(out), false));
    assert_string_equal(out, "-18446744073709551616");
    assert_true(print_int256(data4, 0, out, sizeof(out), true));
    assert_string_equal(out, "-18,446,744,073,709,551,616");
    assert_true(print_int256(data4, 18, out, sizeof(out), true));
    assert_string_equal(out, "-18.446744073709551616");

    uint8_t data5[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_int256(data5, 0, out, sizeof(out), false));
    assert_string_equal(out, "340282366920938463463374607431768211456");
    assert_true(print_int256(data5, 0, out, sizeof(out), true));
    assert_string_equal(out, "340,282,366,920,938,463,463,374,607,431,768,211,456");
    assert_true(print_int256(data5, 18, out, sizeof(out), true));
    assert_string_equal(out, "340,282,366,920,938,463,463.374607431768211456");

    uint8_t data6[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                       0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_int256(data6, 0, out, sizeof(out), false));
    assert_string_equal(out, "-340282366920938463463374607431768211456");
    assert_true(print_int256(data6, 0, out, sizeof(out), true));
    assert_string_equal(out, "-340,282,366,920,938,463,463,374,607,431,768,211,456");
    assert_true(print_int256(data6, 18, out, sizeof(out), true));
    assert_string_equal(out, "-340,282,366,920,938,463,463.374607431768211456");

    uint8_t data7[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_int256(data7, 0, out, sizeof(out), false));
    assert_string_equal(out, "6277101735386680763835789423207666416102355444464034512896");
    assert_true(print_int256(data7, 0, out, sizeof(out), true));
    assert_string_equal(
        out,
        "6,277,101,735,386,680,763,835,789,423,207,666,416,102,355,444,464,034,512,896");
    assert_true(print_int256(data7, 18, out, sizeof(out), true));
    assert_string_equal(out,
                        "6,277,101,735,386,680,763,835,789,423,207,666,416,102.355444464034512896");

    uint8_t data8[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_int256(data8, 0, out, sizeof(out), false));
    assert_string_equal(out, "-6277101735386680763835789423207666416102355444464034512896");
    assert_true(print_int256(data8, 0, out, sizeof(out), true));
    assert_string_equal(
        out,
        "-6,277,101,735,386,680,763,835,789,423,207,666,416,102,355,444,464,034,512,896");
    assert_true(print_int256(data8, 18, out, sizeof(out), true));
    assert_string_equal(
        out,
        "-6,277,101,735,386,680,763,835,789,423,207,666,416,102.355444464034512896");

    uint8_t data9[] = {0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    assert_true(print_int256(data9, 0, out, sizeof(out), false));
    assert_string_equal(
        out,
        "57896044618658097711785492504343953926634992332820282019728792003956564819967");
    assert_true(print_int256(data9, 0, out, sizeof(out), true));
    assert_string_equal(out,
                        "57,896,044,618,658,097,711,785,492,504,343,953,926,634,992,332,820,282,"
                        "019,728,792,003,956,564,819,967");
    assert_true(print_int256(data9, 18, out, sizeof(out), true));
    assert_string_equal(out,
                        "57,896,044,618,658,097,711,785,492,504,343,953,926,634,992,332,820,282,"
                        "019,728.792003956564819967");

    uint8_t data10[] = {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_int256(data10, 0, out, sizeof(out), false));
    assert_string_equal(
        out,
        "-57896044618658097711785492504343953926634992332820282019728792003956564819968");
    assert_true(print_int256(data10, 0, out, sizeof(out), true));
    assert_string_equal(out,
                        "-57,896,044,618,658,097,711,785,492,504,343,953,926,634,992,332,820,282,"
                        "019,728,792,003,956,564,819,968");
    assert_true(print_int256(data10, 18, out, sizeof(out), true));
    assert_string_equal(out,
                        "-57,896,044,618,658,097,711,785,492,504,343,953,926,634,992,332,820,282,"
                        "019,728.792003956564819968");

    uint8_t data11[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xc3, 0x7a, 0xa9};
    assert_true(print_int256(data11, 0, out, sizeof(out), false));
    assert_string_equal(out, "6277101735386680763835789423207666567218082896292727716521");
    assert_true(print_int256(data11, 0, out, sizeof(out), true));
    assert_string_equal(
        out,
        "6,277,101,735,386,680,763,835,789,423,207,666,567,218,082,896,292,727,716,521");
    assert_true(print_int256(data11, 18, out, sizeof(out), true));
    assert_string_equal(out,
                        "6,277,101,735,386,680,763,835,789,423,207,666,567,218.082896292727716521");

    uint8_t data12[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff,
                        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                        0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, 0x3c, 0x85, 0x57};
    assert_true(print_int256(data12, 0, out, sizeof(out), false));
    assert_string_equal(out, "-6277101735386680763835789423207666567218082896292727716521");
    assert_true(print_int256(data12, 0, out, sizeof(out), true));
    assert_string_equal(
        out,
        "-6,277,101,735,386,680,763,835,789,423,207,666,567,218,082,896,292,727,716,521");
    assert_true(print_int256(data12, 18, out, sizeof(out), true));
    assert_string_equal(
        out,
        "-6,277,101,735,386,680,763,835,789,423,207,666,567,218.082896292727716521");
}

void test_print_uint256() {
    char out[104];
    uint8_t data0[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_uint256(data0, 0, out, sizeof(out), false));
    assert_string_equal(out, "0");
    assert_true(print_uint256(data0, 0, out, sizeof(out), true));
    assert_string_equal(out, "0");
    assert_true(print_uint256(data0, 13, out, sizeof(out), true));
    assert_string_equal(out, "0");

    uint8_t data1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    assert_true(print_uint256(data1, 0, out, sizeof(out), false));
    assert_string_equal(out, "1");
    assert_true(print_uint256(data1, 0, out, sizeof(out), true));
    assert_string_equal(out, "1");
    assert_true(print_uint256(data1, 13, out, sizeof(out), true));
    assert_string_equal(out, "0.0000000000001");

    uint8_t data2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_uint256(data2, 0, out, sizeof(out), false));
    assert_string_equal(out, "18446744073709551616");
    assert_true(print_uint256(data2, 0, out, sizeof(out), true));
    assert_string_equal(out, "18,446,744,073,709,551,616");
    assert_true(print_uint256(data2, 13, out, sizeof(out), true));
    assert_string_equal(out, "1,844,674.4073709551616");

    uint8_t data3[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_uint256(data3, 0, out, sizeof(out), false));
    assert_string_equal(out, "340282366920938463463374607431768211456");
    assert_true(print_uint256(data3, 0, out, sizeof(out), true));
    assert_string_equal(out, "340,282,366,920,938,463,463,374,607,431,768,211,456");
    assert_true(print_uint256(data3, 13, out, sizeof(out), true));
    assert_string_equal(out, "34,028,236,692,093,846,346,337,460.7431768211456");

    uint8_t data4[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_uint256(data4, 0, out, sizeof(out), false));
    assert_string_equal(out, "6277101735386680763835789423207666416102355444464034512896");
    assert_true(print_uint256(data4, 0, out, sizeof(out), true));
    assert_string_equal(
        out,
        "6,277,101,735,386,680,763,835,789,423,207,666,416,102,355,444,464,034,512,896");
    assert_true(print_uint256(data4, 13, out, sizeof(out), true));
    assert_string_equal(
        out,
        "627,710,173,538,668,076,383,578,942,320,766,641,610,235,544.4464034512896");

    uint8_t data5[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xc3, 0x7a, 0xa9};
    assert_true(print_uint256(data5, 0, out, sizeof(out), false));
    assert_string_equal(out, "6277101735386680763835789423207666567218082896292727716521");
    assert_true(print_uint256(data5, 0, out, sizeof(out), true));
    assert_string_equal(
        out,
        "6,277,101,735,386,680,763,835,789,423,207,666,567,218,082,896,292,727,716,521");
    assert_true(print_uint256(data5, 13, out, sizeof(out), true));
    assert_string_equal(
        out,
        "627,710,173,538,668,076,383,578,942,320,766,656,721,808,289.6292727716521");

    uint8_t data6[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    assert_true(print_uint256(data6, 0, out, sizeof(out), false));
    assert_string_equal(
        out,
        "115792089237316195423570985008687907853269984665640564039457584007913129639935");
    assert_true(print_uint256(data6, 0, out, sizeof(out), true));
    assert_string_equal(out,
                        "115,792,089,237,316,195,423,570,985,008,687,907,853,269,984,665,640,564,"
                        "039,457,584,007,913,129,639,935");
    assert_true(print_uint256(data6, 13, out, sizeof(out), true));
    assert_string_equal(out,
                        "11,579,208,923,731,619,542,357,098,500,868,790,785,326,998,466,564,056,"
                        "403,945,758,400.7913129639935");
}

void test_print_int32() {
    char out[104];
    uint8_t data0[] = {0x00, 0x00, 0x00, 0x00};
    assert_true(print_int32(data0, 0, out, sizeof(out), false));
    assert_string_equal(out, "0");
    assert_true(print_int32(data0, 0, out, sizeof(out), true));
    assert_string_equal(out, "0");
    assert_true(print_int32(data0, 7, out, sizeof(out), true));
    assert_string_equal(out, "0");

    uint8_t data1[] = {0x00, 0x00, 0x00, 0x01};
    assert_true(print_int32(data1, 0, out, sizeof(out), false));
    assert_string_equal(out, "1");
    assert_true(print_int32(data1, 0, out, sizeof(out), true));
    assert_string_equal(out, "1");
    assert_true(print_int32(data1, 7, out, sizeof(out), true));
    assert_string_equal(out, "0.0000001");

    uint8_t data2[] = {0xff, 0xff, 0xff, 0xff};
    assert_true(print_int32(data2, 0, out, sizeof(out), false));
    assert_string_equal(out, "-1");
    assert_true(print_int32(data2, 0, out, sizeof(out), true));
    assert_string_equal(out, "-1");
    assert_true(print_int32(data2, 7, out, sizeof(out), true));
    assert_string_equal(out, "-0.0000001");

    uint8_t data3[] = {0x00, 0x01, 0x00, 0x00};
    assert_true(print_int32(data3, 0, out, sizeof(out), false));
    assert_string_equal(out, "65536");
    assert_true(print_int32(data3, 0, out, sizeof(out), true));
    assert_string_equal(out, "65,536");
    assert_true(print_int32(data3, 7, out, sizeof(out), true));
    assert_string_equal(out, "0.0065536");

    uint8_t data4[] = {0xff, 0xff, 0x00, 0x00};
    assert_true(print_int32(data4, 0, out, sizeof(out), false));
    assert_string_equal(out, "-65536");
    assert_true(print_int32(data4, 0, out, sizeof(out), true));
    assert_string_equal(out, "-65,536");
    assert_true(print_int32(data4, 7, out, sizeof(out), true));
    assert_string_equal(out, "-0.0065536");

    uint8_t data5[] = {0x02, 0x10, 0x17, 0x4b};
    assert_true(print_int32(data5, 0, out, sizeof(out), false));
    assert_string_equal(out, "34608971");
    assert_true(print_int32(data5, 0, out, sizeof(out), true));
    assert_string_equal(out, "34,608,971");
    assert_true(print_int32(data5, 7, out, sizeof(out), true));
    assert_string_equal(out, "3.4608971");

    uint8_t data6[] = {0xfd, 0xf1, 0xe8, 0xb5};
    assert_true(print_int32(data6, 0, out, sizeof(out), false));
    assert_string_equal(out, "-34477899");
    assert_true(print_int32(data6, 0, out, sizeof(out), true));
    assert_string_equal(out, "-34,477,899");
    assert_true(print_int32(data6, 7, out, sizeof(out), true));
    assert_string_equal(out, "-3.4477899");

    uint8_t data7[] = {0x7f, 0xff, 0xff, 0xff};
    assert_true(print_int32(data7, 0, out, sizeof(out), false));
    assert_string_equal(out, "2147483647");
    assert_true(print_int32(data7, 0, out, sizeof(out), true));
    assert_string_equal(out, "2,147,483,647");
    assert_true(print_int32(data7, 7, out, sizeof(out), true));
    assert_string_equal(out, "214.7483647");

    uint8_t data8[] = {0x80, 0x00, 0x00, 0x00};
    assert_true(print_int32(data8, 0, out, sizeof(out), false));
    assert_string_equal(out, "-2147483648");
    assert_true(print_int32(data8, 0, out, sizeof(out), true));
    assert_string_equal(out, "-2,147,483,648");
    assert_true(print_int32(data8, 7, out, sizeof(out), true));
    assert_string_equal(out, "-214.7483648");
}

void test_print_uint32() {
    char out[104];
    uint8_t data0[] = {0x00, 0x00, 0x00, 0x00};
    assert_true(print_uint32(data0, 0, out, sizeof(out), false));
    assert_string_equal(out, "0");
    assert_true(print_uint32(data0, 0, out, sizeof(out), true));
    assert_string_equal(out, "0");
    assert_true(print_uint32(data0, 7, out, sizeof(out), true));
    assert_string_equal(out, "0");

    uint8_t data1[] = {0x00, 0x00, 0x00, 0x01};
    assert_true(print_uint32(data1, 0, out, sizeof(out), false));
    assert_string_equal(out, "1");
    assert_true(print_uint32(data1, 0, out, sizeof(out), true));
    assert_string_equal(out, "1");
    assert_true(print_uint32(data1, 7, out, sizeof(out), true));
    assert_string_equal(out, "0.0000001");

    uint8_t data2[] = {0x00, 0x01, 0x00, 0x00};
    assert_true(print_uint32(data2, 0, out, sizeof(out), false));
    assert_string_equal(out, "65536");
    assert_true(print_uint32(data2, 0, out, sizeof(out), true));
    assert_string_equal(out, "65,536");
    assert_true(print_uint32(data2, 7, out, sizeof(out), true));
    assert_string_equal(out, "0.0065536");

    uint8_t data3[] = {0xfd, 0xf0, 0xe8, 0xb5};
    assert_true(print_uint32(data3, 0, out, sizeof(out), false));
    assert_string_equal(out, "4260423861");
    assert_true(print_uint32(data3, 0, out, sizeof(out), true));
    assert_string_equal(out, "4,260,423,861");
    assert_true(print_uint32(data3, 7, out, sizeof(out), true));
    assert_string_equal(out, "426.0423861");

    uint8_t data4[] = {0xff, 0xff, 0xff, 0xff};
    assert_true(print_uint32(data4, 0, out, sizeof(out), false));
    assert_string_equal(out, "4294967295");
    assert_true(print_uint32(data4, 0, out, sizeof(out), true));
    assert_string_equal(out, "4,294,967,295");
    assert_true(print_uint32(data4, 7, out, sizeof(out), true));
    assert_string_equal(out, "429.4967295");
}

void test_print_int64() {
    char out[104];
    uint8_t data0[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_int64(data0, 0, out, sizeof(out), false));
    assert_string_equal(out, "0");
    assert_true(print_int64(data0, 0, out, sizeof(out), true));
    assert_string_equal(out, "0");
    assert_true(print_int64(data0, 7, out, sizeof(out), true));
    assert_string_equal(out, "0");

    uint8_t data1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    assert_true(print_int64(data1, 0, out, sizeof(out), false));
    assert_string_equal(out, "1");
    assert_true(print_int64(data1, 0, out, sizeof(out), true));
    assert_string_equal(out, "1");
    assert_true(print_int64(data1, 7, out, sizeof(out), true));
    assert_string_equal(out, "0.0000001");

    uint8_t data2[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    assert_true(print_int64(data2, 0, out, sizeof(out), false));
    assert_string_equal(out, "-1");
    assert_true(print_int64(data2, 0, out, sizeof(out), true));
    assert_string_equal(out, "-1");
    assert_true(print_int64(data2, 7, out, sizeof(out), true));
    assert_string_equal(out, "-0.0000001");

    uint8_t data3[] = {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_int64(data3, 0, out, sizeof(out), false));
    assert_string_equal(out, "4294967296");
    assert_true(print_int64(data3, 0, out, sizeof(out), true));
    assert_string_equal(out, "4,294,967,296");
    assert_true(print_int64(data3, 7, out, sizeof(out), true));
    assert_string_equal(out, "429.4967296");

    uint8_t data4[] = {0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_int64(data4, 0, out, sizeof(out), false));
    assert_string_equal(out, "-4294967296");
    assert_true(print_int64(data4, 0, out, sizeof(out), true));
    assert_string_equal(out, "-4,294,967,296");
    assert_true(print_int64(data4, 7, out, sizeof(out), true));
    assert_string_equal(out, "-429.4967296");

    uint8_t data5[] = {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x38, 0xb9};
    assert_true(print_int64(data5, 0, out, sizeof(out), false));
    assert_string_equal(out, "4294981817");
    assert_true(print_int64(data5, 0, out, sizeof(out), true));
    assert_string_equal(out, "4,294,981,817");
    assert_true(print_int64(data5, 7, out, sizeof(out), true));
    assert_string_equal(out, "429.4981817");

    uint8_t data6[] = {0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xc7, 0x47};
    assert_true(print_int64(data6, 0, out, sizeof(out), false));
    assert_string_equal(out, "-4294981817");
    assert_true(print_int64(data6, 0, out, sizeof(out), true));
    assert_string_equal(out, "-4,294,981,817");
    assert_true(print_int64(data6, 7, out, sizeof(out), true));
    assert_string_equal(out, "-429.4981817");

    uint8_t data7[] = {0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    assert_true(print_int64(data7, 0, out, sizeof(out), false));
    assert_string_equal(out, "9223372036854775807");
    assert_true(print_int64(data7, 0, out, sizeof(out), true));
    assert_string_equal(out, "9,223,372,036,854,775,807");
    assert_true(print_int64(data7, 7, out, sizeof(out), true));
    assert_string_equal(out, "922,337,203,685.4775807");

    uint8_t data8[] = {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_int64(data8, 0, out, sizeof(out), false));
    assert_string_equal(out, "-9223372036854775808");
    assert_true(print_int64(data8, 0, out, sizeof(out), true));
    assert_string_equal(out, "-9,223,372,036,854,775,808");
    assert_true(print_int64(data8, 7, out, sizeof(out), true));
    assert_string_equal(out, "-922,337,203,685.4775808");
}

void test_print_uint64() {
    char out[104];
    uint8_t data0[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_uint64(data0, 0, out, sizeof(out), false));
    assert_string_equal(out, "0");
    assert_true(print_uint64(data0, 0, out, sizeof(out), true));
    assert_string_equal(out, "0");
    assert_true(print_uint64(data0, 7, out, sizeof(out), true));
    assert_string_equal(out, "0");

    uint8_t data1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    assert_true(print_uint64(data1, 0, out, sizeof(out), false));
    assert_string_equal(out, "1");
    assert_true(print_uint64(data1, 0, out, sizeof(out), true));
    assert_string_equal(out, "1");
    assert_true(print_uint64(data1, 7, out, sizeof(out), true));
    assert_string_equal(out, "0.0000001");

    uint8_t data2[] = {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
    assert_true(print_uint64(data2, 0, out, sizeof(out), false));
    assert_string_equal(out, "4294967296");
    assert_true(print_uint64(data2, 0, out, sizeof(out), true));
    assert_string_equal(out, "4,294,967,296");
    assert_true(print_uint64(data2, 7, out, sizeof(out), true));
    assert_string_equal(out, "429.4967296");

    uint8_t data3[] = {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x38, 0xb9};
    assert_true(print_uint64(data3, 0, out, sizeof(out), false));
    assert_string_equal(out, "4294981817");
    assert_true(print_uint64(data3, 0, out, sizeof(out), true));
    assert_string_equal(out, "4,294,981,817");
    assert_true(print_uint64(data3, 7, out, sizeof(out), true));
    assert_string_equal(out, "429.4981817");

    uint8_t data4[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    assert_true(print_uint64(data4, 0, out, sizeof(out), false));
    assert_string_equal(out, "18446744073709551615");
    assert_true(print_uint64(data4, 0, out, sizeof(out), true));
    assert_string_equal(out, "18,446,744,073,709,551,615");
    assert_true(print_uint64(data4, 7, out, sizeof(out), true));
    assert_string_equal(out, "1,844,674,407,370.9551615");
}

void test_print_scv_symbol() {
    char out[89];

    uint8_t data0[] = {0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x00, 0x00, 0x00};
    scv_symbol_t symbol0 = {.symbol = data0, .size = 5};
    assert_true(print_scv_symbol(&symbol0, out, sizeof(out)));
    assert_string_equal(out, "hello");

    uint8_t data1[] = {0x61, 0x62, 0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61, 0x62, 0x63,
                       0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61, 0x62,
                       0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d};
    scv_symbol_t symbol1 = {.symbol = data1, .size = 32};
    assert_true(print_scv_symbol(&symbol1, out, sizeof(out)));
    assert_string_equal(out, "abc-abc-abc-abc-abc-abc-abc-abc-");

    uint8_t data2[] = {0x01};
    scv_symbol_t symbol2 = {.symbol = data2, .size = 0};
    assert_true(print_scv_symbol(&symbol2, out, sizeof(out)));
    assert_string_equal(out, "[empty symbol]");

    uint8_t data3[] = {0x61, 0x62, 0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61, 0x62, 0x63,
                       0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61, 0x62,
                       0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d, 0x2d};
    scv_symbol_t symbol3 = {.symbol = data3, .size = 33};
    assert_false(print_scv_symbol(&symbol3, out, sizeof(out)));

    uint8_t data4[] = {0x61, 0x62, 0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61, 0x62, 0x63,
                       0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61, 0x62,
                       0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61, 0x62, 0x63, 0x7f};
    scv_symbol_t symbol4 = {.symbol = data4, .size = 33};
    assert_false(print_scv_symbol(&symbol4, out, sizeof(out)));
}

void test_print_scv_string() {
    char out[28];

    uint8_t data0[] = {0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x00, 0x00, 0x00};
    scv_string_t string0 = {.string = data0, .size = 5};
    assert_true(print_scv_string(&string0, out, sizeof(out)));
    assert_string_equal(out, "hello");

    uint8_t data1[] = {0x01};
    scv_string_t string1 = {.string = data1, .size = 0};
    assert_true(print_scv_string(&string1, out, sizeof(out)));
    assert_string_equal(out, "[empty string]");

    uint8_t data2[] = {0x61, 0x62, 0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61,
                       0x62, 0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61, 0x62,
                       0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61, 0x62, 0x63};
    scv_string_t string2 = {.string = data2, .size = 27};
    assert_true(print_scv_string(&string2, out, sizeof(out)));
    assert_string_equal(out, "abc-abc-abc-abc-abc-abc-abc");

    uint8_t data3[] = {0x61, 0x2d, 0x62, 0x2d, 0x63, 0x2d, 0x64, 0x2d, 0x65, 0x2d, 0x66,
                       0x2d, 0x67, 0x2d, 0x68, 0x2d, 0x69, 0x2d, 0x6a, 0x2d, 0x6b, 0x2d,
                       0x6c, 0x2d, 0x6d, 0x2d, 0x6e, 0x2d, 0x6f, 0x2d, 0x70, 0x2d, 0x71,
                       0x2d, 0x72, 0x2d, 0x73, 0x2d, 0x74, 0x2d, 0x75, 0x2d, 0x76, 0x2d,
                       0x77, 0x2d, 0x78, 0x2d, 0x79, 0x2d, 0x7a, 0x2d};
    scv_string_t string3 = {.string = data3, .size = 51};
    assert_true(print_scv_string(&string3, out, sizeof(out)));
    assert_string_equal(out, "a-b-c-d-e-f-g..-u-v-w-x-y-z");

    uint8_t data4[] = {0x61, 0x2d, 0x62, 0x2d, 0x63, 0x2d, 0x64, 0x2d, 0x65, 0x2d, 0x66,
                       0x2d, 0x67, 0x2d, 0x68, 0x2d, 0x69, 0x2d, 0x6a, 0x2d, 0x6b, 0x2d,
                       0x6c, 0x2d, 0x6d, 0x2d, 0x6e, 0x2d, 0x6f, 0x2d, 0x70, 0x2d, 0x71,
                       0x2d, 0x72, 0x2d, 0x73, 0x2d, 0x74, 0x2d, 0x75, 0x2d, 0x76, 0x2d,
                       0x77, 0x2d, 0x78, 0x2d, 0x79, 0x2d, 0x7a, 0x00};
    scv_string_t string4 = {.string = data4, .size = 28};
    assert_true(print_scv_string(&string4, out, sizeof(out)));
    assert_string_equal(out, "a-b-c-d-e-f-g..i-j-k-l-m-n-");

    char out1[27];

    uint8_t data5[] = {0x61, 0x2d, 0x62, 0x2d, 0x63, 0x2d, 0x64, 0x2d, 0x65, 0x2d, 0x66,
                       0x2d, 0x67, 0x2d, 0x68, 0x2d, 0x69, 0x2d, 0x6a, 0x2d, 0x6b, 0x2d,
                       0x6c, 0x2d, 0x6d, 0x2d, 0x6e, 0x2d, 0x6f, 0x2d, 0x70, 0x2d, 0x71,
                       0x2d, 0x72, 0x2d, 0x73, 0x2d, 0x74, 0x2d, 0x75, 0x2d, 0x76, 0x2d,
                       0x77, 0x2d, 0x78, 0x2d, 0x79, 0x2d, 0x7a, 0x2d};
    scv_string_t string5 = {.string = data5, .size = 51};
    assert_true(print_scv_string(&string5, out1, sizeof(out1)));
    assert_string_equal(out1, "a-b-c-d-e-f-g..u-v-w-x-y-z");

    uint8_t data6[] = {0x61, 0x2d, 0x62, 0x2d, 0x63, 0x2d, 0x64, 0x2d, 0x65, 0x2d, 0x66,
                       0x2d, 0x67, 0x2d, 0x68, 0x2d, 0x69, 0x2d, 0x6a, 0x2d, 0x6b, 0x2d,
                       0x6c, 0x2d, 0x6d, 0x2d, 0x6e, 0x2d, 0x6f, 0x2d, 0x70, 0x2d, 0x71,
                       0x2d, 0x72, 0x2d, 0x73, 0x2d, 0x74, 0x2d, 0x75, 0x2d, 0x76, 0x2d,
                       0x77, 0x2d, 0x78, 0x2d, 0x79, 0x2d, 0x7a, 0x00};
    scv_string_t string6 = {.string = data6, .size = 27};
    assert_true(print_scv_string(&string6, out1, sizeof(out1)));
    assert_string_equal(out1, "a-b-c-d-e-f-g..i-j-k-l-m-n");

    uint8_t data7[] = {0x61, 0x62, 0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61,
                       0x62, 0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61, 0x62,
                       0x63, 0x2d, 0x61, 0x62, 0x63, 0x2d, 0x61, 0x62, 0x63};
    scv_string_t string7 = {.string = data7, .size = 26};
    assert_true(print_scv_string(&string7, out, sizeof(out)));
    assert_string_equal(out, "abc-abc-abc-abc-abc-abc-ab");
}

void test_add_separator_to_integer_positive() {
    // len = 1
    char out1[104] = {'3', '\0'};
    assert_true(add_separator_to_number(out1, sizeof(out1)));
    assert_string_equal(out1, "3");

    // len = 2
    char out2[104] = {'6', '3', '\0'};
    assert_true(add_separator_to_number(out2, sizeof(out2)));
    assert_string_equal(out2, "63");

    // len = 3
    char out3[104] = {'9', '6', '4', '\0'};
    assert_true(add_separator_to_number(out3, sizeof(out3)));
    assert_string_equal(out3, "964");

    // len = 4
    char out4[104] = {'2', '9', '4', '4', '\0'};
    assert_true(add_separator_to_number(out4, sizeof(out4)));
    assert_string_equal(out4, "2,944");

    // len = 5
    char out5[104] = {'3', '8', '5', '3', '5', '\0'};
    assert_true(add_separator_to_number(out5, sizeof(out5)));
    assert_string_equal(out5, "38,535");

    // len = 6
    char out6[104] = {'5', '2', '3', '7', '1', '3', '\0'};
    assert_true(add_separator_to_number(out6, sizeof(out6)));
    assert_string_equal(out6, "523,713");

    // len = 7
    char out7[104] = {'5', '3', '7', '7', '4', '5', '5', '\0'};
    assert_true(add_separator_to_number(out7, sizeof(out7)));
    assert_string_equal(out7, "5,377,455");

    // len = 8
    char out8[104] = {'6', '4', '1', '7', '8', '9', '5', '8', '\0'};
    assert_true(add_separator_to_number(out8, sizeof(out8)));
    assert_string_equal(out8, "64,178,958");

    // len = 9
    char out9[104] = {'2', '7', '4', '7', '7', '6', '6', '6', '1', '\0'};
    assert_true(add_separator_to_number(out9, sizeof(out9)));
    assert_string_equal(out9, "274,776,661");

    // len = 10
    char out10[104] = {'6', '2', '5', '4', '6', '2', '4', '9', '7', '6', '\0'};
    assert_true(add_separator_to_number(out10, sizeof(out10)));
    assert_string_equal(out10, "6,254,624,976");

    // len = 11
    char out11[104] = {'2', '8', '8', '2', '7', '5', '6', '1', '2', '2', '8', '\0'};
    assert_true(add_separator_to_number(out11, sizeof(out11)));
    assert_string_equal(out11, "28,827,561,228");

    // len = 12
    char out12[104] = {'2', '2', '9', '4', '8', '1', '2', '4', '6', '2', '6', '5', '\0'};
    assert_true(add_separator_to_number(out12, sizeof(out12)));
    assert_string_equal(out12, "229,481,246,265");

    // len = 13
    char out13[104] = {'5', '2', '6', '3', '3', '3', '6', '2', '4', '6', '7', '8', '2', '\0'};
    assert_true(add_separator_to_number(out13, sizeof(out13)));
    assert_string_equal(out13, "5,263,336,246,782");

    // len = 14
    char out14[104] = {'3', '8', '9', '5', '8', '7', '9', '1', '9', '9', '9', '3', '1', '2', '\0'};
    assert_true(add_separator_to_number(out14, sizeof(out14)));
    assert_string_equal(out14, "38,958,791,999,312");

    // len = 15
    char out15[104] =
        {'1', '2', '4', '6', '2', '9', '4', '6', '6', '2', '7', '5', '7', '7', '1', '\0'};
    assert_true(add_separator_to_number(out15, sizeof(out15)));
    assert_string_equal(out15, "124,629,466,275,771");

    // len = 16
    char out16[104] =
        {'5', '4', '8', '9', '8', '5', '2', '9', '8', '9', '9', '4', '3', '6', '6', '3', '\0'};
    assert_true(add_separator_to_number(out16, sizeof(out16)));
    assert_string_equal(out16, "5,489,852,989,943,663");

    // len = 17
    char out17[104] =
        {'8', '7', '6', '4', '8', '6', '4', '9', '6', '5', '2', '7', '8', '4', '9', '4', '9', '\0'};
    assert_true(add_separator_to_number(out17, sizeof(out17)));
    assert_string_equal(out17, "87,648,649,652,784,949");

    // len = 18
    char out18[104] = {'4',
                       '4',
                       '6',
                       '3',
                       '7',
                       '8',
                       '3',
                       '6',
                       '5',
                       '1',
                       '6',
                       '3',
                       '7',
                       '4',
                       '3',
                       '8',
                       '5',
                       '1',
                       '\0'};
    assert_true(add_separator_to_number(out18, sizeof(out18)));
    assert_string_equal(out18, "446,378,365,163,743,851");

    // len = 19
    char out19[104] = {'8', '8', '6', '3', '1', '8', '9', '3', '5', '8',
                       '7', '5', '8', '8', '7', '9', '8', '7', '5', '\0'};
    assert_true(add_separator_to_number(out19, sizeof(out19)));
    assert_string_equal(out19, "8,863,189,358,758,879,875");

    // len = 20
    char out20[104] = {'5', '6', '1', '5', '6', '6', '7', '7', '4', '9', '7',
                       '4', '1', '9', '8', '6', '3', '5', '1', '2', '\0'};
    assert_true(add_separator_to_number(out20, sizeof(out20)));
    assert_string_equal(out20, "56,156,677,497,419,863,512");

    // len = 21
    char out21[104] = {'9', '8', '3', '4', '1', '7', '4', '6', '4', '2', '8',
                       '5', '8', '8', '4', '1', '2', '3', '2', '4', '2', '\0'};
    assert_true(add_separator_to_number(out21, sizeof(out21)));
    assert_string_equal(out21, "983,417,464,285,884,123,242");

    // len = 22
    char out22[104] = {'4', '2', '1', '9', '9', '7', '9', '2', '5', '7', '9', '7',
                       '3', '5', '3', '6', '6', '6', '1', '4', '5', '8', '\0'};
    assert_true(add_separator_to_number(out22, sizeof(out22)));
    assert_string_equal(out22, "4,219,979,257,973,536,661,458");

    // len = 23
    char out23[104] = {'9', '8', '6', '8', '9', '4', '7', '2', '2', '1', '9', '8',
                       '9', '9', '6', '8', '1', '7', '7', '8', '6', '8', '5', '\0'};
    assert_true(add_separator_to_number(out23, sizeof(out23)));
    assert_string_equal(out23, "98,689,472,219,899,681,778,685");

    // len = 24
    char out24[104] = {'4', '4', '1', '8', '4', '2', '8', '8', '5', '5', '3', '5', '7',
                       '7', '2', '2', '4', '2', '4', '1', '4', '1', '1', '5', '\0'};
    assert_true(add_separator_to_number(out24, sizeof(out24)));
    assert_string_equal(out24, "441,842,885,535,772,242,414,115");

    // len = 25
    char out25[104] = {'5', '9', '7', '6', '2', '8', '6', '7', '4', '7', '6', '6', '4',
                       '8', '4', '3', '2', '5', '9', '5', '3', '5', '4', '7', '5', '\0'};
    assert_true(add_separator_to_number(out25, sizeof(out25)));
    assert_string_equal(out25, "5,976,286,747,664,843,259,535,475");

    // len = 26
    char out26[104] = {'6', '2', '3', '7', '3', '4', '6', '5', '9', '3', '8', '4', '3', '8',
                       '1', '9', '1', '1', '6', '3', '8', '4', '2', '9', '3', '7', '\0'};
    assert_true(add_separator_to_number(out26, sizeof(out26)));
    assert_string_equal(out26, "62,373,465,938,438,191,163,842,937");

    // len = 27
    char out27[104] = {'9', '4', '7', '3', '3', '9', '7', '7', '9', '5', '5', '1', '9', '1',
                       '7', '5', '5', '9', '2', '2', '5', '2', '7', '3', '2', '9', '3', '\0'};
    assert_true(add_separator_to_number(out27, sizeof(out27)));
    assert_string_equal(out27, "947,339,779,551,917,559,225,273,293");

    // len = 28
    char out28[104] = {'9', '1', '9', '4', '2', '3', '8', '7', '5', '6', '2', '4', '2', '3', '6',
                       '9', '3', '5', '6', '9', '5', '2', '1', '3', '5', '7', '6', '1', '\0'};
    assert_true(add_separator_to_number(out28, sizeof(out28)));
    assert_string_equal(out28, "9,194,238,756,242,369,356,952,135,761");

    // len = 29
    char out29[104] = {'2', '3', '4', '2', '5', '2', '7', '9', '8', '8', '9', '8', '4', '8', '9',
                       '4', '1', '7', '4', '2', '8', '8', '2', '7', '8', '4', '3', '8', '5', '\0'};
    assert_true(add_separator_to_number(out29, sizeof(out29)));
    assert_string_equal(out29, "23,425,279,889,848,941,742,882,784,385");

    // len = 30
    char out30[104] = {'4', '3', '6', '3', '1', '8', '7', '8', '4', '8', '9',
                       '9', '2', '4', '6', '7', '8', '5', '8', '9', '1', '5',
                       '1', '9', '3', '5', '7', '7', '7', '4', '\0'};
    assert_true(add_separator_to_number(out30, sizeof(out30)));
    assert_string_equal(out30, "436,318,784,899,246,785,891,519,357,774");

    // len = 31
    char out31[104] = {'4', '5', '3', '4', '6', '6', '9', '1', '9', '9', '6',
                       '3', '3', '4', '2', '1', '1', '7', '2', '8', '4', '8',
                       '6', '7', '2', '8', '7', '8', '5', '8', '2', '\0'};
    assert_true(add_separator_to_number(out31, sizeof(out31)));
    assert_string_equal(out31, "4,534,669,199,633,421,172,848,672,878,582");

    // len = 32
    char out32[104] = {'6', '8', '2', '5', '5', '2', '4', '4', '7', '4', '8',
                       '7', '7', '7', '4', '5', '2', '1', '1', '2', '4', '6',
                       '8', '6', '7', '8', '2', '4', '6', '5', '3', '9', '\0'};
    assert_true(add_separator_to_number(out32, sizeof(out32)));
    assert_string_equal(out32, "68,255,244,748,777,452,112,468,678,246,539");

    // len = 33
    char out33[104] = {'1', '4', '8', '7', '1', '7', '6', '5', '5', '4', '3', '7',
                       '5', '3', '1', '8', '7', '7', '9', '8', '2', '9', '7', '9',
                       '7', '3', '8', '3', '3', '9', '3', '4', '7', '\0'};
    assert_true(add_separator_to_number(out33, sizeof(out33)));
    assert_string_equal(out33, "148,717,655,437,531,877,982,979,738,339,347");

    // len = 34
    char out34[104] = {'2', '1', '1', '4', '3', '8', '1', '8', '2', '3', '1', '4',
                       '8', '7', '1', '4', '4', '4', '1', '7', '4', '6', '9', '4',
                       '5', '6', '8', '5', '5', '8', '6', '6', '7', '1', '\0'};
    assert_true(add_separator_to_number(out34, sizeof(out34)));
    assert_string_equal(out34, "2,114,381,823,148,714,441,746,945,685,586,671");

    // len = 35
    char out35[104] = {'3', '2', '5', '9', '3', '8', '1', '2', '9', '2', '1', '5',
                       '4', '4', '7', '9', '8', '7', '3', '2', '6', '5', '4', '4',
                       '3', '8', '3', '8', '1', '5', '7', '7', '2', '6', '6', '\0'};
    assert_true(add_separator_to_number(out35, sizeof(out35)));
    assert_string_equal(out35, "32,593,812,921,544,798,732,654,438,381,577,266");

    // len = 36
    char out36[104] = {'5', '1', '9', '1', '6', '8', '3', '8', '6', '5', '3', '2', '6',
                       '7', '7', '3', '2', '1', '9', '8', '6', '5', '9', '7', '2', '6',
                       '8', '2', '6', '7', '5', '6', '5', '2', '1', '9', '\0'};
    assert_true(add_separator_to_number(out36, sizeof(out36)));
    assert_string_equal(out36, "519,168,386,532,677,321,986,597,268,267,565,219");

    // len = 37
    char out37[104] = {'3', '5', '1', '7', '8', '3', '9', '5', '4', '1', '1', '1', '7',
                       '4', '2', '8', '2', '8', '1', '8', '5', '2', '2', '5', '2', '7',
                       '6', '3', '2', '3', '2', '2', '7', '6', '5', '1', '7', '\0'};
    assert_true(add_separator_to_number(out37, sizeof(out37)));
    assert_string_equal(out37, "3,517,839,541,117,428,281,852,252,763,232,276,517");

    // len = 38
    char out38[104] = {'9', '1', '7', '1', '2', '6', '2', '7', '9', '7', '1', '3', '6',
                       '7', '9', '5', '7', '5', '3', '1', '6', '5', '3', '9', '8', '2',
                       '3', '3', '1', '5', '4', '8', '3', '8', '5', '2', '2', '1', '\0'};
    assert_true(add_separator_to_number(out38, sizeof(out38)));
    assert_string_equal(out38, "91,712,627,971,367,957,531,653,982,331,548,385,221");

    // len = 39
    char out39[104] = {'7', '9', '3', '6', '4', '5', '9', '2', '3', '5', '5', '5', '3', '3',
                       '6', '8', '3', '3', '5', '2', '2', '2', '2', '6', '3', '8', '7', '6',
                       '2', '5', '3', '1', '5', '9', '5', '6', '4', '7', '7', '\0'};
    assert_true(add_separator_to_number(out39, sizeof(out39)));
    assert_string_equal(out39, "793,645,923,555,336,833,522,226,387,625,315,956,477");

    // len = 40
    char out40[104] = {'5', '3', '8', '5', '3', '6', '6', '7', '9', '4', '8', '9', '8', '1',
                       '2', '8', '9', '2', '3', '2', '4', '9', '9', '7', '5', '1', '6', '2',
                       '4', '7', '9', '8', '5', '8', '6', '1', '2', '7', '9', '6', '\0'};
    assert_true(add_separator_to_number(out40, sizeof(out40)));
    assert_string_equal(out40, "5,385,366,794,898,128,923,249,975,162,479,858,612,796");

    // len = 41
    char out41[104] = {'1', '5', '7', '9', '3', '7', '2', '2', '4', '7', '1', '3', '8', '7',
                       '3', '4', '3', '6', '8', '1', '2', '5', '6', '2', '4', '5', '1', '5',
                       '3', '9', '7', '3', '8', '3', '2', '7', '2', '5', '4', '7', '2', '\0'};
    assert_true(add_separator_to_number(out41, sizeof(out41)));
    assert_string_equal(out41, "15,793,722,471,387,343,681,256,245,153,973,832,725,472");

    // len = 42
    char out42[104] = {'2', '6', '8', '4', '7', '4', '9', '6', '6', '4', '9', '1', '6', '1', '2',
                       '9', '8', '8', '4', '2', '2', '7', '6', '7', '9', '3', '5', '4', '8', '1',
                       '5', '7', '3', '9', '7', '4', '5', '7', '4', '9', '3', '4', '\0'};
    assert_true(add_separator_to_number(out42, sizeof(out42)));
    assert_string_equal(out42, "268,474,966,491,612,988,422,767,935,481,573,974,574,934");

    // len = 43
    char out43[104] = {'7', '2', '3', '4', '2', '7', '1', '6', '8', '5', '4', '7', '1', '1', '9',
                       '9', '1', '8', '2', '4', '9', '7', '4', '2', '1', '6', '6', '9', '8', '4',
                       '1', '4', '9', '4', '4', '6', '3', '6', '4', '8', '4', '7', '8', '\0'};
    assert_true(add_separator_to_number(out43, sizeof(out43)));
    assert_string_equal(out43, "7,234,271,685,471,199,182,497,421,669,841,494,463,648,478");

    // len = 44
    char out44[104] = {'3', '9', '1', '9', '2', '6', '4', '3', '7', '2', '5', '3', '9', '6', '6',
                       '7', '7', '5', '7', '7', '6', '7', '8', '1', '8', '5', '3', '8', '6', '5',
                       '2', '7', '3', '4', '2', '8', '5', '4', '7', '9', '5', '3', '3', '1', '\0'};
    assert_true(add_separator_to_number(out44, sizeof(out44)));
    assert_string_equal(out44, "39,192,643,725,396,677,577,678,185,386,527,342,854,795,331");

    // len = 45
    char out45[104] = {'3', '3', '9', '7', '3', '5', '9', '7', '2', '3', '7', '2',
                       '1', '4', '2', '7', '5', '2', '1', '7', '2', '8', '4', '2',
                       '6', '2', '5', '7', '9', '1', '8', '9', '9', '8', '5', '3',
                       '8', '3', '1', '3', '4', '4', '6', '1', '1', '\0'};
    assert_true(add_separator_to_number(out45, sizeof(out45)));
    assert_string_equal(out45, "339,735,972,372,142,752,172,842,625,791,899,853,831,344,611");

    // len = 46
    char out46[104] = {'4', '7', '7', '9', '4', '1', '2', '1', '4', '7', '9', '3',
                       '4', '6', '8', '5', '9', '6', '3', '7', '8', '6', '1', '2',
                       '6', '6', '9', '4', '3', '5', '3', '9', '2', '6', '8', '6',
                       '9', '3', '2', '4', '3', '7', '4', '5', '9', '6', '\0'};
    assert_true(add_separator_to_number(out46, sizeof(out46)));
    assert_string_equal(out46, "4,779,412,147,934,685,963,786,126,694,353,926,869,324,374,596");

    // len = 47
    char out47[104] = {'2', '1', '8', '1', '4', '8', '9', '1', '7', '4', '1', '6',
                       '4', '6', '2', '7', '3', '5', '1', '5', '9', '4', '1', '4',
                       '7', '1', '9', '6', '7', '4', '7', '5', '1', '6', '6', '5',
                       '6', '1', '7', '1', '4', '7', '8', '6', '2', '6', '2', '\0'};
    assert_true(add_separator_to_number(out47, sizeof(out47)));
    assert_string_equal(out47, "21,814,891,741,646,273,515,941,471,967,475,166,561,714,786,262");

    // len = 48
    char out48[104] = {'6', '9', '1', '8', '3', '7', '6', '3', '7', '2', '7', '6', '4',
                       '2', '4', '4', '4', '1', '1', '5', '6', '9', '7', '3', '7', '2',
                       '5', '1', '7', '9', '1', '6', '4', '1', '4', '2', '7', '1', '7',
                       '1', '5', '2', '5', '6', '8', '6', '2', '7', '\0'};
    assert_true(add_separator_to_number(out48, sizeof(out48)));
    assert_string_equal(out48, "691,837,637,276,424,441,156,973,725,179,164,142,717,152,568,627");

    // len = 49
    char out49[104] = {'9', '9', '2', '7', '3', '3', '8', '2', '6', '1', '7', '7', '7',
                       '5', '1', '3', '6', '5', '2', '8', '8', '6', '9', '3', '7', '1',
                       '4', '8', '7', '6', '4', '7', '5', '4', '4', '5', '5', '6', '5',
                       '3', '9', '8', '9', '9', '1', '2', '3', '5', '9', '\0'};
    assert_true(add_separator_to_number(out49, sizeof(out49)));
    assert_string_equal(out49, "9,927,338,261,777,513,652,886,937,148,764,754,455,653,989,912,359");

    // len = 50
    char out50[104] = {'9', '9', '9', '8', '6', '8', '5', '3', '4', '4', '1', '1', '3',
                       '9', '7', '8', '3', '2', '7', '1', '5', '6', '3', '1', '9', '8',
                       '4', '2', '6', '3', '6', '1', '3', '4', '2', '9', '9', '2', '5',
                       '8', '9', '2', '1', '1', '2', '9', '6', '7', '1', '1', '\0'};
    assert_true(add_separator_to_number(out50, sizeof(out50)));
    assert_string_equal(out50,
                        "99,986,853,441,139,783,271,563,198,426,361,342,992,589,211,296,711");

    // len = 51
    char out51[104] = {'9', '8', '5', '5', '3', '8', '4', '1', '2', '1', '6', '3', '7',
                       '9', '1', '7', '3', '5', '3', '7', '4', '3', '2', '4', '2', '3',
                       '8', '1', '3', '3', '8', '2', '6', '4', '2', '5', '2', '7', '4',
                       '3', '3', '2', '2', '2', '8', '3', '7', '8', '7', '3', '6', '\0'};
    assert_true(add_separator_to_number(out51, sizeof(out51)));
    assert_string_equal(out51,
                        "985,538,412,163,791,735,374,324,238,133,826,425,274,332,228,378,736");

    // len = 52
    char out52[104] = {'2', '1', '7', '7', '3', '2', '3', '9', '2', '9', '7', '9', '1', '5',
                       '5', '5', '9', '1', '9', '7', '1', '1', '5', '4', '5', '8', '9', '2',
                       '2', '3', '9', '4', '7', '4', '9', '8', '6', '3', '2', '2', '5', '4',
                       '5', '7', '1', '7', '4', '4', '1', '8', '6', '6', '\0'};
    assert_true(add_separator_to_number(out52, sizeof(out52)));
    assert_string_equal(out52,
                        "2,177,323,929,791,555,919,711,545,892,239,474,986,322,545,717,441,866");

    // len = 53
    char out53[104] = {'7', '6', '6', '6', '9', '6', '6', '8', '5', '3', '9', '8', '7', '3',
                       '8', '8', '6', '9', '1', '4', '5', '2', '3', '6', '6', '7', '4', '1',
                       '4', '5', '5', '2', '4', '8', '9', '2', '8', '8', '6', '8', '8', '2',
                       '1', '7', '3', '9', '6', '1', '2', '7', '8', '9', '2', '\0'};
    assert_true(add_separator_to_number(out53, sizeof(out53)));
    assert_string_equal(out53,
                        "76,669,668,539,873,886,914,523,667,414,552,489,288,688,217,396,127,892");

    // len = 54
    char out54[104] = {'9', '3', '1', '2', '8', '1', '2', '1', '5', '9', '9', '8', '7', '2',
                       '5', '6', '8', '9', '1', '1', '6', '9', '7', '9', '7', '4', '7', '6',
                       '5', '3', '9', '6', '7', '5', '6', '2', '4', '6', '5', '8', '8', '2',
                       '3', '8', '5', '1', '8', '5', '2', '9', '4', '1', '1', '3', '\0'};
    assert_true(add_separator_to_number(out54, sizeof(out54)));
    assert_string_equal(out54,
                        "931,281,215,998,725,689,116,979,747,653,967,562,465,882,385,185,294,113");

    // len = 55
    char out55[104] = {'7', '3', '2', '8', '3', '4', '2', '6', '7', '2', '4', '3', '2', '2',
                       '1', '8', '4', '6', '2', '6', '2', '5', '8', '9', '3', '4', '2', '3',
                       '1', '9', '5', '5', '5', '9', '7', '4', '7', '9', '3', '2', '6', '3',
                       '1', '8', '4', '4', '6', '4', '7', '8', '2', '7', '2', '6', '8', '\0'};
    assert_true(add_separator_to_number(out55, sizeof(out55)));
    assert_string_equal(
        out55,
        "7,328,342,672,432,218,462,625,893,423,195,559,747,932,631,844,647,827,268");

    // len = 56
    char out56[104] = {'9', '9', '4', '2', '3', '6', '2', '1', '7', '2', '3', '1', '6', '5', '4',
                       '6', '3', '1', '8', '8', '7', '4', '2', '8', '1', '1', '9', '9', '4', '1',
                       '1', '6', '7', '5', '6', '6', '2', '3', '7', '7', '4', '3', '8', '4', '2',
                       '2', '8', '7', '6', '6', '2', '2', '7', '9', '1', '6', '\0'};
    assert_true(add_separator_to_number(out56, sizeof(out56)));
    assert_string_equal(
        out56,
        "99,423,621,723,165,463,188,742,811,994,116,756,623,774,384,228,766,227,916");

    // len = 57
    char out57[104] = {'5', '6', '8', '8', '3', '9', '7', '6', '3', '3', '7', '3', '9', '5', '6',
                       '3', '9', '4', '1', '7', '8', '4', '6', '4', '4', '4', '2', '3', '1', '2',
                       '6', '5', '7', '4', '1', '2', '8', '9', '8', '2', '3', '3', '5', '5', '5',
                       '4', '1', '9', '5', '1', '9', '4', '1', '7', '6', '9', '9', '\0'};
    assert_true(add_separator_to_number(out57, sizeof(out57)));
    assert_string_equal(
        out57,
        "568,839,763,373,956,394,178,464,442,312,657,412,898,233,555,419,519,417,699");

    // len = 58
    char out58[104] = {'7', '6', '1', '8', '2', '7', '6', '3', '1', '6', '6', '3', '5', '5', '7',
                       '5', '6', '4', '5', '6', '6', '1', '3', '2', '4', '1', '2', '6', '6', '4',
                       '9', '3', '8', '4', '7', '1', '6', '8', '4', '8', '9', '2', '8', '7', '9',
                       '9', '6', '5', '3', '5', '6', '6', '4', '1', '1', '6', '8', '3', '\0'};
    assert_true(add_separator_to_number(out58, sizeof(out58)));
    assert_string_equal(
        out58,
        "7,618,276,316,635,575,645,661,324,126,649,384,716,848,928,799,653,566,411,683");

    // len = 59
    char out59[104] = {'6', '6', '1', '8', '2', '2', '5', '2', '2', '6', '4', '3', '7', '4', '6',
                       '5', '6', '8', '8', '8', '4', '2', '9', '8', '5', '1', '2', '5', '8', '4',
                       '6', '9', '3', '4', '3', '2', '7', '9', '4', '9', '5', '1', '4', '2', '7',
                       '6', '5', '6', '9', '9', '9', '4', '5', '5', '5', '9', '9', '1', '6', '\0'};
    assert_true(add_separator_to_number(out59, sizeof(out59)));
    assert_string_equal(
        out59,
        "66,182,252,264,374,656,888,429,851,258,469,343,279,495,142,765,699,945,559,916");

    // len = 60
    char out60[104] = {'3', '8', '6', '6', '9', '9', '8', '1', '7', '6', '4', '2', '2',
                       '1', '7', '5', '2', '8', '4', '3', '6', '6', '4', '3', '2', '6',
                       '4', '9', '4', '7', '8', '3', '3', '7', '1', '5', '4', '4', '2',
                       '1', '1', '8', '9', '4', '9', '6', '8', '3', '3', '1', '4', '1',
                       '3', '3', '3', '5', '3', '1', '3', '8', '\0'};
    assert_true(add_separator_to_number(out60, sizeof(out60)));
    assert_string_equal(
        out60,
        "386,699,817,642,217,528,436,643,264,947,833,715,442,118,949,683,314,133,353,138");

    // len = 61
    char out61[104] = {'8', '7', '4', '8', '8', '7', '6', '9', '7', '1', '6', '4', '8',
                       '2', '5', '1', '6', '4', '5', '8', '3', '3', '6', '6', '5', '3',
                       '1', '8', '9', '8', '6', '2', '2', '6', '9', '4', '4', '6', '1',
                       '1', '7', '8', '4', '3', '6', '1', '6', '2', '1', '4', '9', '3',
                       '1', '4', '4', '6', '9', '8', '2', '1', '2', '\0'};
    assert_true(add_separator_to_number(out61, sizeof(out61)));
    assert_string_equal(
        out61,
        "8,748,876,971,648,251,645,833,665,318,986,226,944,611,784,361,621,493,144,698,212");

    // len = 62
    char out62[104] = {'4', '6', '4', '8', '1', '9', '4', '6', '4', '1', '2', '4', '5',
                       '6', '5', '4', '2', '7', '2', '8', '1', '3', '1', '4', '6', '3',
                       '3', '7', '2', '7', '2', '6', '4', '1', '8', '9', '1', '5', '6',
                       '1', '9', '3', '8', '3', '3', '6', '6', '8', '6', '8', '6', '5',
                       '6', '6', '4', '1', '5', '3', '2', '5', '6', '4', '\0'};
    assert_true(add_separator_to_number(out62, sizeof(out62)));
    assert_string_equal(
        out62,
        "46,481,946,412,456,542,728,131,463,372,726,418,915,619,383,366,868,656,641,532,564");

    // len = 63
    char out63[104] = {'4', '6', '1', '4', '3', '8', '6', '3', '2', '3', '4', '5', '2',
                       '6', '6', '2', '4', '2', '9', '6', '4', '7', '4', '8', '3', '5',
                       '7', '6', '2', '9', '2', '7', '4', '8', '4', '1', '1', '8', '5',
                       '9', '2', '2', '3', '3', '9', '3', '8', '2', '7', '9', '1', '6',
                       '2', '1', '4', '1', '6', '1', '4', '6', '8', '2', '7', '\0'};
    assert_true(add_separator_to_number(out63, sizeof(out63)));
    assert_string_equal(
        out63,
        "461,438,632,345,266,242,964,748,357,629,274,841,185,922,339,382,791,621,416,146,827");

    // len = 64
    char out64[104] = {'7', '7', '1', '2', '8', '2', '2', '3', '3', '4', '7', '4', '8',
                       '1', '7', '8', '4', '6', '7', '3', '6', '2', '6', '4', '6', '3',
                       '4', '5', '4', '8', '6', '1', '8', '5', '7', '1', '5', '6', '2',
                       '2', '8', '9', '7', '5', '5', '8', '7', '3', '4', '6', '2', '5',
                       '7', '8', '5', '4', '6', '9', '3', '7', '9', '4', '5', '3', '\0'};
    assert_true(add_separator_to_number(out64, sizeof(out64)));
    assert_string_equal(
        out64,
        "7,712,822,334,748,178,467,362,646,345,486,185,715,622,897,558,734,625,785,469,379,453");

    // len = 65
    char out65[104] = {'8', '5', '5', '8', '1', '2', '2', '8', '1', '4', '4', '6', '3', '5',
                       '3', '3', '1', '3', '4', '9', '6', '4', '7', '6', '2', '5', '8', '9',
                       '4', '3', '2', '2', '7', '3', '7', '8', '5', '3', '6', '3', '1', '4',
                       '1', '1', '7', '2', '3', '4', '5', '9', '6', '8', '6', '8', '5', '9',
                       '5', '2', '9', '4', '6', '2', '6', '4', '5', '\0'};
    assert_true(add_separator_to_number(out65, sizeof(out65)));
    assert_string_equal(
        out65,
        "85,581,228,144,635,331,349,647,625,894,322,737,853,631,411,723,459,686,859,529,462,645");

    // len = 66
    char out66[104] = {'3', '3', '6', '5', '5', '9', '1', '5', '6', '8', '2', '6', '3', '9',
                       '6', '2', '3', '9', '9', '4', '8', '8', '2', '6', '8', '2', '7', '3',
                       '4', '3', '1', '4', '1', '8', '8', '6', '9', '1', '3', '3', '5', '6',
                       '7', '4', '1', '1', '7', '6', '4', '7', '9', '4', '8', '2', '5', '7',
                       '4', '4', '8', '8', '3', '2', '3', '8', '5', '1', '\0'};
    assert_true(add_separator_to_number(out66, sizeof(out66)));
    assert_string_equal(
        out66,
        "336,559,156,826,396,239,948,826,827,343,141,886,913,356,741,176,479,482,574,488,323,851");

    // len = 67
    char out67[104] = {'4', '6', '4', '3', '5', '6', '2', '1', '2', '5', '5', '1', '7', '1',
                       '2', '4', '4', '8', '5', '2', '8', '6', '4', '5', '1', '4', '2', '8',
                       '7', '3', '9', '4', '6', '4', '5', '5', '9', '3', '8', '7', '4', '2',
                       '4', '9', '4', '2', '1', '6', '9', '1', '1', '6', '1', '2', '7', '7',
                       '4', '4', '3', '2', '6', '5', '2', '9', '4', '6', '8', '\0'};
    assert_true(add_separator_to_number(out67, sizeof(out67)));
    assert_string_equal(out67,
                        "4,643,562,125,517,124,485,286,451,428,739,464,559,387,424,942,169,116,127,"
                        "744,326,529,468");

    // len = 68
    char out68[104] = {'1', '4', '8', '9', '3', '8', '2', '2', '2', '9', '7', '6', '9', '7',
                       '6', '4', '4', '2', '8', '6', '2', '9', '6', '2', '5', '2', '2', '2',
                       '4', '1', '5', '1', '7', '3', '6', '3', '2', '4', '3', '2', '3', '5',
                       '6', '7', '8', '5', '4', '9', '4', '4', '8', '3', '3', '9', '4', '7',
                       '1', '4', '2', '9', '3', '3', '3', '7', '7', '4', '2', '7', '\0'};
    assert_true(add_separator_to_number(out68, sizeof(out68)));
    assert_string_equal(out68,
                        "14,893,822,297,697,644,286,296,252,224,151,736,324,323,567,854,944,833,"
                        "947,142,933,377,427");

    // len = 69
    char out69[104] = {'3', '3', '4', '1', '2', '7', '9', '4', '2', '2', '2', '7', '1', '6',
                       '5', '1', '5', '1', '5', '1', '2', '1', '4', '2', '7', '9', '7', '6',
                       '4', '5', '7', '9', '5', '4', '4', '8', '8', '7', '3', '2', '3', '3',
                       '5', '5', '8', '2', '2', '7', '4', '6', '9', '8', '2', '1', '8', '4',
                       '2', '1', '8', '9', '4', '6', '8', '6', '3', '6', '9', '1', '3', '\0'};
    assert_true(add_separator_to_number(out69, sizeof(out69)));
    assert_string_equal(out69,
                        "334,127,942,227,165,151,512,142,797,645,795,448,873,233,558,227,469,821,"
                        "842,189,468,636,913");

    // len = 70
    char out70[104] = {'2', '2', '5', '5', '3', '1', '3', '3', '3', '3', '5', '7', '4', '2', '6',
                       '2', '7', '9', '8', '3', '6', '5', '5', '6', '8', '8', '2', '1', '8', '3',
                       '7', '5', '8', '3', '1', '4', '8', '8', '3', '2', '5', '7', '9', '4', '9',
                       '5', '8', '5', '1', '4', '2', '6', '7', '9', '7', '1', '7', '9', '8', '1',
                       '8', '2', '5', '6', '1', '1', '3', '8', '6', '9', '\0'};
    assert_true(add_separator_to_number(out70, sizeof(out70)));
    assert_string_equal(out70,
                        "2,255,313,333,574,262,798,365,568,821,837,583,148,832,579,495,851,426,797,"
                        "179,818,256,113,869");

    // len = 71
    char out71[104] = {'1', '8', '9', '8', '2', '2', '2', '4', '4', '9', '4', '8', '4', '7', '1',
                       '4', '6', '7', '3', '8', '4', '7', '3', '6', '4', '6', '3', '5', '9', '4',
                       '7', '4', '1', '2', '4', '8', '1', '9', '9', '7', '7', '4', '3', '7', '3',
                       '3', '1', '6', '3', '7', '2', '3', '4', '4', '9', '9', '8', '1', '1', '1',
                       '6', '4', '6', '6', '2', '7', '7', '5', '2', '1', '3', '\0'};
    assert_true(add_separator_to_number(out71, sizeof(out71)));
    assert_string_equal(out71,
                        "18,982,224,494,847,146,738,473,646,359,474,124,819,977,437,331,637,234,"
                        "499,811,164,662,775,213");

    // len = 72
    char out72[104] = {'8', '7', '5', '4', '5', '3', '3', '1', '1', '4', '6', '6', '1', '6', '5',
                       '1', '8', '7', '6', '1', '5', '8', '5', '1', '5', '5', '8', '2', '7', '5',
                       '5', '8', '5', '9', '2', '9', '1', '5', '6', '4', '7', '7', '8', '4', '2',
                       '1', '4', '4', '3', '6', '4', '5', '5', '5', '8', '5', '8', '7', '5', '7',
                       '2', '4', '7', '5', '1', '2', '9', '6', '6', '1', '9', '2', '\0'};
    assert_true(add_separator_to_number(out72, sizeof(out72)));
    assert_string_equal(out72,
                        "875,453,311,466,165,187,615,851,558,275,585,929,156,477,842,144,364,555,"
                        "858,757,247,512,966,192");

    // len = 73
    char out73[104] = {'7', '8', '7', '5', '2', '2', '1', '5', '6', '8', '1', '5', '4', '9', '1',
                       '1', '6', '5', '4', '9', '8', '2', '3', '4', '9', '6', '1', '1', '9', '9',
                       '6', '3', '5', '8', '3', '3', '5', '1', '3', '4', '5', '5', '5', '4', '7',
                       '7', '9', '5', '1', '3', '2', '2', '3', '5', '1', '1', '6', '5', '3', '9',
                       '8', '4', '9', '1', '3', '5', '2', '1', '1', '4', '7', '2', '4', '\0'};
    assert_true(add_separator_to_number(out73, sizeof(out73)));
    assert_string_equal(out73,
                        "7,875,221,568,154,911,654,982,349,611,996,358,335,134,555,477,951,322,351,"
                        "165,398,491,352,114,724");

    // len = 74
    char out74[104] = {'2', '4', '9', '8', '6', '9', '8', '5', '8', '1', '9', '6', '7', '3', '4',
                       '6', '4', '5', '1', '5', '2', '2', '8', '7', '8', '9', '1', '8', '5', '9',
                       '7', '7', '2', '2', '7', '6', '5', '9', '4', '5', '8', '1', '1', '3', '4',
                       '1', '1', '5', '5', '1', '3', '8', '3', '8', '5', '9', '1', '6', '3', '7',
                       '6', '7', '3', '8', '5', '4', '4', '2', '6', '6', '6', '7', '3', '5', '\0'};
    assert_true(add_separator_to_number(out74, sizeof(out74)));
    assert_string_equal(out74,
                        "24,986,985,819,673,464,515,228,789,185,977,227,659,458,113,411,551,383,"
                        "859,163,767,385,442,666,735");

    // len = 75
    char out75[104] = {'4', '6', '5', '9', '1', '1', '4', '6', '1', '6', '4', '9', '9',
                       '7', '8', '3', '1', '1', '8', '2', '3', '6', '7', '6', '3', '2',
                       '7', '8', '8', '8', '9', '1', '1', '7', '7', '6', '4', '7', '3',
                       '7', '8', '5', '7', '1', '7', '9', '5', '4', '8', '3', '4', '1',
                       '9', '3', '4', '1', '3', '5', '4', '5', '5', '7', '8', '1', '4',
                       '7', '1', '8', '2', '8', '7', '6', '5', '9', '6', '\0'};
    assert_true(add_separator_to_number(out75, sizeof(out75)));
    assert_string_equal(out75,
                        "465,911,461,649,978,311,823,676,327,888,911,776,473,785,717,954,834,193,"
                        "413,545,578,147,182,876,596");

    // len = 76
    char out76[104] = {'5', '1', '2', '1', '5', '9', '3', '8', '9', '8', '4', '5', '3',
                       '7', '2', '6', '6', '8', '4', '9', '7', '2', '1', '2', '3', '4',
                       '2', '7', '4', '7', '1', '4', '2', '9', '5', '5', '4', '8', '9',
                       '8', '1', '9', '6', '8', '8', '5', '6', '2', '5', '5', '1', '8',
                       '2', '5', '9', '7', '6', '6', '9', '7', '1', '1', '9', '9', '1',
                       '2', '2', '1', '4', '8', '5', '9', '9', '2', '5', '4', '\0'};
    assert_true(add_separator_to_number(out76, sizeof(out76)));
    assert_string_equal(out76,
                        "5,121,593,898,453,726,684,972,123,427,471,429,554,898,196,885,625,518,259,"
                        "766,971,199,122,148,599,254");

    // len = 77
    char out77[104] = {'7', '2', '7', '9', '7', '5', '6', '8', '6', '8', '1', '8', '2',
                       '5', '4', '4', '5', '7', '9', '7', '9', '2', '2', '1', '5', '8',
                       '9', '4', '8', '1', '4', '8', '6', '4', '2', '5', '1', '8', '7',
                       '7', '2', '5', '2', '7', '4', '9', '4', '8', '6', '3', '3', '9',
                       '7', '1', '2', '9', '5', '5', '1', '8', '3', '2', '2', '7', '4',
                       '6', '5', '3', '1', '5', '6', '3', '4', '8', '3', '2', '2', '\0'};
    assert_true(add_separator_to_number(out77, sizeof(out77)));
    assert_string_equal(out77,
                        "72,797,568,681,825,445,797,922,158,948,148,642,518,772,527,494,863,397,"
                        "129,551,832,274,653,156,348,322");

    // len = 78
    char out78[104] = {'1', '3', '3', '6', '3', '8', '5', '7', '7', '2', '3', '5', '5', '1',
                       '3', '8', '1', '6', '2', '5', '9', '2', '6', '1', '1', '1', '9', '2',
                       '4', '1', '2', '4', '4', '9', '6', '7', '7', '9', '6', '4', '3', '1',
                       '6', '8', '9', '1', '6', '7', '4', '9', '9', '6', '6', '8', '4', '5',
                       '2', '8', '6', '4', '2', '7', '8', '4', '4', '7', '1', '4', '2', '5',
                       '7', '6', '1', '4', '1', '1', '7', '7', '\0'};
    assert_true(add_separator_to_number(out78, sizeof(out78)));
    assert_string_equal(out78,
                        "133,638,577,235,513,816,259,261,119,241,244,967,796,431,689,167,499,668,"
                        "452,864,278,447,142,576,141,177");

    // len = 0
    char out79[104] = {'\0'};
    assert_false(add_separator_to_number(out79, sizeof(out79)));
}

void test_add_separator_to_integer_negative() {
    // len = 1
    char out1[104] = {'-', '3', '\0'};
    assert_true(add_separator_to_number(out1, sizeof(out1)));
    assert_string_equal(out1, "-3");

    // len = 2
    char out2[104] = {'-', '5', '9', '\0'};
    assert_true(add_separator_to_number(out2, sizeof(out2)));
    assert_string_equal(out2, "-59");

    // len = 3
    char out3[104] = {'-', '5', '3', '9', '\0'};
    assert_true(add_separator_to_number(out3, sizeof(out3)));
    assert_string_equal(out3, "-539");

    // len = 4
    char out4[104] = {'-', '5', '4', '2', '1', '\0'};
    assert_true(add_separator_to_number(out4, sizeof(out4)));
    assert_string_equal(out4, "-5,421");

    // len = 5
    char out5[104] = {'-', '8', '4', '5', '5', '4', '\0'};
    assert_true(add_separator_to_number(out5, sizeof(out5)));
    assert_string_equal(out5, "-84,554");

    // len = 6
    char out6[104] = {'-', '8', '9', '1', '7', '5', '2', '\0'};
    assert_true(add_separator_to_number(out6, sizeof(out6)));
    assert_string_equal(out6, "-891,752");

    // len = 7
    char out7[104] = {'-', '1', '2', '1', '4', '4', '4', '7', '\0'};
    assert_true(add_separator_to_number(out7, sizeof(out7)));
    assert_string_equal(out7, "-1,214,447");

    // len = 8
    char out8[104] = {'-', '8', '4', '3', '7', '3', '3', '4', '7', '\0'};
    assert_true(add_separator_to_number(out8, sizeof(out8)));
    assert_string_equal(out8, "-84,373,347");

    // len = 9
    char out9[104] = {'-', '9', '2', '1', '4', '1', '9', '3', '6', '6', '\0'};
    assert_true(add_separator_to_number(out9, sizeof(out9)));
    assert_string_equal(out9, "-921,419,366");

    // len = 10
    char out10[104] = {'-', '7', '7', '1', '4', '9', '2', '6', '6', '1', '7', '\0'};
    assert_true(add_separator_to_number(out10, sizeof(out10)));
    assert_string_equal(out10, "-7,714,926,617");

    // len = 11
    char out11[104] = {'-', '9', '3', '4', '3', '9', '9', '9', '1', '5', '6', '3', '\0'};
    assert_true(add_separator_to_number(out11, sizeof(out11)));
    assert_string_equal(out11, "-93,439,991,563");

    // len = 12
    char out12[104] = {'-', '8', '9', '6', '7', '9', '9', '7', '2', '3', '8', '4', '4', '\0'};
    assert_true(add_separator_to_number(out12, sizeof(out12)));
    assert_string_equal(out12, "-896,799,723,844");

    // len = 13
    char out13[104] = {'-', '4', '8', '8', '1', '2', '7', '2', '6', '8', '1', '9', '1', '8', '\0'};
    assert_true(add_separator_to_number(out13, sizeof(out13)));
    assert_string_equal(out13, "-4,881,272,681,918");

    // len = 14
    char out14[104] =
        {'-', '8', '2', '6', '2', '7', '1', '6', '4', '7', '7', '6', '8', '8', '7', '\0'};
    assert_true(add_separator_to_number(out14, sizeof(out14)));
    assert_string_equal(out14, "-82,627,164,776,887");

    // len = 15
    char out15[104] =
        {'-', '7', '3', '1', '5', '3', '9', '7', '4', '4', '1', '9', '7', '7', '2', '4', '\0'};
    assert_true(add_separator_to_number(out15, sizeof(out15)));
    assert_string_equal(out15, "-731,539,744,197,724");

    // len = 16
    char out16[104] =
        {'-', '3', '3', '7', '4', '4', '1', '7', '2', '2', '7', '1', '3', '7', '5', '4', '2', '\0'};
    assert_true(add_separator_to_number(out16, sizeof(out16)));
    assert_string_equal(out16, "-3,374,417,227,137,542");

    // len = 17
    char out17[104] = {'-',
                       '6',
                       '1',
                       '6',
                       '2',
                       '6',
                       '5',
                       '9',
                       '6',
                       '8',
                       '6',
                       '8',
                       '6',
                       '2',
                       '5',
                       '4',
                       '3',
                       '6',
                       '\0'};
    assert_true(add_separator_to_number(out17, sizeof(out17)));
    assert_string_equal(out17, "-61,626,596,868,625,436");

    // len = 18
    char out18[104] = {'-', '3', '2', '8', '5', '7', '9', '8', '3', '6',
                       '1', '7', '3', '2', '1', '1', '4', '3', '8', '\0'};
    assert_true(add_separator_to_number(out18, sizeof(out18)));
    assert_string_equal(out18, "-328,579,836,173,211,438");

    // len = 19
    char out19[104] = {'-', '4', '4', '8', '7', '7', '9', '7', '3', '7', '8',
                       '1', '3', '6', '1', '7', '7', '8', '3', '7', '\0'};
    assert_true(add_separator_to_number(out19, sizeof(out19)));
    assert_string_equal(out19, "-4,487,797,378,136,177,837");

    // len = 20
    char out20[104] = {'-', '9', '6', '3', '8', '2', '8', '4', '1', '7', '9',
                       '4', '9', '9', '5', '6', '3', '1', '2', '7', '6', '\0'};
    assert_true(add_separator_to_number(out20, sizeof(out20)));
    assert_string_equal(out20, "-96,382,841,794,995,631,276");

    // len = 21
    char out21[104] = {'-', '3', '5', '1', '7', '2', '5', '9', '1', '3', '7', '6',
                       '2', '5', '4', '6', '8', '9', '8', '4', '7', '4', '\0'};
    assert_true(add_separator_to_number(out21, sizeof(out21)));
    assert_string_equal(out21, "-351,725,913,762,546,898,474");

    // len = 22
    char out22[104] = {'-', '2', '6', '8', '5', '7', '7', '7', '8', '9', '9', '4',
                       '9', '2', '4', '3', '6', '4', '1', '7', '1', '7', '6', '\0'};
    assert_true(add_separator_to_number(out22, sizeof(out22)));
    assert_string_equal(out22, "-2,685,777,899,492,436,417,176");

    // len = 23
    char out23[104] = {'-', '4', '6', '7', '6', '2', '1', '9', '6', '7', '1', '1', '5',
                       '3', '4', '3', '8', '8', '1', '7', '1', '3', '1', '2', '\0'};
    assert_true(add_separator_to_number(out23, sizeof(out23)));
    assert_string_equal(out23, "-46,762,196,711,534,388,171,312");

    // len = 24
    char out24[104] = {'-', '9', '9', '1', '9', '4', '7', '7', '6', '2', '1', '1', '8',
                       '9', '9', '5', '9', '2', '7', '6', '3', '5', '8', '3', '9', '\0'};
    assert_true(add_separator_to_number(out24, sizeof(out24)));
    assert_string_equal(out24, "-991,947,762,118,995,927,635,839");

    // len = 25
    char out25[104] = {'-', '8', '4', '5', '8', '5', '6', '2', '2', '6', '8', '9', '1', '6',
                       '8', '7', '2', '2', '4', '9', '9', '4', '8', '1', '8', '2', '\0'};
    assert_true(add_separator_to_number(out25, sizeof(out25)));
    assert_string_equal(out25, "-8,458,562,268,916,872,249,948,182");

    // len = 26
    char out26[104] = {'-', '6', '2', '9', '5', '2', '3', '8', '5', '1', '3', '7', '7', '4',
                       '7', '9', '1', '2', '4', '6', '5', '1', '6', '9', '7', '2', '9', '\0'};
    assert_true(add_separator_to_number(out26, sizeof(out26)));
    assert_string_equal(out26, "-62,952,385,137,747,912,465,169,729");

    // len = 27
    char out27[104] = {'-', '8', '9', '8', '1', '1', '4', '1', '9', '8', '8', '9', '3', '6', '9',
                       '9', '1', '3', '9', '6', '7', '5', '6', '2', '3', '8', '2', '5', '\0'};
    assert_true(add_separator_to_number(out27, sizeof(out27)));
    assert_string_equal(out27, "-898,114,198,893,699,139,675,623,825");

    // len = 28
    char out28[104] = {'-', '4', '8', '9', '8', '4', '5', '8', '4', '9', '8', '2', '4', '7', '2',
                       '4', '5', '4', '2', '9', '2', '6', '3', '4', '1', '2', '8', '2', '7', '\0'};
    assert_true(add_separator_to_number(out28, sizeof(out28)));
    assert_string_equal(out28, "-4,898,458,498,247,245,429,263,412,827");

    // len = 29
    char out29[104] = {'-', '9', '6', '1', '8', '4', '8', '8', '4', '9', '5',
                       '1', '3', '5', '3', '1', '9', '3', '4', '1', '9', '5',
                       '8', '4', '8', '7', '6', '5', '3', '5', '\0'};
    assert_true(add_separator_to_number(out29, sizeof(out29)));
    assert_string_equal(out29, "-96,184,884,951,353,193,419,584,876,535");

    // len = 30
    char out30[104] = {'-', '7', '8', '1', '1', '1', '7', '5', '8', '1', '7',
                       '5', '2', '4', '1', '2', '3', '4', '5', '4', '2', '8',
                       '8', '1', '2', '7', '7', '7', '2', '5', '4', '\0'};
    assert_true(add_separator_to_number(out30, sizeof(out30)));
    assert_string_equal(out30, "-781,117,581,752,412,345,428,812,777,254");

    // len = 31
    char out31[104] = {'-', '2', '8', '6', '3', '1', '7', '1', '8', '8', '5',
                       '4', '5', '6', '4', '5', '7', '7', '2', '7', '9', '7',
                       '4', '4', '1', '8', '2', '6', '3', '1', '8', '5', '\0'};
    assert_true(add_separator_to_number(out31, sizeof(out31)));
    assert_string_equal(out31, "-2,863,171,885,456,457,727,974,418,263,185");

    // len = 32
    char out32[104] = {'-', '7', '7', '4', '5', '4', '1', '7', '8', '5', '1', '5',
                       '8', '1', '8', '9', '4', '1', '2', '7', '5', '6', '9', '6',
                       '2', '3', '7', '2', '9', '3', '4', '3', '2', '\0'};
    assert_true(add_separator_to_number(out32, sizeof(out32)));
    assert_string_equal(out32, "-77,454,178,515,818,941,275,696,237,293,432");

    // len = 33
    char out33[104] = {'-', '8', '2', '3', '3', '4', '9', '2', '6', '6', '3', '8',
                       '2', '8', '8', '4', '6', '3', '7', '3', '7', '3', '3', '7',
                       '2', '6', '4', '7', '5', '9', '5', '8', '7', '5', '\0'};
    assert_true(add_separator_to_number(out33, sizeof(out33)));
    assert_string_equal(out33, "-823,349,266,382,884,637,373,372,647,595,875");

    // len = 34
    char out34[104] = {'-', '3', '4', '4', '7', '4', '8', '6', '5', '8', '2', '8',
                       '5', '7', '1', '1', '3', '1', '5', '5', '3', '4', '8', '2',
                       '1', '4', '3', '5', '4', '6', '1', '1', '4', '9', '9', '\0'};
    assert_true(add_separator_to_number(out34, sizeof(out34)));
    assert_string_equal(out34, "-3,447,486,582,857,113,155,348,214,354,611,499");

    // len = 35
    char out35[104] = {'-', '8', '3', '4', '1', '7', '1', '7', '5', '4', '3', '4', '2',
                       '1', '7', '8', '9', '3', '5', '6', '9', '1', '7', '1', '2', '1',
                       '6', '6', '3', '9', '6', '6', '4', '2', '1', '2', '\0'};
    assert_true(add_separator_to_number(out35, sizeof(out35)));
    assert_string_equal(out35, "-83,417,175,434,217,893,569,171,216,639,664,212");

    // len = 36
    char out36[104] = {'-', '2', '7', '8', '8', '9', '2', '3', '1', '4', '9', '2', '2',
                       '7', '2', '3', '2', '5', '9', '1', '5', '8', '6', '6', '8', '9',
                       '2', '5', '5', '2', '9', '1', '4', '8', '8', '2', '9', '\0'};
    assert_true(add_separator_to_number(out36, sizeof(out36)));
    assert_string_equal(out36, "-278,892,314,922,723,259,158,668,925,529,148,829");

    // len = 37
    char out37[104] = {'-', '7', '6', '2', '1', '5', '6', '7', '5', '4', '4', '1', '6',
                       '6', '7', '2', '2', '4', '1', '6', '3', '9', '5', '5', '2', '4',
                       '3', '3', '2', '3', '1', '3', '2', '6', '4', '6', '7', '4', '\0'};
    assert_true(add_separator_to_number(out37, sizeof(out37)));
    assert_string_equal(out37, "-7,621,567,544,166,722,416,395,524,332,313,264,674");

    // len = 38
    char out38[104] = {'-', '5', '8', '5', '2', '7', '9', '1', '6', '4', '6', '3', '1', '3',
                       '8', '8', '2', '7', '8', '8', '1', '2', '6', '1', '7', '4', '5', '9',
                       '3', '9', '6', '8', '6', '2', '2', '1', '4', '6', '3', '\0'};
    assert_true(add_separator_to_number(out38, sizeof(out38)));
    assert_string_equal(out38, "-58,527,916,463,138,827,881,261,745,939,686,221,463");

    // len = 39
    char out39[104] = {'-', '7', '3', '6', '9', '3', '8', '3', '2', '3', '1', '8', '8', '2',
                       '5', '2', '4', '1', '7', '8', '3', '6', '5', '3', '3', '1', '7', '1',
                       '6', '7', '9', '4', '4', '8', '4', '5', '5', '5', '4', '1', '\0'};
    assert_true(add_separator_to_number(out39, sizeof(out39)));
    assert_string_equal(out39, "-736,938,323,188,252,417,836,533,171,679,448,455,541");

    // len = 40
    char out40[104] = {'-', '1', '1', '9', '3', '3', '5', '6', '5', '5', '2', '6', '4', '4',
                       '3', '5', '5', '9', '2', '8', '3', '8', '2', '6', '6', '3', '7', '7',
                       '5', '2', '1', '8', '5', '2', '4', '9', '3', '7', '6', '2', '3', '\0'};
    assert_true(add_separator_to_number(out40, sizeof(out40)));
    assert_string_equal(out40, "-1,193,356,552,644,355,928,382,663,775,218,524,937,623");

    // len = 41
    char out41[104] = {'-', '2', '3', '6', '6', '2', '2', '1', '4', '8', '7', '9', '5', '4', '1',
                       '5', '5', '8', '2', '7', '8', '9', '9', '5', '2', '9', '4', '2', '4', '6',
                       '2', '3', '2', '8', '3', '5', '3', '7', '9', '3', '2', '5', '\0'};
    assert_true(add_separator_to_number(out41, sizeof(out41)));
    assert_string_equal(out41, "-23,662,214,879,541,558,278,995,294,246,232,835,379,325");

    // len = 42
    char out42[104] = {'-', '3', '2', '7', '5', '8', '3', '8', '7', '4', '3', '4', '9', '1', '6',
                       '2', '8', '1', '6', '1', '4', '1', '1', '9', '2', '7', '2', '5', '1', '5',
                       '4', '3', '2', '2', '3', '6', '6', '4', '3', '6', '8', '4', '3', '\0'};
    assert_true(add_separator_to_number(out42, sizeof(out42)));
    assert_string_equal(out42, "-327,583,874,349,162,816,141,192,725,154,322,366,436,843");

    // len = 43
    char out43[104] = {'-', '3', '5', '7', '7', '6', '3', '2', '7', '5', '4', '4', '8', '7', '7',
                       '3', '9', '5', '9', '9', '2', '7', '5', '6', '7', '7', '3', '1', '6', '3',
                       '4', '1', '2', '4', '1', '4', '5', '2', '7', '7', '7', '1', '9', '8', '\0'};
    assert_true(add_separator_to_number(out43, sizeof(out43)));
    assert_string_equal(out43, "-3,577,632,754,487,739,599,275,677,316,341,241,452,777,198");

    // len = 44
    char out44[104] = {'-', '6', '2', '3', '3', '5', '1', '3', '9', '9', '5', '8',
                       '7', '7', '7', '5', '5', '8', '8', '9', '8', '5', '2', '5',
                       '8', '8', '7', '7', '8', '2', '3', '3', '6', '7', '3', '8',
                       '9', '8', '1', '1', '7', '4', '6', '2', '4', '\0'};
    assert_true(add_separator_to_number(out44, sizeof(out44)));
    assert_string_equal(out44, "-62,335,139,958,777,558,898,525,887,782,336,738,981,174,624");

    // len = 45
    char out45[104] = {'-', '6', '4', '9', '4', '3', '9', '7', '5', '4', '3', '5',
                       '1', '2', '4', '3', '7', '6', '5', '6', '3', '7', '5', '2',
                       '4', '1', '6', '3', '2', '9', '7', '3', '4', '5', '9', '5',
                       '9', '7', '2', '4', '1', '5', '1', '8', '1', '9', '\0'};
    assert_true(add_separator_to_number(out45, sizeof(out45)));
    assert_string_equal(out45, "-649,439,754,351,243,765,637,524,163,297,345,959,724,151,819");

    // len = 46
    char out46[104] = {'-', '6', '1', '2', '9', '7', '1', '5', '5', '7', '2', '8',
                       '6', '5', '4', '4', '2', '8', '5', '8', '1', '6', '3', '5',
                       '2', '8', '7', '8', '2', '4', '4', '5', '1', '6', '1', '1',
                       '4', '7', '3', '2', '6', '4', '4', '5', '7', '6', '5', '\0'};
    assert_true(add_separator_to_number(out46, sizeof(out46)));
    assert_string_equal(out46, "-6,129,715,572,865,442,858,163,528,782,445,161,147,326,445,765");

    // len = 47
    char out47[104] = {'-', '2', '2', '5', '2', '2', '5', '2', '3', '2', '8', '6', '4',
                       '7', '3', '3', '2', '5', '9', '5', '1', '5', '4', '5', '9', '5',
                       '8', '1', '6', '5', '9', '5', '6', '6', '8', '6', '3', '5', '7',
                       '9', '8', '5', '1', '4', '8', '7', '4', '9', '\0'};
    assert_true(add_separator_to_number(out47, sizeof(out47)));
    assert_string_equal(out47, "-22,522,523,286,473,325,951,545,958,165,956,686,357,985,148,749");

    // len = 48
    char out48[104] = {'-', '2', '5', '8', '5', '5', '6', '9', '4', '4', '4', '9', '2',
                       '4', '8', '3', '3', '1', '6', '5', '8', '6', '1', '2', '8', '8',
                       '2', '2', '5', '1', '9', '1', '2', '4', '6', '8', '6', '5', '5',
                       '5', '6', '1', '6', '4', '2', '4', '8', '1', '1', '\0'};
    assert_true(add_separator_to_number(out48, sizeof(out48)));
    assert_string_equal(out48, "-258,556,944,492,483,316,586,128,822,519,124,686,555,616,424,811");

    // len = 49
    char out49[104] = {'-', '4', '1', '5', '7', '2', '7', '3', '3', '6', '8', '3', '9',
                       '9', '4', '1', '8', '6', '7', '6', '5', '1', '7', '9', '6', '7',
                       '8', '3', '7', '3', '1', '1', '3', '5', '9', '8', '3', '9', '5',
                       '8', '8', '8', '9', '4', '4', '7', '7', '4', '4', '5', '\0'};
    assert_true(add_separator_to_number(out49, sizeof(out49)));
    assert_string_equal(out49,
                        "-4,157,273,368,399,418,676,517,967,837,311,359,839,588,894,477,445");

    // len = 50
    char out50[104] = {'-', '1', '6', '1', '9', '9', '5', '7', '8', '6', '5', '7', '5',
                       '3', '3', '4', '4', '4', '4', '2', '6', '1', '3', '4', '4', '5',
                       '1', '3', '1', '7', '8', '9', '5', '4', '6', '2', '9', '5', '1',
                       '3', '9', '3', '4', '9', '6', '6', '3', '2', '7', '7', '4', '\0'};
    assert_true(add_separator_to_number(out50, sizeof(out50)));
    assert_string_equal(out50,
                        "-16,199,578,657,533,444,426,134,451,317,895,462,951,393,496,632,774");

    // len = 51
    char out51[104] = {'-', '3', '4', '5', '1', '2', '8', '8', '8', '7', '1', '6', '7', '2',
                       '4', '9', '9', '9', '5', '4', '7', '2', '2', '4', '6', '6', '9', '5',
                       '8', '5', '6', '1', '4', '3', '1', '7', '6', '5', '7', '1', '3', '3',
                       '2', '2', '6', '7', '9', '6', '5', '5', '9', '2', '\0'};
    assert_true(add_separator_to_number(out51, sizeof(out51)));
    assert_string_equal(out51,
                        "-345,128,887,167,249,995,472,246,695,856,143,176,571,332,267,965,592");

    // len = 52
    char out52[104] = {'-', '4', '7', '4', '9', '3', '3', '6', '2', '5', '4', '2', '1', '2',
                       '1', '5', '3', '7', '1', '2', '6', '5', '1', '7', '5', '2', '6', '9',
                       '8', '9', '1', '5', '9', '5', '7', '5', '6', '4', '9', '9', '3', '6',
                       '6', '7', '5', '6', '5', '1', '9', '8', '9', '5', '7', '\0'};
    assert_true(add_separator_to_number(out52, sizeof(out52)));
    assert_string_equal(out52,
                        "-4,749,336,254,212,153,712,651,752,698,915,957,564,993,667,565,198,957");

    // len = 53
    char out53[104] = {'-', '5', '6', '8', '2', '9', '7', '7', '2', '1', '1', '8', '6', '6',
                       '9', '8', '7', '2', '4', '3', '8', '2', '6', '5', '6', '2', '4', '5',
                       '2', '1', '4', '3', '1', '1', '7', '7', '9', '6', '5', '5', '9', '2',
                       '3', '5', '5', '1', '4', '9', '2', '1', '6', '3', '4', '8', '\0'};
    assert_true(add_separator_to_number(out53, sizeof(out53)));
    assert_string_equal(out53,
                        "-56,829,772,118,669,872,438,265,624,521,431,177,965,592,355,149,216,348");

    // len = 54
    char out54[104] = {'-', '9', '3', '7', '4', '7', '9', '1', '8', '1', '8', '7', '2', '4',
                       '8', '6', '3', '3', '1', '9', '7', '9', '5', '3', '8', '1', '8', '5',
                       '2', '3', '7', '8', '2', '7', '9', '1', '6', '3', '3', '6', '6', '8',
                       '9', '2', '1', '7', '7', '4', '6', '7', '7', '2', '1', '9', '1', '\0'};
    assert_true(add_separator_to_number(out54, sizeof(out54)));
    assert_string_equal(out54,
                        "-937,479,181,872,486,331,979,538,185,237,827,916,336,689,217,746,772,191");

    // len = 55
    char out55[104] = {'-', '7', '4', '7', '7', '3', '5', '4', '9', '2', '2', '2', '6', '3', '9',
                       '6', '8', '1', '8', '7', '9', '1', '1', '7', '3', '6', '8', '3', '5', '9',
                       '4', '6', '9', '8', '9', '4', '1', '3', '1', '1', '9', '5', '9', '2', '9',
                       '4', '9', '6', '2', '1', '7', '8', '6', '1', '3', '1', '\0'};
    assert_true(add_separator_to_number(out55, sizeof(out55)));
    assert_string_equal(
        out55,
        "-7,477,354,922,263,968,187,911,736,835,946,989,413,119,592,949,621,786,131");

    // len = 56
    char out56[104] = {'-', '5', '2', '7', '3', '9', '9', '6', '4', '4', '3', '4', '5', '8', '4',
                       '8', '1', '7', '6', '7', '4', '2', '9', '2', '5', '7', '3', '2', '4', '9',
                       '1', '3', '3', '5', '7', '8', '6', '1', '4', '1', '5', '1', '3', '6', '9',
                       '8', '5', '6', '2', '6', '2', '9', '2', '7', '8', '4', '2', '\0'};
    assert_true(add_separator_to_number(out56, sizeof(out56)));
    assert_string_equal(
        out56,
        "-52,739,964,434,584,817,674,292,573,249,133,578,614,151,369,856,262,927,842");

    // len = 57
    char out57[104] = {'-', '2', '5', '8', '3', '4', '9', '5', '9', '1', '5', '8', '3', '9', '2',
                       '9', '1', '7', '2', '1', '9', '4', '7', '1', '8', '3', '5', '2', '4', '8',
                       '5', '2', '8', '6', '9', '1', '1', '2', '1', '8', '4', '3', '6', '3', '8',
                       '6', '6', '9', '8', '7', '7', '9', '8', '1', '8', '4', '9', '4', '\0'};
    assert_true(add_separator_to_number(out57, sizeof(out57)));
    assert_string_equal(
        out57,
        "-258,349,591,583,929,172,194,718,352,485,286,911,218,436,386,698,779,818,494");

    // len = 58
    char out58[104] = {'-', '8', '7', '5', '9', '6', '8', '2', '6', '4', '3', '5', '1', '8', '2',
                       '4', '4', '9', '3', '5', '9', '2', '5', '2', '2', '9', '7', '9', '1', '5',
                       '2', '5', '2', '8', '4', '3', '2', '5', '9', '5', '1', '7', '9', '8', '1',
                       '7', '6', '8', '9', '2', '3', '5', '3', '9', '9', '9', '5', '8', '1', '\0'};
    assert_true(add_separator_to_number(out58, sizeof(out58)));
    assert_string_equal(
        out58,
        "-8,759,682,643,518,244,935,925,229,791,525,284,325,951,798,176,892,353,999,581");

    // len = 59
    char out59[104] = {'-', '7', '9', '7', '7', '4', '6', '2', '3', '4', '4', '1', '1',
                       '4', '6', '4', '7', '5', '6', '1', '3', '5', '6', '1', '2', '9',
                       '2', '4', '9', '5', '8', '6', '4', '1', '2', '5', '2', '3', '9',
                       '2', '9', '7', '2', '6', '2', '6', '8', '8', '7', '9', '5', '5',
                       '4', '4', '3', '8', '7', '8', '9', '3', '\0'};
    assert_true(add_separator_to_number(out59, sizeof(out59)));
    assert_string_equal(
        out59,
        "-79,774,623,441,146,475,613,561,292,495,864,125,239,297,262,688,795,544,387,893");

    // len = 60
    char out60[104] = {'-', '3', '1', '4', '1', '9', '5', '5', '4', '2', '7', '4', '5',
                       '9', '5', '4', '2', '5', '3', '8', '6', '3', '6', '3', '7', '6',
                       '9', '4', '2', '1', '6', '4', '7', '8', '9', '7', '4', '9', '3',
                       '6', '5', '8', '9', '7', '6', '5', '3', '6', '3', '7', '1', '3',
                       '7', '9', '3', '6', '5', '2', '4', '1', '2', '\0'};
    assert_true(add_separator_to_number(out60, sizeof(out60)));
    assert_string_equal(
        out60,
        "-314,195,542,745,954,253,863,637,694,216,478,974,936,589,765,363,713,793,652,412");

    // len = 61
    char out61[104] = {'-', '1', '5', '4', '3', '5', '7', '3', '4', '8', '8', '9', '5',
                       '1', '5', '1', '8', '4', '4', '1', '3', '2', '2', '4', '2', '2',
                       '8', '1', '7', '2', '6', '8', '2', '7', '2', '6', '9', '6', '5',
                       '1', '6', '7', '1', '9', '2', '2', '9', '4', '5', '7', '3', '3',
                       '1', '5', '9', '6', '2', '7', '8', '6', '6', '8', '\0'};
    assert_true(add_separator_to_number(out61, sizeof(out61)));
    assert_string_equal(
        out61,
        "-1,543,573,488,951,518,441,322,422,817,268,272,696,516,719,229,457,331,596,278,668");

    // len = 62
    char out62[104] = {'-', '8', '7', '7', '4', '7', '8', '3', '3', '5', '1', '9', '4',
                       '2', '8', '1', '3', '4', '7', '4', '6', '8', '8', '5', '1', '9',
                       '7', '9', '2', '2', '1', '3', '1', '9', '6', '6', '1', '3', '1',
                       '2', '4', '4', '4', '7', '1', '2', '2', '9', '7', '2', '9', '7',
                       '7', '5', '2', '1', '3', '4', '2', '7', '6', '4', '6', '\0'};
    assert_true(add_separator_to_number(out62, sizeof(out62)));
    assert_string_equal(
        out62,
        "-87,747,833,519,428,134,746,885,197,922,131,966,131,244,471,229,729,775,213,427,646");

    // len = 63
    char out63[104] = {'-', '6', '1', '8', '3', '3', '4', '3', '2', '5', '4', '2', '2',
                       '9', '5', '5', '2', '7', '8', '3', '7', '5', '2', '3', '2', '2',
                       '5', '8', '1', '1', '5', '4', '6', '5', '4', '5', '6', '3', '2',
                       '1', '5', '6', '4', '7', '8', '8', '7', '8', '1', '1', '2', '7',
                       '2', '5', '2', '8', '8', '6', '4', '5', '5', '1', '1', '1', '\0'};
    assert_true(add_separator_to_number(out63, sizeof(out63)));
    assert_string_equal(
        out63,
        "-618,334,325,422,955,278,375,232,258,115,465,456,321,564,788,781,127,252,886,455,111");

    // len = 64
    char out64[104] = {'-', '4', '4', '3', '9', '1', '1', '6', '2', '3', '9', '7', '5', '2',
                       '3', '5', '4', '9', '5', '7', '2', '2', '7', '1', '3', '5', '9', '8',
                       '8', '9', '2', '5', '3', '5', '3', '9', '1', '4', '9', '6', '7', '5',
                       '1', '9', '9', '1', '7', '1', '7', '8', '6', '3', '2', '7', '6', '6',
                       '6', '1', '7', '4', '5', '1', '4', '4', '8', '\0'};
    assert_true(add_separator_to_number(out64, sizeof(out64)));
    assert_string_equal(
        out64,
        "-4,439,116,239,752,354,957,227,135,988,925,353,914,967,519,917,178,632,766,617,451,448");

    // len = 65
    char out65[104] = {'-', '3', '9', '4', '9', '4', '5', '5', '6', '3', '2', '7', '3', '4',
                       '3', '5', '6', '5', '4', '1', '3', '6', '9', '9', '4', '6', '9', '6',
                       '5', '4', '7', '1', '4', '2', '9', '4', '2', '3', '1', '8', '2', '4',
                       '2', '7', '1', '7', '5', '5', '3', '5', '7', '8', '5', '6', '5', '4',
                       '6', '2', '9', '4', '8', '1', '1', '3', '7', '3', '\0'};
    assert_true(add_separator_to_number(out65, sizeof(out65)));
    assert_string_equal(
        out65,
        "-39,494,556,327,343,565,413,699,469,654,714,294,231,824,271,755,357,856,546,294,811,373");

    // len = 66
    char out66[104] = {'-', '5', '6', '9', '1', '9', '9', '7', '5', '7', '4', '7', '3', '2',
                       '4', '4', '1', '5', '1', '6', '1', '9', '2', '1', '5', '8', '2', '6',
                       '7', '3', '6', '7', '2', '1', '5', '9', '2', '4', '8', '3', '1', '5',
                       '7', '6', '9', '7', '7', '7', '9', '8', '9', '9', '5', '5', '5', '8',
                       '9', '5', '9', '3', '9', '6', '6', '6', '4', '3', '8', '\0'};
    assert_true(add_separator_to_number(out66, sizeof(out66)));
    assert_string_equal(
        out66,
        "-569,199,757,473,244,151,619,215,826,736,721,592,483,157,697,779,899,555,895,939,666,438");

    // len = 67
    char out67[104] = {'-', '8', '1', '6', '4', '2', '4', '7', '3', '2', '2', '4', '4', '7',
                       '4', '5', '5', '5', '6', '7', '6', '7', '8', '5', '8', '2', '5', '4',
                       '6', '8', '9', '1', '4', '1', '4', '3', '5', '5', '9', '5', '1', '1',
                       '1', '9', '7', '7', '6', '8', '4', '1', '1', '1', '6', '7', '7', '9',
                       '7', '8', '5', '7', '4', '6', '8', '7', '3', '7', '5', '7', '\0'};
    assert_true(add_separator_to_number(out67, sizeof(out67)));
    assert_string_equal(out67,
                        "-8,164,247,322,447,455,567,678,582,546,891,414,355,951,119,776,841,116,"
                        "779,785,746,873,757");

    // len = 68
    char out68[104] = {'-', '7', '1', '8', '1', '6', '1', '3', '4', '6', '6', '9', '1', '3',
                       '4', '3', '6', '6', '2', '3', '4', '7', '6', '5', '8', '1', '9', '1',
                       '1', '1', '9', '3', '9', '4', '4', '6', '4', '2', '8', '8', '9', '8',
                       '9', '3', '8', '1', '1', '5', '8', '2', '1', '6', '4', '9', '8', '4',
                       '6', '8', '8', '1', '6', '4', '8', '8', '3', '3', '2', '5', '2', '\0'};
    assert_true(add_separator_to_number(out68, sizeof(out68)));
    assert_string_equal(out68,
                        "-71,816,134,669,134,366,234,765,819,111,939,446,428,898,938,115,821,649,"
                        "846,881,648,833,252");

    // len = 69
    char out69[104] = {'-', '4', '9', '4', '2', '2', '6', '4', '4', '2', '6', '2', '1', '9', '9',
                       '7', '9', '6', '9', '9', '7', '4', '4', '6', '4', '9', '1', '2', '5', '8',
                       '4', '8', '4', '7', '5', '6', '5', '5', '1', '6', '4', '7', '4', '7', '1',
                       '4', '2', '9', '8', '1', '8', '3', '2', '2', '5', '6', '4', '2', '4', '7',
                       '1', '1', '7', '8', '9', '5', '9', '1', '5', '7', '\0'};
    assert_true(add_separator_to_number(out69, sizeof(out69)));
    assert_string_equal(out69,
                        "-494,226,442,621,997,969,974,464,912,584,847,565,516,474,714,298,183,225,"
                        "642,471,178,959,157");

    // len = 70
    char out70[104] = {'-', '1', '8', '4', '7', '6', '2', '8', '8', '9', '3', '6', '8', '1', '5',
                       '5', '2', '2', '7', '1', '3', '2', '8', '4', '8', '1', '7', '7', '8', '6',
                       '5', '6', '4', '5', '9', '6', '9', '5', '6', '5', '7', '3', '9', '1', '2',
                       '3', '9', '2', '5', '2', '1', '6', '2', '5', '7', '9', '5', '5', '3', '2',
                       '1', '9', '6', '1', '2', '1', '2', '6', '2', '5', '7', '\0'};
    assert_true(add_separator_to_number(out70, sizeof(out70)));
    assert_string_equal(out70,
                        "-1,847,628,893,681,552,271,328,481,778,656,459,695,657,391,239,252,162,"
                        "579,553,219,612,126,257");

    // len = 71
    char out71[104] = {'-', '1', '1', '6', '6', '6', '6', '6', '7', '3', '9', '8', '3', '2', '5',
                       '5', '3', '5', '3', '6', '9', '6', '9', '5', '4', '6', '7', '6', '1', '2',
                       '2', '5', '9', '2', '6', '6', '5', '3', '1', '7', '6', '8', '1', '9', '3',
                       '1', '7', '2', '6', '4', '1', '8', '6', '8', '4', '6', '7', '3', '6', '6',
                       '4', '4', '4', '1', '6', '3', '9', '2', '6', '6', '8', '6', '\0'};
    assert_true(add_separator_to_number(out71, sizeof(out71)));
    assert_string_equal(out71,
                        "-11,666,667,398,325,535,369,695,467,612,259,266,531,768,193,172,641,868,"
                        "467,366,444,163,926,686");

    // len = 72
    char out72[104] = {'-', '7', '5', '3', '1', '8', '7', '8', '2', '9', '6', '6', '8', '8', '4',
                       '4', '7', '7', '4', '7', '4', '1', '9', '3', '6', '3', '9', '3', '1', '3',
                       '4', '2', '9', '5', '4', '7', '9', '7', '5', '1', '7', '8', '2', '3', '4',
                       '2', '8', '1', '9', '4', '3', '7', '1', '6', '9', '4', '8', '1', '8', '7',
                       '3', '4', '9', '8', '4', '5', '7', '7', '1', '1', '4', '7', '3', '\0'};
    assert_true(add_separator_to_number(out72, sizeof(out72)));
    assert_string_equal(out72,
                        "-753,187,829,668,844,774,741,936,393,134,295,479,751,782,342,819,437,169,"
                        "481,873,498,457,711,473");

    // len = 73
    char out73[104] = {'-', '4', '1', '7', '2', '9', '1', '8', '6', '1', '8', '9', '7', '8', '9',
                       '4', '6', '4', '4', '3', '7', '6', '3', '9', '2', '7', '5', '1', '2', '8',
                       '9', '8', '8', '8', '5', '5', '4', '9', '5', '4', '4', '5', '2', '8', '7',
                       '6', '6', '5', '3', '5', '3', '6', '5', '7', '8', '8', '5', '3', '8', '4',
                       '5', '5', '9', '1', '5', '3', '9', '1', '9', '9', '6', '1', '7', '7', '\0'};
    assert_true(add_separator_to_number(out73, sizeof(out73)));
    assert_string_equal(out73,
                        "-4,172,918,618,978,946,443,763,927,512,898,885,549,544,528,766,535,365,"
                        "788,538,455,915,391,996,177");

    // len = 74
    char out74[104] = {'-', '6', '1', '5', '4', '6', '5', '9', '8', '8', '8', '8', '6',
                       '2', '9', '1', '2', '9', '6', '2', '4', '6', '6', '3', '9', '9',
                       '2', '3', '6', '2', '6', '8', '9', '4', '3', '8', '3', '9', '2',
                       '9', '3', '1', '5', '9', '8', '8', '5', '7', '7', '8', '1', '1',
                       '3', '4', '7', '1', '3', '3', '1', '7', '7', '7', '4', '9', '6',
                       '5', '7', '2', '1', '1', '5', '9', '4', '8', '8', '\0'};
    assert_true(add_separator_to_number(out74, sizeof(out74)));
    assert_string_equal(out74,
                        "-61,546,598,888,629,129,624,663,992,362,689,438,392,931,598,857,781,134,"
                        "713,317,774,965,721,159,488");

    // len = 75
    char out75[104] = {'-', '2', '4', '9', '6', '3', '4', '1', '9', '6', '3', '1', '5',
                       '1', '1', '2', '7', '9', '1', '8', '1', '3', '2', '5', '6', '1',
                       '6', '2', '1', '7', '8', '7', '7', '7', '2', '6', '6', '2', '4',
                       '9', '7', '9', '3', '5', '4', '4', '8', '7', '2', '7', '9', '1',
                       '5', '6', '2', '8', '6', '2', '9', '7', '4', '3', '4', '7', '8',
                       '9', '6', '1', '7', '9', '9', '4', '3', '1', '2', '6', '\0'};
    assert_true(add_separator_to_number(out75, sizeof(out75)));
    assert_string_equal(out75,
                        "-249,634,196,315,112,791,813,256,162,178,777,266,249,793,544,872,791,562,"
                        "862,974,347,896,179,943,126");

    // len = 76
    char out76[104] = {'-', '1', '5', '8', '9', '4', '4', '5', '9', '7', '4', '7', '1',
                       '6', '1', '3', '5', '6', '5', '8', '8', '2', '5', '7', '8', '5',
                       '7', '2', '3', '3', '1', '6', '9', '4', '7', '4', '9', '5', '9',
                       '9', '2', '9', '5', '6', '1', '1', '9', '3', '7', '1', '1', '7',
                       '1', '5', '1', '2', '7', '1', '6', '8', '9', '9', '8', '9', '5',
                       '1', '3', '6', '2', '1', '8', '8', '5', '6', '5', '4', '3', '\0'};
    assert_true(add_separator_to_number(out76, sizeof(out76)));
    assert_string_equal(out76,
                        "-1,589,445,974,716,135,658,825,785,723,316,947,495,992,956,119,371,171,"
                        "512,716,899,895,136,218,856,543");

    // len = 77
    char out77[104] = {'-', '6', '7', '8', '1', '1', '5', '3', '6', '1', '4', '3', '7', '4',
                       '4', '2', '1', '3', '3', '6', '9', '2', '9', '7', '6', '9', '5', '1',
                       '7', '1', '9', '9', '4', '3', '1', '3', '5', '3', '1', '2', '3', '7',
                       '2', '4', '4', '3', '4', '6', '8', '1', '5', '3', '5', '4', '2', '7',
                       '6', '4', '3', '2', '3', '5', '4', '5', '1', '7', '3', '9', '9', '6',
                       '5', '2', '4', '5', '2', '3', '1', '2', '\0'};
    assert_true(add_separator_to_number(out77, sizeof(out77)));
    assert_string_equal(out77,
                        "-67,811,536,143,744,213,369,297,695,171,994,313,531,237,244,346,815,354,"
                        "276,432,354,517,399,652,452,312");

    // invalid
    char out78[104] = {'-', '\0'};
    assert_false(add_separator_to_number(out78, sizeof(out78)));
}

void test_add_separator_to_float_positive() {
    char out1[105] = {'-', '0', '.', '1', '\0'};
    assert_true(add_separator_to_number(out1, sizeof(out1)));
    assert_string_equal(out1, "-0.1");

    char out2[105] = {'-', '0', '.', '1', '2', '3', '4', '5', '6', '7', '8', '9', '\0'};
    assert_true(add_separator_to_number(out2, sizeof(out2)));
    assert_string_equal(out2, "-0.123456789");

    char out3[105] = {'-', '1', '2', '3', '4', '5', '6', '7', '8', '.', '9', '0', '\0'};
    assert_true(add_separator_to_number(out3, sizeof(out3)));
    assert_string_equal(out3, "-12,345,678.90");

    char out4[105] = {'-', '1', '1', '5', '7', '9', '2', '0', '8', '9', '2', '3', '7', '3',
                      '1', '6', '1', '9', '5', '4', '2', '3', '5', '7', '0', '9', '8', '5',
                      '0', '0', '8', '6', '8', '7', '9', '0', '7', '8', '5', '3', '2', '6',
                      '9', '9', '8', '4', '6', '6', '5', '6', '4', '0', '5', '6', '4', '0',
                      '3', '9', '4', '5', '7', '5', '8', '4', '0', '0', '7', '9', '1', '3',
                      '1', '2', '9', '6', '3', '.', '9', '9', '3', '5', '\0'};
    assert_true(add_separator_to_number(out4, sizeof(out4)));
    assert_string_equal(out4,
                        "-11,579,208,923,731,619,542,357,098,500,868,790,785,326,998,466,564,056,"
                        "403,945,758,400,791,312,963.9935");

    char out5[105] = {'-', '1', '1', '5', '7', '9', '2', '0', '8', '9', '2', '3', '7', '3',
                      '1', '6', '1', '9', '5', '4', '2', '3', '5', '.', '7', '0', '9', '8',
                      '5', '0', '0', '8', '6', '8', '7', '9', '0', '7', '8', '5', '3', '2',
                      '6', '9', '9', '8', '4', '6', '6', '5', '6', '4', '0', '5', '6', '4',
                      '0', '3', '9', '4', '5', '7', '5', '8', '4', '0', '0', '7', '9', '1',
                      '3', '1', '2', '9', '6', '3', '9', '9', '3', '5', '\0'};
    assert_true(add_separator_to_number(out5, sizeof(out5)));
    assert_string_equal(
        out5,
        "-1,157,920,892,373,161,954,235.70985008687907853269984665640564039457584007913129639935");
}

void test_add_separator_to_float_negative() {
    char out1[105] = {'0', '.', '1', '\0'};
    assert_true(add_separator_to_number(out1, sizeof(out1)));
    assert_string_equal(out1, "0.1");

    char out2[105] = {'0', '.', '1', '2', '3', '4', '5', '6', '7', '8', '9', '\0'};
    assert_true(add_separator_to_number(out2, sizeof(out2)));
    assert_string_equal(out2, "0.123456789");

    char out3[105] = {'1', '2', '3', '4', '5', '6', '7', '8', '.', '9', '0', '\0'};
    assert_true(add_separator_to_number(out3, sizeof(out3)));
    assert_string_equal(out3, "12,345,678.90");

    char out4[105] = {'1', '1', '5', '7', '9', '2', '0', '8', '9', '2', '3', '7', '3', '1',
                      '6', '1', '9', '5', '4', '2', '3', '5', '7', '0', '9', '8', '5', '0',
                      '0', '8', '6', '8', '7', '9', '0', '7', '8', '5', '3', '2', '6', '9',
                      '9', '8', '4', '6', '6', '5', '6', '4', '0', '5', '6', '4', '0', '3',
                      '9', '4', '5', '7', '5', '8', '4', '0', '0', '7', '9', '1', '3', '1',
                      '2', '9', '6', '3', '.', '9', '9', '3', '5', '\0'};
    assert_true(add_separator_to_number(out4, sizeof(out4)));
    assert_string_equal(out4,
                        "11,579,208,923,731,619,542,357,098,500,868,790,785,326,998,466,564,056,"
                        "403,945,758,400,791,312,963.9935");

    char out5[105] = {'1', '1', '5', '7', '9', '2', '0', '8', '9', '2', '3', '7', '3', '1',
                      '6', '1', '9', '5', '4', '2', '3', '5', '.', '7', '0', '9', '8', '5',
                      '0', '0', '8', '6', '8', '7', '9', '0', '7', '8', '5', '3', '2', '6',
                      '9', '9', '8', '4', '6', '6', '5', '6', '4', '0', '5', '6', '4', '0',
                      '3', '9', '4', '5', '7', '5', '8', '4', '0', '0', '7', '9', '1', '3',
                      '1', '2', '9', '6', '3', '9', '9', '3', '5', '\0'};
    assert_true(add_separator_to_number(out5, sizeof(out5)));
    assert_string_equal(
        out5,
        "1,157,920,892,373,161,954,235.70985008687907853269984665640564039457584007913129639935");
}

void test_add_decimal_point() {
    char out0[105] = {'1', '\0'};
    assert_true(add_decimal_point(out0, sizeof(out0), 0));
    assert_string_equal(out0, "1");

    char out1[105] = {'1', '\0'};
    assert_true(add_decimal_point(out1, sizeof(out1), 1));
    assert_string_equal(out1, "0.1");

    char out2[105] = {'1', '\0'};
    assert_true(add_decimal_point(out2, sizeof(out2), 103));
    assert_string_equal(out2,
                        "0."
                        "00000000000000000000000000000000000000000000000000000000000000000000000000"
                        "00000000000000000000000000001");
    char out3[105] = {'-', '1', '\0'};
    assert_false(add_decimal_point(out3, sizeof(out3), 104));

    char out4[105] = {'-', '1', '\0'};
    assert_true(add_decimal_point(out4, sizeof(out4), 0));
    assert_string_equal(out4, "-1");

    char out5[105] = {'-', '1', '\0'};
    assert_true(add_decimal_point(out5, sizeof(out5), 1));
    assert_string_equal(out5, "-0.1");

    char out6[105] = {'-', '1', '\0'};
    assert_true(add_decimal_point(out6, sizeof(out6), 102));
    assert_string_equal(out6,
                        "-0."
                        "00000000000000000000000000000000000000000000000000000000000000000000000000"
                        "0000000000000000000000000001");
    char out7[105] = {'-', '1', '\0'};
    assert_false(add_decimal_point(out7, sizeof(out7), 103));

    char out8[105] = {'1', '2', '3', '4', '5', '0', '0', '0', '0', '0', '\0'};
    assert_true(add_decimal_point(out8, sizeof(out8), 3));
    assert_string_equal(out8, "1234500");

    char out9[105] = {'1', '2', '3', '4', '5', '0', '0', '0', '0', '0', '\0'};
    assert_true(add_decimal_point(out9, sizeof(out9), 5));
    assert_string_equal(out9, "12345");

    char out10[105] = {'1', '2', '3', '4', '5', '0', '0', '0', '0', '0', '\0'};
    assert_true(add_decimal_point(out10, sizeof(out10), 6));
    assert_string_equal(out10, "1234.5");

    char out11[105] = {'1', '2', '3', '4', '5', '0', '0', '0', '0', '0', '\0'};
    assert_true(add_decimal_point(out11, sizeof(out11), 7));
    assert_string_equal(out11, "123.45");

    char out12[105] = {'1', '2', '3', '4', '5', '0', '0', '0', '0', '0', '\0'};
    assert_true(add_decimal_point(out12, sizeof(out12), 10));
    assert_string_equal(out12, "0.12345");

    char out13[105] = {'1', '2', '3', '4', '5', '0', '0', '0', '0', '0', '\0'};
    assert_true(add_decimal_point(out13, sizeof(out13), 16));
    assert_string_equal(out13, "0.00000012345");

    char out14[105] = {'-', '1', '2', '3', '4', '5', '0', '0', '0', '0', '0', '\0'};
    assert_true(add_decimal_point(out14, sizeof(out14), 3));
    assert_string_equal(out14, "-1234500");

    char out15[105] = {'-', '1', '2', '3', '4', '5', '0', '0', '0', '0', '0', '\0'};
    assert_true(add_decimal_point(out15, sizeof(out15), 5));
    assert_string_equal(out15, "-12345");

    char out16[105] = {'-', '1', '2', '3', '4', '5', '0', '0', '0', '0', '0', '\0'};
    assert_true(add_decimal_point(out16, sizeof(out16), 6));
    assert_string_equal(out16, "-1234.5");

    char out17[105] = {'-', '1', '2', '3', '4', '5', '0', '0', '0', '0', '0', '\0'};
    assert_true(add_decimal_point(out17, sizeof(out17), 7));
    assert_string_equal(out17, "-123.45");

    char out18[105] = {'-', '1', '2', '3', '4', '5', '0', '0', '0', '0', '0', '\0'};
    assert_true(add_decimal_point(out18, sizeof(out18), 10));
    assert_string_equal(out18, "-0.12345");

    char out19[105] = {'-', '1', '2', '3', '4', '5', '0', '0', '0', '0', '0', '\0'};
    assert_true(add_decimal_point(out19, sizeof(out19), 16));
    assert_string_equal(out19, "-0.00000012345");

    char out20[105] = {'1', '1', '5', '7', '9', '2', '0', '8', '9', '2', '3', '7', '3', '1',
                       '6', '1', '9', '5', '4', '2', '3', '5', '7', '0', '9', '8', '5', '0',
                       '0', '8', '6', '8', '7', '9', '0', '7', '8', '5', '3', '2', '6', '9',
                       '9', '8', '4', '6', '6', '5', '6', '4', '0', '5', '6', '4', '0', '3',
                       '9', '4', '5', '7', '5', '8', '4', '0', '0', '7', '9', '1', '3', '1',
                       '2', '9', '6', '3', '9', '9', '3', '5', '\0'};
    assert_true(add_decimal_point(out20, sizeof(out20), 0));
    assert_string_equal(
        out20,
        "115792089237316195423570985008687907853269984665640564039457584007913129639935");

    char out21[105] = {'1', '1', '5', '7', '9', '2', '0', '8', '9', '2', '3', '7', '3', '1',
                       '6', '1', '9', '5', '4', '2', '3', '5', '7', '0', '9', '8', '5', '0',
                       '0', '8', '6', '8', '7', '9', '0', '7', '8', '5', '3', '2', '6', '9',
                       '9', '8', '4', '6', '6', '5', '6', '4', '0', '5', '6', '4', '0', '3',
                       '9', '4', '5', '7', '5', '8', '4', '0', '0', '7', '9', '1', '3', '1',
                       '2', '9', '6', '3', '9', '9', '3', '5', '\0'};
    assert_true(add_decimal_point(out21, sizeof(out21), 20));
    assert_string_equal(
        out21,
        "1157920892373161954235709850086879078532699846656405640394.57584007913129639935");

    char out22[105] = {'1', '1', '5', '7', '9', '2', '0', '8', '9', '2', '3', '7', '3', '1',
                       '6', '1', '9', '5', '4', '2', '3', '5', '7', '0', '9', '8', '5', '0',
                       '0', '8', '6', '8', '7', '9', '0', '7', '8', '5', '3', '2', '6', '9',
                       '9', '8', '4', '6', '6', '5', '6', '4', '0', '5', '6', '4', '0', '3',
                       '9', '4', '5', '7', '5', '8', '4', '0', '0', '7', '9', '1', '3', '1',
                       '2', '9', '6', '3', '9', '9', '3', '5', '\0'};
    assert_true(add_decimal_point(out22, sizeof(out22), 78));
    assert_string_equal(
        out22,
        "0.115792089237316195423570985008687907853269984665640564039457584007913129639935");

    char out23[105] = {'1', '1', '5', '7', '9', '2', '0', '8', '9', '2', '3', '7', '3', '1',
                       '6', '1', '9', '5', '4', '2', '3', '5', '7', '0', '9', '8', '5', '0',
                       '0', '8', '6', '8', '7', '9', '0', '7', '8', '5', '3', '2', '6', '9',
                       '9', '8', '4', '6', '6', '5', '6', '4', '0', '5', '6', '4', '0', '3',
                       '9', '4', '5', '7', '5', '8', '4', '0', '0', '7', '9', '1', '3', '1',
                       '2', '9', '6', '3', '9', '9', '3', '5', '\0'};
    assert_true(add_decimal_point(out23, sizeof(out23), 80));
    assert_string_equal(
        out23,
        "0.00115792089237316195423570985008687907853269984665640564039457584007913129639935");

    char out24[105] = {'-', '1', '1', '5', '7', '9', '2', '0', '8', '9', '2', '3', '7', '3',
                       '1', '6', '1', '9', '5', '4', '2', '3', '5', '7', '0', '9', '8', '5',
                       '0', '0', '8', '6', '8', '7', '9', '0', '7', '8', '5', '3', '2', '6',
                       '9', '9', '8', '4', '6', '6', '5', '6', '4', '0', '5', '6', '4', '0',
                       '3', '9', '4', '5', '7', '5', '8', '4', '0', '0', '7', '9', '1', '3',
                       '1', '2', '9', '6', '3', '9', '9', '3', '5', '\0'};
    assert_true(add_decimal_point(out24, sizeof(out24), 0));
    assert_string_equal(
        out24,
        "-115792089237316195423570985008687907853269984665640564039457584007913129639935");

    char out25[105] = {'-', '1', '1', '5', '7', '9', '2', '0', '8', '9', '2', '3', '7', '3',
                       '1', '6', '1', '9', '5', '4', '2', '3', '5', '7', '0', '9', '8', '5',
                       '0', '0', '8', '6', '8', '7', '9', '0', '7', '8', '5', '3', '2', '6',
                       '9', '9', '8', '4', '6', '6', '5', '6', '4', '0', '5', '6', '4', '0',
                       '3', '9', '4', '5', '7', '5', '8', '4', '0', '0', '7', '9', '1', '3',
                       '1', '2', '9', '6', '3', '9', '9', '3', '5', '\0'};
    assert_true(add_decimal_point(out25, sizeof(out25), 20));
    assert_string_equal(
        out25,
        "-1157920892373161954235709850086879078532699846656405640394.57584007913129639935");

    char out26[105] = {'-', '1', '1', '5', '7', '9', '2', '0', '8', '9', '2', '3', '7', '3',
                       '1', '6', '1', '9', '5', '4', '2', '3', '5', '7', '0', '9', '8', '5',
                       '0', '0', '8', '6', '8', '7', '9', '0', '7', '8', '5', '3', '2', '6',
                       '9', '9', '8', '4', '6', '6', '5', '6', '4', '0', '5', '6', '4', '0',
                       '3', '9', '4', '5', '7', '5', '8', '4', '0', '0', '7', '9', '1', '3',
                       '1', '2', '9', '6', '3', '9', '9', '3', '5', '\0'};
    assert_true(add_decimal_point(out26, sizeof(out26), 78));
    assert_string_equal(
        out26,
        "-0.115792089237316195423570985008687907853269984665640564039457584007913129639935");

    char out27[105] = {'-', '1', '1', '5', '7', '9', '2', '0', '8', '9', '2', '3', '7', '3',
                       '1', '6', '1', '9', '5', '4', '2', '3', '5', '7', '0', '9', '8', '5',
                       '0', '0', '8', '6', '8', '7', '9', '0', '7', '8', '5', '3', '2', '6',
                       '9', '9', '8', '4', '6', '6', '5', '6', '4', '0', '5', '6', '4', '0',
                       '3', '9', '4', '5', '7', '5', '8', '4', '0', '0', '7', '9', '1', '3',
                       '1', '2', '9', '6', '3', '9', '9', '3', '5', '\0'};
    assert_true(add_decimal_point(out27, sizeof(out27), 80));
    assert_string_equal(
        out27,
        "-0.00115792089237316195423570985008687907853269984665640564039457584007913129639935");
}

void test_print_raw_bytes() {
    char out[100];

    // Test with printable ASCII characters
    uint8_t printable_data[] = "Hello World";
    assert_true(print_raw_bytes(printable_data, strlen((char *) printable_data), out, sizeof(out)));
    assert_string_equal(out, "Hello World");

    // Test with non-printable characters
    uint8_t non_printable_data[] = {0x01, 0x02, 0x03, 0x1F, 0x7F, 0xFF};
    assert_true(print_raw_bytes(non_printable_data, sizeof(non_printable_data), out, sizeof(out)));
    assert_string_equal(out, "\\x01\\x02\\x03\\x1f\\x7f\\xff");

    // Test with mixed printable and non-printable
    uint8_t mixed_data[] = {'H', 'i', 0x00, '!', 0xFF};
    assert_true(print_raw_bytes(mixed_data, sizeof(mixed_data), out, sizeof(out)));
    assert_string_equal(out, "Hi\\x00!\\xff");

    // Test with empty data
    assert_true(print_raw_bytes(NULL, 0, out, sizeof(out)));
    assert_string_equal(out, "");

    // Test with boundary characters (space and DEL)
    uint8_t boundary_data[] = {0x1F, 0x20, 0x7E, 0x7F};
    assert_true(print_raw_bytes(boundary_data, sizeof(boundary_data), out, sizeof(out)));
    assert_string_equal(out, "\\x1f ~\\x7f");

    // Test with special characters that need escaping
    uint8_t special_data[] = {0x5C, 0x0A, 0x09, 0x0D};
    assert_true(print_raw_bytes(special_data, sizeof(special_data), out, sizeof(out)));
    assert_string_equal(
        out,
        "\\\\x0a\\x09\\x0d");  // 0x5C is printable backslash, others are hex-escaped

    // Test with buffer too small (should fail)
    char small_out[5];
    uint8_t large_data[] = "Hello World";
    assert_false(
        print_raw_bytes(large_data, strlen((char *) large_data), small_out, sizeof(small_out)));

    // Test with buffer barely fitting hex escape
    char tiny_out[5];  // Space for "\\x01" + null terminator
    uint8_t hex_data[] = {0x01};
    assert_true(print_raw_bytes(hex_data, sizeof(hex_data), tiny_out, sizeof(tiny_out)));
    assert_string_equal(tiny_out, "\\x01");

    // Test with buffer too small for complete hex escape
    char micro_out[3];  // Not enough space for "\\x01"
    assert_false(print_raw_bytes(hex_data, sizeof(hex_data), micro_out, sizeof(micro_out)));

    // Test with NULL parameters
    assert_false(print_raw_bytes(NULL, 5, out, sizeof(out)));
    assert_false(print_raw_bytes(printable_data, 5, NULL, sizeof(out)));
    assert_false(print_raw_bytes(printable_data, 5, out, 0));

    // Test with all printable ASCII range
    uint8_t ascii_range[95];
    for (int i = 0; i < 95; i++) {
        ascii_range[i] = 0x20 + i;  // Space to tilde
    }
    char large_out[96];
    assert_true(print_raw_bytes(ascii_range, sizeof(ascii_range), large_out, sizeof(large_out)));
    assert_int_equal(strlen(large_out), 95);  // Should be exactly 95 printable characters

    // Test with zero byte in middle
    uint8_t zero_data[] = {'A', 0x00, 'B'};
    assert_true(print_raw_bytes(zero_data, sizeof(zero_data), out, sizeof(out)));
    assert_string_equal(out, "A\\x00B");
}

void test_calculate_safe_chunk_size() {
    // Test all printable characters
    uint8_t printable_data[] = "Hello World!";
    size_t printable_len = strlen((char *) printable_data);

    // With large buffer, should fit all characters
    assert_int_equal(calculate_safe_chunk_size(printable_data, printable_len, 100), printable_len);

    // With exact size buffer (plus null terminator space)
    assert_int_equal(calculate_safe_chunk_size(printable_data, printable_len, printable_len + 1),
                     printable_len);

    // With small buffer, should fit partial characters
    assert_int_equal(calculate_safe_chunk_size(printable_data, printable_len, 6),
                     5);  // "Hello" fits in 6 bytes (5 chars + null)

    // Test mixed printable and non-printable
    uint8_t mixed_data[] = {'A', 0x00, 'B', 0x01, 'C'};  // A + null + B + 0x01 + C
    // Expected output: "A\\x00B\\x01C" = 1+4+1+4+1 = 11 chars + null = 12 bytes needed
    assert_int_equal(calculate_safe_chunk_size(mixed_data, 5, 12), 5);  // All data fits
    assert_int_equal(calculate_safe_chunk_size(mixed_data, 5, 10),
                     3);  // Only "A\x00B" fits (1+4+1 = 6 chars)
    assert_int_equal(calculate_safe_chunk_size(mixed_data, 5, 6),
                     2);  // Only "A\x00" fits (1+4 = 5 chars)

    // Test all non-printable characters
    uint8_t nonprint_data[] = {0x00, 0x01, 0x02};
    // Each needs 4 chars: \x00\x01\x02 = 12 chars + null = 13 bytes needed
    assert_int_equal(calculate_safe_chunk_size(nonprint_data, 3, 13), 3);  // All fit
    assert_int_equal(calculate_safe_chunk_size(nonprint_data, 3, 9),
                     2);  // Only 2 fit (8 chars + null)
    assert_int_equal(calculate_safe_chunk_size(nonprint_data, 3, 5),
                     1);  // Only 1 fits (4 chars + null)
    assert_int_equal(calculate_safe_chunk_size(nonprint_data, 3, 4),
                     0);  // None fit (need at least 5 bytes)

    // Test edge cases
    assert_int_equal(calculate_safe_chunk_size(NULL, 5, 10), 0);            // NULL data
    assert_int_equal(calculate_safe_chunk_size(printable_data, 5, 0), 0);   // Zero output size
    assert_int_equal(calculate_safe_chunk_size(printable_data, 0, 10), 0);  // Zero input length

    // Test exact boundary conditions
    uint8_t boundary_data[] = "AB";
    assert_int_equal(calculate_safe_chunk_size(boundary_data, 2, 3),
                     2);  // Exactly fits: 2 chars + null
    assert_int_equal(calculate_safe_chunk_size(boundary_data, 2, 2), 1);  // Only 1 char fits + null
    assert_int_equal(calculate_safe_chunk_size(boundary_data, 2, 1),
                     0);  // No chars fit, need space for null
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_print_account_id),
        cmocka_unit_test(test_print_hash_x_key),
        cmocka_unit_test(test_print_pre_auth_x_key),
        cmocka_unit_test(test_print_muxed_account),
        cmocka_unit_test(test_print_sc_address),
        cmocka_unit_test(test_print_med25519_key),
        cmocka_unit_test(test_print_claimable_balance),
        cmocka_unit_test(test_print_liquidity_pool),
        cmocka_unit_test(test_print_ed25519_signed_payload),
        cmocka_unit_test(test_print_asset),
        cmocka_unit_test(test_print_amount_asset_native),
        cmocka_unit_test(test_print_amount_asset_alphanum4),
        cmocka_unit_test(test_print_amount_asset_alphanum12),
        cmocka_unit_test(test_print_claimable_balance_id),
        cmocka_unit_test(test_print_account_flags),
        cmocka_unit_test(test_print_trust_line_flags),
        cmocka_unit_test(test_print_allow_trust_flags),
        cmocka_unit_test(test_print_uint64_num),
        cmocka_unit_test(test_print_int64_num),
        cmocka_unit_test(test_is_printable_binary),
        cmocka_unit_test(test_print_binary),
        cmocka_unit_test(test_print_time),
        cmocka_unit_test(test_print_int128),
        cmocka_unit_test(test_print_uint128),
        cmocka_unit_test(test_print_int256),
        cmocka_unit_test(test_print_uint256),
        cmocka_unit_test(test_print_int32),
        cmocka_unit_test(test_print_uint32),
        cmocka_unit_test(test_print_int64),
        cmocka_unit_test(test_print_uint64),
        cmocka_unit_test(test_print_scv_symbol),
        cmocka_unit_test(test_print_scv_string),
        cmocka_unit_test(test_add_separator_to_integer_positive),
        cmocka_unit_test(test_add_separator_to_integer_negative),
        cmocka_unit_test(test_add_separator_to_float_positive),
        cmocka_unit_test(test_add_separator_to_float_negative),
        cmocka_unit_test(test_add_decimal_point),
        cmocka_unit_test(test_print_raw_bytes),
        cmocka_unit_test(test_calculate_safe_chunk_size),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}