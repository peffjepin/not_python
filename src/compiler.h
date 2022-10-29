#ifndef C_COMPILER_H
#define C_COMPILER_H

#include <stdio.h>

#include "lexer.h"

#define NPLIB_PRINT "builtin_print"

#define NPLIB_ALLOC "np_alloc"
#define NPLIB_FREE "np_free"

#define NPLIB_LIST_APPEND "np_list_append"
#define NPLIB_LIST_CLEAR "np_list_clear"
#define NPLIB_LIST_COUNT "np_list_count"
#define NPLIB_LIST_EXTEND "np_list_extend"
#define NPLIB_LIST_INDEX "np_list_index"
#define NPLIB_LIST_INSERT "np_list_insert"
#define NPLIB_LIST_POP "np_list_pop"
#define NPLIB_LIST_REMOVE "np_list_remove"
#define NPLIB_LIST_REVERSE "np_list_reverse"
#define NPLIB_LIST_SORT "np_list_sort"
#define NPLIB_LIST_COPY "np_list_copy"
#define NPLIB_LIST_GET_ITEM "np_list_get_item"
#define NPLIB_LIST_SET_ITEM "np_list_set_item"
#define NPLIB_LIST_ADD "np_list_add"
#define NPLIB_LIST_INIT "np_list_init"
#define NPLIB_LIST_ITER "np_list_iter"

#define NPLIB_DICT_CLEAR "np_dict_clear"
#define NPLIB_DICT_COPY "np_dict_copy"
#define NPLIB_DICT_GET "np_dict_get"
#define NPLIB_DICT_ITEMS "np_dict_iter_items"
#define NPLIB_DICT_KEYS "np_dict_iter_keys"
#define NPLIB_DICT_VALUES "np_dict_iter_vals"
#define NPLIB_DICT_POP "np_dict_pop_val"
#define NPLIB_DICT_POPITEM "np_dict_popitem"
#define NPLIB_DICT_UPDATE "np_dict_update"
#define NPLIB_DICT_GET_ITEM "np_dict_get_val"
#define NPLIB_DICT_INIT "np_dict_init"
#define NPLIB_DICT_SET_ITEM "np_dict_set_item"

#define NPLIB_STR_ADD "np_str_add"
#define NPLIB_STR_EQ "np_str_eq"
#define NPLIB_STR_GT "np_str_gt"
#define NPLIB_STR_GTE "np_str_gte"
#define NPLIB_STR_LT "np_str_lt"
#define NPLIB_STR_LTE "np_str_lte"
#define NPLIB_STR_TO_CSTR "np_str_to_cstr"
#define NPLIB_STR_FMT "np_str_fmt"

#define NPLIB_CURRENT_EXCEPTS_CTYPE "uint64_t"
#define NPLIB_CURRENT_EXCEPTS "current_excepts"
#define NPLIB_GET_EXCEPTION "get_exception"
#define NPLIB_ASSERTION_ERROR "assertion_error"

#define NPLIB_FMOD "fmod"
#define NPLIB_POW "pow"

#define UNTYPED                                                                          \
    (TypeInfo) { .type = NPTYPE_UNTYPED }
#define NONE_TYPE                                                                        \
    (TypeInfo) { .type = NPTYPE_NONE }
#define INT_TYPE                                                                         \
    (TypeInfo) { .type = NPTYPE_INT }
#define UNSIGNED_TYPE                                                                    \
    (TypeInfo) { .type = NPTYPE_UNSIGNED }
#define FLOAT_TYPE                                                                       \
    (TypeInfo) { .type = NPTYPE_FLOAT }
#define STRING_TYPE                                                                      \
    (TypeInfo) { .type = NPTYPE_STRING }
#define BOOL_TYPE                                                                        \
    (TypeInfo) { .type = NPTYPE_BOOL }
#define EXCEPTION_TYPE                                                                   \
    (TypeInfo) { .type = NPTYPE_EXCEPTION }
#define CONTEXT_TYPE                                                                     \
    (TypeInfo) { .type = NPTYPE_CONTEXT }
#define POINTER_TYPE                                                                     \
    (TypeInfo) { .type = NPTYPE_POINTER }

typedef struct Instruction Instruction;

