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
#include "stlr_utils.h"

void test_print_amount(uint64_t amount, char *expected) {
    char *asset = "XLM";
    char printed[24];
    print_amount(amount, asset, printed, 22);

    if (strcmp(printed, expected) != 0) {
        printf("test_print_amount failed. Expected: %s; Actual: %s\n", expected, printed);
    }
}

void test_print_id(uint64_t id, char* expected) {
    char printed[24];
    print_id(id, printed, 22);
    if (strcmp(printed, expected) != 0) {
        printf("test_print_id_memo failed. Expected: %s; Actual: %s\n", expected, printed);
    }
}

void test_print_summary(char *msg, char *expected) {
    char summery[16];
    print_summary(msg, summery);
    if (strcmp(summery, expected) != 0) {
        printf("test_summerize_message failed. Expected: %s; Actual: %s\n", expected, summery);
    }
}

void test_print_hash(uint8_t *hash, char *expected) {
    char hex[16];
    print_hash_summary(hash, hex);
    if (strcmp(hex, expected) != 0) {
        printf("test_print_hash failed. Expected: %s; Actual: %s\n", expected, hex);
    }
}

void test_print_operation_type(uint8_t type, char *expected) {
    char s[15];
    print_operation_type(type, s);
    if (strcmp(s, expected) != 0) {
        printf("test_print_operation_type failed. Expected: %s; Actual: %s\n", expected, s);
    }
}

int main(int argc, char *argv[]) {

    test_print_amount(1, "0.0000001 XLM");
    test_print_amount(10000000, "1 XLM");
    test_print_amount(100000000000001, "10000000.0000001 XLM");
    test_print_amount(100000001, "10.0000001 XLM");
    test_print_amount(100000001000000, "10000000.1 XLM");

    test_print_id(1, "1");
    test_print_id(12, "12");
    test_print_id(100, "100");

    test_print_summary("sending starlight", "sendin...light");
    test_print_summary("GBGBTCCP7WG2E5XFYLQFJP2DYOQZPCCDCHK62K6TZD4BHMNYI5WSXESH", "GBGBTC...SXESH");

    uint8_t hash[64];
    uint8_t i;
    for (i = 0; i < 64; i++) {
        hash[i] = i;
    }
    test_print_hash(hash, "000102...3D3E3F");
    test_print_operation_type(14, "Unknown");
    return 0;

}

