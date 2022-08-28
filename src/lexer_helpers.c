#include "lexer_helpers.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
out_of_memory(void)
{
    fprintf(stderr, "ERROR: out of memory");
    exit(1);
}

Token
tq_peek(TokenQueue* tq, size_t offset)
{
    assert(tq->length && "peeking empty queue");
    assert(tq->length > offset && "peeking past the end of queue");
    return tq->tokens[(tq->head + offset) % TOKEN_QUEUE_CAPACITY];
}

Token
tq_consume(TokenQueue* tq)
{
    assert(tq->length && "consuming empty queue");
    tq->length -= 1;
    return tq->tokens[tq->head++ % TOKEN_QUEUE_CAPACITY];
}

void
tq_push(TokenQueue* tq, Token token)
{
    assert(tq->length < TOKEN_QUEUE_CAPACITY && "pushing to full queue");
    tq->tokens[(tq->head + tq->length++) % TOKEN_QUEUE_CAPACITY] = token;
}

void
expression_vector_append(ExpressionVector* vec, Expression expr)
{
    if (vec->length == vec->capacity) {
        if (vec->capacity == 0)
            vec->capacity = 8;
        else
            vec->capacity *= 2;
        vec->buf = realloc(vec->buf, sizeof(Expression) * vec->capacity);
        if (!vec->buf) out_of_memory();
    }
    vec->buf[vec->length++] = expr;
}

void
expression_vector_free(ExpressionVector* vec)
{
    free(vec->buf);
}

inline void
operation_vector_push(OperationVector* vec, Operation operation)
{
    if (vec->capacity == vec->count) {
        vec->capacity += 16;
        vec->operations = realloc(vec->operations, sizeof(Operation) * vec->capacity);
        if (!vec->operations) out_of_memory();
    }
    vec->operations[vec->count++] = operation;
}

inline void
et_push_operand(ExpressionTable* et, Operand operand)
{
    if (et->operands_count == et->operands_capacity) {
        et->operands_capacity += 16;
        et->operands = realloc(et->operands, sizeof(Operand) * et->operands_capacity);
        if (!et->operands) out_of_memory();
    }
    et->operands[et->operands_count++] = operand;
}

void
et_push_operation(ExpressionTable* et, Operation operation)
{
    unsigned int precedence = PRECENDENCE_TABLE[operation.op_type];
    operation_vector_push(et->operation_vectors + precedence, operation);
    et->operations_count += 1;
}

// TODO: portability
size_t
filename_offset(const char* filepath)
{
    size_t length = strlen(filepath);
    for (size_t i = 0; i < length; i++) {
        size_t index = length - 1 - i;
        if (filepath[index] == '/') return index + 1;
    }
    return 0;
}
