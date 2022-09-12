#include <not_python.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syntax.h"

#define SV_CHAR_AT(str, i) (str).data[(str).offset + i]

// TODO: error messages and error handlers with try blocks
void
memory_error(void)
{
    fprintf(stderr, "ERROR: out of memory\n");
    exit(1);
}

void
index_error(void)
{
    fprintf(stderr, "ERROR: index error\n");
    exit(1);
}

void
value_error(void)
{
    fprintf(stderr, "ERROR: value error\n");
    exit(1);
}

PYBOOL
str_eq(PYSTRING str1, PYSTRING str2)
{
    if (str1.length != str2.length) return false;
    for (size_t i = 0; i < str1.length; i++) {
        if (SV_CHAR_AT(str1, i) != SV_CHAR_AT(str2, i)) return false;
    }
    return true;
}

PYBOOL
str_gt(PYSTRING str1, PYSTRING str2)
{
    if (str1.length == 0) return false;

    size_t minlen = (str1.length < str2.length) ? str1.length : str2.length;
    for (size_t i = 0; i < minlen; i++) {
        char c1 = SV_CHAR_AT(str1, i);
        char c2 = SV_CHAR_AT(str2, i);
        if (c1 != c2) return c1 > c2;
    }

    return str1.length > str2.length;
}

PYBOOL
str_gte(PYSTRING str1, PYSTRING str2)
{
    if (str1.length == 0) return str2.length == 0;

    size_t minlen = (str1.length < str2.length) ? str1.length : str2.length;
    for (size_t i = 0; i < minlen; i++) {
        char c1 = SV_CHAR_AT(str1, i);
        char c2 = SV_CHAR_AT(str2, i);
        if (c1 != c2) return c1 > c2;
    }

    return str1.length >= str2.length;
}

PYBOOL
str_lt(PYSTRING str1, PYSTRING str2)
{
    if (str1.length == 0) return str2.length > 0;

    size_t minlen = (str1.length < str2.length) ? str1.length : str2.length;
    for (size_t i = 0; i < minlen; i++) {
        char c1 = SV_CHAR_AT(str1, i);
        char c2 = SV_CHAR_AT(str2, i);
        if (c1 != c2) return c1 < c2;
    }

    return str1.length < str2.length;
}

PYBOOL
str_lte(PYSTRING str1, PYSTRING str2)
{
    if (str1.length == 0) return true;

    size_t minlen = (str1.length < str2.length) ? str1.length : str2.length;
    for (size_t i = 0; i < minlen; i++) {
        char c1 = SV_CHAR_AT(str1, i);
        char c2 = SV_CHAR_AT(str2, i);
        if (c1 != c2) return c1 < c2;
    }

    return str1.length <= str2.length;
}

PYSTRING
str_add(PYSTRING str1, PYSTRING str2)
{
    PYSTRING str = {
        .data = np_alloc(str1.length + str2.length + 1),
        .length = str1.length + str2.length};
    memcpy(str.data, str1.data + str1.offset, str1.length);
    memcpy(str.data + str1.length, str2.data + str2.offset, str2.length);
    return str;
}

PYSTRING
np_int_to_str(PYINT num)
{
    size_t required_length = snprintf(NULL, 0, "%lli", num);
    PYSTRING str = {.data = np_alloc(required_length + 1), .length = required_length};
    snprintf(str.data, required_length + 1, "%lli", num);
    return str;
}

PYSTRING
np_float_to_str(PYFLOAT num)
{
    size_t required_length = snprintf(NULL, 0, "%f", num);
    PYSTRING str = {.data = np_alloc(required_length + 1), .length = required_length};
    snprintf(str.data, required_length + 1, "%f", num);
    return str;
}

const PYSTRING true_str = {.data = "True", .length = 4};
const PYSTRING false_str = {.data = "False", .length = 5};

PYSTRING
np_bool_to_str(PYBOOL value) { return (value) ? true_str : false_str; }

