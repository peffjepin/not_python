#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H

#include "lexer_types.h"

#define EXPRESSION_CAPACITY 32

typedef struct Expression Expression;

typedef enum {
    OPERAND_EXPRESSION,
    OPERAND_CONSTANT,
    OPERAND_FUNCTION_CALL,
    OPERAND_STORAGE,
    OPERAND_INLINE_STORAGE,
} OperandKind;

typedef struct {
    char* identifier;
} Storage;

// something like x = [1, 2, 3]
// not sure what this struct aught to look like yet
typedef struct {
} InlineStorage;

typedef struct {
    char* value;
} Constant;

#define ARGUMENTS_CAPACITY 10
typedef struct {
    char* name;
    Expression* args[ARGUMENTS_CAPACITY];
} FunctionCall;

typedef struct {
    OperandKind kind;
    union {
        Storage storage;
        InlineStorage inline_storage;
        Constant constant;
        FunctionCall function_call;
        Expression* expression;
    };
} Operand;

typedef struct {
    Operator op_type;
    size_t left;
    size_t right;
} Operation;

typedef struct Expression {
    Operand operands[EXPRESSION_CAPACITY];
    Operation operations[EXPRESSION_CAPACITY];
    size_t n_operations;
} Expression;

typedef enum {
    NULL_INST,
    INST_EXPR,
    INST_FOR_LOOP,
    INST_EOF,
} InstructionType;

typedef struct {
    char* it;
    Expression iterable_expr;
} ForLoopInstruction;

typedef struct {
    InstructionType type;
    union {
        ForLoopInstruction for_loop;
        Expression expr;
    };
} Instruction;

#endif
