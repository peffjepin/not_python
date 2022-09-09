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

bool str_eq(PYSTRING str1, PYSTRING str2);
PYSTRING str_add(PYSTRING str1, PYSTRING str2);
PYSTRING np_int_to_str(PYINT num);
PYSTRING np_float_to_str(PYFLOAT num);

void builtin_print(size_t argc, ...);

void* np_alloc(size_t bytes);
void* np_realloc(void* ptr, size_t bytes);
void np_free(void* ptr);

#endif
