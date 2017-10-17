/*******************************************************************************
*   Ledger Stellar App
*   (c) 2017 LeNonDupe
*
*  adapted from https://github.com/mjg59/tpmtotp/blob/master/base32.c
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

#include "base32.h"

static const unsigned char PADDING_CHAR = '=';
static const unsigned char base32[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

static size_t min(size_t x, size_t y) {
	return x < y ? x : y;
}

static void pad(unsigned char *buf, int len) {
	int i;
	for (i = 0; i < len; i++)
		buf[i] = PADDING_CHAR;
}

static unsigned char shift_right(unsigned char byte, char offset) {
	return (offset > 0) ? byte >> offset : byte << -offset;
}

static void encode_sequence(const unsigned char *plain, int len, unsigned char *coded) {
	int block;
	for (block = 0; block < 8; block++) {
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
	size_t i, j;
	for (i = 0, j = 0; i < len; i += 5, j += 8) {
		encode_sequence(&plain[i], min(len - i, 5), &coded[j]);
	}
}
