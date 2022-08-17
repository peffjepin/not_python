#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "tokens.h"

#define STATEMENT_TOKEN_CAPACITY 32

typedef struct {
    Token tokens[STATEMENT_TOKEN_CAPACITY];
    size_t length;
} Statement;

typedef enum {
    NULL_INST,
    INST_STMT,
    INST_FOR_LOOP,
    INST_EOF,
} InstructionType;

typedef struct {
    ShortStr it;
    Statement iterable_stmt;
} ForLoopInstruction;

typedef struct {
    InstructionType type;
    union {
        ForLoopInstruction for_loop;
        Statement stmt;
    };
} Instruction;

#endif
