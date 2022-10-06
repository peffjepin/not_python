#include <not_python.h>
#include <np_hash.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syntax.h"

const NpContext global_context = {0};

uint64_t current_excepts = 0;
Exception* global_exception = NULL;

void
set_exception(ExceptionType type, const char* msg)
{
    // TODO: don't overwrite existing exception
    if (current_excepts & type) {
        global_exception = np_alloc(sizeof(Exception));
        global_exception->type = type;
        global_exception->msg = msg;
        return;
    }
    else {
        // TODO: use diagnostics module
        fprintf(stderr, "ERROR: %s\n", msg);
        exit(1);
    }
}

void
set_exceptionf(ExceptionType type, const char* fmt, ...)
{
    char linebuffer[1024];
    va_list args;
    va_start(args, fmt);
    size_t wrote = vsnprintf(linebuffer, 1024, fmt, args);
    va_end(args);
    char* msg = np_alloc(sizeof(char) * wrote);
    memcpy(msg, linebuffer, wrote);
    set_exception(type, (const char*)msg);
}

Exception*
get_exception(void)
{
    Exception* exc = global_exception;
    global_exception = NULL;
    return exc;
}

#define SV_CHAR_AT(str, i) (str).data[(str).offset + i]

NpBool
np_str_eq(NpString str1, NpString str2)
{
    if (str1.length != str2.length) return false;
    for (size_t i = 0; i < str1.length; i++) {
        if (SV_CHAR_AT(str1, i) != SV_CHAR_AT(str2, i)) return false;
    }
    return true;
}

NpBool
np_str_gt(NpString str1, NpString str2)
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

NpBool
np_str_gte(NpString str1, NpString str2)
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

NpBool
np_str_lt(NpString str1, NpString str2)
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

NpBool
np_str_lte(NpString str1, NpString str2)
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

NpString
np_str_add(NpString str1, NpString str2)
{
    NpString str = {
        .data = np_alloc(str1.length + str2.length + 1),
        .length = str1.length + str2.length};
    memcpy(str.data, str1.data + str1.offset, str1.length);
    memcpy(str.data + str1.length, str2.data + str2.offset, str2.length);
    return str;
}

NpString
np_str_fmt(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    size_t required_length = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    NpString str = {.data = np_alloc(required_length + 1), .length = required_length};

    va_start(args, fmt);
    vsnprintf(str.data, required_length + 1, fmt, args);
    va_end(args);

    return str;
}

char*
np_str_to_cstr(NpString str)
{
    if (str.data[str.offset + str.length] == '\0') {
        return str.data + str.offset;
    }
    char* cstr = np_alloc(str.length + 1);
    memcpy(cstr, str.data + str.offset, str.length);
    return cstr;
}

NpString
np_int_to_str(NpInt num)
{
    size_t required_length = snprintf(NULL, 0, "%li", num);
    NpString str = {.data = np_alloc(required_length + 1), .length = required_length};
    snprintf(str.data, required_length + 1, "%li", num);
    return str;
}

NpString
np_float_to_str(NpFloat num)
{
    size_t required_length = snprintf(NULL, 0, "%f", num);
    NpString str = {.data = np_alloc(required_length + 1), .length = required_length};
    snprintf(str.data, required_length + 1, "%f", num);
    return str;
}

const NpString true_str = {.data = "True", .length = 4};
const NpString false_str = {.data = "False", .length = 5};

NpString
np_bool_to_str(NpBool value)
{
    return (value) ? true_str : false_str;
}

void
builtin_print(size_t argc, ...)
{
    va_list vargs;
    va_start(vargs, argc);

    for (size_t i = 0; i < argc; i++) {
        if (i > 0) fprintf(stdout, " ");
        NpString str = va_arg(vargs, NpString);
        fprintf(stdout, "%.*s", (int)str.length, str.data + str.offset);
    }

    va_end(vargs);

    fprintf(stdout, "\n");
    fflush(stdout);
}

#define LIST_COPY_TO_OUT(list, index, out)                                               \
    memcpy(out, list->data + index * list->element_size, list->element_size)
#define LIST_COPY_FROM_ITEM(list, index, item)                                           \
    memcpy(list->data + index * list->element_size, item, list->element_size)
#define LIST_ELEMENT(list, index) ((list->data) + ((index) * (list->element_size)))
#define LIST_WRAP(list, index) (((index) < 0) ? (list)->count + (index) : (index))
#define LIST_BOUNDS(list, wrapped_index)                                                 \
    if ((wrapped_index) < 0 || (wrapped_index) >= (list)->count) index_error()

