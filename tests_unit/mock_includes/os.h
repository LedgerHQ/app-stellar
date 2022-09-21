#include <stdio.h>

#define PRINTF(...)
#define THROW(code)                \
    do {                           \
        printf("error: %d", code); \
    } while (0)
#define PIC(code) code
