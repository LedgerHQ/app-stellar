#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include <cmocka.h>

#include "cx.h"
#include "../src/stellar_api.h"
#include "../src/stellar_types.h"
#include "../src/swap/swap_lib_calls.h"

void test_check_address(void **state) {
    (void) state;

    check_address_parameters_t params = {
        .address_to_check = "GCNCEJIAZ5D3APIF5XWAJ3JSSTHM4HPHE7GK3NAB6R6WWSZDB2A2BQ5B",
        .address_parameters = (uint8_t *) "\x03\x80\x00\x00\x2c\x80\x00\x00\x94\x80\x00\x00\x00",
        .address_parameters_length = 13, /* XXX: never used */
    };

    /* The public key is hardcoded. Indeed, get_public_key is not available from tests because of
     * exceptions and cryptographic calls. */
    cx_ecfp_public_key_t public_key = {
        .curve = CX_CURVE_Ed25519,
        .W_len = 32,
        .W = "\x9a\x22\x25\x00\xcf\x47\xb0\x3d\x05\xed\xec\x04\xed\x32\x94\xce\xce\x1d\xe7\x27\xcc"
             "\xad\xb4\x01\xf4\x7d\x6b\x4b\x23\x0e\x81\xa0",
    };

    uint32_t bip32_path_parsed[MAX_BIP32_LEN];
    uint8_t *bip32_path_ptr = params.address_parameters;
    uint8_t bip32_path_length = *(bip32_path_ptr++);
    assert_true(
        parse_bip32_path(bip32_path_ptr, bip32_path_length, bip32_path_parsed, MAX_BIP32_LEN));

    char address[57];
    encode_public_key(public_key.W, address);

    assert_string_equal(address, params.address_to_check);
}

void test_get_printable_amount(void **state) {
    (void) state;

    get_printable_amount_parameters_t params = {
        .amount = (uint8_t *) "\x00\x04\xd2",
        .amount_length = 3,
    };

    uint64_t amount;
    assert_true(swap_str_to_u64(params.amount, params.amount_length, &amount));
    assert_int_equal(amount, 1234);
    assert_int_equal(
        print_amount(amount, "XLM", params.printable_amount, sizeof(params.printable_amount)),
        0);
    assert_string_equal(params.printable_amount, "0.0001234 XLM");
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_check_address),
        cmocka_unit_test(test_get_printable_amount),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}