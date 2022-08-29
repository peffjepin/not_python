#ifndef LEXER_HELPERS_H
#define LEXER_HELPERS_H

#include <stddef.h>

#include "arena.h"
#include "syntax.h"
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
    Arena* arena;
    Expression** data;
    size_t capacity;
    size_t count;
    size_t bytes;
} ExpressionVector;

ExpressionVector expr_vector_init(Arena* arena);
void expr_vector_append(ExpressionVector* vec, Expression* expr);
Expression** expr_vector_finalize(ExpressionVector* vec);

typedef struct {
    Arena* arena;
    char** data;
    size_t capacity;
    size_t count;
    size_t bytes;
} StringVector;

StringVector str_vector_init(Arena* arena);
void str_vector_append(StringVector* vec, char* str);
char** str_vector_finalize(StringVector* vec);

typedef struct {
    Arena* arena;
    ItIdentifier* data;
    size_t capacity;
    size_t count;
    size_t bytes;
} ItIdentifierVector;

ItIdentifierVector itid_vector_init(Arena* arena);
void itid_vector_append(ItIdentifierVector* vec, ItIdentifier itid);
ItIdentifier* itid_vector_finalize(ItIdentifierVector* vec);

typedef struct {
    size_t count;
    size_t capacity;
    Operation* operations;
} OperationVector;

void operation_vector_push(OperationVector* vec, Operation operation);

typedef struct {
    Arena* arena;
    size_t operands_count;
    size_t operands_capacity;
    size_t operands_nbytes;
    Operand* operands;
    size_t operations_count;
    OperationVector operation_vectors[MAX_PRECEDENCE + 1];
} ExpressionTable;

ExpressionTable et_init(Arena* arena);
void et_push_operand(ExpressionTable* et, Operand operand);
void et_push_operation(ExpressionTable* et, Operation operation);
void et_push_operation_type(ExpressionTable* et, Operator op_type);
Expression* et_to_expr(ExpressionTable* et);

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
