#include "not_python.h"

#include <stdio.h>

#define SV_CHAR_AT(str, i) (str).data[(str).offset + i]

bool
str_eq(StringView str1, StringView str2)
{
    if (str1.length != str2.length) return false;
    for (size_t i = 0; i < str1.length; i++) {
        if (SV_CHAR_AT(str1, i) != SV_CHAR_AT(str2, i)) return false;
    }
    return true;
}

void
builtin_print(StringView str)
{
    fwrite(str.data, str.length, 1, stdout);
    fflush(stdout);
}
