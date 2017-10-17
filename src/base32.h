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

#include <stddef.h>

/**
 * Returns the length of the output buffer required to encode len bytes of
 * data into base32. This is a macro to allow users to define buffer size at
 * compilation time.
 */
#define BASE32_LEN(len)  (((len)/5)*8 + ((len) % 5 ? 8 : 0))

/**
 * Encode the data pointed to by plain into base32 and store the
 * result at the address pointed to by coded. The "coded" argument
 * must point to a location that has enough available space
 * to store the whole coded string. The resulting string will only
 * contain characters from the [A-Z2-7=] set. The "len" arguments
 * define how many bytes will be read from the "plain" buffer.
 **/
void base32_encode(const unsigned char *plain, size_t len, unsigned char *coded);