NpList*
np_list_init(
    size_t elem_size,
    NpSortFunction sort_fn,
    NpSortFunction rev_sort_fn,
    NpCompareFunction cmp_fn
)
{
    NpList* list = np_alloc(sizeof(NpList));
    list->cmp_fn = cmp_fn;
    list->sort_fn = sort_fn;
    list->rev_sort_fn = rev_sort_fn;
    list->count = 0;
    list->capacity = LIST_MIN_CAPACITY;
    list->element_size = elem_size;
    list->data = np_alloc(elem_size * list->capacity);
    return list;
}

NpList*
np_list_add(NpList* list1, NpList* list2)
{
    // TODO: it would be better to allocate a large enough list to begin with and copy
    // into it.
    NpList* new_list = np_list_copy(list1);
    np_list_extend(new_list, list2);
    return new_list;
}

void
np_list_clear(NpList* list)
{
    list->count = 0;
    list->capacity = LIST_MIN_CAPACITY;
    list->data = np_realloc(list->data, list->element_size * list->capacity);
}

NpList*
np_list_copy(NpList* list)
{
    NpList* new_list = np_alloc(sizeof(NpList));
    new_list->count = list->count;
    new_list->capacity = list->capacity;
    new_list->element_size = list->element_size;
    size_t bytes = list->capacity * list->element_size;
    new_list->data = np_alloc(bytes);
    memcpy(new_list->data, list->data, bytes);
    return new_list;
}

void
np_list_extend(NpList* list, NpList* other)
{
    NpInt required_capacity = list->count + other->count + 1;
    if (list->capacity < required_capacity)
        np_realloc(list->data, list->element_size * required_capacity);
    memcpy(
        list->data + list->element_size * list->count,
        other->data,
        other->element_size * other->count
    );
    list->count += other->count;
}

void
np_list_shrink(NpList* list)
{
    if (list->capacity == LIST_MIN_CAPACITY) return;
    size_t new_capacity = list->capacity * LIST_SHRINK_FACTOR;
    if (new_capacity < LIST_MIN_CAPACITY) new_capacity = LIST_MIN_CAPACITY;
    list->capacity = new_capacity;
    list->data = np_realloc(list->data, list->element_size * list->capacity);
}

void
np_list_del(NpList* list, NpInt index)
{
    // assume bounds checking has already occured
    memmove(
        list->data + (list->element_size * index),
        list->data + (list->element_size * (index + 1)),
        list->element_size * (list->count - index + 1)
    );
    list->count -= 1;
    if (list->count <= list->capacity * LIST_SHRINK_THRESHOLD) np_list_shrink(list);
}

void
np_list_grow(NpList* list)
{
    list->capacity *= 2;
    list->data = np_realloc(list->data, list->element_size * list->capacity);
}

void
np_list_reverse(NpList* list)
{
    NpByte tmp[list->element_size];
    for (NpInt i = 0; i < list->count / 2; i++) {
        NpByte* left = LIST_ELEMENT(list, i);
        NpByte* right = LIST_ELEMENT(list, list->count - 1 - i);
        memcpy(tmp, right, list->element_size);
        memcpy(right, left, list->element_size);
        memcpy(left, tmp, list->element_size);
    }
}

void
np_list_get_item(NpList* list, NpInt index, void* out)
{
    index = LIST_WRAP(list, index);
    LIST_BOUNDS(list, index);
    LIST_COPY_TO_OUT(list, index, out);
}

void
np_list_set_item(NpList* list, NpInt index, void* item)
{
    index = LIST_WRAP(list, index);
    LIST_BOUNDS(list, index);
    LIST_COPY_FROM_ITEM(list, index, item);
}

void
np_list_append(NpList* list, void* item)
{
    NpInt index = list->count++;
    LIST_COPY_FROM_ITEM(list, index, item);
    if (list->count == list->capacity) np_list_grow(list);
}

void
np_list_pop(NpList* list, NpInt index, void* out)
{
    index = LIST_WRAP(list, index);
    LIST_BOUNDS(list, index);
    LIST_COPY_TO_OUT(list, index, out);
    np_list_del(list, index);
}

NpInt
np_list_index(NpList* list, void* item)
{
    NpInt location = -1;
    for (NpInt index = 0; index < list->count; index++) {
        if (list->cmp_fn(item, LIST_ELEMENT(list, index))) {
            location = (NpInt)index;
        }
    }
    if (location < 0) value_error();
    return location;
}

void
np_list_remove(NpList* list, void* item)
{
    NpInt index = np_list_index(list, item);
    np_list_del(list, index);
}

