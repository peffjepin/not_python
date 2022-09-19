#include <not_python.h>
#include <np_hash.h>
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

void
key_error(void)
{
    fprintf(stderr, "ERROR: key error\n");
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
str_fmt(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t required_length = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    PYSTRING str = {.data = np_alloc(required_length + 1), .length = required_length};

    va_start(args, fmt);
    vsnprintf(str.data, required_length + 1, fmt, args);
    va_end(args);

    return str;
}

char*
np_str_to_cstr(PYSTRING str)
{
    if (str.data[str.offset + str.length] == '\0') {
        return str.data + str.offset;
    }
    char* cstr = np_alloc(str.length + 1);
    memcpy(cstr, str.data + str.offset, str.length);
    return cstr;
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

void
list_sort(List* list, int (*cmp_fn)(const void*, const void*))
{
    qsort(list->data, list->count, list->element_size, cmp_fn);
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

int
pyint_sort_fn(const void* elem1, const void* elem2)
{
    if (*(PYINT*)elem1 < *(PYINT*)elem2) return -1;
    if (*(PYINT*)elem1 > *(PYINT*)elem2) return 1;
    return 0;
}

int
pyfloat_sort_fn(const void* elem1, const void* elem2)
{
    if (*(PYFLOAT*)elem1 < *(PYFLOAT*)elem2) return -1;
    if (*(PYFLOAT*)elem1 > *(PYFLOAT*)elem2) return 1;
    return 0;
}

int
pybool_sort_fn(const void* elem1, const void* elem2)
{
    if (*(PYBOOL*)elem1 < *(PYBOOL*)elem2) return -1;
    if (*(PYBOOL*)elem1 > *(PYBOOL*)elem2) return 1;
    return 0;
}

int
ptstr_sort_fn(const void* elem1, const void* elem2)
{
    if (str_lt(*(PYSTRING*)elem1, *(PYSTRING*)elem2)) return -1;
    if (str_gt(*(PYSTRING*)elem1, *(PYSTRING*)elem2)) return 1;
    return 0;
}

int
pyint_sort_fn_rev(const void* elem1, const void* elem2)
{
    if (*(PYINT*)elem1 < *(PYINT*)elem2) return 1;
    if (*(PYINT*)elem1 > *(PYINT*)elem2) return -1;
    return 0;
}

int
pyfloat_sort_fn_rev(const void* elem1, const void* elem2)
{
    if (*(PYFLOAT*)elem1 < *(PYFLOAT*)elem2) return 1;
    if (*(PYFLOAT*)elem1 > *(PYFLOAT*)elem2) return -1;
    return 0;
}

int
pybool_sort_fn_rev(const void* elem1, const void* elem2)
{
    if (*(PYBOOL*)elem1 < *(PYBOOL*)elem2) return 1;
    if (*(PYBOOL*)elem1 > *(PYBOOL*)elem2) return -1;
    return 0;
}

int
ptstr_sort_fn_rev(const void* elem1, const void* elem2)
{
    if (str_lt(*(PYSTRING*)elem1, *(PYSTRING*)elem2)) return 1;
    if (str_gt(*(PYSTRING*)elem1, *(PYSTRING*)elem2)) return -1;
    return 0;
}

PYBOOL
int_eq(PYINT int1, PYINT int2) { return int1 == int2; }
PYBOOL
float_eq(PYFLOAT float1, PYFLOAT float2) { return float1 == float2; }
PYBOOL
bool_eq(PYBOOL bool1, PYBOOL bool2) { return bool1 == bool2; }

PYBOOL
void_int_eq(void* int1, void* int2) { return *(PYINT*)int1 == *(PYINT*)int2; }
PYBOOL
void_float_eq(void* float1, void* float2)
{
    return *(PYFLOAT*)float1 == *(PYFLOAT*)float2;
}
PYBOOL
void_bool_eq(void* bool1, void* bool2) { return *(PYBOOL*)bool1 == *(PYBOOL*)bool2; }
PYBOOL
void_str_eq(void* str1, void* str2) { return str_eq(*(PYSTRING*)str1, *(PYSTRING*)str2); }

typedef struct {
    Dict* dict;
    size_t yielded;
    size_t item_idx;
} DictIter;

void*
dict_keys_next(void* iter)
{
    DictIter* iterd = iter;
    if (iterd->yielded == iterd->dict->count) return NULL;
    size_t current_item_offset = iterd->item_idx * iterd->dict->item_size;
    while (!iterd->dict->data[current_item_offset]) {
        iterd->item_idx += 1;
        current_item_offset += iterd->dict->item_size;
    }
    iterd->item_idx += 1;
    iterd->yielded += 1;
    return (void*)(iterd->dict->data + current_item_offset + iterd->dict->key_offset);
}

Iterator
dict_iter_keys(Dict* dict)
{
    // TODO: empty iterable is probably a bug
    DictIter* iter = np_alloc(sizeof(DictIter));
    iter->dict = dict;
    Iterator iterd = {.next = (ITER_NEXT_FN)dict_keys_next, .iter = iter};
    return iterd;
}

void*
dict_vals_next(void* iter)
{
    DictIter* iterd = iter;
    if (iterd->yielded == iterd->dict->count) return NULL;
    size_t current_item_offset = iterd->item_idx * iterd->dict->item_size;
    while (!iterd->dict->data[current_item_offset]) {
        iterd->item_idx += 1;
        current_item_offset += iterd->dict->item_size;
    }
    iterd->item_idx += 1;
    iterd->yielded += 1;
    return (void*)(iterd->dict->data + current_item_offset + iterd->dict->val_offset);
}

Iterator
dict_iter_vals(Dict* dict)
{
    // TODO: empty iterable is probably a bug
    DictIter* iter = np_alloc(sizeof(DictIter));
    iter->dict = dict;
    Iterator iterd = {.next = (ITER_NEXT_FN)dict_vals_next, .iter = iter};
    return iterd;
}

typedef struct {
    Dict* dict;
    size_t yielded;
    size_t item_idx;
    DictItem current_item;
} DictItemsIter;

void*
dict_items_next(void* iter)
{
    DictItemsIter* iterd = iter;
    if (iterd->yielded == iterd->dict->count) return NULL;
    size_t current_item_offset = iterd->item_idx * iterd->dict->item_size;
    while (!iterd->dict->data[current_item_offset]) {
        iterd->item_idx += 1;
        current_item_offset += iterd->dict->item_size;
    }
    iterd->item_idx += 1;
    iterd->yielded += 1;
    iterd->current_item.key =
        (void*)(iterd->dict->data + current_item_offset + iterd->dict->key_offset);
    iterd->current_item.val =
        (void*)(iterd->dict->data + current_item_offset + iterd->dict->val_offset);
    return (void*)&iterd->current_item;
}

Iterator
dict_iter_items(Dict* dict)
{
    DictItemsIter* iter = np_alloc(sizeof(DictItemsIter));
    iter->dict = dict;
    Iterator iterd = {.next = (ITER_NEXT_FN)dict_items_next, .iter = iter};
    return iterd;
}

Dict*
dict_init(size_t key_size, size_t val_size, DICT_KEYCMP_FUNCTION cmp)
{
    Dict* dict = np_alloc(sizeof(Dict));

    dict->keycmp = cmp;
    dict->key_size = key_size;
    dict->val_size = val_size;
    dict->item_size = 1 + key_size + val_size;
    dict->key_offset = 1;
    dict->val_offset = 1 + key_size;

    dict->capacity = DICT_MIN_CAPACITY;
    dict->lut_capacity = dict->capacity * DICT_LUT_FACTOR;
    dict->count = 0;
    dict->data = np_alloc(dict->item_size * dict->capacity);

    size_t lut_bytes = dict->lut_capacity * sizeof(int);
    dict->lut = np_alloc(lut_bytes);
    memset(dict->lut, -1, lut_bytes);

    return dict;
}

Dict*
dict_copy(Dict* other)
{
    Dict* dict = np_alloc(sizeof(Dict));
    memcpy(dict, other, sizeof(Dict));

    size_t data_bytes = dict->item_size * dict->capacity;
    size_t lut_bytes = dict->lut_capacity * sizeof(int);
    dict->data = np_alloc(data_bytes);
    dict->lut = np_alloc(lut_bytes);
    memcpy(dict->data, other->data, data_bytes);
    memcpy(dict->lut, other->lut, lut_bytes);

    return dict;
}

void
dict_clear(Dict* dict)
{
    np_free(dict->data);
    np_free(dict->lut);

    dict->capacity = DICT_MIN_CAPACITY;
    dict->lut_capacity = dict->capacity * DICT_LUT_FACTOR;
    dict->count = 0;
    dict->tombstone_count = 0;
    dict->data = np_alloc(dict->item_size * dict->capacity);

    size_t lut_bytes = dict->lut_capacity * sizeof(int);
    dict->lut = np_alloc(lut_bytes);
    memset(dict->lut, -1, lut_bytes);
}

#define DICT_EFFECTIVE_COUNT(dict) dict->count + dict->tombstone_count
#define DICT_FULL(dict) DICT_EFFECTIVE_COUNT(dict) == dict->capacity
#define DICT_KEY_AT(dict, idx) dict->data + (idx * dict->item_size) + dict->key_offset

size_t
dict_find_lut_location(Dict* dict, void* key, int* lut_value)
{
    uint64_t hash = hash_bytes(key, dict->key_size);
    size_t probe = hash % dict->lut_capacity;

    for (;;) {
        int index = dict->lut[probe];
        if (index < 0 || dict->keycmp(key, DICT_KEY_AT(dict, index))) {
            *lut_value = index;
            return probe;
        }
        probe = (probe + 1) % dict->lut_capacity;
    }
}

void
dict_lut_rehash_item(Dict* dict, void* key, size_t item_index)
{
    uint64_t hash = hash_bytes(key, dict->key_size);
    size_t probe = hash % dict->lut_capacity;

    for (;;) {
        int index = dict->lut[probe];
        if (index < 0) {
            dict->lut[probe] = item_index;
            return;
        }
        probe = (probe + 1) % dict->lut_capacity;
    }
}

void
dict_grow(Dict* dict)
{
    dict->capacity *= DICT_GROW_FACTOR;
    dict->lut_capacity *= DICT_GROW_FACTOR;
    dict->data = np_realloc(dict->data, sizeof(dict->item_size) * dict->capacity);
    dict->lut = np_realloc(dict->lut, sizeof(int) * dict->lut_capacity);
    memset(dict->lut, -1, sizeof(int) * dict->lut_capacity);

    uint8_t* write = dict->data;
    size_t update_count = 0;
    for (size_t i = 0; i < DICT_EFFECTIVE_COUNT(dict); i++) {
        // update data array
        size_t item_offset = i * dict->item_size;
        if (!dict->data[item_offset]) continue;  // tombstone
        write[0] = 1;
        memmove(
            write + dict->key_offset,
            dict->data + item_offset + dict->key_offset,
            dict->key_size
        );
        memmove(
            write + dict->val_offset,
            dict->data + item_offset + dict->val_offset,
            dict->val_size
        );
        write += dict->item_size;

        // update lut
        dict_lut_rehash_item(
            dict, dict->data + item_offset + dict->key_offset, update_count++
        );
    }

    dict->tombstone_count = 0;
}

void
dict_shrink(Dict* dict)
{
    dict->capacity *= DICT_SHRINK_FACTOR;
    if (dict->capacity < DICT_MIN_CAPACITY) dict->capacity = DICT_MIN_CAPACITY;
    dict->lut_capacity = dict->capacity * DICT_LUT_FACTOR;
    uint8_t* old_data = dict->data;
    dict->data = np_alloc(sizeof(dict->item_size) * dict->capacity);
    dict->lut = np_realloc(dict->lut, sizeof(int) * dict->lut_capacity);
    memset(dict->lut, -1, sizeof(int) * dict->lut_capacity);

    uint8_t* write = dict->data;
    size_t update_count = 0;
    for (size_t i = 0; i < DICT_EFFECTIVE_COUNT(dict); i++) {
        // update data array
        size_t item_offset = i * dict->item_size;
        if (!old_data[item_offset]) continue;  // tombstone
        write[0] = 1;
        memcpy(
            write + dict->key_offset,
            old_data + item_offset + dict->key_offset,
            dict->key_size
        );
        memcpy(
            write + dict->val_offset,
            old_data + item_offset + dict->val_offset,
            dict->val_size
        );
        write += dict->item_size;

        // update lut
        dict_lut_rehash_item(
            dict, old_data + item_offset + dict->key_offset, update_count++
        );
    }

    np_free(old_data);
    dict->tombstone_count = 0;
}

void
dict_set_item(Dict* dict, void* key, void* val)
{
    if (DICT_FULL(dict)) dict_grow(dict);
    int lut_value;
    size_t lut_location = dict_find_lut_location(dict, key, &lut_value);

    if (lut_value < 0) {
        // enter new item into dict
        size_t item_index = DICT_EFFECTIVE_COUNT(dict);
        size_t item_offset = item_index * dict->item_size;
        dict->data[item_offset] = 1;
        memcpy(dict->data + item_offset + dict->key_offset, key, dict->key_size);
        memcpy(dict->data + item_offset + dict->val_offset, val, dict->val_size);
        dict->lut[lut_location] = item_index;
        dict->count += 1;
    }
    else {
        // replace existing value
        size_t item_offset = lut_value * dict->item_size;
        memcpy(dict->data + item_offset + dict->val_offset, val, dict->val_size);
    }
}

void*
dict_get_val(Dict* dict, void* key)
{
    if (dict->count == 0) return NULL;
    int item_index;
    dict_find_lut_location(dict, key, &item_index);

    if (item_index < 0)
        return NULL;
    else
        return dict->data + (dict->item_size * item_index) + dict->val_offset;
}

void*
dict_pop_val(Dict* dict, void* key)
{
    if (dict->count == 0) return NULL;
    int item_index;
    size_t lut_index = dict_find_lut_location(dict, key, &item_index);

    if (item_index < 0)
        return NULL;
    else {
        void* rtval = dict->data + (dict->item_size * item_index) + dict->val_offset;
        dict->lut[lut_index] = -1;
        dict->data[dict->item_size * item_index] = 0;
        dict->tombstone_count += 1;
        dict->count -= 1;
        if (DICT_EFFECTIVE_COUNT(dict) >= dict->capacity * DICT_SHRINK_THRESHOLD)
            dict_shrink(dict);
        return rtval;
    }
}

void
dict_del(Dict* dict, void* key)
{
    if (dict->count == 0) key_error();
    int item_index;
    size_t lut_index = dict_find_lut_location(dict, key, &item_index);
    if (item_index < 0) key_error();
    dict->lut[lut_index] = -1;
    dict->data[dict->item_size * item_index] = 0;
    dict->tombstone_count += 1;
    dict->count -= 1;
    if (DICT_EFFECTIVE_COUNT(dict) >= dict->capacity * DICT_SHRINK_THRESHOLD)
        dict_shrink(dict);
}

void
dict_update(Dict* dict, Dict* other)
{
    size_t updated_count = 0;
    uint8_t* data = other->data;
    while (updated_count != other->count) {
        while (!data[0]) data += other->item_size;
        dict_set_item(dict, data + other->key_offset, data + other->val_offset);
        data += other->item_size;
        updated_count += 1;
    }
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
