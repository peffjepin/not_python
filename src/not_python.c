#include <not_python.h>
#include <stdarg.h>
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
builtin_print(size_t argc, ...)
{
    va_list vargs;
    va_start(vargs, argc);

    for (size_t i = 0; i < argc; i++) {
        if (i > 0) fprintf(stdout, " ");
        StringView str = va_arg(vargs, StringView);
        fprintf(stdout, "%.*s", (int)str.length, str.data + str.offset);
    }
    fprintf(stdout, "\n");
    fflush(stdout);
}