NpInt
np_list_count(NpList* list, void* item)
{
    NpInt count = 0;
    for (NpInt i = 0; i < list->count; i++) {
        if (list->cmp_fn(LIST_ELEMENT(list, i), item)) count++;
    }
    return count;
}

void
np_list_insert(NpList* list, NpInt index, void* item)
{
    index = LIST_WRAP(list, index);
    LIST_BOUNDS(list, index);
    if (list->count == list->capacity - 1) np_list_grow(list);

    list->count += 1;
    memmove(
        list->data + (list->element_size * (index + 1)),
        list->data + (list->element_size * index),
        list->element_size * (list->count - index)
    );
    LIST_COPY_FROM_ITEM(list, index, item);
}

void
np_list_sort(NpList* list, NpBool reverse)
{
    NpSortFunction fn = (reverse) ? list->rev_sort_fn : list->sort_fn;
    // TODO: better exception here
    if (!fn) value_error();
    qsort(list->data, list->count, list->element_size, fn);
}

int
np_int_sort_fn(const void* elem1, const void* elem2)
{
    if (*(NpInt*)elem1 < *(NpInt*)elem2) return -1;
    if (*(NpInt*)elem1 > *(NpInt*)elem2) return 1;
    return 0;
}

int
np_float_sort_fn(const void* elem1, const void* elem2)
{
    if (*(NpFloat*)elem1 < *(NpFloat*)elem2) return -1;
    if (*(NpFloat*)elem1 > *(NpFloat*)elem2) return 1;
    return 0;
}

int
np_bool_sort_fn(const void* elem1, const void* elem2)
{
    if (*(NpBool*)elem1 < *(NpBool*)elem2) return -1;
    if (*(NpBool*)elem1 > *(NpBool*)elem2) return 1;
    return 0;
}

int
np_str_sort_fn(const void* elem1, const void* elem2)
{
    if (np_str_lt(*(NpString*)elem1, *(NpString*)elem2)) return -1;
    if (np_str_gt(*(NpString*)elem1, *(NpString*)elem2)) return 1;
    return 0;
}

int
np_int_sort_fn_rev(const void* elem1, const void* elem2)
{
    if (*(NpInt*)elem1 < *(NpInt*)elem2) return 1;
    if (*(NpInt*)elem1 > *(NpInt*)elem2) return -1;
    return 0;
}

int
pyfloat_sort_fn_rev(const void* elem1, const void* elem2)
{
    if (*(NpFloat*)elem1 < *(NpFloat*)elem2) return 1;
    if (*(NpFloat*)elem1 > *(NpFloat*)elem2) return -1;
    return 0;
}

int
np_bool_sort_fn_rev(const void* elem1, const void* elem2)
{
    if (*(NpBool*)elem1 < *(NpBool*)elem2) return 1;
    if (*(NpBool*)elem1 > *(NpBool*)elem2) return -1;
    return 0;
}

int
np_str_sort_fn_rev(const void* elem1, const void* elem2)
{
    if (np_str_lt(*(NpString*)elem1, *(NpString*)elem2)) return 1;
    if (np_str_gt(*(NpString*)elem1, *(NpString*)elem2)) return -1;
    return 0;
}

NpBool
np_int_eq(NpInt int1, NpInt int2)
{
    return int1 == int2;
}
NpBool
np_float_eq(NpFloat float1, NpFloat float2)
{
    return float1 == float2;
}
NpBool
np_bool_eq(NpBool bool1, NpBool bool2)
{
    return bool1 == bool2;
}

NpBool
np_void_int_eq(void* int1, void* int2)
{
    return *(NpInt*)int1 == *(NpInt*)int2;
}
NpBool
np_void_float_eq(void* float1, void* float2)
{
    return *(NpFloat*)float1 == *(NpFloat*)float2;
}
NpBool
np_void_bool_eq(void* bool1, void* bool2)
{
    return *(NpBool*)bool1 == *(NpBool*)bool2;
}
NpBool
np_void_str_eq(void* str1, void* str2)
{
    return np_str_eq(*(NpString*)str1, *(NpString*)str2);
}

typedef struct {
    NpDict* dict;
    size_t yielded;
    size_t item_idx;
} NpDictIter;

