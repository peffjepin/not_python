#ifndef NOT_PYTHON_H
#define NOT_PYTHON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef int64_t PyInt;
typedef double PyFloat;
typedef bool PyBool;
typedef uint8_t PyByte;

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

PyBool int_eq(PyInt int1, PyInt int2);
PyBool float_eq(PyFloat float1, PyFloat float2);
PyBool bool_eq(PyBool bool1, PyBool bool2);

typedef PyBool (*PyCompareFunction)(const void*, const void*);

PyBool void_int_eq(void* int1, void* int2);
PyBool void_float_eq(void* float1, void* float2);
PyBool void_bool_eq(void* bool1, void* bool2);
PyBool void_str_eq(void* str1, void* str2);

typedef int (*PySortFunction)(const void*, const void*);

int pyint_sort_fn(const void*, const void*);
int pyfloat_sort_fn(const void*, const void*);
int pybool_sort_fn(const void*, const void*);
int ptstr_sort_fn(const void*, const void*);
int pyint_sort_fn_rev(const void*, const void*);
int pyfloat_sort_fn_rev(const void*, const void*);
int pybool_sort_fn_rev(const void*, const void*);
int ptstr_sort_fn_rev(const void*, const void*);

typedef struct {
    char* data;
    size_t offset;
    size_t length;
} PyString;

PyString str_add(PyString str1, PyString str2);
PyString str_fmt(const char* fmt, ...);
PyBool str_eq(PyString str1, PyString str2);
PyBool str_gt(PyString str1, PyString str2);
PyBool str_gte(PyString str1, PyString str2);
PyBool str_lt(PyString str1, PyString str2);
PyBool str_lte(PyString str1, PyString str2);

PyString np_int_to_str(PyInt num);
PyString np_float_to_str(PyFloat num);
PyString np_bool_to_str(PyBool value);
char* np_str_to_cstr(PyString str);

typedef void* (*ITER_NEXT_FN)(void* iter);

typedef struct {
    ITER_NEXT_FN next;
    void* iter;
} PyIter;

#define DICT_MIN_CAPACITY 10
#define DICT_LUT_FACTOR 2
#define DICT_GROW_FACTOR 2
#define DICT_SHRINK_THRESHOLD 0.35
#define DICT_SHRINK_FACTOR 0.5

typedef bool (*DICT_KEYCMP_FUNCTION)(void* key1, void* key2);

typedef struct {
    DICT_KEYCMP_FUNCTION keycmp;
    size_t key_size;
    size_t val_size;
    size_t item_size;
    size_t key_offset;
    size_t val_offset;
    size_t count;
    size_t tombstone_count;
    size_t capacity;
    size_t lut_capacity;
    PyByte* data;
    int* lut;
} PyDict;

typedef struct {
    void* key;
    void* val;
} DictItem;

PyIter dict_iter_keys(PyDict* dict);
PyIter dict_iter_vals(PyDict* dict);
PyIter dict_iter_items(PyDict* dict);

PyDict* dict_init(size_t key_size, size_t val_size, DICT_KEYCMP_FUNCTION cmp);
PyDict* dict_copy(PyDict* other);
void dict_clear(PyDict* dict);
void dict_set_item(PyDict* dict, void* key, void* val);
void dict_get_val(PyDict* dict, void* key, void* out);
void dict_pop_val(PyDict* dict, void* key, void* out);
void dict_update(PyDict* dict, PyDict* other);
void dict_del(PyDict* dict, void* key);

#define DICT_INIT(key_type, val_type, cmp)                                               \
    dict_init(sizeof(key_type), sizeof(val_type), cmp);

// TODO: handle this in the compiler
#define DICT_ITER_KEYS(dict, key_type, it, iter_var)                                     \
    PyIter iter_var = dict_iter_keys(dict);                                              \
    key_type it;                                                                         \
    void* __##it;                                                                        \
    while (                                                                              \
        (__##it = iter_var.next(iter_var.iter),                                          \
         it = (__##it) ? *(key_type*)__##it : it,                                        \
         __##it)                                                                         \
    )

typedef struct {
    PyInt count;
    PyInt capacity;
    size_t element_size;
    PySortFunction sort_fn;
    PySortFunction rev_sort_fn;
    PyCompareFunction cmp_fn;
    PyByte* data;
} PyList;

PyList* list_init(
    size_t elem_size,
    PySortFunction sort_fn,
    PySortFunction rev_sort_fn,
    PyCompareFunction cmp_fn
);
PyList* list_add(PyList* list1, PyList* list2);
void list_clear(PyList* list);
PyList* list_copy(PyList* list);
void list_extend(PyList* list, PyList* other);
void list_del(PyList* list, PyInt index);
void list_grow(PyList* list);
void list_reverse(PyList* list);
void list_sort(PyList* list, PyBool reverse);
void list_get_item(PyList* list, PyInt index, void* out);
void list_set_item(PyList* list, PyInt index, void* item);
void list_append(PyList* list, void* item);
void list_pop(PyList* list, PyInt index, void* out);
PyInt list_index(PyList* list, void* item);
void list_remove(PyList* list, void* item);
PyInt list_count(PyList* list, void* item);
void list_insert(PyList* list, PyInt index, void* item);

#define LIST_INIT(type, sort, rev_sort, cmp) list_init(sizeof(type), sort, rev_sort, cmp)

#define LIST_MIN_CAPACITY 10
#define LIST_SHRINK_THRESHOLD 0.35
#define LIST_SHRINK_FACTOR 0.5
#define LIST_GROW_FACTOR 2

void builtin_print(size_t argc, ...);

#endif
