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

#include "xdr_parser.h"
#include "test_utils.h"

int main(int argc, char *argv[]) {

    if(argc != 2 ) {
        printf("Expected argument missing\n");
        return 1;
    }

    char *filename = argv[1];
    uint8_t buffer[4096];
    tx_content_t txContent;

    int read = read_file(filename, buffer, 4096);
    if (read) {
//        printHexBlocks(buffer, read/2);
        parseTxXdr(buffer, &txContent);
    }

    return 0;

}
