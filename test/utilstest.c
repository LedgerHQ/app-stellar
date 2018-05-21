/*******************************************************************************
 *   Ledger Stellar App
 *   (c) 2017 Ledger
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
#include <stdio.h>
#include <string.h>
#include "stellar_api.h"

void test_print_amount(uint64_t amount, char *expected) {
    char *asset = "XLM";
    char printed[24];
    print_amount(amount, asset, printed);

    if (strcmp(printed, expected) != 0) {
        printf("test_print_amount failed. Expected: %s; Actual: %s\n", expected, printed);
    }
}

void test_print_uint(uint64_t i, char* expected) {
    char printed[24];
    print_uint(i, printed);
    if (strcmp(printed, expected) != 0) {
        printf("test_print_uint failed. Expected: %s; Actual: %s\n", expected, printed);
    }
}

void test_print_int(int64_t i, char* expected) {
    char printed[24];
    print_int(i, printed);
    if (strcmp(printed, expected) != 0) {
        printf("test_print_int failed. Expected: %s; Actual: %s\n", expected, printed);
    }
}

void test_print_summary(char *msg, char *expected, uint8_t numCharsL, uint8_t numCharsR) {
    char summery[27];
    print_summary(msg, summery, numCharsL, numCharsR);
    if (strcmp(summery, expected) != 0) {
        printf("test_print_summary failed. Expected: %s; Actual: %s\n", expected, summery);
    }
}

void test_print_binary(uint8_t *binary, char *expected) {
    char hex[16];
    print_binary_summary(binary, hex, 32);
    if (strcmp(hex, expected) != 0) {
        printf("test_print_hash failed. Expected: %s; Actual: %s\n", expected, hex);
    }
}

int main(int argc, char *argv[]) {

    test_print_amount(1, "0.0000001 XLM");
    test_print_amount(10000000, "1 XLM");
    test_print_amount(100000000000001, "10000000.0000001 XLM");
    test_print_amount(100000001, "10.0000001 XLM");
    test_print_amount(100000001000000, "10000000.1 XLM");

    test_print_uint(1230, "1230");

    test_print_int(1230, "1230");
    test_print_int(-1230, "-1230");

    test_print_summary("GADFVW3UXVKDOU626XUPYDJU2BFCGFJHQ6SREYOZ6IJV4XSHOALEQN2I", "GADFVW3UXVKD..4XSHOALEQN2I", 12, 12);
    test_print_summary("GADFVW3UXVKDOU626XUPYDJU2BFCGFJHQ6SREYOZ6IJV4XSHOALEQN2I", "GADFVW..LEQN2I", 6, 6);

    uint8_t binary[32];
    uint8_t i;
    for (i = 0; i < 32; i++) {
        binary[i] = i;
    }
    test_print_binary(binary, "0x00010203..1C1D1E1F");

    return 0;

}
