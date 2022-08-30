#include "lexer_helpers.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Notes on vector definitions
 *
 * Vectors defined with these macros are allocated in dynamic arena memory
 * and are meant to be finalized (allocated into static arena memory) once
 * their size has been determined.
 *
 * Vectors defined with PTR_VECTOR_DEFINITION take their elements by pointers
 * and when finalized return type**;
 *
 * Vectors defined with VALUE_VECTOR_DEFINITION take their elements by value
 * and when finalized return type*
 */

#define PTR_VECTOR_DEFINITION(type, prefix)                                              \
    type##Vector prefix##_vector_init(Arena* arena)                                      \
    {                                                                                    \
        type##Vector vec = {.arena = arena};                                             \
        vec.data = arena_dynamic_alloc(arena, &vec.bytes);                               \
        vec.capacity = vec.bytes / sizeof(type*);                                        \
        return vec;                                                                      \
    }                                                                                    \
    void prefix##_vector_append(type##Vector* vec, type* ptr)                            \
    {                                                                                    \
        if (vec->count == vec->capacity) {                                               \
            vec->data = arena_dynamic_grow(vec->arena, vec->data, &vec->bytes);          \
            vec->capacity = vec->bytes / sizeof(type*);                                  \
        }                                                                                \
        vec->data[vec->count++] = ptr;                                                   \
    }                                                                                    \
    type** prefix##_vector_finalize(type##Vector* vec)                                   \
    {                                                                                    \
        return arena_dynamic_finalize(                                                   \
            vec->arena, vec->data, sizeof(type*) * vec->count                            \
        );                                                                               \
    }

#define VALUE_VECTOR_DEFINITION(type, prefix)                                            \
    type##Vector prefix##_vector_init(Arena* arena)                                      \
    {                                                                                    \
        type##Vector vec = {.arena = arena};                                             \
        vec.data = arena_dynamic_alloc(arena, &vec.bytes);                               \
        vec.capacity = vec.bytes / sizeof(type);                                         \
        return vec;                                                                      \
    }                                                                                    \
    void prefix##_vector_append(type##Vector* vec, type value)                           \
    {                                                                                    \
        if (vec->count == vec->capacity) {                                               \
            vec->data = arena_dynamic_grow(vec->arena, vec->data, &vec->bytes);          \
            vec->capacity = vec->bytes / sizeof(type);                                   \
        }                                                                                \
        vec->data[vec->count++] = value;                                                 \
    }                                                                                    \
    type* prefix##_vector_finalize(type##Vector* vec)                                    \
    {                                                                                    \
        return arena_dynamic_finalize(vec->arena, vec->data, sizeof(type) * vec->count); \
    }

// kind of a hack to define StringVector without having to typedef String
#define String char
PTR_VECTOR_DEFINITION(String, str)
#undef String

PTR_VECTOR_DEFINITION(Expression, expr)
PTR_VECTOR_DEFINITION(ItGroup, itgroup)
VALUE_VECTOR_DEFINITION(ItIdentifier, itid)

void
out_of_memory(void)
{
    fprintf(stderr, "ERROR: out of memory");
    exit(1);
}

static const char* TOKEN_TYPE_TO_CSTR_TABLE[19] = {
    [TOK_KEYWORD] = "keyword",
    [TOK_COMMA] = ",",
    [TOK_COLON] = ":",
    [TOK_STRING] = "string",
    [TOK_NUMBER] = "number",
    [TOK_OPERATOR] = "operator",
    [TOK_NEWLINE] = "\n",
    [TOK_OPEN_PARENS] = "(",
    [TOK_CLOSE_PARENS] = ")",
    [TOK_OPEN_SQUARE] = "[",
    [TOK_CLOSE_SQUARE] = "]",
    [TOK_OPEN_CURLY] = "{",
    [TOK_CLOSE_CURLY] = "}",
    [TOK_IDENTIFIER] = "identifier",
    [TOK_DOT] = ".",
    [TOK_EOF] = "EOF",
};

const char*
token_type_to_cstr(TokenType type)
{
    assert(type < 19 && "token type not in table");
    return TOKEN_TYPE_TO_CSTR_TABLE[type];
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
operation_vector_push(OperationVector* vec, Operation operation)
{
    if (vec->capacity == vec->count) {
        vec->capacity += 16;
        vec->operations = realloc(vec->operations, sizeof(Operation) * vec->capacity);
        if (!vec->operations) out_of_memory();
    }
    vec->operations[vec->count++] = operation;
}

ExpressionTable
et_init(Arena* arena)
{
    ExpressionTable et = {.arena = arena};
    et.operands = arena_dynamic_alloc(arena, &et.operands_nbytes);
    et.operands_capacity = et.operands_nbytes / sizeof(Operand);
    return et;
}

void
et_push_operand(ExpressionTable* et, Operand operand)
{
    if (et->operands_count == et->operands_capacity) {
        et->operands = arena_dynamic_grow(et->arena, et->operands, &et->operands_nbytes);
        et->operands_capacity = et->operands_nbytes / sizeof(Operand);
    }
    et->operands[et->operands_count++] = operand;
    et->previous = ET_OPERAND;
}

void
et_push_operation(ExpressionTable* et, Operation operation)
{
    unsigned int precedence = PRECEDENCE_TABLE[operation.op_type];
    OperationVector* vec = et->operation_vectors + precedence;
    operation_vector_push(vec, operation);
    et->operations_count += 1;
    et->previous = ET_OPERATION;
}

void
et_push_operation_type(ExpressionTable* et, Operator op_type)
{
    Operation operation = {
        .op_type = op_type, .left = et->operands_count - 1, .right = et->operands_count};
    et_push_operation(et, operation);
}

Expression*
et_to_expr(ExpressionTable* et)
{
    assert(et->previous != ET_NONE && "got an empty expression");
    assert(et->previous == ET_OPERAND && "expression ended on an operator token");

    const unsigned int POW_PREC = PRECEDENCE_TABLE[OPERATOR_POW];

    Expression* expr = arena_alloc(et->arena, sizeof(Expression));
    expr->operations = arena_alloc(et->arena, sizeof(Operation) * et->operations_count);
    expr->operations_count = et->operations_count;
    expr->operands = arena_dynamic_finalize(
        et->arena, et->operands, sizeof(Operand) * et->operands_count
    );
    expr->operands_count = et->operands_count;

    size_t sorted_count = 0;
    for (int prec = MAX_PRECEDENCE; prec >= 0; prec--) {
        if (sorted_count == et->operations_count) break;
        OperationVector vec = et->operation_vectors[prec];
        if (vec.count == 0) continue;

        // OPERATOR_POW proceeds right->left
        if ((unsigned)prec == POW_PREC) {
            for (size_t i = 0; i < vec.count; i++)
                expr->operations[sorted_count++] = vec.operations[vec.count - 1 - i];
        }
        // everything else proceeds left->right so we can just copy
        else {
            memcpy(
                expr->operations + sorted_count,
                vec.operations,
                sizeof(Operation) * vec.count
            );
            sorted_count += vec.count;
        }
        free(vec.operations);
    }
    return expr;
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