typedef struct {
    Arena* arena;
    size_t count;
    size_t capacity;
    Instruction* instructions;
} InstructionSequence;

typedef struct {
    enum {
        IDENT_CSTR,
        IDENT_VAR,
        IDENT_STRING_LITERAL,
        IDENT_INT_LITERAL,
        IDENT_FLOAT_LITERAL
    } kind;
    union {
        // CSTR
        const char* cstr;
        // VAR
        Variable* var;
        // STRING LITERAL
        size_t str_literal_index;
        // INT LITERAL
        int int_value;
        // FLOAT LITERAL
        float float_value;
    };
    bool reference;
    TypeInfo info;
} StorageIdent;

typedef struct {
    enum {
        OPERATION_INTRINSIC,
        OPERATION_FUNCTION_CALL,
        OPERATION_C_CALL,
        OPERATION_C_CALL1,
        OPERATION_GET_ATTR,
        OPERATION_SET_ATTR,
        OPERATION_COPY,
        OPERATION_DEREF,
    } kind;

    union {
        // INSTRINSIC
        struct {
            Operator op;
            StorageIdent left;
            StorageIdent right;
        };
        // FUNCTION CALL
        struct {
            StorageIdent function;
            StorageIdent* args;
        };
        // C CALL
        struct {
            const char* c_fn_name;
            size_t c_fn_argc;
            union {
                // C_CALL
                StorageIdent* c_fn_argv;
                // C_CALL1
                StorageIdent c_fn_arg;
            };
        };
        // TODO: migrate attribute access to StorageIdent
        // SET/GET ATTR
        struct {
            StorageIdent object;
            SourceString attr;
            StorageIdent value;
        };
        // COPY
        StorageIdent copy;
        // DEREF
        struct {
            StorageIdent ref;  // the reference
            TypeInfo info;     // the type being referenced
        };
    };
} OperationInst;

typedef struct {
    // The NpIterator object
    StorageIdent iter;
    // The variable to unpack into
    StorageIdent unpack;
} IterNextInst;

typedef struct {
    StorageIdent left;
    OperationInst right;
} AssignmentInst;

typedef struct {
    StorageIdent condition;
    const char* after_label;
    InstructionSequence init;
    InstructionSequence before;
    InstructionSequence body;
    InstructionSequence after;
} LoopInst;

typedef struct {
    const char* function_name;
    StorageIdent var_ident;
    Signature signature;
    InstructionSequence body;
} DefineFunctionInst;

typedef struct {
    const char* class_name;
    Signature signature;
    InstructionSequence body;
} DefineClassInst;

typedef struct {
    bool negate;
    StorageIdent condition_ident;
    InstructionSequence body;
} IfInst;

typedef struct {
    StorageIdent rtval;
    bool should_free_closure;
} ReturnInst;

struct Instruction {
    enum {
        INST_NO_OP,
        INST_ASSIGNMENT,
        INST_OPERATION,
        INST_LOOP,
        INST_DECLARE_VARIABLE,
        INST_DECL_ASSIGNMENT,
        INST_DEFINE_FUNCTION,
        INST_DEFINE_CLASS,
        INST_IF,
        INST_ELSE,
        INST_GOTO,
        INST_LABEL,
        INST_BREAK,
        INST_CONTINUE,
        INST_RETURN,
        INST_ITER_NEXT,
        INST_INIT_CLOSURE,
    } kind;
    union {
        AssignmentInst assignment;
        OperationInst operation;
        LoopInst loop;
        StorageIdent declare_variable;
        DefineFunctionInst define_function;
        DefineClassInst define_class;
        IfInst if_;
        InstructionSequence else_;
        const char* label;
        ReturnInst return_;
        IterNextInst iter_next;
        size_t* closure_size;
    };
};

#define NO_OP                                                                            \
    (Instruction) { 0 }

typedef enum { LIB_MATH, LIB_COUNT } Libraries;

typedef struct {
    size_t capacity;
    size_t count;
    SourceString* elements;
    int* lookup;
} StringHashmap;

typedef struct {
    bool libs[LIB_COUNT];
} Requirements;

typedef struct {
    StringHashmap str_constants;
    InstructionSequence seq;
    Requirements req;
} CompiledInstructions;

CompiledInstructions compile(Lexer* lexer);

#endif