void*
np_dict_keys_next(void* iter)
{
    NpDictIter* iterd = iter;
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

NpIter
np_dict_iter_keys(NpDict* dict)
{
    // TODO: empty iterable is probably a bug
    NpDictIter* iter = np_alloc(sizeof(NpDictIter));
    iter->dict = dict;
    NpIter iterd = {.next = (NpIterNextFunc)np_dict_keys_next, .iter = iter};
    return iterd;
}

void*
np_dict_vals_next(void* iter)
{
    NpDictIter* iterd = iter;
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

NpIter
np_dict_iter_vals(NpDict* dict)
{
    // TODO: empty iterable is probably a bug
    NpDictIter* iter = np_alloc(sizeof(NpDictIter));
    iter->dict = dict;
    NpIter iterd = {.next = (NpIterNextFunc)np_dict_vals_next, .iter = iter};
    return iterd;
}

typedef struct {
    NpDict* dict;
    size_t yielded;
    size_t item_idx;
    DictItem current_item;
} DictItemsIter;

void*
np_dict_items_next(void* iter)
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

NpIter
np_dict_iter_items(NpDict* dict)
{
    DictItemsIter* iter = np_alloc(sizeof(DictItemsIter));
    iter->dict = dict;
    NpIter iterd = {.next = (NpIterNextFunc)np_dict_items_next, .iter = iter};
    return iterd;
}

NpDict*
np_dict_init(size_t key_size, size_t val_size, NpDictKeyCmpFunc cmp)
{
    NpDict* dict = np_alloc(sizeof(NpDict));

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

NpDict*
np_dict_copy(NpDict* other)
{
    NpDict* dict = np_alloc(sizeof(NpDict));
    memcpy(dict, other, sizeof(NpDict));

    size_t data_bytes = dict->item_size * dict->capacity;
    size_t lut_bytes = dict->lut_capacity * sizeof(int);
    dict->data = np_alloc(data_bytes);
    dict->lut = np_alloc(lut_bytes);
    memcpy(dict->data, other->data, data_bytes);
    memcpy(dict->lut, other->lut, lut_bytes);

    return dict;
}

void
np_dict_clear(NpDict* dict)
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
np_dict_find_lut_location(NpDict* dict, void* key, int* lut_value)
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
np_dict_lut_rehash_item(NpDict* dict, void* key, size_t item_index)
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
np_dict_grow(NpDict* dict)
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
        np_dict_lut_rehash_item(
            dict, dict->data + item_offset + dict->key_offset, update_count++
        );
    }

    dict->tombstone_count = 0;
}

void
np_dict_shrink(NpDict* dict)
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
        np_dict_lut_rehash_item(
            dict, old_data + item_offset + dict->key_offset, update_count++
        );
    }

    np_free(old_data);
    dict->tombstone_count = 0;
}

void
np_dict_set_item(NpDict* dict, void* key, void* val)
{
    if (DICT_FULL(dict)) np_dict_grow(dict);
    int lut_value;
    size_t lut_location = np_dict_find_lut_location(dict, key, &lut_value);

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

void
np_dict_get_val(NpDict* dict, void* key, void* out)
{
    if (dict->count == 0) {
        key_error();
        return;
    }

    int item_index;
    np_dict_find_lut_location(dict, key, &item_index);

    if (item_index < 0)
        key_error();
    else
        memcpy(
            out,
            dict->data + (dict->item_size * item_index) + dict->val_offset,
            dict->val_size
        );
}

void
np_dict_pop_val(NpDict* dict, void* key, void* out)
{
    if (dict->count == 0) {
        key_error();
        return;
    }

    int item_index;
    size_t lut_index = np_dict_find_lut_location(dict, key, &item_index);

    if (item_index < 0)
        key_error();
    else {
        memcpy(
            out,
            dict->data + (dict->item_size * item_index) + dict->val_offset,
            dict->val_size
        );
        dict->lut[lut_index] = -1;
        dict->data[dict->item_size * item_index] = 0;
        dict->tombstone_count += 1;
        dict->count -= 1;
        if (DICT_EFFECTIVE_COUNT(dict) >= dict->capacity * DICT_SHRINK_THRESHOLD)
            np_dict_shrink(dict);
    }
}

void
np_dict_del(NpDict* dict, void* key)
{
    if (dict->count == 0) key_error();
    int item_index;
    size_t lut_index = np_dict_find_lut_location(dict, key, &item_index);
    if (item_index < 0) key_error();
    dict->lut[lut_index] = -1;
    dict->data[dict->item_size * item_index] = 0;
    dict->tombstone_count += 1;
    dict->count -= 1;
    if (DICT_EFFECTIVE_COUNT(dict) >= dict->capacity * DICT_SHRINK_THRESHOLD)
        np_dict_shrink(dict);
}

void
np_dict_update(NpDict* dict, NpDict* other)
{
    size_t updated_count = 0;
    uint8_t* data = other->data;
    while (updated_count != other->count) {
        while (!data[0]) data += other->item_size;
        np_dict_set_item(dict, data + other->key_offset, data + other->val_offset);
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
