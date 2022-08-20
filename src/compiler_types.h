#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H

#include "aliases.h"
#include "lexer_types.h"

typedef enum {
    OPERAND_EXPRESSION,
    OPERAND_TOKEN,
} OperandKind;

typedef struct {
    OperandKind kind;
    union {
        Token token;
        ArenaRef ref;
    };
} Operand;

typedef struct {
    Operator op_type;
    size_t left;
    size_t right;
} Operation;

#define EXPRESSION_MAX_OPERATIONS 10

typedef struct {
    size_t n_operations;
    Operand operands[EXPRESSION_MAX_OPERATIONS + 1];
    Operation operations[EXPRESSION_MAX_OPERATIONS];
} Expression;

typedef enum {
    NULL_STMT,
    STMT_EXPR,
    STMT_FOR_LOOP,
    STMT_EOF,
} StatementKind;

typedef struct {
    ArenaRef it_ref;
    ArenaRef expr_ref;
} ForLoopStatement;

typedef struct {
    StatementKind kind;
    union {
        ForLoopStatement for_loop;
        ArenaRef expr_ref;
    };
} Statement;

#endif
