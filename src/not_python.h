#ifndef NOT_PYTHON_H
#define NOT_PYTHON_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char* data;
    size_t offset;
    size_t length;
} StringView;

bool str_eq(StringView str1, StringView str2);

#define PYINT long long int
#define PYFLOAT double
#define PYSTRING StringView

#endif