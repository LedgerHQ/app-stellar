/*******************************************************************************
 *   Ledger Stellar App
 *   (c) 2017 LeNonDupe
 *
 *  adapted from https://github.com/mjg59/tpmtotp/blob/master/base32.h
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
#include <stdlib.h>

int read_file(char *filename, char *buffer, int maxlen) {
   int size, read;
   FILE *handler = fopen(filename, "r");

   if (handler) {

       // calculate the needed buffer size
       fseek(handler, 0, SEEK_END);
       size = ftell(handler);
       rewind(handler);

       if (size/2 > maxlen) {
           printf("Buffer too small\n");
       }

       char *hex = (char*) malloc(size);
       read = fread(hex, 1, size, handler);
       fclose(handler);

       if (size != read) {
           printf("Failed to read file\n");
           free(hex);
           hex = NULL;
           return 0;
       }

       int i;
       for (i = 0; i < maxlen; i++) {
           sscanf(hex + 2*i, "%2hhx", &buffer[i]);
       }
       return read;
    } else {
        printf("No such file\n");
        return 0;
    }
}

void printHexBlocks(char *buffer, int size) {
    int i = 0;
    for (i = 0; i < size; i++) {
        if (i % 4 == 0) {
            if (i > 0) {
                printf("]");
            }
            printf("\n[");
        } else if (i > 0) {
            printf(":");
        }
        printf("%02hhx", buffer[i]);
    }
    printf("]\n");
}
