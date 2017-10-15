#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unsigned char PADDING_CHAR = '=';
static const unsigned char base32[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

static size_t min(size_t x, size_t y) {
	return x < y ? x : y;
}

static void pad(unsigned char *buf, int len) {
	for (int i = 0; i < len; i++)
		buf[i] = PADDING_CHAR;
}

static unsigned char shift_right(unsigned char byte, char offset) {
	return (offset > 0) ? byte >> offset : byte << -offset;
}

static unsigned char shift_left(unsigned char byte, char offset) {
	return shift_right(byte, -offset);
}

static void encode_sequence(const unsigned char *plain, int len, unsigned char *coded) {
	for (int block = 0; block < 8; block++) {
		int octet = (block * 5) / 8;
		int junk = (8 - 5 - (5 * block) % 8);

		if (octet >= len) {
			pad(&coded[block], 8 - block);
			return;
		}

		unsigned char c = shift_right(plain[octet], junk);

		if (junk < 0 &&  octet < len - 1) {
			c |= shift_right(plain[octet+1], 8 + junk);
		}
		coded[block] = base32[c & 0x1F];
	}
}

void base32_encode(const unsigned char *plain, size_t len, unsigned char *coded) {
	for (size_t i = 0, j = 0; i < len; i += 5, j += 8) {
		encode_sequence(&plain[i], min(len - i, 5), &coded[j]);
	}
}

unsigned short crc16(unsigned char *ptr, int count) {
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

void printHexBlocks(unsigned char *buffer, int size) {
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

uint32_t readUInt32Block(unsigned char *buffer) {
    return buffer[3] + (buffer[2] << 8) + (buffer[1] <<  16) + (buffer[0] << 24);
}

//uint64_t readUInt64Block(unsigned char *buffer) {
//    return buffer[3] + (buffer[2] << 8) + (buffer[1] <<  16) + (buffer[0] << 24);
//}

unsigned char* readAddress(unsigned char *buffer) {
    unsigned char *in = (unsigned char*) malloc(35);
    in[0] = 6 << 3;
    int i;
    for (i = 0; i < 32; i++) {
        in[i+1] = buffer[i];
    }
    unsigned short crc = crc16(in, 33);
    in[33] = crc;
    in[34] = crc >> 8;
    unsigned char *out = (unsigned char*) malloc(56);
    base32_encode(in, 35, out);
    return out;
}

void parseTxXdr(unsigned char *buffer, int size) {
    buffer += 8*4; // skip networkId
    buffer += 2*4; // skip envelopeType;
//    unsigned char *sourceAccount = readAddress((unsigned char*)buffer);
//    printf("%s", sourceAccount);
    buffer += 8*4; // skip sourceAccount
    uint32_t fee = readUInt32Block(buffer);
    printf("fee: %d\n", fee);
    buffer += 4; // skip fee
    buffer += 8; // skip seqNum
    uint32_t timeBounds = readUInt32Block(buffer);
    if (timeBounds != 0) {
       // todo: throw error
       printf("unexpected timeBounds");
       return;
    }
    buffer += 4; // skip timeBounds
    uint32_t memoType = readUInt32Block(buffer);
    if (memoType != 0) {
      // todo: throw error
      printf("unexpected memoType");
      return;
    }
    buffer += 4; // skip memoType
    uint32_t operationsCount = readUInt32Block(buffer);
    if (operationsCount != 1) {
        // todo: throw error
        printf("only one operation supported");
        return;
    }
    buffer += 2*4; // skip operationsCount
    uint32_t operationType = readUInt32Block(buffer);
    if (operationType != 1) {
        // todo: throw error
        printf("unsupported operation");
        return;
    }
    buffer += 2*4; // skip operationType
    unsigned char *destination = readAddress((unsigned char*)buffer);
    printf("destination: %s\n", destination);
    buffer += 8*4;
    uint32_t asset = readUInt32Block(buffer);
    if (asset != 0) {
        // todo: throw error
        printf("unsupported asset");
        return;
    }
    buffer += 2*4; // skip asset
    uint32_t amount = readUInt32Block(buffer);
    printf("amount: %d\n", amount/10000000);
    //    printf("next: %d\n", buffer[0]);
}

int main() {
   char *filename = "/tmp/txSignatureBase";
   unsigned char *buffer;

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

       buffer = (unsigned char*) malloc(read/2);
       int i;
       for (i = 0; i < read/2; i++) {
           sscanf(hex + 2*i, "%2hhx", &buffer[i]);
       }

//       printHexBlocks(buffer, read/2);

       parseTxXdr(buffer, read/2);

    } else {
        printf("No such file\n");
    }

    return 0;
}