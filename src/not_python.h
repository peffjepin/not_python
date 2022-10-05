#ifndef NOT_PYTHON_H
#define NOT_PYTHON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef int64_t NpInt;
typedef double NpFloat;
typedef bool NpBool;
typedef uint8_t NpByte;

typedef enum {
    MEMORY_ERROR = 1u << 0,
    INDEX_ERROR = 1u << 1,
    VALUE_ERROR = 1u << 2,
    KEY_ERROR = 1u << 3,
    ASSERTION_ERROR = 1u << 4,
} ExceptionType;

typedef struct {
    ExceptionType type;
    const char* msg;
} Exception;

extern Exception* global_exception;
extern uint64_t current_excepts;
void set_exception(ExceptionType type, const char* msg);
void set_exceptionf(ExceptionType type, const char* fmt, ...);
Exception* get_exception(void);

// TODO: better exception messages
#define key_error() set_exception(KEY_ERROR, "key not present")
#define index_error() set_exception(INDEX_ERROR, "index out of range")
#define memory_error() set_exception(MEMORY_ERROR, "out of memory")
#define value_error() set_exception(VALUE_ERROR, "value error")
// TODO: a good error message here will require saving metadata into executable
// so will require some kind of `debug` option in the front end
#define assertion_error(ln)                                                              \
    set_exceptionf(ASSERTION_ERROR, "AssertionError on line: %i\n", ln)

void* np_alloc(size_t bytes);
void* np_realloc(void* ptr, size_t bytes);
void np_free(void* ptr);

NpBool np_int_eq(NpInt int1, NpInt int2);
NpBool np_float_eq(NpFloat float1, NpFloat float2);
NpBool np_bool_eq(NpBool bool1, NpBool bool2);

typedef NpBool (*NpCompareFunction)(const void*, const void*);

NpBool np_void_int_eq(void* int1, void* int2);
NpBool np_void_float_eq(void* float1, void* float2);
NpBool np_void_bool_eq(void* bool1, void* bool2);
NpBool np_void_str_eq(void* str1, void* str2);

typedef int (*NpSortFunction)(const void*, const void*);

int np_int_sort_fn(const void*, const void*);
int np_float_sort_fn(const void*, const void*);
int np_bool_sort_fn(const void*, const void*);
int np_str_sort_fn(const void*, const void*);
int np_int_sort_fn_rev(const void*, const void*);
int np_float_sort_fn_rev(const void*, const void*);
int np_bool_sort_fn_rev(const void*, const void*);
int np_str_sort_fn_rev(const void*, const void*);

typedef struct {
    char* data;
    size_t offset;
    size_t length;
} NpString;

NpString np_str_add(NpString str1, NpString str2);
NpString np_str_fmt(const char* fmt, ...);
NpBool np_str_eq(NpString str1, NpString str2);
NpBool np_str_gt(NpString str1, NpString str2);
NpBool np_str_gte(NpString str1, NpString str2);
NpBool np_str_lt(NpString str1, NpString str2);
NpBool np_str_lte(NpString str1, NpString str2);

NpString np_int_to_str(NpInt num);
NpString np_float_to_str(NpFloat num);
NpString np_bool_to_str(NpBool value);
char* np_str_to_cstr(NpString str);

typedef void* (*NpIterNextFunc)(void* iter);

typedef struct {
    NpIterNextFunc next;
    void* iter;
} NpIter;

#define DICT_MIN_CAPACITY 10
#define DICT_LUT_FACTOR 2
#define DICT_GROW_FACTOR 2
#define DICT_SHRINK_THRESHOLD 0.35
#define DICT_SHRINK_FACTOR 0.5

typedef bool (*NpDictKeyCmpFunc)(void* key1, void* key2);

typedef struct {
    NpDictKeyCmpFunc keycmp;
    size_t key_size;
    size_t val_size;
    size_t item_size;
    size_t key_offset;
    size_t val_offset;
    size_t count;
    size_t tombstone_count;
    size_t capacity;
    size_t lut_capacity;
    NpByte* data;
    int* lut;
} NpDict;

typedef struct {
    void* key;
    void* val;
} DictItem;

NpIter np_dict_iter_keys(NpDict* dict);
NpIter np_dict_iter_vals(NpDict* dict);
NpIter np_dict_iter_items(NpDict* dict);

NpDict* np_dict_init(size_t key_size, size_t val_size, NpDictKeyCmpFunc cmp);
NpDict* np_dict_copy(NpDict* other);
void np_dict_clear(NpDict* dict);
void np_dict_set_item(NpDict* dict, void* key, void* val);
void np_dict_get_val(NpDict* dict, void* key, void* out);
void np_dict_pop_val(NpDict* dict, void* key, void* out);
void np_dict_update(NpDict* dict, NpDict* other);
void np_dict_del(NpDict* dict, void* key);

#define DICT_INIT(key_type, val_type, cmp)                                               \
    np_dict_init(sizeof(key_type), sizeof(val_type), cmp);

// TODO: handle this in the compiler
#define DICT_ITER_KEYS(dict, key_type, it, iter_var)                                     \
    NpIter iter_var = np_dict_iter_keys(dict);                                           \
    key_type it;                                                                         \
    void* __##it;                                                                        \
    while (                                                                              \
        (__##it = iter_var.next(iter_var.iter),                                          \
         it = (__##it) ? *(key_type*)__##it : it,                                        \
         __##it)                                                                         \
    )

typedef struct {
    NpInt count;
    NpInt capacity;
    size_t element_size;
    NpSortFunction sort_fn;
    NpSortFunction rev_sort_fn;
    NpCompareFunction cmp_fn;
    NpByte* data;
} NpList;

NpList* np_list_init(
    size_t elem_size,
    NpSortFunction sort_fn,
    NpSortFunction rev_sort_fn,
    NpCompareFunction cmp_fn
);
NpList* np_list_add(NpList* list1, NpList* list2);
void np_list_clear(NpList* list);
NpList* np_list_copy(NpList* list);
void np_list_extend(NpList* list, NpList* other);
void np_list_del(NpList* list, NpInt index);
void np_list_grow(NpList* list);
void np_list_reverse(NpList* list);
void np_list_sort(NpList* list, NpBool reverse);
void np_list_get_item(NpList* list, NpInt index, void* out);
void np_list_set_item(NpList* list, NpInt index, void* item);
void np_list_append(NpList* list, void* item);
void np_list_pop(NpList* list, NpInt index, void* out);
NpInt np_list_index(NpList* list, void* item);
void np_list_remove(NpList* list, void* item);
NpInt np_list_count(NpList* list, void* item);
void np_list_insert(NpList* list, NpInt index, void* item);

#define LIST_INIT(type, sort, rev_sort, cmp)                                             \
    np_list_init(sizeof(type), sort, rev_sort, cmp)

#define LIST_MIN_CAPACITY 10
#define LIST_SHRINK_THRESHOLD 0.35
#define LIST_SHRINK_FACTOR 0.5
#define LIST_GROW_FACTOR 2

void builtin_print(size_t argc, ...);

#endif