void
builtin_print(size_t argc, ...)
{
    va_list vargs;
    va_start(vargs, argc);

    for (size_t i = 0; i < argc; i++) {
        if (i > 0) fprintf(stdout, " ");
        PYSTRING str = va_arg(vargs, PYSTRING);
        fprintf(stdout, "%.*s", (int)str.length, str.data + str.offset);
    }

    va_end(vargs);

    fprintf(stdout, "\n");
    fflush(stdout);
}

List*
list_add(List* list1, List* list2)
{
    // TODO: it would be better to allocate a large enough list to begin with and copy
    // into it.
    List* new_list = list_copy(list1);
    list_extend(new_list, list2);
    return new_list;
}

void
list_clear(List* list)
{
    list->count = 0;
    list->capacity = LIST_MIN_CAPACITY;
    list->data = np_realloc(list->data, list->element_size * list->capacity);
}

List*
list_copy(List* list)
{
    List* new_list = np_alloc(sizeof(List));
    new_list->count = list->count;
    new_list->capacity = list->capacity;
    new_list->element_size = list->element_size;
    size_t bytes = list->capacity * list->element_size;
    new_list->data = np_alloc(bytes);
    memcpy(new_list->data, list->data, bytes);
    return new_list;
}

void
list_extend(List* list, List* other)
{
    size_t required_capacity = list->count + other->count + 1;
    if (list->capacity < required_capacity)
        np_realloc(list->data, list->element_size * required_capacity);
    memcpy(
        (uint8_t*)list->data + list->element_size * list->count,
        other->data,
        other->element_size * other->count
    );
    list->count += other->count;
}

void
list_shrink(List* list)
{
    if (list->capacity == LIST_MIN_CAPACITY) return;
    size_t new_capacity = list->capacity * LIST_SHRINK_FACTOR;
    if (new_capacity < LIST_MIN_CAPACITY) new_capacity = LIST_MIN_CAPACITY;
    list->capacity = new_capacity;
    list->data = np_realloc(list->data, list->element_size * list->capacity);
}

void
list_del(List* list, PYINT index)
{
    // assume bounds checking has already occured
    memmove(
        (uint8_t*)list->data + (list->element_size * index),
        (uint8_t*)list->data + (list->element_size * (index + 1)),
        list->element_size * (list->count - index + 1)
    );
    list->count -= 1;
    if (list->count <= list->capacity * LIST_SHRINK_THRESHOLD) list_shrink(list);
}

void
list_grow(List* list)
{
    list->capacity *= 2;
    list->data = np_realloc(list->data, list->element_size * list->capacity);
}

void
list_reverse(List* list)
{
    uint8_t tmp[list->element_size];
    for (size_t i = 0; i < list->count / 2; i++) {
        uint8_t* left = (uint8_t*)list->data + (list->element_size * i);
        uint8_t* right =
            (uint8_t*)list->data + (list->element_size * (list->count - 1 - i));
        memcpy(tmp, right, list->element_size);
        memcpy(right, left, list->element_size);
        memcpy(left, tmp, list->element_size);
    }
}

List*
np_internal_list_init(size_t elem_size)
{
    List* list = np_alloc(sizeof(List));
    list->count = 0;
    list->capacity = LIST_MIN_CAPACITY;
    list->element_size = elem_size;
    list->data = np_alloc(elem_size * list->capacity);
    return list;
}

void
np_internal_list_prepare_insert(List* list, PYINT index)
{
    if (list->count == list->capacity - 1) list_grow(list);
    list->count += 1;
    memmove(
        (uint8_t*)list->data + (list->element_size * (index + 1)),
        (uint8_t*)list->data + (list->element_size * index),
        list->element_size * (list->count - index)
    );
}

// TODO: these are just stubs for now so I can keep track of where python
// allocs are coming from -- eventually these will need to be garbage collected
// Until I get around to implementing that all python allocs will leak

void*
np_alloc(size_t bytes)
{
    void* ptr = calloc(1, bytes);
    if (!ptr) memory_error();
    return ptr;
}
void*
np_realloc(void* ptr, size_t bytes)
{
    void* newptr = realloc(ptr, bytes);
    if (!newptr) memory_error();
    return newptr;
}
void
np_free(void* ptr)
{
    free(ptr);
}
