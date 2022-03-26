/*******************************************************************************
 *   Ledger Stellar App
 *   (c) 2019 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include "stellar_api.h"

void test_print_amount(void **state) {
    (void) state;

    char printed[24];
    const Asset asset = {.type = ASSET_TYPE_NATIVE};

    print_amount(1, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed));
    assert_string_equal(printed, "0.0000001 XLM");
    print_amount(10000000, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed));
    assert_string_equal(printed, "1 XLM");
    print_amount(100000000000001, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed));
    assert_string_equal(printed, "10000000.0000001 XLM");
    print_amount(100000001, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed));
    assert_string_equal(printed, "10.0000001 XLM");
    print_amount(100000001000000, &asset, NETWORK_TYPE_PUBLIC, printed, sizeof(printed));
    assert_string_equal(printed, "10000000.1 XLM");
}

void test_print_uint(void **state) {
    (void) state;

    char printed[24];

    assert_int_equal(print_uint(0, printed, sizeof(printed)), 0);
    assert_string_equal(printed, "0");

    assert_int_equal(print_uint(1230, printed, sizeof(printed)), 0);
    assert_string_equal(printed, "1230");

    // output buffer too small
    assert_int_equal(print_uint(1230, printed, 4), -1);

    // output buffer just big enough to store output data
    assert_int_equal(print_uint(9999, printed, 5), 0);
    assert_string_equal(printed, "9999");
}

void test_print_int(void **state) {
    (void) state;

    char printed[24];

    assert_int_equal(print_int(0, printed, sizeof(printed)), 0);
    assert_string_equal(printed, "0");

    assert_int_equal(print_int(1230, printed, sizeof(printed)), 0);
    assert_string_equal(printed, "1230");

    assert_int_equal(print_int(-1230, printed, sizeof(printed)), 0);
    assert_string_equal(printed, "-1230");

    // output buffer too small
    assert_int_equal(print_int(-1230, printed, 5), -1);
    assert_int_equal(print_int(1230, printed, 4), -1);

    // output buffer just big enough to store output data
    assert_int_equal(print_int(-9999, printed, 6), 0);
    assert_string_equal(printed, "-9999");
    assert_int_equal(print_int(9999, printed, 5), 0);
    assert_string_equal(printed, "9999");
}

void test_print_summary(void **state) {
    (void) state;

    char summary[27];
    print_summary("GADFVW3UXVKDOU626XUPYDJU2BFCGFJHQ6SREYOZ6IJV4XSHOALEQN2I", summary, 12, 12);
    assert_string_equal(summary, "GADFVW3UXVKD..4XSHOALEQN2I");
    print_summary("GADFVW3UXVKDOU626XUPYDJU2BFCGFJHQ6SREYOZ6IJV4XSHOALEQN2I", summary, 6, 6);
    assert_string_equal(summary, "GADFVW..LEQN2I");
}

void test_print_binary(void **state) {
    (void) state;

    const uint8_t binary[32] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
                                16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
    char hex[16];

    print_binary_summary(binary, hex, 32);
    assert_string_equal(hex, "0x000102..1D1E1F");
}

void test_base64_encode(void **state) {
    (void) state;

    char base64[20];
    base64_encode((uint8_t *) "starlight", 9, base64);
    assert_string_equal(base64, "c3RhcmxpZ2h0");
}

void test_print_muxed_account(void **state) {
    // https://github.com/stellar/stellar-protocol/blob/master/ecosystem/sep-0023.md#valid-test-cases
    (void) state;

    char printed[89];
    // GA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJVSGZ
    const uint8_t ed25519[] = {
        0x3f, 0x0c, 0x34, 0xbf, 0x93, 0xad, 0x0d, 0x99, 0x71, 0xd0, 0x4c,
        0xcc, 0x90, 0xf7, 0x05, 0x51, 0x1c, 0x83, 0x8a, 0xad, 0x97, 0x34,
        0xa4, 0xa2, 0xfb, 0x0d, 0x7a, 0x03, 0xfc, 0x7f, 0xe8, 0x9a,
    };
    // Valid non-multiplexed account
    MuxedAccount account1 = {.type = KEY_TYPE_ED25519, .ed25519 = ed25519};
    print_muxed_account(&account1, printed, 0, 0);
    assert_string_equal(printed, "GA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJVSGZ");
    print_muxed_account(&account1, printed, 12, 12);
    assert_string_equal(printed, "GA7QYNF7SOWQ..UA74P7UJVSGZ");

    // Valid multiplexed account
    MuxedAccount account2 = {.type = KEY_TYPE_MUXED_ED25519,
                             .med25519 = {.id = 0, .ed25519 = ed25519}};
    print_muxed_account(&account2, printed, 0, 0);
    assert_string_equal(printed,
                        "MA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJUAAAAAAAAAAAACJUQ");
    print_muxed_account(&account2, printed, 12, 12);
    assert_string_equal(printed, "MA7QYNF7SOWQ..AAAAAAAACJUQ");

    // Valid multiplexed account in which unsigned id exceeds maximum signed 64-bit integer
    MuxedAccount account3 = {.type = KEY_TYPE_MUXED_ED25519,
                             .med25519 = {.id = 9223372036854775808, .ed25519 = ed25519}};
    print_muxed_account(&account3, printed, 0, 0);
    assert_string_equal(printed,
                        "MA7QYNF7SOWQ3GLR2BGMZEHXAVIRZA4KVWLTJJFC7MGXUA74P7UJVAAAAAAAAAAAAAJLK");
    print_muxed_account(&account3, printed, 12, 12);
    assert_string_equal(printed, "MA7QYNF7SOWQ..AAAAAAAAAJLK");
}

void test_print_claimable_balance_id(void **state) {
    (void) state;

    ClaimableBalanceID claimableBalanceID = {
        .type = CLAIMABLE_BALANCE_ID_TYPE_V0,
        .v0 = {0xc9, 0xc4, 0xa9, 0xe3, 0xa4, 0x68, 0x91, 0xa3, 0x60, 0x15, 0xc3,
               0x17, 0xb3, 0xdf, 0x17, 0xb4, 0x2b, 0xf,  0x2a, 0xd8, 0xa2, 0xee,
               0xa6, 0xc9, 0x34, 0xc9, 0xf7, 0xc8, 0x42, 0x5d, 0xa7, 0xad}};

    char printed[89];
    print_claimable_balance_id(&claimableBalanceID, printed);
    assert_string_equal(
        printed,
        "0x00000000C9C4A9E3A46891A36015C317B3DF17B42B0F2AD8A2EEA6C934C9F7C8425DA7AD");
}

void test_print_time(void **state) {
    (void) state;
    char printed[89];
    assert_true(print_time(0, printed, 89));
    assert_string_equal(printed, "1970-01-01 00:00:00");
    assert_true(print_time(1648263853, printed, 89));
    assert_string_equal(printed, "2022-03-26 03:04:13");
    assert_true(print_time(2147483647, printed, 89));
    assert_string_equal(printed, "2038-01-19 03:14:07");
    assert_true(print_time(4294967295, printed, 89));
    assert_string_equal(printed, "2106-02-07 06:28:15");
    assert_true(print_time(253402300799, printed, 89));
    assert_string_equal(printed, "9999-12-31 23:59:59");
    assert_false(print_time(253402300800, printed, 89));
    assert_false(print_time(18446744073709551615, printed, 89));
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_print_amount),
        cmocka_unit_test(test_print_uint),
        cmocka_unit_test(test_print_int),
        cmocka_unit_test(test_print_summary),
        cmocka_unit_test(test_print_binary),
        cmocka_unit_test(test_base64_encode),
        cmocka_unit_test(test_print_muxed_account),
        cmocka_unit_test(test_print_claimable_balance_id),
        cmocka_unit_test(test_print_time),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
