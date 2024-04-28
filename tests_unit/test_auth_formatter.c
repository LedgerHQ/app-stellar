#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <cmocka.h>
#include "stellar/parser.h"
#include "stellar/formatter.h"

#define MAX_ENVELOPE_SIZE 1024
#define MAX_CAPTION_SIZE  20
#define MAX_VALUE_SIZE    104

const char *testcases[] = {
    "../testcases/sorobanAuthCreateSmartContract.raw",
    "../testcases/sorobanAuthInvokeContract.raw",
    "../testcases/sorobanAuthInvokeContractWithoutArgs.raw",
    "../testcases/sorobanAuthPublic.raw",
    "../testcases/sorobanAuthTestnet.raw",
    "../testcases/sorobanAuthUnknownNetwork.raw",
};

static bool is_string_empty(const char *str) {
    return str == NULL || str[0] == '\0';
}

static void get_result_filename(const char *filename, char *path, size_t size) {
    strncpy(path, filename, size);

    char *ext = strstr(path, ".raw");
    assert_non_null(ext);
    memcpy(ext, ".txt", 4);
}

void test_format_envelope(void **state) {
    const char *filename = (char *) *state;
    char result_filename[1024] = {0};
    get_result_filename(filename, result_filename, sizeof(result_filename));

    FILE *file = fopen(filename, "rb");
    assert_non_null(file);

    envelope_t envelope;

    memset(&envelope, 0, sizeof(envelope_t));
    uint8_t data[MAX_ENVELOPE_SIZE];
    size_t read_count = fread(data, sizeof(char), MAX_ENVELOPE_SIZE, file);

    assert_true(parse_soroban_authorization_envelope(data, read_count, &envelope));

    char caption[MAX_CAPTION_SIZE];
    char value[MAX_VALUE_SIZE];
    uint8_t signing_key[] = {0xe9, 0x33, 0x88, 0xbb, 0xfd, 0x2f, 0xbd, 0x11, 0x80, 0x6d, 0xd0,
                             0xbd, 0x59, 0xce, 0xa9, 0x7,  0x9e, 0x7c, 0xc7, 0xc,  0xe7, 0xb1,
                             0xe1, 0x54, 0xf1, 0x14, 0xcd, 0xfe, 0x4e, 0x46, 0x6e, 0xcd};

    formatter_data_t fdata = {.raw_data = data,
                              .raw_data_len = read_count,
                              .envelope = &envelope,
                              .signing_key = signing_key,
                              .caption = caption,
                              .value = value,
                              .value_len = MAX_VALUE_SIZE,
                              .caption_len = MAX_CAPTION_SIZE,
                              .display_sequence = true};

    char output[1024] = {0};
    bool data_exists = true;
    bool is_op_header = false;
    reset_formatter();
    while (true) {
        assert_true(get_next_data(&fdata, true, &data_exists, &is_op_header));
        if (!data_exists) {
            break;
        }
        char temp[256] = {0};
        sprintf(temp,
                "%s;%s%s\n",
                fdata.caption,
                is_string_empty(fdata.value) ? "" : " ",
                fdata.value);
        strcat(output, temp);
    }

    char expected_result[1024] = {0};
    FILE *result_file = fopen(result_filename, "r");
    assert_non_null(result_file);
    fread(expected_result, sizeof(char), 1024, result_file);
    assert_string_equal(output, expected_result);

    fclose(file);
    fclose(result_file);
}

int main() {
    struct CMUnitTest tests[sizeof(testcases) / sizeof(testcases[0])];
    for (int i = 0; i < sizeof(testcases) / sizeof(testcases[0]); i++) {
        tests[i].name = testcases[i];
        tests[i].test_func = test_format_envelope;
        tests[i].initial_state = (void *) testcases[i];
        tests[i].setup_func = NULL;
        tests[i].teardown_func = NULL;
    }
    return cmocka_run_group_tests(tests, NULL, NULL);
}