#ifndef NOT_PYTHON_H
#define NOT_PYTHON_H

#include <stdbool.h>
#include <stddef.h>

// TODO: maybe should be typedefs
#define PYINT long long int
#define PYFLOAT double
#define PYSTRING StringView
#define PYBOOL bool
#define PYLIST List*

void memory_error(void);
void index_error(void);
void value_error(void);

void* np_alloc(size_t bytes);
void* np_realloc(void* ptr, size_t bytes);
void np_free(void* ptr);

typedef struct {
    char* data;
    size_t offset;
    size_t length;
} StringView;

PYSTRING str_add(PYSTRING str1, PYSTRING str2);
PYBOOL str_eq(PYSTRING str1, PYSTRING str2);
PYBOOL str_gt(PYSTRING str1, PYSTRING str2);
PYBOOL str_gte(PYSTRING str1, PYSTRING str2);
PYBOOL str_lt(PYSTRING str1, PYSTRING str2);
PYBOOL str_lte(PYSTRING str1, PYSTRING str2);

PYSTRING np_int_to_str(PYINT num);
PYSTRING np_float_to_str(PYFLOAT num);
PYSTRING np_bool_to_str(PYBOOL value);

typedef struct {
    size_t count;
    size_t capacity;
    size_t element_size;
    void* data;
} List;

List* list_add(List* list1, List* list2);
void list_clear(List* list);
List* list_copy(List* list);
void list_extend(List* list, List* other);
void list_del(List* list, PYINT index);
void list_grow(List* list);
List* np_internal_list_init(size_t elem_size);

#define LIST_MIN_CAPACITY 10
#define LIST_SHRINK_THRESHOLD 0.35
#define LIST_SHRINK_FACTOR 0.5
#define LIST_GROW_FACTOR 2

#define LIST_GET_ITEM(list, type, index, var)                                            \
    do {                                                                                 \
        PYINT i = (index < 0) ? list->count - i : index;                                 \
        if (i < 0 || i >= list->count)                                                   \
            index_error();                                                               \
        else                                                                             \
            var = ((type*)list->data)[i];                                                \
    } while (0)

#define LIST_SET_ITEM(list, type, index, item)                                           \
    do {                                                                                 \
        PYINT i = (index < 0) ? list->count - i : index;                                 \
        if (i < 0 || i >= list->count)                                                   \
            index_error();                                                               \
        else                                                                             \
            ((type*)list->data)[i] = item;                                               \
    } while (0)

#define LIST_APPEND(list, type, item)                                                    \
    ((type*)list->data)[list->count++] = item;                                           \
    if (list->count == list->capacity) list_grow(list)

#define LIST_POP(list, type, index, var)                                                 \
    do {                                                                                 \
        PYINT i = (index < 0) ? list->count - i : index;                                 \
        if (i < 0 || i >= list->count)                                                   \
            index_error();                                                               \
        else {                                                                           \
            var = ((type*)list->data)[i];                                                \
            list_del(list, i);                                                           \
        }                                                                                \
    } while (0)

#define LIST_INDEX(list, type, cmp, item, var)                                           \
    do {                                                                                 \
        bool found = false;                                                              \
        for (size_t i = 0; i < list->count; i++) {                                       \
            if (cmp(item, list->data[i])) {                                              \
                var = (PYINT)i;                                                          \
                found = true;                                                            \
                break;                                                                   \
            }                                                                            \
        }                                                                                \
        if (!found) value_error();                                                       \
    } while (0)

#define LIST_REMOVE(list, type, cmp, item)                                               \
    do {                                                                                 \
        PYINT index;                                                                     \
        LIST_INDEX(list, type, cmp, item, index);                                        \
        list_del(list, index);                                                           \
    } while (0)

#define LIST_INIT(type) np_internal_list_init(sizeof(type))

#define LIST_FOR_EACH(list, type, it, idx)                                               \
    size_t idx = 0;                                                                      \
    for (type it = ((type*)list->data)[idx]; idx < list->count;                          \
         it = ((type*)list->data)[++idx])

void builtin_print(size_t argc, ...);

#endif
