#ifndef NOT_PYTHON_H
#define NOT_PYTHON_H

#include <stdbool.h>
#include <stddef.h>

#define PYINT long long int
#define PYFLOAT double
#define PYSTRING StringView

typedef struct {
    char* data;
    size_t offset;
    size_t length;
} StringView;

bool str_eq(StringView str1, StringView str2);
StringView str_add(StringView str1, StringView str2);

void builtin_print(size_t argc, ...);

void* np_alloc(size_t bytes);
void* np_realloc(void* ptr, size_t bytes);
void np_free(void* ptr);

#endif
