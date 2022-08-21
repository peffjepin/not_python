#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H

#include "aliases.h"
#include "lexer_types.h"

typedef enum {
    OPERAND_EXPRESSION,
    OPERAND_ENCLOSURE_LITERAL,
    OPERAND_COMPREHENSION,
    OPERAND_TOKEN,
    OPERAND_ARGUMENTS
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

#define MAX_ARGUMENTS 10

typedef struct {
    ArenaRef value_refs[MAX_ARGUMENTS];
    Token kwds[MAX_ARGUMENTS];
    size_t length;
    size_t n_positional;
} Arguments;

typedef enum {
    ENCLOSURE_LIST,
    ENCLOSURE_DICT,
    ENCLOSURE_TUPLE,
} EnclosureType;

typedef enum {
    COMPREHENSION_MAPPED,
    COMPREHENSION_SEQUENCE,
} ComprehensionKind;

#define MAX_COMPREHENSION_NESTING 10

typedef struct {
    size_t nesting;
    ArenaRef its[MAX_COMPREHENSION_NESTING];
    ArenaRef iterables[MAX_COMPREHENSION_NESTING];
} ComprehensionBody;

typedef struct {
    ArenaRef key_expr;
    ArenaRef val_expr;
} MappedComprehension;

typedef struct {
    ArenaRef expr;
} SequenceComprehension;

typedef struct {
    EnclosureType type;
    ComprehensionBody body;
    union {
        MappedComprehension mapped;
        SequenceComprehension sequence;
    };
} Comprehension;

typedef struct {
    EnclosureType type;
    ArenaRef first_expr;
    ArenaRef last_expr;
} Enclosure;

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
