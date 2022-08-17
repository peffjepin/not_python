#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "lexer_types.h"

#define STATEMENT_TOKEN_CAPACITY 32

typedef struct {
    Token tokens[STATEMENT_TOKEN_CAPACITY];
    size_t length;
} Expression;

typedef enum {
    NULL_INST,
    INST_EXPR,
    INST_FOR_LOOP,
    INST_EOF,
} InstructionType;

typedef struct {
    ShortStr it;
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
