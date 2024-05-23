#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "base64.h"

static const char BASE64_ALPHABET[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int BASE64_MOD_TABLE[] = {0, 2, 1};

bool base64_encode(const uint8_t *data, size_t in_len, char *out, size_t out_len) {
    size_t encoded_len = 4 * ((in_len + 2) / 3);
    if (encoded_len > out_len) {
        return false;
    }

    for (unsigned int i = 0, j = 0; i < in_len;) {
        uint32_t octet_a = i < in_len ? data[i++] : 0;
        uint32_t octet_b = i < in_len ? data[i++] : 0;
        uint32_t octet_c = i < in_len ? data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        out[j++] = BASE64_ALPHABET[(triple >> 3 * 6) & 0x3F];
        out[j++] = BASE64_ALPHABET[(triple >> 2 * 6) & 0x3F];
        out[j++] = BASE64_ALPHABET[(triple >> 1 * 6) & 0x3F];
        out[j++] = BASE64_ALPHABET[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < BASE64_MOD_TABLE[in_len % 3]; i++) {
        out[encoded_len - 1 - i] = '=';
    }

    out[encoded_len] = '\0';
    return true;
}
