#include <not_python.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SV_CHAR_AT(str, i) (str).data[(str).offset + i]

static void
out_of_memory(void)
{
    fprintf(stderr, "ERROR: out of memory\n");
    exit(1);
}

bool
str_eq(StringView str1, StringView str2)
{
    if (str1.length != str2.length) return false;
    for (size_t i = 0; i < str1.length; i++) {
        if (SV_CHAR_AT(str1, i) != SV_CHAR_AT(str2, i)) return false;
    }
    return true;
}

StringView
str_add(StringView str1, StringView str2)
{
    StringView str = {
        .data = np_alloc(str1.length + str2.length + 1),
        .length = str1.length + str2.length};
    if (!str.data) out_of_memory();
    memcpy(str.data, str1.data + str1.offset, str1.length);
    memcpy(str.data + str1.length, str2.data + str2.offset, str2.length);
    return str;
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

    va_end(vargs);

    fprintf(stdout, "\n");
    fflush(stdout);
}

// TODO: these are just stubs for now so I can keep track of where python
// allocs are coming from -- eventually these will need to be garbage collected
// Until I get around to implementing that all python allocs will leak

void*
np_alloc(size_t bytes)
{
    return calloc(1, bytes);
}
void*
np_realloc(void* ptr, size_t bytes)
{
    return realloc(ptr, bytes);
}
void
np_free(void* ptr)
{
    free(ptr);
}
