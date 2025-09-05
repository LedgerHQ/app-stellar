#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <cmocka.h>
#include "stellar/parser.h"

#define MAX_ENVELOPE_SIZE 1024

const char *testcases[] = {
    "../testcases/sorobanAuthCreateSmartContract.raw",
    "../testcases/sorobanAuthCreateSmartContractV2.raw",
    "../testcases/sorobanAuthInvokeContract.raw",
    "../testcases/sorobanAuthInvokeContractWithoutArgs.raw",
    "../testcases/sorobanAuthPublic.raw",
    "../testcases/sorobanAuthTestnet.raw",
    "../testcases/sorobanAuthUnknownNetwork.raw",
    "../testcases/sorobanAuthInvokeContractWithComplexSubInvocation.raw",
    "../testcases/sorobanAuthInvokeContractTestPlugin.raw",
};

void test_parse_data(void **state) {
    const char *filename = (char *) *state;
    FILE *file = fopen(filename, "rb");
    assert_non_null(file);
    envelope_t envelope;

    memset(&envelope, 0, sizeof(envelope_t));
    uint8_t data[MAX_ENVELOPE_SIZE];
    size_t read_count = fread(data, sizeof(char), MAX_ENVELOPE_SIZE, file);
    assert_true(parse_soroban_authorization_envelope(data, read_count, &envelope));
}

int main() {
    struct CMUnitTest tests[sizeof(testcases) / sizeof(testcases[0])];
    for (int i = 0; i < sizeof(testcases) / sizeof(testcases[0]); i++) {
        tests[i].name = testcases[i];
        tests[i].test_func = test_parse_data;
        tests[i].initial_state = (void *) testcases[i];
        tests[i].setup_func = NULL;
        tests[i].teardown_func = NULL;
    }
    return cmocka_run_group_tests(tests, NULL, NULL);
}
