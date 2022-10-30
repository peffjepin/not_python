#ifndef C_COMPILER_H
#define C_COMPILER_H

#include <stdio.h>

#include "lexer.h"

#define NPLIB_GLOBAL_EXCEPTION "global_exception"

typedef struct {
    const char* name;
    int argc;  // argc < 0 means variable args count not yet set
    bool unsafe;
} NpLibFunctionData;

typedef enum {
    NPLIB_PRINT,

    NPLIB_ALLOC,
    NPLIB_REALLOC,
    NPLIB_FREE,

    NPLIB_LIST_APPEND,
    NPLIB_LIST_CLEAR,
    NPLIB_LIST_COUNT,
    NPLIB_LIST_EXTEND,
    NPLIB_LIST_INDEX,
    NPLIB_LIST_INSERT,
    NPLIB_LIST_POP,
    NPLIB_LIST_REMOVE,
    NPLIB_LIST_REVERSE,
    NPLIB_LIST_SORT,
    NPLIB_LIST_COPY,
    NPLIB_LIST_GET_ITEM,
    NPLIB_LIST_SET_ITEM,
    NPLIB_LIST_ADD,
    NPLIB_LIST_INIT,
    NPLIB_LIST_ITER,

    NPLIB_DICT_CLEAR,
    NPLIB_DICT_COPY,
    NPLIB_DICT_ITEMS,
    NPLIB_DICT_KEYS,
    NPLIB_DICT_VALUES,
    NPLIB_DICT_POP,
    NPLIB_DICT_POPITEM,
    NPLIB_DICT_UPDATE,
    NPLIB_DICT_GET_ITEM,
    NPLIB_DICT_INIT,
    NPLIB_DICT_SET_ITEM,

    NPLIB_STR_ADD,
    NPLIB_STR_EQ,
    NPLIB_STR_GT,
    NPLIB_STR_GTE,
    NPLIB_STR_LT,
    NPLIB_STR_LTE,
    NPLIB_STR_TO_CSTR,
    NPLIB_STR_FMT,

    NPLIB_INT_TO_STR,
    NPLIB_FLOAT_TO_STR,
    NPLIB_BOOL_TO_STR,

    NPLIB_GET_EXCEPTION,
    NPLIB_ASSERTION_ERROR,

    NPLIB_FMOD,
    NPLIB_POW,

    NPLIB_FUNCTION_COUNT
} NpLibFunction;

extern NpLibFunctionData NPLIB_FUNCTION_DATA[NPLIB_FUNCTION_COUNT];

#define NPLIB_CURRENT_EXCEPTS_CTYPE "uint64_t"
#define NPLIB_CURRENT_EXCEPTS "current_excepts"

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
            NpLibFunctionData c_function;
            union {
                // C_CALL
                StorageIdent* c_function_args;
                // C_CALL1
                StorageIdent c_function_arg;
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

#define UNSAFE_INST(inst)                                                                \
    ((inst.kind == INST_OPERATION && (inst.operation.kind == OPERATION_FUNCTION_CALL ||  \
                                      inst.operation.kind == OPERATION_C_CALL ||         \
                                      inst.operation.kind == OPERATION_C_CALL1)) ||      \
     (inst.kind == INST_ASSIGNMENT &&                                                    \
      (inst.assignment.right.kind == OPERATION_FUNCTION_CALL ||                          \
       inst.assignment.right.kind == OPERATION_C_CALL ||                                 \
       inst.assignment.right.kind == OPERATION_C_CALL1)))

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
