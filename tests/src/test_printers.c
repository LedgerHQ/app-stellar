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

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_print_amount),
        cmocka_unit_test(test_print_uint),
        cmocka_unit_test(test_print_int),
        cmocka_unit_test(test_print_summary),
        cmocka_unit_test(test_print_binary),
        cmocka_unit_test(test_base64_encode),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
