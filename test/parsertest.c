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
#include <string.h>
#include <stdint.h>

typedef struct txContent_t {
    char source[57];
    char destination[57];
    uint64_t amount;
    uint32_t fee;
} txContent_t;

static const char PADDING_CHAR = '=';
static const char base32[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

static size_t min(size_t x, size_t y) {
	return x < y ? x : y;
}

static void pad(char *buf, int len) {
	int i;
	for (i = 0; i < len; i++)
		buf[i] = PADDING_CHAR;
}

static char shift_right(char byte, char offset) {
	return (offset > 0) ? byte >> offset : byte << -offset;
}

static void encode_sequence(const char *plain, int len, char *coded) {
	int block;
	for (block = 0; block < 8; block++) {
		int octet = (block * 5) / 8;
		int junk = (8 - 5 - (5 * block) % 8);

		if (octet >= len) {
			pad(&coded[block], 8 - block);
			return;
		}

		char c = shift_right(plain[octet], junk);

		if (junk < 0 &&  octet < len - 1) {
			c |= shift_right(plain[octet+1], 8 + junk);
		}
		coded[block] = base32[c & 0x1F];
	}
}

void base32_encode(const char *plain, size_t len, char *coded) {
	size_t i, j;
	for (i = 0, j = 0; i < len; i += 5, j += 8) {
		encode_sequence(&plain[i], min(len - i, 5), &coded[j]);
	}
}

short crc16(char *ptr, int count) {
   int  crc;
   char i;
   crc = 0;
   while (--count >= 0) {
      crc = crc ^ (int) *ptr++ << 8;
      i = 8;
      do
      {
         if (crc & 0x8000)
            crc = crc << 1 ^ 0x1021;
         else
            crc = crc << 1;
      } while(--i);
   }
   return (crc);
}

/**
 * convert the raw public key to a stellar address
 */
void public_key_to_address(char *in, char *out) {
    char *buffer = (char*) malloc(35);
    buffer[0] = 6 << 3; // version bit 'G'
    int i;
    for (i = 0; i < 32; i++) {
        buffer[i+1] = in[i];
    }
    short crc = crc16(buffer, 33); // checksum
    buffer[33] = crc;
    buffer[34] = crc >> 8;
    base32_encode(buffer, 35, out);
    out[56] = '\0';
}

void summarize_address(char *in, char *out) {
    strncpy(out, in, 5);
    strncpy(out + 5, "...", 3);
    strncpy(out + 8, in + 52, 5);
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

static const uint8_t ASSET_TYPE_NATIVE = 0;
static const uint8_t PUBLIC_KEY_TYPE_ED25519 = 0;

uint32_t readUInt32Block(char *buffer) {
    return buffer[3] + (buffer[2] << 8) + (buffer[1] <<  16) + (buffer[0] << 24);
}

uint64_t readUInt64Block(char *buffer) {
    return buffer[7] + (buffer[6] << 8) + (buffer[5] <<  16) + (buffer[4] << 24)
        + buffer[3] + (buffer[2] << 8) + (buffer[1] <<  16) + (buffer[0] << 24);
}

void parsePaymentOpXdr(char *buffer, txContent_t *txContent) {
    uint32_t destinationAccountType = readUInt32Block(buffer);
    if (destinationAccountType != PUBLIC_KEY_TYPE_ED25519) {
        printf("unsupported destination account type");
        return;
    }
    buffer += 4;
    public_key_to_address((char*)buffer, txContent->destination);
    printf("destination: %s\n", txContent->destination);
    char address_summary[14];
    summarize_address(txContent->destination, address_summary);
    printf("summary: %s\n", address_summary);
    buffer += 8*4;
    uint32_t asset = readUInt32Block(buffer);
    if (asset != ASSET_TYPE_NATIVE) {
        printf("only native assets supported");
        return;
    }
    buffer += 4;
    txContent->amount = readUInt64Block(buffer);
    char amount_summary[32];
    snprintf(amount_summary, 32, "%.2f", (float)txContent->amount/10000000);
    printf("amount: %s", amount_summary);
}

void parseOpXdr(char *buffer, txContent_t *txContent) {
    uint32_t hasAccountId = readUInt32Block(buffer);
    if (hasAccountId) {
        printf("account id inside operation not supported");
        return;
    }
    buffer += 4;
    uint32_t operationType = readUInt32Block(buffer);
    if (operationType != 1) {
        printf("operation type not supported");
        return;
    }
    buffer += 4;
    parsePaymentOpXdr(buffer, txContent);
}

void parseOpsXdr(char *buffer, txContent_t *txContent) {
    uint32_t operationsCount = readUInt32Block(buffer);
    if (operationsCount != 1) {
        printf("only single operation supported");
        return;
    }
    buffer += 4;
    parseOpXdr(buffer, txContent);
}

void parseTxXdr(char *buffer, int size, txContent_t *txContent) {
    buffer += 8*4; // skip networkId
    buffer += 4; // skip envelopeType
    uint32_t sourceAccountType = readUInt32Block(buffer);
    if (sourceAccountType != PUBLIC_KEY_TYPE_ED25519) {
        printf("unsupported destination account type");
        return;
    }
    buffer += 4;
    public_key_to_address((char*)buffer, txContent->source);
    printf("source: %s\n", txContent->source);
    buffer += 8*4;
    txContent->fee = readUInt32Block(buffer);
    char fee_summary[32];
    snprintf(fee_summary, 32, "%f", (float)txContent->fee/10000000);
    printf("fee: %s\n", fee_summary);
    buffer += 4;
    buffer += 8; // skip seqNum
    uint32_t timeBounds = readUInt32Block(buffer);
    if (timeBounds != 0) {
        printf("time bounds not supported");
        return;
    }
    buffer += 4;
    uint32_t memoType = readUInt32Block(buffer);
    if (memoType != 0) {
        printf("memo type not supported");
        return;
    }
    buffer += 4; // skip memoType
    parseOpsXdr(buffer, txContent);
}

int main() {
   char *filename = "/tmp/txSignatureBase";
   char *buffer;
   txContent_t txContent;

   int size, read;
   FILE *handler = fopen(filename, "r");

   if (handler) {

       // calculate the needed buffer size
       fseek(handler, 0, SEEK_END);
       size = ftell(handler);
       rewind(handler);

       char *hex = (char*) malloc(size);
       read = fread(hex, 1, size, handler);
       fclose(handler);

       if (size != read) {
           printf("Failed to read file\n");
           free(hex);
           hex = NULL;
           return 1;
       }

       buffer = (char*) malloc(read/2);
       int i;
       for (i = 0; i < read/2; i++) {
           sscanf(hex + 2*i, "%2hhx", &buffer[i]);
       }

//       printHexBlocks(buffer, read/2);

       parseTxXdr(buffer, read/2, &txContent);

    } else {
        printf("No such file\n");
    }

    return 0;
}

