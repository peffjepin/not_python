#ifndef LEXER_HELPERS_H
#define LEXER_HELPERS_H

#include <stddef.h>

#include "compiler_types.h"
#include "lexer_types.h"

void out_of_memory(void);

#define TOKEN_QUEUE_CAPACITY 8

typedef struct {
    size_t head;
    size_t length;
    Token tokens[TOKEN_QUEUE_CAPACITY];
} TokenQueue;

Token tq_peek(TokenQueue* tq, size_t offset);
Token tq_consume(TokenQueue* tq);
void tq_push(TokenQueue* tq, Token token);

typedef struct {
    Expression* buf;
    size_t capacity;
    size_t length;
} ExpressionVector;

void expression_vector_append(ExpressionVector* vec, Expression expr);
void expression_vector_free(ExpressionVector* vec);

typedef struct {
    size_t count;
    size_t capacity;
    Operation* operations;
} OperationVector;

inline void operation_vector_push(OperationVector* vec, Operation operation);

typedef struct {
    size_t operands_count;
    size_t operands_capacity;
    Operand* operands;
    size_t operations_count;
    OperationVector operation_vectors[MAX_PRECEDENCE + 1];
} ExpressionTable;

inline void et_push_operand(ExpressionTable* et, Operand operand);
void et_push_operation(ExpressionTable* et, Operation operation);

size_t filename_offset(const char* filepath);

#define UNIMPLEMENTED(msg) assert(0 && msg)

#define SYNTAX_ERROR(loc, msg)                                                           \
    do {                                                                                 \
        fprintf(                                                                         \
            stderr,                                                                      \
            "%s:%u:%u SYNTAX ERROR: %s\n",                                               \
            (loc).filename,                                                              \
            (loc).line,                                                                  \
            (loc).col,                                                                   \
            msg                                                                          \
        );                                                                               \
        exit(1);                                                                         \
    } while (0)

#define SYNTAX_ERRORF(loc, msg, ...)                                                     \
    do {                                                                                 \
        fprintf(                                                                         \
            stderr,                                                                      \
            "%s:%u:%u SYNTAX ERROR: " msg "\n",                                          \
            (loc).filename,                                                              \
            (loc).line,                                                                  \
            (loc).col,                                                                   \
            __VA_ARGS__                                                                  \
        );                                                                               \
        exit(1);                                                                         \
    } while (0)

#endif
